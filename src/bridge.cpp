/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include "Arduino_RouterBridge.h"

bool BridgeClass::begin(unsigned long baud) {
    k_mutex_init(&read_mutex);
    k_mutex_init(&write_mutex);
    k_mutex_init(&bridge_mutex);

    if (is_started()) return true;

    k_mutex_lock(&bridge_mutex, K_FOREVER);

    serial_ptr->begin(baud);
    while (!*serial_ptr){
        k_yield();
    }
#if defined(ARDUINO_UNO_Q)
    // Flush broken RPCs from the previous run on hardware serial.
    serial_ptr->write("MCU starting RPC Bridge communication");
#endif
    transport = new SerialTransport(*serial_ptr);

    client = new RPCClient(*transport);
    server = new RPCServer(*transport);

    upd_stack_area = k_thread_stack_alloc(UPDATE_THREAD_STACK_SIZE, 0);
    upd_tid = k_thread_create(&upd_thread_data, upd_stack_area,
                            UPDATE_THREAD_STACK_SIZE,
                            updateEntryPoint,
                            NULL, NULL, NULL,
                            UPDATE_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(upd_tid, "bridge");

    bool res = false;
    started = call(RESET_METHOD).result(res) && res;
    notify(SET_BUF_METHOD, BRIDGE_RPC_BUFFER_SIZE);     // no effect if Router is not exposing SET_BUF_METHOD
    k_mutex_unlock(&bridge_mutex);
    return res;
}

bool BridgeClass::getRouterVersion(MsgPack::str_t& version) {
    return call(GET_VERSION_METHOD).result(version);
}

void BridgeClass::update() {

    // Lock read mutex
    if (k_mutex_lock(&read_mutex, K_MSEC(10)) != 0 ) return;

    RPCRequest<BRIDGE_RPC_BUFFER_SIZE> req;
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
            break;
        } else {
            k_yield();
        }

    }

}

void BridgeClass::update_safe() {

    // Lock read mutex
    if (k_mutex_lock(&read_mutex, K_MSEC(10)) != 0 ) return;

    RPCRequest<BRIDGE_RPC_BUFFER_SIZE> req;
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
            break;
        } else {
            k_yield();
        }

    }

}

void updateEntryPoint(void *, void *, void *){
    while (true) {
        if (Bridge) {
            Bridge.update();
        }
        k_yield();
    }
}
