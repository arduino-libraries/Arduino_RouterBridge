#include <Arduino_BridgeImola.h>


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

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial1.begin(115200);
    while (!Serial1);
    
    pinMode(LED_BUILTIN, OUTPUT);

    if (!Bridge.begin()) {
        Serial.println("Error initializing Bridge");
        while(1);
    } else {
        Serial.println("Bridge initialized successfully");
    }

    if (!Bridge.provide("set_led", set_led)) {
        Serial.println("Error providing method: set_led");
    } else {
        Serial.println("Registered method: set_led");
    }

    Bridge.provide("add", add);

    Bridge.provide("greet", greet);

}

void loop() {
    float res;
    if (!Bridge.call("multiply", res, 1.0, 2.0)) {
        Serial.println("Error calling method: multiply");
        Serial.println(Bridge.get_error_code());
        Serial.println(Bridge.get_error_message());
        delay(1000);
    };

    Bridge.notify("signal", 200);

    //Bridge.update();
}
