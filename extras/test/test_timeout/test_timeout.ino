/*
This file is part of the Arduino_RPClite library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <Arduino_RouterBridge.h>

void setup() {
    Bridge.begin();
    //Bridge.setTimeout(0);
    Serial.begin(115200);
}

void loop()
{
    MsgPack::str_t message;

    if (Bridge.call("answer").result(message)) {
        Serial.println(message);

    } else {
        Serial.println("tO");
    }

    delay(5000);

    /*if (Bridge.setTimeout(0).call("answer").result(message))
    {
        Serial.print("No timeout: ");
        Serial.println(message);
    } else
    {
        Serial.println("No timeout error");
    }


    if (!Bridge.setTimeout(500).call("answer").result(message))
    {
        Serial.println("timeout");
        delay(1000);
    } else
    {
        Serial.println("odd");
        delay(1000);
    }*/

}