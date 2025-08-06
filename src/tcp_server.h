/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#pragma once

#ifndef BRIDGE_TCP_SERVER_H
#define BRIDGE_TCP_SERVER_H

#define TCP_LISTEN_METHOD           "tcp/listen"
#define TCP_ACCEPT_METHOD           "tcp/accept"

#include <api/Server.h>
#include <api/Client.h>
#include "bridge.h"
#include "tcp_client.h"

#define DEFAULT_TCP_SERVER_BUF_SIZE    512


template<size_t BufferSize=DEFAULT_TCP_SERVER_BUF_SIZE>
class BridgeTCPServer final: public Server {
    BridgeClass* bridge;
    IPAddress _addr{};
    uint16_t _port;
    bool _listening = false;
    uint32_t listener_id;
    struct k_mutex server_mutex{};

public:
    explicit BridgeTCPServer(BridgeClass& bridge, const IPAddress& addr, uint16_t port): bridge(&bridge), _addr(addr), _port(port) {}

    explicit BridgeTCPServer(BridgeClass& bridge, uint16_t port): bridge(&bridge), _addr(IP_ANY_TYPE), _port(port) {}

    bool begin() {
        k_mutex_init(&server_mutex);
        if (!(*bridge)) {
            if (!bridge->begin()) {
                return false;
            }
        }

        k_mutex_lock(&server_mutex, K_FOREVER);

        String conn_str = addr.toString() + String(_port);
        const bool ret = bridge->call(TCP_LISTEN_METHOD, listener_id, conn_str);
        // TODO is listener_id one per server obj?

        if (ret) {
            _listening = true;
        }

        k_mutex_unlock(&server_mutex);

        return ret;
    }

    void begin(uint16_t port) {
        _port = port;
        begin();
    }


    BridgeTCPClient<BufferSize> accept() {
        k_mutex_lock(&server_mutex, K_FOREVER);

        uint32_t connection_id = 0;
        const bool ret = bridge->call(TCP_ACCEPT_METHOD, connection_id, listener_id);

        k_mutex_unlock(&server_mutex);

        if (ret && connection_id != 0) {    // TODO is connection_id 0 acceptable???
            return BridgeTCPClient<BufferSize>(*bridge, connection_id);
        }

        // Return invalid client
        return BridgeTCPClient<BufferSize>(*bridge, 0);
    }

    size_t write(uint8_t c) override {
        return write(&c, 1);
    }

    size_t write(const uint8_t *buf, size_t size) override {
        // Broadcasting to all clients would require tracking them
        // For now, this is not implemented
        // TODO a logic to resolve which port-socket is the target of the write
        return 0;
    }

    bool is_listening() const {
        return _listening;
    }

    uint16_t getPort() const {
        return _port;
    }

    operator bool() const {
        return _listening;
    }

    using Print::write;

};

#endif //BRIDGE_TCP_SERVER_H