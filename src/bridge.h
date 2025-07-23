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

public:

    BridgeClass(HardwareSerial& serial) {
        serial_ptr = &serial;
        transport = new SerialTransport(serial);
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
        return call(RESET_METHOD, res);
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

        k_msleep(1);
        // Lock read mutex
        k_mutex_lock(&read_mutex, K_FOREVER);
        if (!server->get_rpc()) {
            k_mutex_unlock(&read_mutex);
            return;
        }
        k_mutex_unlock(&read_mutex);

        server->process_request();

        // Lock write mutex
        k_mutex_lock(&write_mutex, K_FOREVER);
        server->send_response();
        k_mutex_unlock(&write_mutex);

    }

    template<typename RType, typename... Args>
    bool call(const MsgPack::str_t method, RType& result, Args&&... args) {

        // Lock write mutex
        k_mutex_lock(&write_mutex, K_FOREVER);
        client->send_rpc(method, std::forward<Args>(args)...);
        k_mutex_unlock(&write_mutex);

        // Lock read mutex
        while(1) {
            k_mutex_lock(&read_mutex, K_FOREVER);
            if (client->get_response(result)) {
                k_mutex_unlock(&read_mutex);
                break;
            }
            k_mutex_unlock(&read_mutex);
            k_msleep(1);
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
        k_msleep(1);
        k_mutex_lock(&read_mutex, K_FOREVER);
        if (!server->get_rpc()) {
            k_mutex_unlock(&read_mutex);
            return;
        }
        k_mutex_unlock(&read_mutex);

        server->process_request("__safe__");

        // Lock write mutex
        k_mutex_lock(&write_mutex, K_FOREVER);
        server->send_response();
        k_mutex_unlock(&write_mutex);

    }

    friend class BridgeClassUpdater;

};

class BridgeClassUpdater {
public:
    static void safeUpdate(BridgeClass& bridge) {
        bridge.update_safe();  // access private method
    }

private:
    BridgeClassUpdater() = delete; // prevents instantiation
};

BridgeClass Bridge(Serial1);

void updateEntryPoint(void *, void *, void *){

    Serial1.begin(115200);
    while (!Serial1);

    while(1){
        Bridge.update();
    }
}

static void safeUpdate(){
    BridgeClassUpdater::safeUpdate(Bridge);
}

void __loopHook(){
    safeUpdate();
}

#endif // ROUTER_BRIDGE_H