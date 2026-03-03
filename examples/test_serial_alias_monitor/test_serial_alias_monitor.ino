/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <Arduino_RouterBridge.h>

void setup() {
    //Bridge.begin();         // optional when Serial.begin is called
    Serial.begin(115200);     // same as Monitor.begin();
}

void loop() {
    Serial.println("### Serial as Monitor ALIAS Test ###");
    Serial.println("Serial.print and Monitor.print should write on the same stream");

    Serial.println("-- Serial --");
    delay(1000);
    Monitor.println("-- Monitor --");
}