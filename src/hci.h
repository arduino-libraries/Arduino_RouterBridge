/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#pragma once

#ifndef BRIDGE_HCI_H
#define BRIDGE_HCI_H

#include "bridge.h"

#define HCI_OPEN_METHOD     "hci/open"
#define HCI_CLOSE_METHOD    "hci/close"
#define HCI_SEND_METHOD     "hci/send"
#define HCI_RECV_METHOD     "hci/recv"
#define HCI_AVAIL_METHOD    "hci/avail"
#define HCI_BUFFER_SIZE     1024

template<size_t BufferSize=HCI_BUFFER_SIZE> class BridgeHCI {
    BridgeClass *bridge;
    struct k_mutex hci_mutex;
    bool initialized = false;

public:
    explicit BridgeHCI(BridgeClass &bridge): bridge(&bridge) {

    }

    bool begin(const char *device = "hci0") {
        k_mutex_init(&hci_mutex);

        if (!(*bridge) && !bridge->begin()) {
            return false;
        }

        bool result;
        if (bridge->call(HCI_OPEN_METHOD, String(device)).result(result)) {
            initialized = result;
        }

        return initialized;
    }

    void end() {
        if (!initialized) {
            return;
        }

        bool result;
        bridge->call(HCI_CLOSE_METHOD).result(result);
        initialized = false;
    }

    explicit operator bool() const {
        return initialized;
    }

    int send(const uint8_t *buffer, size_t size) {
        if (!initialized) {
            return -1;
        }

        k_mutex_lock(&hci_mutex, K_FOREVER);

        MsgPack::arr_t<uint8_t> send_buffer;
        for (size_t i = 0; i < size; ++i) {
            send_buffer.push_back(buffer[i]);
        }

        size_t bytes_sent;
        const bool ret = bridge->call(HCI_SEND_METHOD, send_buffer).result(bytes_sent);

        k_mutex_unlock(&hci_mutex);

        if (ret) {
            return bytes_sent;
        }
        return -1;
    }

    int recv(uint8_t *buffer, size_t max_size) {
        if (!initialized) {
            return -1;
        }

        k_mutex_lock(&hci_mutex, K_FOREVER);

        MsgPack::arr_t<uint8_t> message;
        bool ret = bridge->call(HCI_RECV_METHOD, max_size).result(message);

        if (ret) {
            size_t bytes_to_copy = message.size() < max_size ? message.size() : max_size;
            for (size_t i = 0; i < bytes_to_copy; ++i) {
                buffer[i] = message[i];
            }
            k_mutex_unlock(&hci_mutex);
            return bytes_to_copy;
        }

        k_mutex_unlock(&hci_mutex);
        return 0;
    }

    int available() {
        if (!initialized) {
            return 0;
        }

        k_mutex_lock(&hci_mutex, K_FOREVER);

        bool result;
        bool ret = bridge->call(HCI_AVAIL_METHOD).result(result);

        k_mutex_unlock(&hci_mutex);

        return ret && result;
    }

};

extern BridgeClass Bridge;

namespace RouterBridge {
    BridgeHCI<> HCI(Bridge);
}

// Make available in global namespace for backward compatibility
using RouterBridge::HCI;

#endif // BRIDGE_HCI_H
