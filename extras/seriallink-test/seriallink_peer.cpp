/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

// A seriallink peer for interoperability tests with the Go implementation
// (internal/seriallink in arduino-router). It runs the protocol over
// stdin/stdout, as if they were a serial port, and echoes back every byte
// received through the link. Like the MCU firmware, it starts a new session
// whenever the current one is reset by the peer or times out.
//
// Events are logged on stderr. On EOF on stdin the session is closed
// (notifying the peer with a RST) and the program exits.
//
// Usage: seriallink_peer [options]
//   --nonce N          use always the nonce N (default: random)
//   --stall MS         do not read from the link for MS ms after the first
//                      session is established (to test flow control)
//   --rto MS           retransmission timeout
//   --max-retries N    retransmissions before declaring the link dead
//   --ack-delay MS     acknowledgement delay
//   --keepalive MS     keep-alive interval
//   --link-timeout MS  link timeout

#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include <poll.h>
#include <unistd.h>

#include "../../src/seriallink/link.h"

using namespace seriallink;

static uint32_t nowMs() {
    using namespace std::chrono;
    return uint32_t(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

static void output(void*, const uint8_t* data, size_t len) {
    while (len > 0) {
        const ssize_t n = ::write(STDOUT_FILENO, data, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            std::fprintf(stderr, "peer: write error: %s\n", std::strerror(errno));
            std::exit(1);
        }
        data += n;
        len -= size_t(n);
    }
}

static const char* errorName(Error e) {
    switch (e) {
    case Error::None: return "none";
    case Error::Closed: return "closed";
    case Error::SessionReset: return "session reset";
    case Error::LinkTimeout: return "link timeout";
    }
    return "?";
}

static void usage() {
    std::fprintf(stderr, "usage: seriallink_peer [--nonce N] [--stall MS] [--rto MS] [--max-retries N]"
                         " [--ack-delay MS] [--keepalive MS] [--link-timeout MS]\n");
    std::exit(2);
}

int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);

    Timing timing;
    bool fixedNonce = false;
    uint32_t nonceArg = 0;
    uint32_t stallMs = 0;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (i + 1 >= argc) usage();
        const unsigned long v = std::strtoul(argv[++i], nullptr, 0);
        if (arg == "--nonce") {
            fixedNonce = true;
            nonceArg = uint32_t(v);
        } else if (arg == "--stall") {
            stallMs = uint32_t(v);
        } else if (arg == "--rto") {
            timing.retransmitTimeoutMs = uint32_t(v);
        } else if (arg == "--max-retries") {
            timing.maxRetries = uint8_t(v);
        } else if (arg == "--ack-delay") {
            timing.ackDelayMs = uint32_t(v);
        } else if (arg == "--keepalive") {
            timing.keepAliveMs = uint32_t(v);
        } else if (arg == "--link-timeout") {
            timing.linkTimeoutMs = uint32_t(v);
        } else {
            usage();
        }
    }

    std::random_device rd;
    auto newNonce = [&] { return fixedNonce ? nonceArg : uint32_t(rd()); };

    Link link(output, nullptr, timing);
    link.begin(newNonce(), nowMs());

    std::vector<uint8_t> pending; // received bytes not yet accepted by link.write()
    bool everEstablished = false;
    uint32_t stallUntil = 0;
    State lastState = State::Idle;

    for (;;) {
        uint32_t wait = link.poll(nowMs());

        const State st = link.state();
        if (st != lastState) {
            if (st == State::Established) {
                std::fprintf(stderr, "peer: established (max payload %u, window %u)\n", link.maxPayload(), link.sendWindow());
                if (!everEstablished) stallUntil = nowMs() + stallMs;
                everEstablished = true;
            }
            lastState = st;
        }
        if (st == State::Closed) {
            // Like the MCU firmware: start a new session
            std::fprintf(stderr, "peer: %s, starting a new session\n", errorName(link.error()));
            pending.clear();
            link.begin(newNonce(), nowMs());
            continue;
        }

        // Echo
        if (st == State::Established && int32_t(nowMs() - stallUntil) >= 0) {
            if (pending.empty() && link.available() > 0) {
                pending.resize(link.available());
                pending.resize(link.read(pending.data(), pending.size()));
            }
            if (!pending.empty()) {
                const size_t n = link.write(pending.data(), pending.size());
                pending.erase(pending.begin(), pending.begin() + n);
            }
            wait = link.poll(nowMs());
        } else if (st == State::Established) {
            const uint32_t stall = stallUntil - nowMs();
            if (stall < wait) wait = stall;
        }

        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        const int r = ::poll(&pfd, 1, int(wait));
        if (r < 0 && errno != EINTR) {
            std::fprintf(stderr, "peer: poll error: %s\n", std::strerror(errno));
            return 1;
        }
        if (r <= 0) continue;

        uint8_t buf[1024];
        const ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            std::fprintf(stderr, "peer: read error: %s\n", std::strerror(errno));
            return 1;
        }
        if (n == 0) {
            // EOF: close the session gracefully and exit
            link.close();
            link.poll(nowMs());
            const Stats& s = link.stats();
            std::fprintf(stderr, "peer: exiting, tx_frames=%u rx_frames=%u rx_errors=%u retransmits=%u\n",
                         s.txFrames, s.rxFrames, s.rxErrors, s.retransmits);
            return 0;
        }
        link.onRx(buf, size_t(n), nowMs());
    }
}
