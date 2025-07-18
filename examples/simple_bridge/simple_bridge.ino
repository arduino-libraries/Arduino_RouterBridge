#include <Arduino_RouterBridge.h>

Bridge bridge(Serial0);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial0.begin(115200);
    while (!Serial0);
    
    pinMode(LED_BUILTIN, OUTPUT);

    if (!bridge.begin()) {
        Serial.println("Error initializing bridge");
        while(1);
    } else {
        Serial.println("Bridge initialized successfully");
    }

    if (!bridge.provide("set_led", set_led)) {
        Serial.println("Error providing method: set_led");
    } else {
        Serial.println("Registered method: set_led");
    }

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
        delay(1000);
    };

    bridge.notify("signal", 200);

    bridge.update();
}
