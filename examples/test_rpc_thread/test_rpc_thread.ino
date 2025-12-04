#include <Arduino_RouterBridge.h>
#include <zephyr/kernel.h>

// Thread config
#define THREAD_STACK_SIZE 500
#define THREAD_PRIORITY   5

// 2nd thread definition
struct k_thread rpc_thread;
k_thread_stack_t *rpc_stack_area = k_thread_stack_alloc(THREAD_STACK_SIZE, 0);

// RPC call and its mutex
struct k_mutex mtx;


void rpc_thread_entry(void *p1, void *p2, void *p3) {
    RpcCall<MsgPack::str_t> *call  = reinterpret_cast<RpcCall<MsgPack::str_t>*>(p1);

    k_sleep(K_MSEC(400));

    Serial.println("\n*** Second Thread ***");
    Serial.println("*** Calling result() again...");

    k_mutex_lock(&mtx, K_FOREVER);

    MsgPack::str_t msg;
    bool ok = call->result(msg);

    if (ok) {
        Serial.println("*** Second call succeeded (unexpected!)");
        Serial.print("**** Message: ");
        Serial.println(msg.c_str());
    } else {
        Serial.println("*** Second call FAILED as expected (already executed)");
        Serial.print("*** Error Code: 0x");
        Serial.println(call->getErrorCode(), HEX);
        Serial.print("*** Error Message: ");
        Serial.println(call->getErrorMessage().c_str());
    }

    k_mutex_unlock(&mtx);

    Serial.println("*** Second Thread End ***\n");
}


void setup() {
    Serial.begin(115200);
    k_sleep(K_MSEC(2000));

    Serial.println("\n=== Threaded RPC Test ===\n");

    Bridge.begin();
    Monitor.begin();

    k_mutex_init(&mtx);

    // ---- First result() call in main thread ----
    Serial.println("--- First thread waits for the other side ---");
    k_sleep(K_MSEC(5000));
    Serial.println("--- First result() call (main thread) ---");
    RpcCall loopback_call = Bridge.call("loopback", "TEST");

    if (loopback_call.isError()) {
        Serial.println("--- Bridge call before execution");
        Serial.print("--- Error Code: 0x");
        Serial.println(loopback_call.getErrorCode(), HEX);
        Serial.print("--- Error Message: ");
        Serial.println(loopback_call.getErrorMessage().c_str());
    }

    MsgPack::str_t msg;
    k_mutex_lock(&mtx, K_FOREVER);
    bool ok = loopback_call.result(msg);
    k_mutex_unlock(&mtx);

    if (ok) {
        Serial.println("--- First call succeeded.");
        Serial.print("--- Message: ");
        Serial.println(msg.c_str());
    } else {
        Serial.println("--- First call FAILED (unexpected).");
    }

    // ---- Launch second thread ----
    Serial.println("\n--- Starting second thread...");

    k_tid_t rpc_tid = k_thread_create(
                                        &rpc_thread,
                                        rpc_stack_area,
                                        THREAD_STACK_SIZE,
                                        rpc_thread_entry,
                                        &loopback_call,
                                        NULL,
                                        NULL,
                                        THREAD_PRIORITY,
                                        0,
                                        K_NO_WAIT
                                    );

    k_thread_name_set(rpc_tid, "test");

    Serial.println("--- Second thread launched.\n");

    //k_thread_join(rpc_tid, K_NO_WAIT); // Works in ISRs only!
    k_sleep(K_MSEC(5000));

}

void loop() {
    k_sleep(K_MSEC(5000));
}
