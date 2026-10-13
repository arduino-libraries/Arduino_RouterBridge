/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include "link.h"

#include <string.h>

namespace seriallink {

// Arduino cores may define min/max as macros: use our own helpers.
template<typename T> static inline T minOf(T a, T b) { return a < b ? a : b; }
template<typename T> static inline T maxOf(T a, T b) { return a > b ? a : b; }

// Returns true if the deadline has been reached (the time may wrap around).
static inline bool due(uint32_t now, uint32_t deadline) {
    return int32_t(now - deadline) >= 0;
}

Link::Link(OutputFn output, void* ctx, const Timing& timing)
    : output(output), outputCtx(ctx), timing(timing) {}

void Link::begin(uint32_t sessionNonce, uint32_t now) {
    // A pending RST (from a previous close or failure) is kept: it targets the
    // previous session and it is harmless for the new one.
    st = State::Connecting;
    err = Error::None;
    nonce = sessionNonce;
    peer = SynParams{};
    peerKnown = false;
    peerActive = false;
    negMaxPayload = SERIALLINK_MAX_PAYLOAD;
    negWindow = SERIALLINK_WINDOW;
    synDeadline = now;
    synPending = false;
    synAckPending = false;
    nakPending = false;
    ackNeeded = false;
    txBase = 0;
    txCount = 0;
    txSent = 0;
    nextSeq = 0;
    peerWnd = 0;
    retries = 0;
    rtoStart = now;
    lastTx = now;
    rstSent = false;
    rxHead = 0;
    rxCount = 0;
    expected = 0;
    ackPending = 0;
    nakSent = false;
    lastAdvWnd = 0;
    lastRx = now;
    accLen = 0;
    discarding = false;
}

void Link::close() {
    terminate(Error::Closed, st == State::Established);
}

void Link::terminate(Error e, bool sendRst) {
    if (st == State::Closed || st == State::Idle) return;
    st = State::Closed;
    err = e;
    txCount = 0;
    txSent = 0;
    synPending = false;
    synAckPending = false;
    nakPending = false;
    ackNeeded = false;
    if (sendRst) rstPending = true;
}

SynParams Link::localParams() const {
    SynParams p{};
    p.maxPayload = SERIALLINK_MAX_PAYLOAD;
    p.window = SERIALLINK_WINDOW;
    p.nonce = nonce;
    return p;
}

uint8_t Link::recvCredit() const {
    return uint8_t(minOf<size_t>((RX_CAP - rxCount) / negMaxPayload, MAX_WINDOW_LIMIT));
}

uint8_t Link::sendLimit() const {
    return minOf(peerWnd, negWindow);
}

size_t Link::writable() const {
    if (st != State::Established) return 0;
    const uint8_t limit = sendLimit();
    return limit > txCount ? size_t(limit - txCount) * negMaxPayload : 0;
}

size_t Link::write(const uint8_t* data, size_t len) {
    if (st != State::Established) return 0;
    const uint8_t limit = sendLimit();
    size_t done = 0;
    while (done < len && txCount < limit) {
        const size_t size = minOf<size_t>(len - done, negMaxPayload);
        Slot& s = slots[(txBase + txCount) % SERIALLINK_WINDOW];
        memcpy(s.data, data + done, size);
        s.len = uint16_t(size);
        txCount++;
        nextSeq++;
        done += size;
    }
    return done;
}

size_t Link::read(uint8_t* buf, size_t len) {
    const size_t n = minOf(len, rxCount);
    for (size_t i = 0; i < n; i++) {
        buf[i] = rxBuf[rxHead];
        rxHead = (rxHead + 1) % RX_CAP;
    }
    rxCount -= n;

    // Send a window update if the peer may be (almost) stalled
    if (n > 0 && st == State::Established &&
        int(recvCredit()) - int(lastAdvWnd) >= maxOf(1, negWindow / 2)) {
        ackNeeded = true;
    }
    return n;
}

void Link::onRx(const uint8_t* data, size_t len, uint32_t now) {
    for (size_t i = 0; i < len; i++) {
        const uint8_t b = data[i];
        if (b != 0) {
            if (!discarding) {
                if (accLen == sizeof(acc)) {
                    // Too long, a delimiter was lost: skip until the next one
                    discarding = true;
                    accLen = 0;
                } else {
                    acc[accLen++] = b;
                }
            }
            continue;
        }
        if (discarding || accLen > 0) receive(now);
        accLen = 0;
        discarding = false;
    }
}

void Link::receive(uint32_t now) {
    Frame f;
    if (discarding || !decodeFrame(acc, accLen, rxRaw, sizeof(rxRaw), f)) {
        stat.rxErrors++;
        return;
    }
    stat.rxFrames++;
    handleFrame(f, now);
}

void Link::handleFrame(const Frame& f, uint32_t now) {
    SynParams p;
    if (st == State::Connecting) {
        switch (f.type) {
        case FrameType::Syn:
            if (!parseSyn(f.payload, f.len, false, p)) return;
            peer = p;
            peerKnown = true;
            synAckPending = true;
            break;
        case FrameType::SynAck:
            if (!parseSyn(f.payload, f.len, true, p) || p.echo != nonce) return; // stale or for someone else
            peer = p;
            peerKnown = true;
            establish(now);
            synAckPending = true; // let the peer complete the handshake too
            break;
        case FrameType::Data:
        case FrameType::Ack:
        case FrameType::Nak:
            if (peerKnown) {
                // The peer got our SYN_ACK and is already established
                establish(now);
                handleEstablished(f, now);
            } else if (!rstSent || now - lastRst >= timing.retransmitTimeoutMs) {
                // The peer is talking to a previous session
                rstSent = true;
                lastRst = now;
                rstPending = true;
            }
            break;
        case FrameType::Rst:
            break;
        }
        return;
    }

    if (st != State::Established) return;
    lastRx = now;
    switch (f.type) {
    case FrameType::Syn:
        if (!parseSyn(f.payload, f.len, false, p)) return;
        if (p.nonce == peer.nonce && !peerActive) {
            synAckPending = true; // our SYN_ACK was lost
        } else {
            // The peer restarted. A peer sends SYN only until it is
            // established, so a SYN after DATA/ACK/NAK means a new session
            // even if the nonce is the same.
            terminate(Error::SessionReset, false);
        }
        break;
    case FrameType::SynAck:
        break; // duplicate, ignore
    case FrameType::Rst:
        terminate(Error::SessionReset, false);
        break;
    default:
        handleEstablished(f, now);
        break;
    }
}

void Link::establish(uint32_t now) {
    st = State::Established;
    negMaxPayload = minOf<uint16_t>(SERIALLINK_MAX_PAYLOAD, peer.maxPayload);
    negWindow = minOf<uint8_t>(SERIALLINK_WINDOW, peer.window);
    peerWnd = peer.window;
    lastRx = now;
}

void Link::handleEstablished(const Frame& f, uint32_t now) {
    peerActive = true;

    // Process the (piggybacked) acknowledgement
    const uint8_t base = nextSeq - txCount;
    const uint8_t n = f.ack - base;
    if (n <= txCount) {
        if (n > 0) {
            txBase = (txBase + n) % SERIALLINK_WINDOW;
            txCount -= n;
            txSent = txSent > n ? txSent - n : 0;
            retries = 0;
            rtoStart = now;
        }
        peerWnd = f.wnd;
    }

    if (f.type == FrameType::Nak) {
        retransmit();
        return;
    }
    if (f.type != FrameType::Data) return;

    const uint8_t d = f.seq - expected;
    if (d == 0) {
        if (f.len > negMaxPayload || f.len > RX_CAP - rxCount) return; // the peer did not respect the limits
        size_t tail = (rxHead + rxCount) % RX_CAP;
        for (size_t i = 0; i < f.len; i++) {
            rxBuf[tail] = f.payload[i];
            tail = (tail + 1) % RX_CAP;
        }
        rxCount += f.len;
        expected++;
        nakSent = false;
        ackPending++;
        if (ackPending == 1) ackDue = now + timing.ackDelayMs;
        if (ackPending >= maxOf(1, negWindow / 2) || recvCredit() == 0) ackNeeded = true;
    } else if (d >= 128) {
        ackNeeded = true; // duplicate: our ack has probably been lost
    } else if (!nakSent) {
        // A frame has been lost: ask to go back (only once)
        nakSent = true;
        nakPending = true;
    }
}

void Link::retransmit() {
    if (txCount == 0) return;
    txSent = 0;
    stat.retransmits += txCount;
}

uint32_t Link::poll(uint32_t now) {
    if (st == State::Connecting) {
        if (due(now, synDeadline)) {
            synPending = true;
            synDeadline = now + timing.retransmitTimeoutMs;
        }
    } else if (st == State::Established) {
        if (now - lastRx >= timing.linkTimeoutMs) {
            terminate(Error::LinkTimeout, true);
        } else if (txCount > 0 && txSent == txCount && due(now, rtoStart + timing.retransmitTimeoutMs)) {
            if (++retries > timing.maxRetries) {
                terminate(Error::LinkTimeout, true);
            } else {
                retransmit();
                rtoStart = now;
            }
        }
        if (st == State::Established) {
            if (ackPending > 0 && due(now, ackDue)) ackNeeded = true;
            if (due(now, lastTx + timing.keepAliveMs)) ackNeeded = true;
        }
    }

    flush(now);

    // Compute the time until the next deadline
    uint32_t next = timing.keepAliveMs;
    auto consider = [&](uint32_t deadline) {
        const int32_t d = int32_t(deadline - now);
        const uint32_t wait = d > 0 ? uint32_t(d) : 0;
        if (wait < next) next = wait;
    };
    if (st == State::Connecting) {
        consider(synDeadline);
    } else if (st == State::Established) {
        consider(lastRx + timing.linkTimeoutMs);
        consider(lastTx + timing.keepAliveMs);
        if (txCount > 0) consider(rtoStart + timing.retransmitTimeoutMs);
        if (ackPending > 0) consider(ackDue);
    }
    return next;
}

void Link::flush(uint32_t now) {
    uint8_t p[SYN_ACK_SIZE];
    if (rstPending) {
        rstPending = false;
        putUint32(p, nonce);
        emit(FrameType::Rst, 0, p, RST_SIZE, now);
    }
    if (synPending) {
        synPending = false;
        if (st == State::Connecting) {
            emit(FrameType::Syn, 0, p, marshalSyn(localParams(), false, p), now);
        }
    }
    if (synAckPending) {
        synAckPending = false;
        if (st == State::Connecting || st == State::Established) {
            SynParams lp = localParams();
            lp.echo = peer.nonce;
            emit(FrameType::SynAck, 0, p, marshalSyn(lp, true, p), now);
        }
    }
    if (st != State::Established) return;

    // Every DATA/ACK/NAK frame carries an up-to-date acknowledgement
    bool acked = false;
    if (nakPending) {
        nakPending = false;
        emit(FrameType::Nak, 0, nullptr, 0, now);
        acked = true;
    }
    while (txSent < txCount) {
        const Slot& s = slots[(txBase + txSent) % SERIALLINK_WINDOW];
        const uint8_t seq = nextSeq - txCount + txSent;
        emit(FrameType::Data, seq, s.data, s.len, now);
        txSent++;
        rtoStart = now;
        acked = true;
    }
    if (ackNeeded && !acked) {
        emit(FrameType::Ack, 0, nullptr, 0, now);
    }
    ackNeeded = false;
}

void Link::emit(FrameType type, uint8_t seq, const uint8_t* payload, size_t len, uint32_t now) {
    Frame f{type, seq, 0, 0, payload, len};
    if (type == FrameType::Data || type == FrameType::Ack || type == FrameType::Nak) {
        f.ack = expected;
        f.wnd = recvCredit();
        lastAdvWnd = f.wnd;
        ackPending = 0;
    }
    const size_t n = encodeFrame(f, txRaw, txEnc);
    lastTx = now;
    stat.txFrames++;
    output(outputCtx, txEnc, n);
}

} // namespace seriallink
