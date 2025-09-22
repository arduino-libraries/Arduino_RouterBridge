/*
This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#pragma once

#ifndef UDP_BRIDGE_H
#define UDP_BRIDGE_H

#define UDP_CONNECT_METHOD          "udp/connect"
#define UDP_CONNECT_MULTI_METHOD    "udp/connectMulticast"
#define UDP_CLOSE_METHOD            "udp/close"
#define UDP_WRITE_METHOD            "udp/write"
#define UDP_READ_METHOD             "udp/read"

#include <zephyr/sys/atomic.h>
#include <api/Udp.h>

class BridgeUDP final: public UDP {

    BridgeClass* bridge;
    uint32_t connection_id{};
    RingBufferN<BufferSize> temp_buffer;
    struct k_mutex udp_mutex{};
    atomic_t _connected;

    uint16_t _port; // local port to listen on

    IPAddress _remoteIP; // remote IP address for the incoming packet whilst it's being processed
    uint16_t _remotePort; // remote port for the incoming packet whilst it's being processed
    uint16_t _offset; // offset into the packet being sent
    uint16_t _remaining; // remaining bytes of incoming packet yet to be processed

public:

    explicit BridgeUDP(BridgeClass& bridge): bridge(&bridge) {}

    uint8_t begin(uint16_t port) override {

        if (connected()) return 1;

        if (!init()) {
            return 0;
        }

        k_mutex_lock(&udp_mutex, K_FOREVER);
        _port = port;
        k_mutex_unlock(&udp_mutex);

        return 1;
    }

    uint8_t beginMulticast(IPAddress, uint16_t) override;

    void stop() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);

        String msg;
        const bool resp = bridge->call(UDP_CLOSE_METHOD, msg, connection_id);
        if (resp) {
            atomic_set(&_connected, 0);
        }

        k_mutex_unlock(&udp_mutex);
    }

    int beginPacket(IPAddress ip, uint16_t port) override {
        return beginPacket(ip.toString().c_str(), port);
    }

    int beginPacket(const char *host, uint16_t port) override {

        k_mutex_lock(&udp_mutex, K_FOREVER);

        String hostname = host;
        const bool resp = bridge->call(UDP_CONNECT_METHOD, connection_id, hostname, port);

        if (!resp) {
            atomic_set(&_connected, 0);
            k_mutex_unlock(&udp_mutex);
            return 0;
        }
        atomic_set(&_connected, 1);

        k_mutex_unlock(&udp_mutex);

        return 1;
    }

    int endPacket() override;

    size_t write(uint8_t c) override {
        return write(&c, 1);
    }

    size_t write(const uint8_t *buffer, size_t size) override;

    using Print::write;

    int parsePacket() override;

    int available() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        const int size = temp_buffer.availableForStore();
        if (size > 0) _read(size);
        const int _available = temp_buffer.available();
        k_mutex_unlock(&udp_mutex);
        return _available;
    }

    int read() override;

    int read(unsigned char *buffer, size_t len) override;

    int read(char *buffer, size_t len) override;

    int peek() override;

    void flush() override;

    IPAddress remoteIP() override;

    uint16_t remotePort() override;

    bool connected() const {
        return atomic_get(&_connected) > 0;
    }

private:

    bool init() {
        k_mutex_init(&udp_mutex);
        if (!(*bridge)) {
            return bridge->begin();
        }
        return true;
    }

    void _read(size_t size) {

        if (size == 0 || !connected()) return;

        k_mutex_lock(&udp_mutex, K_FOREVER);

        MsgPack::arr_t<uint8_t> message;
        const bool ret = bridge->call(TCP_READ_METHOD, message, connection_id, size);

        if (ret) {
            for (size_t i = 0; i < message.size(); ++i) {
                temp_buffer.store_char(static_cast<char>(message[i]));
            }
        }

        if (bridge->get_last_client_error().code > NO_ERR) {
            atomic_set(&_connected, 0);
        }

        k_mutex_unlock(&udp_mutex);
    }

};

#endif //UDP_BRIDGE_H
