In this repo it will be implemented an Arduino library wrapper for RPClite to be run on Imola boards.

The desired API is shown in https://github.com/bcmi-labs/Arduino_BridgeImola/blob/main/desired.ino.

This is WIP. Expects changes soon.

## The Bridge object ##

Including Arduino_BridgeImola.h gives the user access to a Bridge object that can be used both as a RPC client and/or server to execute and serve RPCs to/from the CPU Host running a GOLANG router.

- The Bridge object is pre-defined on Serial1 and automatically initialized inside the main setup()
- The Bridge.call method is blocking and works the same as in RPClite
- The Bridge can provide callbacks to incoming RPC requests both in a thread-unsafe and thread-safe fashion (provide & provide_safe)
- Thread-safe methods execution is granted in the main loop thread where update_safe is called. By design users cannot access .update_safe() freely
- Thread-unsafe methods are served in an update callback, whose execution is granted in a separate thread. Nonetheless users can access .update() freely with caution


```cpp
bool set_led(bool state) {
    digitalWrite(LED_BUILTIN, state);
    return state;
}

String greet() {
    return String("Hello Friend");
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    pinMode(LED_BUILTIN, OUTPUT);

    if (!Bridge.provide("set_led", set_led)) {
        Serial.println("Error providing method: set_led");
    } else {
        Serial.println("Registered method: set_led");
    }

    Bridge.provide_safe("greet", greet);

}

void loop() {
    float res;
    if (!Bridge.call("multiply", res, 1.0, 2.0)) {
        Serial.println("Error calling method: multiply");
        Serial.println(Bridge.get_error_code());
        Serial.println(Bridge.get_error_message());
    };

    Bridge.notify("signal", 200);

    //Bridge.update(); // Thread-unsafe update execution is granted in its own thread. It can be called manually with caution
}
```
