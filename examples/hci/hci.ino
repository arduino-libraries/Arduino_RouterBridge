/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <Arduino_RouterBridge.h>

void setup() {
    Serial1.begin(115200);

    Serial1.println("Arduino HCI Example - Read Local Version");

    if (!Bridge.begin()) {
        Serial1.println("Failed to setup Bridge");
        return;
    }

    if (!HCI.begin("hci0")) {
        Serial1.println("Failed to open HCI device");
        return;
    }

    Serial1.println("HCI device opened successfully");

    delay(1000);
    readLocalVersion();
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}

void readLocalVersion() {
    uint8_t cmd[4];
    cmd[0] = 0x01;  // HCI Command Packet Type
    cmd[1] = 0x01;  // OCF = 0x0001 (lower byte)
    cmd[2] = 0x10;  // OGF = 0x04 (0x04 << 2 = 0x10 in upper 6 bits)
    cmd[3] = 0x00;  // Parameter length = 0

    Serial1.println("Sending HCI Read Local Version command...");

    int sent = HCI.send(cmd, sizeof(cmd));
    if (sent < 0) {
        Serial1.println("Failed to send HCI command");
        return;
    }

    Serial1.print("Sent ");
    Serial1.print(sent);
    Serial1.println(" bytes");

    // Wait for response with timeout
    Serial1.println("Waiting for response...");
    int avail = 0;
    unsigned long startTime = millis();
    while (avail == 0 && (millis() - startTime) < 1000) {  // 1 second timeout
        avail = HCI.available();
        if (avail == 0) {
            delay(10);  // Small delay between polls
        }
    }

    Serial1.print("Available bytes: ");
    Serial1.println(avail);

    if (avail == 0) {
        Serial1.println("Timeout: No response received");
        return;
    }

    // Read response
    uint8_t response[255];
    int received = HCI.recv(response, sizeof(response));

    if (received > 0) {
        Serial1.print("Received ");
        Serial1.print(received);
        Serial1.println(" bytes:");

        // Print response in hex
        for (int i = 0; i < received; i++) {
            if (response[i] < 0x10) Serial1.print("0");
            Serial1.print(response[i], HEX);
            Serial1.print(" ");
        }
        Serial1.println();

        // Parse Command Complete Event
        // Event format: Packet Type, Event Code, Param Length, Num_HCI_Command_Packets, Opcode, Status, ...
        if (received >= 6 && response[0] == 0x04 && response[1] == 0x0E) {
            Serial1.println("Command Complete Event received");
            Serial1.print("Status: 0x");
            Serial1.println(response[6], HEX);
        }
    } else {
        Serial1.println("No response received");
    }
}
