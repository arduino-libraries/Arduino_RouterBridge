/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <Arduino_RouterBridge.h>

void setup() {

    // This should be a harmless repeated .begin call
    Serial.begin(115200);
    Monitor.begin(115200);

    Serial.println("### Serial as Monitor ALIAS Test ###");

#ifdef ARDUINO_ROUTERBRIDGE_PROVIDES_SERIAL
    // The core defines this macro if it supports aliasing Serial.
    Serial.println("Serial.print and Monitor.print should write on the same stream.");
#else
    // Old cores do not provide support for Serial aliasing.
    Serial.println("NOTE: Please update the current core to support aliasing Serial."); 
    Serial.println("Serial.print and Monitor.print will write on separate streams.");
#endif

}

void loop() {

    Serial.println("-- Serial --");
    delay(1000);
    Monitor.println("-- Monitor --");
    delay(1000);

}
