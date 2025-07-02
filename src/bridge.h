#pragma once

#ifndef BRIDGE_IMOLA_H
#define BRIDGE_IMOLA_H

#define RESET_METHOD "$/reset"
#define BIND_METHOD "$/register"

#include <Arduino_RPClite.h>


class Bridge {

    RPCClient* client = nullptr;
    RPCServer* server = nullptr;
    ITransport* transport;

public:
    Bridge(ITransport& t) : transport(&t) {}
    Bridge(Stream& stream) {
        transport = new SerialTransport(stream);
    }

    // Initialize the bridge
    bool begin() {
        client = new RPCClient(*transport);
        server = new RPCServer(*transport);
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

    void update() {
        // Protect the following calls with a mutex if necessary
        // server->read_request();  // <- inbound
        // server->serve();         // -> outbound
        server->run();
    }

    template<typename RType, typename... Args>
    bool call(const MsgPack::str_t method, RType& result, Args&&... args) {
        // Protect the following calls with a mutex if necessary
        // client->send_call();         // -> outbound
        // client->read_response();     // <- inbound
        return client->call(method, result, std::forward<Args>(args)...);
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

};

#endif // BRIDGE_IMOLA_H