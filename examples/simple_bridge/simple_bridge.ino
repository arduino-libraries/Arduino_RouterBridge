#include <Arduino_BridgeImola.h>

Bridge bridge(Serial1);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial1.begin(115200);
    while (!Serial1);
    
    pinMode(LED_BUILTIN, OUTPUT);

    bridge.begin();

    if (!bridge.provide("set_led", set_led)) {
        Serial.println("Error providing method: set_led");
    };

    bridge.provide("add", add);

    bridge.provide("greet", greet);

}

bool set_led(bool state) {
    digitalWrite(LED_BUILTIN, state);
    return state;
}

int add(int a, int b) {
    return a + b;
}

String greet() {
    return String("Hello Friend");
}

void loop() {
    float res;
    if (!bridge.call("multiply", res, 1.0, 2.0)) {
        Serial.println("Error calling method: multiply");
        Serial.println(bridge.get_error_code());
        Serial.println(bridge.get_error_message());
    };

    bridge.notify("signal", 200);

    bridge.update();
}
