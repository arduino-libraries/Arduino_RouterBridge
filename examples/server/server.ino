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

    Bridge.begin();
    Monitor.begin(115200);
    while (!Monitor) {}

    server.begin();

}

void loop() {

    BridgeTCPClient<> client = server.accept();

    if (client.connected() == 1){
        Monitor.print("client ");
        Monitor.print(client.getId());
        Monitor.println(" connected");
    }

    if (client) {
        Monitor.println("A client established a connection");
    }

    while (client.connected()) {
        size_t len = client.available();
        if (len) {
            Monitor.println("Message received from client");
            uint8_t buff[len];
            client.read(buff, len);
            Monitor.write(buff, len);
        }
    }

    server.disconnect();    // Disconnects the client server-side

}
