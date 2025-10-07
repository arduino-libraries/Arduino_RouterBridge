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
#define TCP_CLOSE_LISTENER_METHOD   "tcp/closeListener"


#include <api/Server.h>
#include "bridge.h"
#include "tcp_client.h"

#define DEFAULT_TCP_SERVER_BUF_SIZE    512


template<size_t BufferSize=DEFAULT_TCP_SERVER_BUF_SIZE>
class BridgeTCPServer final: public Server {
    BridgeClass* bridge;
    IPAddress _addr{};
    uint16_t _port;
    bool _listening = false;
    uint32_t listener_id = 0;
    uint32_t connection_id = 0;
    bool _connected = false;
    struct k_mutex server_mutex{};

public:
    explicit BridgeTCPServer(BridgeClass& bridge, const IPAddress& addr, uint16_t port): bridge(&bridge), _addr(addr), _port(port) {}

    // explicit BridgeTCPServer(BridgeClass& bridge, uint16_t port): bridge(&bridge), _addr(INADDR_NONE), _port(port) {}

    void begin() override {
        k_mutex_init(&server_mutex);

        if (!(*bridge)) {
            while (!bridge->begin());
        }

        k_mutex_lock(&server_mutex, K_FOREVER);
        if (!_listening){
            String hostname = _addr.toString();
            _listening = bridge->call(TCP_LISTEN_METHOD, hostname, _port).result(listener_id);
        }
        k_mutex_unlock(&server_mutex);

    }

    BridgeTCPClient<BufferSize> accept() {

        k_mutex_lock(&server_mutex, K_FOREVER);

        if (!_listening) {  // Not listening -> return disconnected (invalid) client
            k_mutex_unlock(&server_mutex);
            return BridgeTCPClient<BufferSize>(*bridge, 0, false);
        }

        if (_connected) {   // Connection already established return a client copy
            k_mutex_unlock(&server_mutex);
            return BridgeTCPClient<BufferSize>(*bridge, connection_id);
        }

        // Accept a connection
        const bool ret = bridge->call(TCP_ACCEPT_METHOD, listener_id).result(connection_id);
        _connected = ret;

        k_mutex_unlock(&server_mutex);
        // If no connection established return a disconnected (invalid) client
        return ret? BridgeTCPClient<BufferSize>(*bridge, connection_id) : BridgeTCPClient<BufferSize>(*bridge, 0, false);
    }

    size_t write(uint8_t c) override {
        return write(&c, 1);
    }

    size_t write(const uint8_t *buf, size_t size) override {

        BridgeTCPClient<BufferSize> client = accept();
        if (!client) return 0;

        k_mutex_lock(&server_mutex, K_FOREVER);
        size_t written = 0;
        if (_connected) {
            written = client.write(buf, size);
        }
        k_mutex_unlock(&server_mutex);
        return written;
    }

    void close() {
        k_mutex_lock(&server_mutex, K_FOREVER);
        String msg;
        if (_listening){
            _listening = !bridge->call(TCP_CLOSE_LISTENER_METHOD, listener_id).result(msg);
            // Debug msg?
        }
        k_mutex_unlock(&server_mutex);
    }

    void disconnect() {
        k_mutex_lock(&server_mutex, K_FOREVER);
        _connected = false;
        connection_id = 0;
        k_mutex_unlock(&server_mutex);
    }

    bool is_listening() {
        k_mutex_lock(&server_mutex, K_FOREVER);
        bool out = _listening;
        k_mutex_unlock(&server_mutex);
        return out;
    }

    bool is_connected() {
        k_mutex_lock(&server_mutex, K_FOREVER);
        bool out = _connected;
        k_mutex_unlock(&server_mutex);
        return out;
    }

    uint16_t getPort() {
        k_mutex_lock(&server_mutex, K_FOREVER);
        uint16_t port = _port;
        k_mutex_unlock(&server_mutex);
        return port;
    }

    String getAddr() {
        k_mutex_lock(&server_mutex, K_FOREVER);
        String hostname = _addr.toString();
        k_mutex_unlock(&server_mutex);
        return hostname;
    }

    operator bool() const {
        return is_listening();
    }

    using Print::write;

};

#endif //BRIDGE_TCP_SERVER_H