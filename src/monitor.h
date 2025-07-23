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
    RingBufferN<BufferSize> temp_buffer;
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
        uint8_t c;
        read(&c, 1);
        return c;
    }

    int read(uint8_t* buffer, size_t size) {
        int i = 0;
        while (temp_buffer.available() && i < size) {
            buffer[i++] = temp_buffer.read_char();
        }
        return i;
    }

    int available() override {
        int size = temp_buffer.availableForStore();
        if (size > 0) _read(size);
        return temp_buffer.available();
    }

    int peek() override {
        if (temp_buffer.available()) {
            return temp_buffer.peek();
        }
    }

    size_t write(uint8_t c) override {
        return write(&c, 1);
    }

    size_t write(const uint8_t* buffer, size_t size) override {

        MsgPack::str_t send_buffer;

        for (size_t i = 0; i < size; ++i) {
#ifdef ARDUINO
            send_buffer += (char)buffer[i];
#else
            send_buffer.push_back(static_cast<char>(buffer[i]));
#endif
        }

        size_t written;
        bool ret = bridge.call(MON_WRITE_METHOD, written, send_buffer);
        if (ret) {
            return written;
        }

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

    int _read(size_t size) {

        if (size == 0) return 0;

        MsgPack::str_t message;
        bool ret = bridge.call(MON_READ_METHOD, message, size);

        if (ret) {
            for (size_t i = 0; i < message.length(); ++i) {
                 temp_buffer.store_char(message[i]);
            }
            return message.length();
        }

        // if (bridge.lastError.code > NO_ERR) {
        //     is_connected = false;
        // }

        return 0;

    }


};

extern BridgeClass Bridge;
BridgeMonitor<> Monitor(Bridge);

#endif // BRIDGE_MONITOR_H