#pragma once

#ifndef BRIDGE_MONITOR_H
#define BRIDGE_MONITOR_H

#include <api/RingBuffer.h>
#include "bridge.h"

#define MON_CONNECTED_METHOD    "mon/connected"
#define MON_RESET_METHOD        "mon/reset"
#define MON_READ_METHOD         "mon/read"
#define MON_WRITE_METHOD        "mon/write"

#define DEFAULT_MONITOR_BUF_SIZE    512

template<size_t BufferSize=DEFAULT_MONITOR_BUF_SIZE>
class BridgeMonitor: public Stream {

private:
    BridgeClass& bridge;
    RingBufferN<BufferSize> buffer;
    bool is_connected = false;

public:
    BridgeMonitor(BridgeClass& bridge): bridge(bridge) {}

    bool begin() {
        return bridge.call(MON_CONNECTED_METHOD, is_connected);
    }

    bool isConnected() const {
        return is_connected;
    }

    int read() override {
        return 0;
    }

    int available() override {
        return 0;
    }

    int peek() override {
        return 0;
    }

    size_t write(uint8_t c) override {
        return 0;
    }

    bool reset() {
        bool res;
        bool ok = bridge.call(MON_RESET_METHOD, res);
        if (ok && res) {
            is_connected = false;
        }
        return (ok && res);
    }

    size_t write(String message) {
        size_t size;
        bool ok = bridge.call(MON_WRITE_METHOD, size, message);

        if (!ok) return 0;

        return size;
    }

    bool read(String& message, size_t size) {
        return bridge.call(MON_READ_METHOD, message, size);
    }


};

extern BridgeClass Bridge;
BridgeMonitor<> Monitor(Bridge);

#endif // BRIDGE_MONITOR_H