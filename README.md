Arduino_RouterBridge is a zephyr RTOS multithreading wrapper of [RPClite](https://github.com/arduino-libraries/Arduino_RPClite), designed for Arduino UNO Q boards.

## The Bridge object ##

By including `Arduino_RouterBridge.h` the user gains access to a singleton `Bridge` object that can be used as an RPC client/server to execute and serve RPCs to/from the CPU Host running an [rpclib](http://rpclib.net/spec/) compatible Router.

- The `Bridge` object is defined over an UART port routed by the zephyr core
- The `Bridge.call` method is non-blocking and returns an RpcCall async object
- `RpcCall` class implements a blocking `.result` method that waits for the RPC response and returns true if the RPC returned with no errors
- `RpcCall.result` writes the return value of the remote call to the provided reference parameter. The result can be retrieved *exactly once*; subsequent calls to `.result` return an error.
- The `Bridge` can provide callbacks to incoming RPC requests both in a thread-unsafe and thread-safe fashion (by means of `BridgeClass::provide` and `BridgeClass::provide_safe`)
- Thread-safe methods execution is granted in the main loop thread where `update_safe` is called. By design, users cannot access `.update_safe()` freely
- Thread-unsafe methods are served in an update callback, whose execution is granted in a separate thread. Nonetheless, users can access `.update()` freely with caution

**Serial as Monitor ALIAS**

From Zephyr core ver. 0.53.2 onward, `Arduino_RouterBridge.h` is included by default.
In that core version, the API `Serial` object becomes an alias for `Monitor`, meaning that any stream on `Serial` 
is redirected to the `Bridge` and hence to the Router running on the CPU host.

```cpp
#include <Arduino_RouterBridge.h>

bool set_led(bool state) {
    digitalWrite(LED_BUILTIN, state);
    return state;
}

String greet() {
    return String("Hello Friend");
}

void setup() {

    Bridge.begin();   // optional when Serial.begin is called
    Serial.begin();     // same as Monitor.begin();

    pinMode(LED_BUILTIN, OUTPUT);

    if (!Bridge.provide("set_led", set_led)) {
        Serial.println("Error providing method: set_led");
    } else {
        Serial.println("Registered method: set_led");
    }

    Bridge.provide_safe("greet", greet);

}

void loop() {
    float sum;

    // CALL EXAMPLES

    // Standard chained call: Bridge.call("method", params...).result(res)
    if (!Bridge.call("add", 1.0, 2.0).result(sum)) {
        Serial.println("Error calling method: add");
    };

    // Async call
    RpcCall async_rpc = Bridge.call("add", 3.0, 4.5);
    if (!async_rpc.result(sum)) {
        Serial.println("Error calling method: add");
        Serial.print("Error code: ");
        Serial.println(async_rpc.getErrorCode());
        Serial.print("Error message: ");
        Serial.println(async_rpc.getErrorMessage());
    }

    // Implicit boolean cast. Use with caution as in this case the call is indeed
    // executed expecting a fallback nil result (MsgPack::object::nil_t)
    if (!Bridge.call("send_greeting", "Hello Friend")) {
        Serial.println("Error calling method: send_greeting");
    };

    // Please use notify when no reult (None, null, void, nil etc.) is expected from the opposite side
    // the following is executed immediately
    Bridge.notify("signal", 200);
}
```

**⚠️ Warning**

> Calling `Bridge.call` from within an RPC callback may cause an MCU–CPU IPC deadlock.