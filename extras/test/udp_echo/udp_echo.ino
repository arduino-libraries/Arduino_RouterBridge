/*
 * UDP Echo Test
 * 
 * This sketch tests the BridgeUDP class by sending packets to an echo server
 * and receiving the responses. It demonstrates basic UDP send/receive operations.
 * 
 * Hardware Requirements:
 * - Arduino board with Bridge support
 * - Serial connection for Bridge communication
 * 
 * Server Requirements:
 * - Run the Python UDP Bridge server (udp_bridge_server.py)
 * - Server includes echo functionality on port 5000
 */

#include <Arduino.h>
#include "Arduino_RouterBridge.h"

// Configuration
#define LOCAL_UDP_PORT 8888           // Local port to listen on
#define ECHO_SERVER_HOST "127.0.0.1"  // Echo server address (use actual IP if remote)
#define ECHO_SERVER_PORT 5000         // Echo server port
#define TEST_INTERVAL 5000            // Time between tests (ms)

// Create UDP object
BridgeUDP<4096> udp(Bridge);

// Test statistics
unsigned long testsRun = 0;
unsigned long testsSucceeded = 0;
unsigned long testsFailed = 0;

void setup() {
    
    // Initialize Bridge and Monitor
    Bridge.begin();
    Monitor.begin();

    Monitor.println("\n===========================================");
    Monitor.println("UDP Echo Test");
    Monitor.println("===========================================");

    Monitor.println("Waiting 5s for the other side");
    delay(5000);
    
    // Start UDP
    Monitor.print("Starting UDP on port ");
    Monitor.print(LOCAL_UDP_PORT);
    Monitor.print("... ");
    
    if (udp.begin(LOCAL_UDP_PORT)) {
        Monitor.println("Connected");
    } else {
        Monitor.println("FAILED!");
        Monitor.println("ERROR: Cannot start UDP");
        while (1) {
            delay(1000);
        }
    }
    
    Monitor.println("\nConfiguration:");
    Monitor.print("  Local Port: ");
    Monitor.println(LOCAL_UDP_PORT);
    Monitor.print("  Echo Server: ");
    Monitor.print(ECHO_SERVER_HOST);
    Monitor.print(":");
    Monitor.println(ECHO_SERVER_PORT);
    Monitor.println("\n===========================================");
    Monitor.println("Starting echo tests...\n");
    
    delay(1000);
}

void loop() {
    // Run echo test
    performEchoTest();
    
    // Print statistics
    printStatistics();
    
    // Wait before next test
    delay(TEST_INTERVAL);
}

void performEchoTest() {
    testsRun++;
    
    Monitor.println("-------------------------------------------");
    Monitor.print("Test #");
    Monitor.println(testsRun);
    Monitor.println("-------------------------------------------");
    
    // Prepare test message
    char message[64];
    snprintf(message, sizeof(message), "Echo test #%lu", testsRun);
    
    Monitor.print("Sending: \"");
    Monitor.print(message);
    Monitor.println("\"");
    
    // Send UDP packet
    udp.beginPacket(ECHO_SERVER_HOST, ECHO_SERVER_PORT);
    size_t written = udp.write((const uint8_t*)message, strlen(message));
    udp.endPacket();
    
    if (written != strlen(message)) {
        Monitor.print("WARNING: Only ");
        Monitor.print(written);
        Monitor.print(" bytes written out of ");
        Monitor.println(strlen(message));
    }
    
    Monitor.print("Sent ");
    Monitor.print(written);
    Monitor.println(" bytes");
    
    // Wait for response
    Monitor.println("Waiting for echo response...");
    
    unsigned long startTime = millis();
    unsigned long timeout = 5000; // 5 second timeout
    bool responseReceived = false;
    
    while (millis() - startTime < timeout) {
        int packetSize = udp.parsePacket();
        
        if (packetSize > 0) {
            responseReceived = true;
            
            Monitor.print("Received ");
            Monitor.print(packetSize);
            Monitor.print(" bytes from ");
            
            IPAddress remoteIP = udp.remoteIP();
            Monitor.print(remoteIP);
            Monitor.print(":");
            Monitor.println(udp.remotePort());
            
            // Read the response
            char buffer[256];
            int bytesRead = udp.read(buffer, min(packetSize, 255));
            buffer[bytesRead] = '\0';
            
            Monitor.print("Response: \"");
            Monitor.print(buffer);
            Monitor.println("\"");
            
            // Check if it's an echo
            if (strstr(buffer, message) != NULL) {
                Monitor.println("✓ Echo verified - TEST PASSED");
                testsSucceeded++;
            } else {
                Monitor.println("✗ Response doesn't match - TEST FAILED");
                testsFailed++;
            }
            
            break;
        }
        
        delay(10);
    }
    
    if (!responseReceived) {
        Monitor.println("✗ No response received - TEST FAILED (TIMEOUT)");
        testsFailed++;
    }
    
    Monitor.println();
}

void printStatistics() {
    Monitor.println("===========================================");
    Monitor.println("Test Statistics:");
    Monitor.println("===========================================");
    Monitor.print("Total Tests: ");
    Monitor.println(testsRun);
    Monitor.print("Passed: ");
    Monitor.print(testsSucceeded);
    Monitor.print(" (");
    if (testsRun > 0) {
        Monitor.print((testsSucceeded * 100) / testsRun);
    } else {
        Monitor.print("0");
    }
    Monitor.println("%)");
    Monitor.print("Failed: ");
    Monitor.print(testsFailed);
    Monitor.print(" (");
    if (testsRun > 0) {
        Monitor.print((testsFailed * 100) / testsRun);
    } else {
        Monitor.print("0");
    }
    Monitor.println("%)");
    Monitor.println("===========================================\n");
}
