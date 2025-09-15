#include <Arduino_RouterBridge.h>

IPAddress localhost(127, 0, 0, 1);
BridgeTCPServer<> server(Bridge, localhost, 5678);

void setup() {
    Serial.begin(115200);

    if (!Bridge.begin()) {
        Serial.println("cannot setup Bridge");
        while (true) {}
    }

    server.begin();

}

void loop() {
    Serial.println("loop");

    BridgeTCPClient<> client = server.accept();

    if (client.connected() == 1){
        Serial.print("client ");
        Serial.print(client.getId());
        Serial.println(" connected");
    }

    if (client) {
        Serial.println("A client established a connection");
    }

    while (client.connected()) {
        size_t len = client.available();
        if (len) {
            Serial.println("Message received from client");
            uint8_t buff[len];
            client.read(buff, len);
            Serial.write(buff, len);
        }
    }

    server.disconnect();    // Disconnects the client server-side

}
