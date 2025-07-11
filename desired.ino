// Bridge.begin();   <- statica nel loader di zephyr / core
// -> rpc.call("$/reset", res);

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    if (!Bridge.provide("set_led", set_led)) {  // -> rpc.call("$/register", res, "set_led");
        // ERRORE!
    };
    Bridge.provide("add", add);         // -> rpc.call("$/register", res, "add");
    // ERRORE ignorato
    Bridge.provide("greet", greet);     // -> rpc.call("$/register", res, "greet");

    Bridge.provide("get_index", get_index);
    // metodi forniti con provide possono ricevere anche notifiche (semplicemente il risultato non viene restituito)

    Bridge.provide_safe("get_index", get_index);
}

int index;

int get_index() {
    return index;
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
    
    index++;

    float res;
    if (!Bridge.call("multiply", res, 1.0, 2.0)) {
        // ERRORE!
    };

    Bridge.notify("signal", 200);

    // update viene chiamato automaticamente in un thread separato
    Bridge.update(); // -> server->run();
}
