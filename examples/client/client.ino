/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <Arduino_RouterBridge.h>

BridgeTCPClient<> client(Bridge);

void setup() {
    //Bridge.begin();   // optional when Serial.begin is called
    Serial.begin();     // same as Monitor.begin();
}

void loop() {

    Serial.println("\nStarting connection to server...");

    if (client.connect("arduino.tips", 80) < 0) {
        Serial.println("unable to connect to server");
        return;
    }

    Serial.println("connected to server");

    size_t w = client.println("GET /asciilogo.txt HTTP/1.1");
    w += client.println("Host: arduino.tips");
    w += client.println("User-Agent: Arduino");
    w += client.println("Connection: close");
    w += client.println();

    while (client.connected()) {
        size_t len = client.available();
        if (len) {
            uint8_t buff[len];
            client.read(buff, len);
            Serial.write(buff, len);
        }
        delay(0);
    }

    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
    delay(1000);
}
