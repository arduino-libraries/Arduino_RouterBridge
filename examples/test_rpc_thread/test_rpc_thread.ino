#include <Arduino_RouterBridge.h>
#include <zephyr/kernel.h>

// Thread config
#define THREAD_STACK_SIZE 500
#define THREAD_PRIORITY   5


void rpc_thread_entry(void *p1, void *p2, void *p3) {
    RpcCall<MsgPack::str_t> *call  = reinterpret_cast<RpcCall<MsgPack::str_t>*>(p1);
    struct k_mutex *mtx = reinterpret_cast<struct k_mutex*>(p2);

    // Give setup() time to complete first result()
    k_sleep(K_MSEC(400));

    Serial.println("\n--- Second Thread ---");
    Serial.println("Calling result() again...");

    k_mutex_lock(mtx, K_FOREVER);

    MsgPack::str_t msg;
    bool ok = call->result(msg);

    if (ok) {
        Serial.println("ERR - Second call succeeded (unexpected!)");
        Serial.print("Message: ");
        Serial.println(msg.c_str());
    } else {
        Serial.println("OK - Second call FAILED as expected (already executed)");
        Serial.print("Error Code: 0x");
        Serial.println(call->getErrorCode(), HEX);
        Serial.print("Error Message: ");
        Serial.println(call->getErrorMessage().c_str());
    }

    k_mutex_unlock(mtx);

    Serial.println("--- Second Thread End ---\n");
}


void setup() {
    Serial.begin(115200);
    k_sleep(K_MSEC(2000));

    Serial.println("\n=== Threaded RPC Test ===\n");

    Serial.println("*** Main Thread (setup) ***");

    Bridge.begin();
    Monitor.begin();

    static struct k_mutex loop_mtx;
    k_mutex_init(&loop_mtx);

    RpcCall loopback_call = Bridge.call("loopback", "TEST");

    if (loopback_call.isError()) {
        Serial.println("OK - RPC call in Error mode before execution");
        Serial.print("Error Code: 0x");
        Serial.println(loopback_call.getErrorCode(), HEX);
        Serial.print("Error Message: ");
        Serial.println(loopback_call.getErrorMessage().c_str());
    } else {
        Serial.println("ERR - RPC call not in Error mode before execution (unexpected)");
    }

    Serial.println("Waiting for the other side...\n");
    delay(2000);

    Serial.println("calling .result() on RPC call (main thread)");

    MsgPack::str_t msg;
    k_mutex_lock(&loop_mtx, K_FOREVER);
    bool ok = loopback_call.result(msg);
    k_mutex_unlock(&loop_mtx);

    if (ok) {
        Serial.println("OK - First call succeeded.");
        Serial.print("Message: ");
        Serial.println(msg.c_str());
    } else {
        Serial.println("ERR - First call FAILED (unexpected).");
    }

    // ---- Launch second thread ----
    Serial.println("\nStarting second thread...");

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
    Serial.println("Second thread launched... joining");
    k_thread_join(&rpc_thread, K_FOREVER);
    Serial.println("*** Main thread end ending setup ***");

}

void loop() {
    k_sleep(K_MSEC(5000));
}
