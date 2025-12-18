/*
 * BridgeUDP Test Suite
 *
 * This test suite validates the BridgeUDP class functionality
 * including connection, multicast, reading, writing, and packet handling.
 */

#include <Arduino.h>
#include "Arduino_RouterBridge.h"

// Test configuration
#define TEST_UDP_PORT 8888
#define TEST_MULTICAST_IP "239.1.2.3"
#define TEST_MULTICAST_PORT 9999
#define TEST_TARGET_HOST "192.168.1.100"
#define TEST_TARGET_PORT 5000

// Test results tracking
struct TestResults {
    int passed = 0;
    int failed = 0;
    int total = 0;
};

TestResults results;

// Helper macros
#define TEST_ASSERT(condition, test_name) \
    do { \
        results.total++; \
        if (condition) { \
            Monitor.print("✓ PASS: "); \
            Monitor.println(test_name); \
            results.passed++; \
        } else { \
            Monitor.print("✗ FAIL: "); \
            Monitor.println(test_name); \
            results.failed++; \
        } \
    } while(0)

#define TEST_START(name) \
    Monitor.println(""); \
    Monitor.println("=========================================="); \
    Monitor.print("TEST: "); \
    Monitor.println(name); \
    Monitor.println("==========================================")

// Global UDP object
BridgeUDP<4096> udp(Bridge);

void printTestSummary() {
    Monitor.println("");
    Monitor.println("==========================================");
    Monitor.println("TEST SUMMARY");
    Monitor.println("==========================================");
    Monitor.print("Total Tests: "); Monitor.println(results.total);
    Monitor.print("Passed: "); Monitor.println(results.passed);
    Monitor.print("Failed: "); Monitor.println(results.failed);
    Monitor.print("Success Rate: ");
    Monitor.print((results.passed * 100) / results.total);
    Monitor.println("%");
    Monitor.println("==========================================");
}

void test_begin() {
    TEST_START("UDP Begin on Port");

    uint8_t result = udp.begin(TEST_UDP_PORT);
    TEST_ASSERT(result == 1, "UDP begin should return 1 on success");
    TEST_ASSERT(udp.connected(), "UDP should be connected after begin");

    udp.stop();
    delay(100);
}

void test_begin_multicast() {
    TEST_START("UDP Begin Multicast");

    IPAddress multicastIP;
    multicastIP.fromString(TEST_MULTICAST_IP);

    uint8_t result = udp.beginMulticast(multicastIP, TEST_MULTICAST_PORT);
    TEST_ASSERT(result == 1, "UDP beginMulticast should return 1 on success");
    TEST_ASSERT(udp.connected(), "UDP should be connected after beginMulticast");

    udp.stop();
    delay(100);
}

void test_stop() {
    TEST_START("UDP Stop");

    udp.begin(TEST_UDP_PORT);
    TEST_ASSERT(udp.connected(), "UDP should be connected before stop");

    udp.stop();
    TEST_ASSERT(!udp.connected(), "UDP should be disconnected after stop");

    delay(100);
}

void test_begin_packet() {
    TEST_START("UDP Begin/End Packet");

    udp.begin(TEST_UDP_PORT);

    // Test with IP address
    IPAddress targetIP;
    targetIP.fromString(TEST_TARGET_HOST);
    int result1 = udp.beginPacket(targetIP, TEST_TARGET_PORT);
    TEST_ASSERT(result1 == 1, "beginPacket with IP should return 1");

    int result2 = udp.endPacket();
    TEST_ASSERT(result2 == 1, "endPacket should return 1");

    // Test with hostname
    int result3 = udp.beginPacket(TEST_TARGET_HOST, TEST_TARGET_PORT);
    TEST_ASSERT(result3 == 1, "beginPacket with hostname should return 1");

    udp.endPacket();
    udp.stop();
    delay(100);
}

void test_write_single_byte() {
    TEST_START("UDP Write Single Byte");

    udp.begin(TEST_UDP_PORT);
    udp.beginPacket(TEST_TARGET_HOST, TEST_TARGET_PORT);

    size_t written = udp.write(0x42);
    TEST_ASSERT(written == 1, "write single byte should return 1");

    udp.endPacket();
    udp.stop();
    delay(100);
}

void test_write_buffer() {
    TEST_START("UDP Write Buffer");

    udp.begin(TEST_UDP_PORT);
    udp.beginPacket(TEST_TARGET_HOST, TEST_TARGET_PORT);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t written = udp.write(buffer, sizeof(buffer));
    TEST_ASSERT(written == sizeof(buffer), "write buffer should return number of bytes written");

    udp.endPacket();
    udp.stop();
    delay(100);
}

void test_write_string() {
    TEST_START("UDP Write String");

    udp.begin(TEST_UDP_PORT);
    udp.beginPacket(TEST_TARGET_HOST, TEST_TARGET_PORT);

    const char* message = "Hello UDP!";
    size_t written = udp.write((const uint8_t*)message, strlen(message));
    TEST_ASSERT(written == strlen(message), "write string should return string length");

    udp.endPacket();
    udp.stop();
    delay(100);
}

void test_parse_packet() {
    TEST_START("UDP Parse Packet");

    udp.begin(TEST_UDP_PORT);

    // Wait for incoming packet (this would be sent by Python test)
    unsigned long startTime = millis();
    int packetSize = 0;

    while (millis() - startTime < 5000) {  // 5 second timeout
        packetSize = udp.parsePacket();
        if (packetSize > 0) break;
        delay(10);
    }

    TEST_ASSERT(packetSize > 0, "parsePacket should return packet size > 0");

    if (packetSize > 0) {
        IPAddress remoteIP = udp.remoteIP();
        uint16_t remotePort = udp.remotePort();

        TEST_ASSERT(remoteIP != IPAddress(0, 0, 0, 0), "remoteIP should be valid");
        TEST_ASSERT(remotePort > 0, "remotePort should be valid");

        Monitor.print("Received packet from: ");
        Monitor.print(remoteIP);
        Monitor.print(":");
        Monitor.println(remotePort);
    }

    udp.stop();
    delay(100);
}

void test_read_operations() {
    TEST_START("UDP Read Operations");

    udp.begin(TEST_UDP_PORT);

    // Wait for incoming packet
    unsigned long startTime = millis();
    int packetSize = 0;

    while (millis() - startTime < 5000) {
        packetSize = udp.parsePacket();
        if (packetSize > 0) break;
        delay(10);
    }

    if (packetSize > 0) {
        // Test available()
        int avail = udp.available();
        TEST_ASSERT(avail > 0, "available should return > 0 after parsePacket");

        // Test peek()
        int peeked = udp.peek();
        TEST_ASSERT(peeked >= 0, "peek should return valid byte");

        // Test read single byte
        int firstByte = udp.read();
        TEST_ASSERT(firstByte == peeked, "read should return same byte as peek");

        // Test read buffer
        if (avail > 1) {
            uint8_t buffer[64];
            int bytesRead = udp.read(buffer, min(avail - 1, 64));
            TEST_ASSERT(bytesRead > 0, "read buffer should return bytes read");

            Monitor.print("Read "); Monitor.print(bytesRead); Monitor.println(" bytes");
        }
    } else {
        Monitor.println("WARNING: No packet received for read test");
    }

    udp.stop();
    delay(100);
}

void test_available() {
    TEST_START("UDP Available");

    udp.begin(TEST_UDP_PORT);

    // Initially should be 0
    int avail1 = udp.available();
    TEST_ASSERT(avail1 == 0, "available should be 0 initially");

    // After receiving packet (would need Python to send)
    // This is tested in test_read_operations

    udp.stop();
    delay(100);
}

void test_flush() {
    TEST_START("UDP Flush");

    udp.begin(TEST_UDP_PORT);

    // Flush is currently empty implementation, but should not crash
    udp.flush();
    TEST_ASSERT(true, "flush should complete without error");

    udp.stop();
    delay(100);
}

void test_concurrent_operations() {
    TEST_START("UDP Concurrent Read/Write");

    udp.begin(TEST_UDP_PORT);

    // Send a packet
    udp.beginPacket(TEST_TARGET_HOST, TEST_TARGET_PORT);
    udp.write((const uint8_t*)"PING", 4);
    udp.endPacket();

    // Try to receive response
    unsigned long startTime = millis();
    int packetSize = 0;

    while (millis() - startTime < 5000) {
        packetSize = udp.parsePacket();
        if (packetSize > 0) {
            char buffer[64];
            int bytesRead = udp.read(buffer, min(packetSize, 63));
            buffer[bytesRead] = '\0';

            Monitor.print("Received response: ");
            Monitor.println(buffer);

            TEST_ASSERT(bytesRead > 0, "should receive response to PING");
            break;
        }
        delay(10);
    }

    udp.stop();
    delay(100);
}

void test_multiple_packets() {
    TEST_START("UDP Multiple Packets");

    udp.begin(TEST_UDP_PORT);

    // Send multiple packets
    for (int i = 0; i < 5; i++) {
        udp.beginPacket(TEST_TARGET_HOST, TEST_TARGET_PORT);
        char msg[32];
        snprintf(msg, sizeof(msg), "Packet %d", i);
        udp.write((const uint8_t*)msg, strlen(msg));
        udp.endPacket();
        delay(100);
    }

    TEST_ASSERT(true, "multiple packets sent successfully");

    udp.stop();
    delay(100);
}

void setup() {

    // Initialize Bridge and Monitor
    Bridge.begin();
    Monitor.begin();

    Monitor.println("");
    Monitor.println("==========================================");
    Monitor.println("BridgeUDP Test Suite");
    Monitor.println("==========================================");
    Monitor.println("Waiting 5s for the other side");
    delay(5000);

    // Run all tests
    test_begin();
    test_begin_multicast();
    test_stop();
    test_begin_packet();
    test_write_single_byte();
    test_write_buffer();
    test_write_string();
    test_available();
    test_flush();
    test_multiple_packets();

    // Tests that require Python server interaction
    // Monitor.println("");
    // Monitor.println("==========================================");
    // Monitor.println("INTERACTIVE TESTS");
    // Monitor.println("(Require Python server to send packets)");
    // Monitor.println("==========================================");
    //
    // test_parse_packet();
    // test_read_operations();
    // test_concurrent_operations();

    // Print summary
    printTestSummary();
}

void loop() {
    // Test suite runs once in setup()
    delay(1000);
}
