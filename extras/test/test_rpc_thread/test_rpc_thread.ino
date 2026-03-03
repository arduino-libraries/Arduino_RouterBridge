/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (C) Arduino s.r.l. and/or its affiliated companies

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


#include <Arduino_RouterBridge.h>
#include <zephyr/kernel.h>

// Thread config
#define THREAD_STACK_SIZE 500
#define THREAD_PRIORITY   5


void rpc_thread_entry(void *p1, void *p2, void *p3) {
    (void)p3; // unused argument

    RpcCall<MsgPack::str_t> *call  = reinterpret_cast<RpcCall<MsgPack::str_t>*>(p1);
    struct k_mutex *mtx = reinterpret_cast<struct k_mutex*>(p2);

    // Give setup() time to complete first result()
    k_sleep(K_MSEC(400));

    Serial1.println("\n--- Second Thread ---");
    Serial1.println("Calling result() again...");

    k_mutex_lock(mtx, K_FOREVER);

    MsgPack::str_t msg;
    bool ok = call->result(msg);

    if (ok) {
        Serial1.println("ERR - Second call succeeded (unexpected!)");
        Serial1.print("Message: ");
        Serial1.println(msg.c_str());
    } else {
        Serial1.println("OK - Second call FAILED as expected (already executed)");
        Serial1.print("Error Code: 0x");
        Serial1.println(call->getErrorCode(), HEX);
        Serial1.print("Error Message: ");
        Serial1.println(call->getErrorMessage().c_str());
    }

    k_mutex_unlock(mtx);

    Serial1.println("--- Second Thread End ---\n");
}


void setup() {
    Serial1.begin(115200);          // Serial1 is used here for debugging so to not mess up with the Bridge
    k_sleep(K_MSEC(2000));

    Serial1.println("\n=== Threaded RPC Test ===\n");

    Serial1.println("*** Main Thread (setup) ***");

    Bridge.begin();

    static struct k_mutex loop_mtx;
    k_mutex_init(&loop_mtx);

    RpcCall loopback_call = Bridge.call("loopback", "TEST");

    if (loopback_call.isError()) {
        Serial1.println("OK - RPC call in Error mode before execution");
        Serial1.print("Error Code: 0x");
        Serial1.println(loopback_call.getErrorCode(), HEX);
        Serial1.print("Error Message: ");
        Serial1.println(loopback_call.getErrorMessage().c_str());
    } else {
        Serial1.println("ERR - RPC call not in Error mode before execution (unexpected)");
    }

    Serial1.println("Waiting for the other side...\n");
    delay(2000);

    Serial1.println("calling .result() on RPC call (main thread)");

    MsgPack::str_t msg;
    k_mutex_lock(&loop_mtx, K_FOREVER);
    bool ok = loopback_call.result(msg);
    k_mutex_unlock(&loop_mtx);

    if (ok) {
        Serial1.println("OK - First call succeeded.");
        Serial1.print("Message: ");
        Serial1.println(msg.c_str());
    } else {
        Serial1.println("ERR - First call FAILED (unexpected).");
    }

    // ---- Launch second thread ----
    Serial1.println("\nStarting second thread...");

    struct k_thread rpc_thread;

    k_thread_stack_t *rpc_stack_area = k_thread_stack_alloc(THREAD_STACK_SIZE, 0);

    k_tid_t rpc_tid = k_thread_create(
                                        &rpc_thread,
                                        rpc_stack_area,
                                        THREAD_STACK_SIZE,
                                        rpc_thread_entry,
                                        &loopback_call,   // p1 → RpcCall*
                                        &loop_mtx,        // p2 → mutex
                                        NULL,
                                        THREAD_PRIORITY,
                                        0,
                                        K_FOREVER
                                    );

    k_thread_start(rpc_tid);
    Serial1.println("Second thread launched... joining");
    k_thread_join(&rpc_thread, K_FOREVER);
    Serial1.println("*** Main thread end ending setup ***");

}

void loop() {
    k_sleep(K_MSEC(5000));
}
