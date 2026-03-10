/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <Arduino_RouterBridge.h>

IPAddress localhost(127, 0, 0, 1);
BridgeTCPServer<> server(Bridge, localhost, 5678);

void setup() {
    //Bridge.begin();   // optional when Serial.begin is called
    Serial.begin();     // same as Monitor.begin();
    server.begin();
}

void loop() {

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
