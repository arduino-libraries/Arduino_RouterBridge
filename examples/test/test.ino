/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    
*/

// to run this test example, launch server_test in a shell and then this sketch


/* available registers 
    register("ping")
    register("0_args_no_result")
    register("1_args_no_result")
    register("2_args_no_result")
    register("0_args_bool_result")
    register("1_args_bool_result")
    register("2_args_bool_result")
*/

#include "Arduino_RouterBridge.h"

bool x;
int i=0;

void setup() {
   Bridge.begin();
   Monitor.begin();
}

void loop() {

    //testing monitor
    Monitor.println(String(i)+"\t"+String(x));
    i++;
    //working
    Bridge.call("0_args_no_result");

    if (Bridge.call("0_args_no_result")){
      Monitor.println("ok");                                  // return true because no result
    }
    else{
      Monitor.println("no");
    }

    if (Bridge.call("0_args_bool_result")){
      Monitor.println("ok");
    }
    else{
      Monitor.println("no");                                  // return false because you need check the result
    }

    x=false;
    if (Bridge.call("0_args_bool_result").result(x)){
      Monitor.println("ok "+String(x));                       // return true - the perfect call
    }
    else{
      Monitor.println("no "+String(x));
    }


    //avoid to do followings

    RpcResult result = Bridge.call("0_args_bool_result");    // the call happens but you won't get the result

    bool x = false;
    RpcResult result2 = Bridge.call("0_args_bool_result"); 
    result2.result(x);
    Monitor.println("Result: "+String(x));                   // return true, so the right result

    delay(1000);
}