/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

// Test of the Bridge over the seriallink protocol (BridgeProtocol::Reliable).
// The router must run with --serial-protocol=reliable (or auto).
//
// Provided methods:
//   seriallink/echo(bin) -> bin      returns its argument
//   seriallink/stats() -> [session, tx_frames, rx_frames, rx_errors, retransmits, mcu_calls_ok, mcu_calls_failed]
//
// The MCU also calls the router ($/version) every 100ms, to test the calls in
// the MCU -> router direction, counting successes and failures.

#include <Arduino_RouterBridge.h>

static uint32_t calls_ok = 0;
static uint32_t calls_failed = 0;

MsgPack::bin_t<uint8_t> echo(MsgPack::bin_t<uint8_t> data) {
    return data;
}

MsgPack::arr_t<uint32_t> stats() {
    MsgPack::arr_t<uint32_t> out;
    SerialLinkTransport* link = Bridge.getLinkTransport();
    if (link == nullptr) return out;
    const seriallink::Stats s = link->stats();
    out.push_back(link->session());
    out.push_back(s.txFrames);
    out.push_back(s.rxFrames);
    out.push_back(s.rxErrors);
    out.push_back(s.retransmits);
    out.push_back(calls_ok);
    out.push_back(calls_failed);
    return out;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Bridge.begin(115200, BridgeProtocol::Reliable);
    Bridge.provide("seriallink/echo", echo);
    Bridge.provide("seriallink/stats", stats);
}

void loop() {
    MsgPack::str_t version;
    if (Bridge.getRouterVersion(version)) {
        calls_ok++;
    } else {
        calls_failed++;
    }
    digitalWrite(LED_BUILTIN, (calls_ok / 10) % 2);
    delay(100);
}
