#pragma once

#ifndef BRIDGE_IMOLA_H
#define BRIDGE_IMOLA_H

#include <Arduino_RPClite.h>


class Bridge: {

    RPCClient* client = nullptr;
    RPCServer* server = nullptr;
    ITransport* transport;

public:
    Bridge(ITransport& t) : transport(&t) {}
    Bridge(Stream& stream) : {
        transport = new SerialTransport(stream);
    }

    // Initialize the bridge
    void begin() {
        client = new RPCClient(*transport);
        server = new RPCServer(*transport);
    }

    template<typename F>
    bool provide(const MsgPack::str_t& name, F&& func){
        return server->bind(name, func);
    }

    void update() {
        server->run();
    }

    template<typename RType, typename... Args>
    bool call(const MsgPack::str_t method, RType& result, Args&&... args) {
        return client->call(method, result, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void notify(const MsgPack::str_t method, Args&&... args)  {
        client->notify(method, std::forward<Args>(args)...);
    }

    String get_error_message() const {
        return client->lastError.traceback;
    }

    uint8_t get_error_code() const {
        return (uint8_t) client->lastError.code;
    }

}


#endif // BRIDGE_IMOLA_H