/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    
*/

#pragma once

#ifndef ROUTER_BRIDGE_H
#define ROUTER_BRIDGE_H

#define RESET_METHOD "$/reset"
#define BIND_METHOD "$/register"

#define UPDATE_THREAD_STACK_SIZE    500
#define UPDATE_THREAD_PRIORITY      5

#define DEFAULT_SERIAL_BAUD         115200

#include <zephyr/kernel.h>
#include <Arduino_RPClite.h>


void updateEntryPoint(void *, void *, void *);

class BridgeClass {

    RPCClient* client = nullptr;
    RPCServer* server = nullptr;
    HardwareSerial* serial_ptr = nullptr;
    ITransport* transport;

    struct k_mutex read_mutex;
    struct k_mutex write_mutex;

    k_tid_t upd_tid;
    k_thread_stack_t *upd_stack_area;
    struct k_thread upd_thread_data;

    bool started = false;

public:

    BridgeClass(HardwareSerial& serial) {
        serial_ptr = &serial;
        transport = new SerialTransport(serial);
    }

    operator bool() const {
        return started;
    }

    // Initialize the bridge
    bool begin(unsigned long baud=DEFAULT_SERIAL_BAUD) {
        serial_ptr->begin(baud);

        k_mutex_init(&read_mutex);
        k_mutex_init(&write_mutex);

        client = new RPCClient(*transport);
        server = new RPCServer(*transport);

        upd_stack_area = k_thread_stack_alloc(UPDATE_THREAD_STACK_SIZE, 0);
        upd_tid = k_thread_create(&upd_thread_data, upd_stack_area,
                                UPDATE_THREAD_STACK_SIZE,
                                updateEntryPoint,
                                NULL, NULL, NULL,
                                UPDATE_THREAD_PRIORITY, 0, K_NO_WAIT);

        bool res;
        call(RESET_METHOD, res);
        if (res) {
            started = true;
        }
        return res;
    }

    template<typename F>
    bool provide(const MsgPack::str_t& name, F&& func) {
        bool res;
        if (!call(BIND_METHOD, res, name)) {
            return false;
        }
        return server->bind(name, func);
    }

    template<typename F>
    bool provide_safe(const MsgPack::str_t& name, F&& func) {
        bool res;
        if (!call(BIND_METHOD, res, name)) {
            return false;
        }

        return server->bind(name, func, "__safe__");
    
    }

    void update() {

        // Lock read mutex
        if (k_mutex_lock(&read_mutex, K_MSEC(10)) != 0 ) return;

        RPCRequest<> req;
        if (!server->get_rpc(req)) {
            k_mutex_unlock(&read_mutex);
            k_msleep(1);
            return;
        }

        k_mutex_unlock(&read_mutex);

        server->process_request(req);

        // Lock write mutex
        while (true) {
        
            if (k_mutex_lock(&write_mutex, K_MSEC(10)) == 0){
                server->send_response(req);
                k_mutex_unlock(&write_mutex);
                k_msleep(1);
                break;
            } else {
                k_msleep(1);
            }

        }

    }

    template<typename RType, typename... Args>
    bool call(const MsgPack::str_t method, RType& result, Args&&... args) {

        uint32_t msg_id_wait;

        // Lock write mutex
        while (true) {
            if (k_mutex_lock(&write_mutex, K_MSEC(10)) == 0) {
                client->send_rpc(method, msg_id_wait, std::forward<Args>(args)...);
                k_mutex_unlock(&write_mutex);
                k_msleep(1);
                break;
            } else {
                k_msleep(1);
            }
       }

        // Lock read mutex
        while(true) {
            if (k_mutex_lock(&read_mutex, K_MSEC(10)) == 0 ) {
                if (client->get_response(msg_id_wait, result)) {
                    k_mutex_unlock(&read_mutex);
                    k_msleep(1);
                    break;
                }
                k_mutex_unlock(&read_mutex);
                k_msleep(1);
            } else {
                k_msleep(1);
            }

        }

        return (client->lastError.code == NO_ERR);

    }

    template<typename... Args>
    void notify(const MsgPack::str_t method, Args&&... args)  {
        client->notify(method, std::forward<Args>(args)...);
    }

    String get_error_message() const {
        return (String) client->lastError.traceback;
    }

    uint8_t get_error_code() const {
        return (uint8_t) client->lastError.code;
    }

private:

    void update_safe() {

        // Lock read mutex
        if (k_mutex_lock(&read_mutex, K_MSEC(10)) != 0 ) return;

        RPCRequest<> req;
        if (!server->get_rpc(req, "__safe__")) {
            k_mutex_unlock(&read_mutex);
            k_msleep(1);
            return;
        }

        k_mutex_unlock(&read_mutex);

        server->process_request(req);

        // Lock write mutex
        while (true) {
        
            if (k_mutex_lock(&write_mutex, K_MSEC(10)) == 0){
                server->send_response(req);
                k_mutex_unlock(&write_mutex);
                k_msleep(1);
                break;
            } else {
                k_msleep(1);
            }

        }
    
    }

    friend class BridgeClassUpdater;

};

class BridgeClassUpdater {
public:
    static void safeUpdate(BridgeClass* bridge) {
        if (*bridge) {
            bridge->update_safe();
        }
    }

private:
    BridgeClassUpdater() = delete; // prevents instantiation
};

BridgeClass Bridge(Serial1);

void updateEntryPoint(void *, void *, void *){
    while (1) {
        if (Bridge) {
            Bridge.update();
        }
        k_msleep(1);
    }
}

static void safeUpdate(){
    BridgeClassUpdater::safeUpdate(&Bridge);
}

void __loopHook(){
    k_msleep(1);
    safeUpdate();
}

#endif // ROUTER_BRIDGE_H