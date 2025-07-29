#include <Arduino_RouterBridge.h>


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

    if (!Bridge.begin()) {
        Serial.println("cannot setup Bridge");
    }

    if(!Monitor.begin()){
        Serial.println("cannot setup Monitor");
    }

    pinMode(LED_BUILTIN, OUTPUT);

    if (!Bridge.provide("set_led", set_led)) {
        Serial.println("Error providing method: set_led");
    } else {
        Serial.println("Registered method: set_led");
    }

    Bridge.provide("add", add);
    Bridge.provide("greet", greet);

}

void loop() {

    Bridge.notify("signal", 200);

    Monitor.println("DEBUG: a debug message");

    if (Monitor.available()) {
        String input = Monitor.readStringUntil('\n');  // Read until newline
        Monitor.print("You entered: ");
        Monitor.println(input);
    }

    Bridge.update();

    delay(500);
}