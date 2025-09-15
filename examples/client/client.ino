#include <Arduino_RouterBridge.h>

BridgeTCPClient<> client(Bridge);

void setup() {
    Serial.begin(115200);

    if (!Bridge.begin()) {
        Serial.println("cannot setup Bridge");
        while (true) {}
    }


}

void loop() {

    Serial.println("\nStarting connection to server...");
    /* if you get a connection, report back via serial: */
    if (client.connect("arduino.tips", 80) < 0) {
        Serial.println("unable to connect to server");
        return;
    }

    Serial.println("connected to server");
    /* Make an HTTP request: */
    size_t w = client.println("GET /asciilogo.txt HTTP/1.1");
    w += client.println("Host: arduino.tips");
    w += client.println("User-Agent: Arduino");
    w += client.println("Connection: close");
    w += client.println();

    /* if there are incoming bytes available from the server,
     * read them and print them:
     */
    while (client.connected()) {
        size_t len = client.available();
        if (len) {
            uint8_t buff[len];
            client.read(buff, len);
            Serial.write(buff, len);
        }
        delay(0);
    }

    /* if the server's disconnected, stop the client: */
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
    delay(1000);
}
