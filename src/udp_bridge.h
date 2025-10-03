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

#include <api/Udp.h>

#define DEFAULT_UDP_BUF_SIZE    4096


template<size_t BufferSize=DEFAULT_UDP_BUF_SIZE>
class BridgeUDP final: public UDP {

    BridgeClass* bridge;
    uint32_t connection_id{};
    RingBufferN<BufferSize> temp_buffer;
    struct k_mutex udp_mutex{};
    bool _connected = false;

    uint16_t _port{}; // local port to listen on

    // Outbound packets target
    String _targetHost{};
    uint16_t _targetPort{};

    // Inbound packet info
    IPAddress _remoteIP{}; // remote IP address for the incoming packet whilst it's being processed
    uint16_t _remotePort{}; // remote port for the incoming packet whilst it's being processed
    uint16_t _remaining{}; // remaining bytes of incoming packet yet to be processed

public:

    explicit BridgeUDP(BridgeClass& bridge): bridge(&bridge) {}

    uint8_t begin(uint16_t port) override {

        if (!init()) {
            return 0;
        }

        k_mutex_lock(&udp_mutex, K_FOREVER);

        String hostname = "0.0.0.0";
        const bool ok = _connected || bridge->call(UDP_CONNECT_METHOD, hostname, port).result(connection_id);
        _connected = ok;
        if (_connected) _port = port;

        k_mutex_unlock(&udp_mutex);

        return ok? 1 : 0;
    }

    uint8_t beginMulticast(IPAddress ip, uint16_t port) override {

        if (!init()) {
            return 0;
        }

        k_mutex_lock(&udp_mutex, K_FOREVER);

        String hostname = ip.toString();
        const bool ok = _connected || bridge->call(UDP_CONNECT_MULTI_METHOD, hostname, port).result(connection_id);
        _connected = ok;
        if (_connected) _port = port;

        k_mutex_unlock(&udp_mutex);

        return ok? 1 : 0;
    }

    void stop() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);

        String msg;
        if (_connected) {
            _connected = !bridge->call(UDP_CLOSE_METHOD, connection_id).result(msg);
        }

        k_mutex_unlock(&udp_mutex);
    }

    int beginPacket(IPAddress ip, uint16_t port) override {
        return beginPacket(ip.toString().c_str(), port);
    }

    int beginPacket(const char *host, uint16_t port) override {

        k_mutex_lock(&udp_mutex, K_FOREVER);

        _targetHost = host;
        _targetPort = port;

        k_mutex_unlock(&udp_mutex);

        return 1;
    }

    int endPacket() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);

        _targetHost = "";
        _targetPort = 0;

        k_mutex_unlock(&udp_mutex);
        return 1;
    }

    size_t write(uint8_t c) override {
        return write(&c, 1);
    }

    size_t write(const uint8_t *buffer, size_t size) override {

        if (!connected()) return 0;

        MsgPack::arr_t<uint8_t> payload;

        for (size_t i = 0; i < size; ++i) {
            payload.push_back(buffer[i]);
        }

        size_t written;
        const bool ok = bridge->call(UDP_WRITE_METHOD, connection_id, _targetHost, _targetPort, payload).result(written);
        return ok? written : 0;
    }

    using Print::write;

    int parsePacket() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);

        while (_remaining) read();  // ensure previous packet is read

        int out = 0;
        if (available() >= 8) {
            uint8_t tmpBuf[8];
            for (size_t i = 0; i < 8; ++i) {
                tmpBuf[i] = temp_buffer.read_char();
            }
            _remoteIP = tmpBuf;
            _remotePort = tmpBuf[4];
            _remotePort = (_remotePort << 8) + tmpBuf[5];
            _remaining = tmpBuf[6];
            _remaining = (_remaining << 8) + tmpBuf[7];
            out = _remaining;
        }

        k_mutex_unlock(&udp_mutex);

        return out;
    }

    int available() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        const int size = temp_buffer.availableForStore();
        if (size > 0) _read(size);
        const int _available = temp_buffer.available();
        k_mutex_unlock(&udp_mutex);
        return _available;
    }

    int read() override {
        uint8_t c;
        read(&c, 1);
        return c;
    }

    // reading stops when the UDP package has been read completely (_remaining = 0)
    int read(unsigned char *buffer, size_t len) override {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        int i = 0;
        while (_remaining && i < len && available()) {
            buffer[i++] = temp_buffer.read_char();
            _remaining--;
        }
        k_mutex_unlock(&udp_mutex);
        return i;
    }

    int read(char *buffer, size_t len) override {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        int i = 0;
        while (_remaining && i < len && available()) {
            buffer[i++] = static_cast<char>(temp_buffer.read_char());
            _remaining--;
        }
        k_mutex_unlock(&udp_mutex);
        return i;
    }

    int peek() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        int out = -1;
        if (_remaining && temp_buffer.available()) {
            out = temp_buffer.peek();
        }
        k_mutex_unlock(&udp_mutex);
        return out;
    }

    void flush() override {
        // Implemented only when there's a TX buffer
    }

    IPAddress remoteIP() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        const IPAddress ip = _remoteIP;
        k_mutex_unlock(&udp_mutex);
        return ip;
    }

    uint16_t remotePort() override {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        const uint16_t port = _remotePort;
        k_mutex_unlock(&udp_mutex);
        return port;
    }

    bool connected() {
        k_mutex_lock(&udp_mutex, K_FOREVER);
        const bool ok = _connected;
        k_mutex_unlock(&udp_mutex);
        return ok;
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

        if (size == 0) return;

        k_mutex_lock(&udp_mutex, K_FOREVER);

        MsgPack::arr_t<uint8_t> message;
        RpcResult async_res = bridge->call(TCP_READ_METHOD, connection_id, size);
        const bool ret = _connected && async_res.result(message);

        if (ret) {
            for (size_t i = 0; i < message.size(); ++i) {
                temp_buffer.store_char(static_cast<char>(message[i]));
            }
        }

        if (async_res.error.code > NO_ERR) {
            _connected = false;
        }

        k_mutex_unlock(&udp_mutex);
    }

};

#endif //UDP_BRIDGE_H
