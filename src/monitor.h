#pragma once

#ifndef BRIDGE_MONITOR_H
#define BRIDGE_MONITOR_H

#include "bridge.h"

#define MON_CONNECTED_METHOD    "mon/connected"
#define MON_RESET_METHOD        "mon/reset"
#define MON_READ_METHOD         "mon/read"
#define MON_WRITE_METHOD        "mon/write"



class BridgeMonitor {

    BridgeClass& _bridge;

public:
    BridgeMonitor(BridgeClass& bridge): _bridge(bridge) {}

    bool begin() {
        return _bridge.call(MON_CONNECTED_METHOD, is_connected);
    }

    bool reset() {
        bool res;
        bool ok = _bridge.call(MON_RESET_METHOD, res);
        if (ok && res) {
            is_connected = false;
        }
        return (ok && res);
    }

    size_t write(String message) {
        size_t size;
        bool ok = _bridge.call(MON_WRITE_METHOD, size, message);

        if (!ok) return 0;
        
        return size;
    }

    bool read(String& message, size_t size) {
        return _bridge.call(MON_READ_METHOD, message, size);
    }

    bool is_connected = false;

};

extern BridgeClass Bridge;
BridgeMonitor Monitor(Bridge);

#endif // BRIDGE_MONITOR_H