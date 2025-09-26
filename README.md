In this repo it will be implemented an Arduino library wrapper for RPClite to be run on Arduino UNO Q boards.

## The Bridge object ##

Including Arduino_RouterBridge.h gives the user access to a Bridge object that can be used both as a RPC client and/or server to execute and serve RPCs to/from the CPU Host running a GOLANG router.

- The Bridge object is pre-defined on Serial1 and automatically initialized inside the main setup()
- The Bridge.call method is blocking and works the same as in RPClite
- The Bridge can provide callbacks to incoming RPC requests both in a thread-unsafe and thread-safe fashion (provide & provide_safe)
- Thread-safe methods execution is granted in the main loop thread where update_safe is called. By design users cannot access .update_safe() freely
- Thread-unsafe methods are served in an update callback, whose execution is granted in a separate thread. Nonetheless users can access .update() freely with caution


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

    Bridge.begin();
    Monitor.begin();
    
    pinMode(LED_BUILTIN, OUTPUT);

    if (!Bridge.provide("set_led", set_led)) {
        Monitor.println("Error providing method: set_led");
    } else {
        Monitor.println("Registered method: set_led");
    }

    Bridge.provide_safe("greet", greet);

}

void loop() {
    float res;
    if (!Bridge.call("multiply", 1.0, 2.0).result(res)) {
        Monitor.println("Error calling method: multiply");
        Monitor.println(Bridge.get_error_code());
        Monitor.println(Bridge.get_error_message());
    };

    Bridge.notify("signal", 200);
}
```

## Best practices ##
Avoid catching Bridge call RpcResult without invoking its .result right away
```cpp
// OK
float out;
RpcResult res = Bridge.call("multiply", 1.0, 2.0);
res.result(out);
Monitor.println("TEST");

// NOT OK
//RpcResult res = Bridge.call("multiply", 1.0, 2.0);
//Monitor.println("TEST");
```
