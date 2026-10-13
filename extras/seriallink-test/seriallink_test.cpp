/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

// Host (PC) unit tests for the seriallink protocol core. Build and run with:
//
//   make -C extras/seriallink-test

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "../../src/seriallink/cobs.h"
#include "../../src/seriallink/crc32.h"
#include "../../src/seriallink/frame.h"
#include "../../src/seriallink/link.h"

using namespace seriallink;
using Bytes = std::vector<uint8_t>;

static int failures = 0;
static const char* currentTest = "";

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            std::printf("  FAIL %s (%s:%d): %s\n", currentTest, __FILE__, __LINE__, #cond); \
            failures++;                                                                   \
        }                                                                                 \
    } while (0)

static Bytes seqBytes(int from, int to) {
    Bytes b;
    for (int i = from; i <= to; i++) b.push_back(uint8_t(i));
    return b;
}

static Bytes concat(std::initializer_list<Bytes> parts) {
    Bytes out;
    for (const auto& p : parts) out.insert(out.end(), p.begin(), p.end());
    return out;
}

static Bytes randomData(uint32_t seed, size_t size) {
    std::mt19937 rng(seed);
    Bytes b(size);
    for (auto& x : b) x = rng() % 3 ? uint8_t(rng()) : 0; // plenty of zeros, as in msgpack
    return b;
}

// --- CRC, COBS and frames ---------------------------------------------------

static void testCRC32() {
    const char* s = "123456789";
    CHECK(crc32(reinterpret_cast<const uint8_t*>(s), 9) == 0xCBF43926);
    CHECK(crc32(nullptr, 0) == 0);
    // Incremental computation
    uint32_t c = crc32(reinterpret_cast<const uint8_t*>(s), 4);
    CHECK(crc32(reinterpret_cast<const uint8_t*>(s) + 4, 5, c) == 0xCBF43926);
}

static Bytes cobsEncode(const Bytes& src) {
    Bytes dst(cobs::maxEncodedLen(src.size()));
    dst.resize(cobs::encode(dst.data(), src.data(), src.size()));
    return dst;
}

static bool cobsDecode(const Bytes& src, Bytes& out) {
    out.resize(src.size());
    size_t n;
    if (!cobs::decode(out.data(), out.size(), src.data(), src.size(), n)) return false;
    out.resize(n);
    return true;
}

static void testCOBS() {
    // Same vectors as the Go internal/cobs package
    struct { Bytes decoded, encoded; } vectors[] = {
        {{}, {0x01}},
        {{0x00}, {0x01, 0x01}},
        {{0x00, 0x00}, {0x01, 0x01, 0x01}},
        {{0x00, 0x11, 0x00}, {0x01, 0x02, 0x11, 0x01}},
        {{0x11, 0x22, 0x00, 0x33}, {0x03, 0x11, 0x22, 0x02, 0x33}},
        {{0x11, 0x22, 0x33, 0x44}, {0x05, 0x11, 0x22, 0x33, 0x44}},
        {{0x11, 0x00, 0x00, 0x00}, {0x02, 0x11, 0x01, 0x01, 0x01}},
        {seqBytes(0x01, 0xFE), concat({{0xFF}, seqBytes(0x01, 0xFE)})},
        {seqBytes(0x00, 0xFE), concat({{0x01, 0xFF}, seqBytes(0x01, 0xFE)})},
        {seqBytes(0x01, 0xFF), concat({{0xFF}, seqBytes(0x01, 0xFE), {0x02, 0xFF}})},
        {concat({seqBytes(0x02, 0xFF), {0x00}}), concat({{0xFF}, seqBytes(0x02, 0xFF), {0x01, 0x01}})},
        {concat({seqBytes(0x03, 0xFF), {0x00, 0x01}}), concat({{0xFE}, seqBytes(0x03, 0xFF), {0x02, 0x01}})},
    };
    for (const auto& v : vectors) {
        CHECK(cobsEncode(v.decoded) == v.encoded);
        Bytes dec;
        CHECK(cobsDecode(v.encoded, dec) && dec == v.decoded);
    }

    for (uint32_t seed = 0; seed < 500; seed++) {
        Bytes src = randomData(seed, seed * 3);
        Bytes enc = cobsEncode(src);
        CHECK(std::find(enc.begin(), enc.end(), 0) == enc.end());
        CHECK(enc.size() <= cobs::maxEncodedLen(src.size()));
        Bytes dec;
        CHECK(cobsDecode(enc, dec) && dec == src);
    }

    Bytes dec;
    for (const Bytes& bad : std::vector<Bytes>{{0x05, 0x01, 0x02}, {0x02, 0x00}, {0x00}, {0xFF, 0x01}, {0x02, 0x11, 0x03, 0x22}}) {
        CHECK(!cobsDecode(bad, dec));
    }
    // Output buffer too small
    size_t n;
    uint8_t small[2];
    const uint8_t enc[] = {0x04, 0x11, 0x22, 0x33};
    CHECK(!cobs::decode(small, sizeof(small), enc, sizeof(enc), n));
}

static Bytes encodeFrame(const Frame& f) {
    Bytes raw(rawFrameSize(f.len)), dst(maxEncodedFrameSize(f.len));
    dst.resize(encodeFrame(f, raw.data(), dst.data()));
    return dst;
}

static void testFrameGolden() {
    // Frames encoded by the Go implementation (internal/seriallink)
    uint8_t p[SYN_ACK_SIZE];

    SynParams syn{256, 8, 0xDEADBEEF, 0};
    size_t n = marshalSyn(syn, false, p);
    CHECK(encodeFrame({FrameType::Syn, 0, 0, 0, p, n}) ==
          Bytes({0x02, 0x14, 0x01, 0x01, 0x01, 0x0B, 0x01, 0x08, 0xEF, 0xBE, 0xAD, 0xDE, 0xC2, 0x21, 0x9A, 0xB5, 0x00}));

    SynParams synAck{128, 4, 0x01020304, 0xDEADBEEF};
    n = marshalSyn(synAck, true, p);
    CHECK(encodeFrame({FrameType::SynAck, 0, 0, 0, p, n}) ==
          Bytes({0x02, 0x15, 0x01, 0x01, 0x02, 0x80, 0x0E, 0x04, 0x04, 0x03, 0x02, 0x01, 0xEF, 0xBE, 0xAD, 0xDE, 0x6F, 0x65, 0x2F, 0x74, 0x00}));

    const uint8_t data[] = {0x94, 0x00, 0x01, 0x00, 0xA4, 'p', 'i', 'n', 'g', 0x90};
    const Bytes dataEnc = {0x06, 0x11, 0x07, 0xC8, 0x03, 0x94, 0x02, 0x01, 0x0B, 0xA4, 0x70, 0x69, 0x6E, 0x67, 0x90, 0xD7, 0xC1, 0x5E, 0x39, 0x00};
    CHECK(encodeFrame({FrameType::Data, 7, 200, 3, data, sizeof(data)}) == dataEnc);

    CHECK(encodeFrame({FrameType::Ack, 0, 5, 8, nullptr, 0}) == Bytes({0x02, 0x12, 0x07, 0x05, 0x08, 0x7F, 0x3C, 0xF8, 0xA8, 0x00}));
    CHECK(encodeFrame({FrameType::Nak, 0, 255, 0, nullptr, 0}) == Bytes({0x02, 0x13, 0x02, 0xFF, 0x05, 0x1F, 0xDA, 0xCC, 0xF0, 0x00}));

    putUint32(p, 0xDEADBEEF);
    CHECK(encodeFrame({FrameType::Rst, 0, 0, 0, p, RST_SIZE}) ==
          Bytes({0x02, 0x16, 0x01, 0x01, 0x09, 0xEF, 0xBE, 0xAD, 0xDE, 0xC6, 0x58, 0xE8, 0xE4, 0x00}));

    // A payload longer than a COBS block
    Bytes big(300, 0x55);
    Bytes bigEnc = encodeFrame({FrameType::Data, 1, 2, 3, big.data(), big.size()});
    CHECK(bigEnc.size() == 311);
    CHECK(Bytes(bigEnc.begin(), bigEnc.begin() + 6) == Bytes({0xFF, 0x11, 0x01, 0x02, 0x03, 0x55}));
    CHECK(Bytes(bigEnc.end() - 5, bigEnc.end()) == Bytes({0x44, 0x6F, 0xAA, 0xAE, 0x00}));

    // Decoding
    Bytes raw(rawFrameSize(big.size()));
    Frame f;
    CHECK(decodeFrame(dataEnc.data(), dataEnc.size() - 1, raw.data(), raw.size(), f));
    CHECK(f.type == FrameType::Data && f.seq == 7 && f.ack == 200 && f.wnd == 3);
    CHECK(f.len == sizeof(data) && memcmp(f.payload, data, sizeof(data)) == 0);
    CHECK(decodeFrame(bigEnc.data(), bigEnc.size() - 1, raw.data(), raw.size(), f) && f.len == 300);

    // Every single bit flip and every truncation must be detected
    for (size_t i = 0; i < dataEnc.size() - 1; i++) {
        for (int bit = 0; bit < 8; bit++) {
            Bytes bad(dataEnc.begin(), dataEnc.end() - 1);
            bad[i] ^= 1 << bit;
            CHECK(!decodeFrame(bad.data(), bad.size(), raw.data(), raw.size(), f));
        }
        CHECK(!decodeFrame(dataEnc.data(), i, raw.data(), raw.size(), f));
    }
}

static void testSynParams() {
    uint8_t p[SYN_ACK_SIZE];
    SynParams in{256, 8, 0xDEADBEEF, 0x12345678}, out;
    CHECK(parseSyn(p, marshalSyn(in, true, p), true, out));
    CHECK(out.maxPayload == 256 && out.window == 8 && out.nonce == 0xDEADBEEF && out.echo == 0x12345678);
    CHECK(parseSyn(p, marshalSyn(in, false, p), false, out) && out.echo == 0);
    CHECK(!parseSyn(p, SYN_SIZE, true, out));
    SynParams bad{256, 200, 1, 0};
    CHECK(!parseSyn(p, marshalSyn(bad, false, p), false, out));
}

// --- Link simulation ----------------------------------------------------------

// One direction of a simulated serial line, with fault injection and a
// throughput limit (115200 baud are about 11.5 bytes/ms).
struct Wire {
    Bytes buf;
    std::mt19937 rng;
    double flip = 0, drop = 0, dup = 0, garbage = 0;
    bool blackhole = false;

    explicit Wire(uint32_t seed) : rng(seed) {}

    double rand01() { return std::uniform_real_distribution<double>(0, 1)(rng); }

    void send(const uint8_t* data, size_t len) {
        if (blackhole) return;
        for (size_t i = 0; i < len; i++) {
            uint8_t b = data[i];
            if (rand01() < garbage) buf.push_back(uint8_t(rng()));
            if (rand01() < drop) continue;
            if (rand01() < flip) b ^= 1 << (rng() % 8);
            buf.push_back(b);
            if (rand01() < dup) buf.push_back(b);
        }
    }

    static void output(void* ctx, const uint8_t* data, size_t len) {
        static_cast<Wire*>(ctx)->send(data, len);
    }
};

static Timing testTiming() {
    Timing t;
    t.maxRetries = 20;
    return t;
}

struct Sim {
    static constexpr size_t BYTES_PER_MS = 12;
    uint32_t now = 0x7FFFF000; // close to a wrap around of int32 to test the timers arithmetic
    Wire ab{1}, ba{2};
    Link a, b;

    explicit Sim(const Timing& timing = testTiming())
        : a(Wire::output, &ab, timing), b(Wire::output, &ba, timing) {
        a.begin(0x11111111, now);
        b.begin(0x22222222, now);
    }

    void setFaults(double flip, double drop, double dup, double garbage) {
        for (Wire* w : {&ab, &ba}) {
            w->flip = flip;
            w->drop = drop;
            w->dup = dup;
            w->garbage = garbage;
        }
    }

    static void deliver(Wire& w, Link& dst, uint32_t now) {
        const size_t n = std::min(w.buf.size(), BYTES_PER_MS);
        if (n == 0) return;
        dst.onRx(w.buf.data(), n, now);
        w.buf.erase(w.buf.begin(), w.buf.begin() + n);
    }

    // Advances the simulation by 1ms.
    void step() {
        now++;
        deliver(ab, b, now);
        deliver(ba, a, now);
        a.poll(now);
        b.poll(now);
    }

    // Runs until cond() is true, for at most maxMs simulated milliseconds.
    bool runUntil(const std::function<bool()>& cond, uint32_t maxMs) {
        for (uint32_t i = 0; i < maxMs; i++) {
            if (cond()) return true;
            step();
        }
        return cond();
    }

    void runFor(uint32_t ms) {
        for (uint32_t i = 0; i < ms; i++) step();
    }

    bool established() {
        return runUntil([&] { return a.state() == State::Established && b.state() == State::Established; }, 5000);
    }
};

// Pumps data from src to dst through the simulation, returns true if it
// arrived intact within maxMs.
struct Transfer {
    Link& src;
    Link& dst;
    Bytes data;
    size_t sent = 0;
    Bytes received;

    Transfer(Link& src, Link& dst, Bytes data) : src(src), dst(dst), data(std::move(data)) {}

    void pump() {
        if (sent < data.size()) sent += src.write(data.data() + sent, data.size() - sent);
        uint8_t buf[100];
        while (size_t n = dst.read(buf, sizeof(buf))) received.insert(received.end(), buf, buf + n);
    }
    bool done() const { return received.size() >= data.size(); }
};

static bool exchange(Sim& sim, size_t size, uint32_t maxMs) {
    Transfer t1{sim.a, sim.b, randomData(10, size)};
    Transfer t2{sim.b, sim.a, randomData(20, size)};
    bool ok = sim.runUntil([&] {
        t1.pump();
        t2.pump();
        return (t1.done() && t2.done()) || sim.a.state() != State::Established || sim.b.state() != State::Established;
    }, maxMs);
    return ok && t1.received == t1.data && t2.received == t2.data;
}

static void testCleanLine() {
    Sim sim;
    CHECK(sim.established());
    CHECK(sim.a.maxPayload() == SERIALLINK_MAX_PAYLOAD && sim.a.sendWindow() == SERIALLINK_WINDOW);
    CHECK(exchange(sim, 32 * 1024, 20000));
    CHECK(sim.a.stats().rxErrors == 0 && sim.b.stats().rxErrors == 0);
    CHECK(sim.a.stats().retransmits == 0 && sim.b.stats().retransmits == 0);

    // Idle: keep-alives keep the link up
    sim.runFor(20000);
    CHECK(sim.a.state() == State::Established && sim.b.state() == State::Established);
}

static void testNoisyLine() {
    Sim sim;
    sim.setFaults(5e-4, 2e-4, 2e-4, 2e-4);
    CHECK(sim.established());
    CHECK(exchange(sim, 32 * 1024, 120000));
    CHECK(sim.a.stats().rxErrors + sim.b.stats().rxErrors > 0);
    CHECK(sim.a.stats().retransmits + sim.b.stats().retransmits > 0);
    std::printf("  noisy line: A rx_err=%u retx=%u, B rx_err=%u retx=%u\n",
                sim.a.stats().rxErrors, sim.a.stats().retransmits, sim.b.stats().rxErrors, sim.b.stats().retransmits);
}

static void testPeerStartsLater() {
    Sim sim;
    sim.b.close(); // B is not running yet (never established, so no RST is sent)
    sim.runFor(3000);
    CHECK(sim.a.state() == State::Connecting);
    CHECK(sim.a.write(reinterpret_cast<const uint8_t*>("x"), 1) == 0);
    sim.b.begin(0x33333333, sim.now);
    CHECK(sim.established());
    CHECK(exchange(sim, 4000, 10000));
}

static void testFlowControl() {
    Sim sim;
    CHECK(sim.established());
    Transfer t{sim.a, sim.b, randomData(1, 20 * SERIALLINK_WINDOW * SERIALLINK_MAX_PAYLOAD)};

    // B does not read for longer than the link timeout
    for (int i = 0; i < 10000; i++) {
        if (t.sent < t.data.size()) t.sent += sim.a.write(t.data.data() + t.sent, t.data.size() - t.sent);
        sim.step();
    }
    CHECK(sim.a.state() == State::Established && sim.b.state() == State::Established);
    CHECK(t.sent < t.data.size());
    CHECK(sim.a.writable() == 0);
    CHECK(sim.b.available() == SERIALLINK_WINDOW * SERIALLINK_MAX_PAYLOAD);
    CHECK(sim.a.stats().retransmits == 0);

    CHECK(sim.runUntil([&] { t.pump(); return t.done(); }, 20000));
    CHECK(t.received == t.data);
}

static void testPeerClose() {
    Sim sim;
    CHECK(sim.established());
    CHECK(exchange(sim, 1000, 5000));
    sim.b.close();
    CHECK(sim.runUntil([&] { return sim.a.state() == State::Closed; }, 100));
    CHECK(sim.a.error() == Error::SessionReset);
    CHECK(sim.b.error() == Error::Closed);
}

static void testPeerReboot(uint32_t newNonce) {
    Sim sim;
    CHECK(sim.established());
    CHECK(exchange(sim, 1000, 5000));

    // B reboots without notifying A
    sim.b.begin(newNonce, sim.now);
    CHECK(sim.runUntil([&] { return sim.a.state() == State::Closed; }, 1000));
    CHECK(sim.a.error() == Error::SessionReset);

    // A restarts too and the new session works
    sim.a.begin(0x44444444, sim.now);
    CHECK(sim.established());
    CHECK(exchange(sim, 10000, 10000));
}

static void testStaleSession() {
    Sim sim;
    CHECK(sim.established());
    CHECK(exchange(sim, 1000, 5000));

    // A restarts while A->B is down, so B misses the SYN and keeps talking
    // in the old session: A must reset it with a RST.
    sim.ab.blackhole = true;
    sim.a.begin(0x55555555, sim.now);
    sim.runFor(50);
    sim.ab.blackhole = false;
    sim.b.write(reinterpret_cast<const uint8_t*>("old session"), 11);
    CHECK(sim.runUntil([&] { return sim.b.state() == State::Closed; }, 1000));
    CHECK(sim.b.error() == Error::SessionReset);

    sim.b.begin(0x66666666, sim.now);
    CHECK(sim.established());
    CHECK(exchange(sim, 10000, 10000));
}

static void testLinkTimeout() {
    Sim sim;
    CHECK(sim.established());
    CHECK(exchange(sim, 1000, 5000));
    sim.ab.blackhole = sim.ba.blackhole = true;
    const uint32_t start = sim.now;
    CHECK(sim.runUntil([&] { return sim.a.state() == State::Closed; }, 10000));
    CHECK(sim.a.error() == Error::LinkTimeout);
    CHECK(sim.now - start <= testTiming().linkTimeoutMs + 10);
}

static void testRetriesExhausted() {
    // With the default timing the retries (10 x 250ms) are exhausted before
    // the peer link timeout (4s)
    const Timing timing;
    Sim sim(timing);
    CHECK(sim.established());
    // A keeps receiving B's keep-alives but its data never reaches B
    sim.ab.blackhole = true;
    sim.a.write(reinterpret_cast<const uint8_t*>("lost"), 4);
    const uint32_t start = sim.now;
    CHECK(sim.runUntil([&] { return sim.a.state() == State::Closed; }, 10000));
    CHECK(sim.a.error() == Error::LinkTimeout);
    CHECK(sim.now - start < timing.linkTimeoutMs);
}

static void testInvalidInput() {
    Sim sim;
    CHECK(sim.established());
    // Garbage, an over-long frame and an unknown version are dropped
    Bytes junk = randomData(5, 2000);
    for (auto& x : junk) x |= 1; // no delimiters
    junk.push_back(0);
    sim.b.onRx(junk.data(), junk.size(), sim.now);
    const uint8_t v2[] = {0x02, 0x21, 0x01, 0x01, 0x01, 0x05, 0x11, 0x22, 0x33, 0x44, 0x00};
    sim.b.onRx(v2, sizeof(v2), sim.now);
    CHECK(sim.b.stats().rxErrors == 2);
    CHECK(sim.b.state() == State::Established);
    CHECK(exchange(sim, 1000, 5000));
}

int main() {
    struct { const char* name; std::function<void()> fn; } tests[] = {
        {"CRC32", testCRC32},
        {"COBS", testCOBS},
        {"FrameGolden", testFrameGolden},
        {"SynParams", testSynParams},
        {"CleanLine", testCleanLine},
        {"NoisyLine", testNoisyLine},
        {"PeerStartsLater", testPeerStartsLater},
        {"FlowControl", testFlowControl},
        {"PeerClose", testPeerClose},
        {"PeerReboot", [] { testPeerReboot(0x77777777); }},
        {"PeerRebootSameNonce", [] { testPeerReboot(0x22222222); }},
        {"StaleSession", testStaleSession},
        {"LinkTimeout", testLinkTimeout},
        {"RetriesExhausted", testRetriesExhausted},
        {"InvalidInput", testInvalidInput},
    };
    for (const auto& t : tests) {
        currentTest = t.name;
        const int before = failures;
        t.fn();
        std::printf("%s %s\n", failures == before ? "PASS" : "FAIL", t.name);
    }
    std::printf(failures ? "\n%d check(s) FAILED\n" : "\nall tests passed\n", failures);
    return failures ? 1 : 0;
}
