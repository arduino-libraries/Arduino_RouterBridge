/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    The following example demonstrates async Bridge calls

*/

#include <Arduino_RouterBridge.h>


void mailme(String message) {
    Serial.print("received message: ");
    Serial.println(message);
}


void setup() {
    //Bridge.begin();   // optional when Serial.begin is called
    Serial.begin();     // same as Monitor.begin();
    Bridge.provide("mailme", mailme);
}


void loop() {

    float res;

    // Call with deferred response check
    RpcCall outcome = Bridge.call("multiply", 5.0, 7.0);
    Serial.println("async RPC called");

    delay(10);

    if (outcome.result(res)) {
        Serial.print("Result of the operation is: ");
        Serial.println(res);
    } else {
        Serial.println("Error calling method: multiply Python might not be ready yet");
        Serial.print("ERROR CODE: "); Serial.println(outcome.getErrorCode());
        Serial.print("ERROR MESSAGE: "); Serial.println(outcome.getErrorMessage());
    }

}
