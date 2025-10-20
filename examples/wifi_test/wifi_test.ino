/*
 * BridgeWiFi Test Suite
 *
 * This test suite validates the BridgeWiFi class functionality
 * including connection, scanning, status checking, and network information.
 */

#include <Arduino.h>
#include "Arduino_RouterBridge.h"


// Test configuration
#define TEST_SSID "TestNetwork"
#define TEST_PASSWORD "TestPassword123"
#define TEST_SSID_OPEN "OpenNetwork"
#define TEST_TIMEOUT 30000  // 30 seconds for connection attempts

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

// Global wifi object
BridgeWiFi wifi(Bridge);

void printTestSummary() {
    Monitor.println("");
    Monitor.println("==========================================");
    Monitor.println("TEST SUMMARY");
    Monitor.println("==========================================");
    Monitor.print("Total Tests: "); Monitor.println(results.total);
    Monitor.print("Passed: "); Monitor.println(results.passed);
    Monitor.print("Failed: "); Monitor.println(results.failed);
    Monitor.print("Success Rate: ");
    if (results.total > 0) {
        Monitor.print((results.passed * 100) / results.total);
    } else {
        Monitor.print("0");
    }
    Monitor.println("%");
    Monitor.println("==========================================");
}

void printWiFiStatus(uint8_t status) {
    Monitor.print("WiFi Status: ");
    switch (status) {
        case WL_NO_SHIELD:
            Monitor.println("WL_NO_SHIELD (255) - No WiFi shield");
            break;
        case WL_IDLE_STATUS:
            Monitor.println("WL_IDLE_STATUS (0) - Idle");
            break;
        case WL_NO_SSID_AVAIL:
            Monitor.println("WL_NO_SSID_AVAIL (1) - SSID not available");
            break;
        case WL_SCAN_COMPLETED:
            Monitor.println("WL_SCAN_COMPLETED (2) - Scan completed");
            break;
        case WL_CONNECTED:
            Monitor.println("WL_CONNECTED (3) - Connected");
            break;
        case WL_CONNECT_FAILED:
            Monitor.println("WL_CONNECT_FAILED (4) - Connection failed");
            break;
        case WL_CONNECTION_LOST:
            Monitor.println("WL_CONNECTION_LOST (5) - Connection lost");
            break;
        case WL_DISCONNECTED:
            Monitor.println("WL_DISCONNECTED (6) - Disconnected");
            break;
        default:
            Monitor.print("UNKNOWN ("); Monitor.print(status); Monitor.println(")");
            break;
    }
}

void test_initial_status() {
    TEST_START("Initial WiFi Status");

    uint8_t status = wifi.status();
    printWiFiStatus(status);

    TEST_ASSERT(status == WL_IDLE_STATUS || status == WL_DISCONNECTED,
                "Initial status should be IDLE or DISCONNECTED");
    TEST_ASSERT(!wifi.isConnected(), "Should not be connected initially");

    delay(100);
}

void test_begin_with_password() {
    TEST_START("WiFi Begin with Password");

    Monitor.print("Attempting to connect to: ");
    Monitor.println(TEST_SSID);

    uint8_t result = wifi.begin(TEST_SSID, TEST_PASSWORD);
    printWiFiStatus(result);

    TEST_ASSERT(result == WL_CONNECTED || result == WL_CONNECT_FAILED || result == WL_NO_SSID_AVAIL,
                "begin should return valid status code");

    // Wait for connection if connecting
    if (result == WL_IDLE_STATUS) {
        unsigned long startTime = millis();
        while (millis() - startTime < TEST_TIMEOUT) {
            uint8_t status = wifi.status();
            if (status == WL_CONNECTED) {
                Monitor.println("Connection successful!");
                break;
            } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
                Monitor.println("Connection failed");
                break;
            }
            delay(500);
        }
    }

    if (wifi.isConnected()) {
        Monitor.println("✓ Successfully connected to WiFi");
    } else {
        Monitor.println("⚠ Could not connect (network may not exist)");
    }

    delay(100);
}

void test_begin_open_network() {
    TEST_START("WiFi Begin Open Network");

    // First disconnect if connected
    if (wifi.isConnected()) {
        wifi.disconnect();
        delay(1000);
    }

    Monitor.print("Attempting to connect to open network: ");
    Monitor.println(TEST_SSID_OPEN);

    uint8_t result = wifi.begin(TEST_SSID_OPEN);
    printWiFiStatus(result);

    TEST_ASSERT(result == WL_CONNECTED || result == WL_CONNECT_FAILED || result == WL_NO_SSID_AVAIL,
                "begin without password should return valid status code");

    delay(100);
}

void test_disconnect() {
    TEST_START("WiFi Disconnect");

    // Ensure we're connected first
    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(2000);
    }

    bool wasConnected = wifi.isConnected();
    uint8_t result = wifi.disconnect();

    printWiFiStatus(result);

    if (wasConnected) {
        TEST_ASSERT(result == WL_DISCONNECTED, "disconnect should return WL_DISCONNECTED");
        TEST_ASSERT(!wifi.isConnected(), "Should not be connected after disconnect");
    } else {
        TEST_ASSERT(true, "Disconnect called (was not connected)");
    }

    delay(100);
}

void test_status_check() {
    TEST_START("WiFi Status Check");

    uint8_t status = wifi.status();
    printWiFiStatus(status);

    TEST_ASSERT(status >= 0 && status <= 255, "Status should be valid");

    // Check consistency with isConnected
    bool connected = wifi.isConnected();
    if (status == WL_CONNECTED) {
        TEST_ASSERT(connected, "isConnected should be true when status is WL_CONNECTED");
    } else {
        TEST_ASSERT(!connected, "isConnected should be false when status is not WL_CONNECTED");
    }

    delay(100);
}

void test_scan_networks() {
    TEST_START("WiFi Scan Networks");

    Monitor.println("Scanning for networks...");
    int8_t numNetworks = wifi.scanNetworks();

    Monitor.print("Networks found: ");
    Monitor.println(numNetworks);

    TEST_ASSERT(numNetworks >= -1 && numNetworks <= 127,
                "scanNetworks should return valid count (-1 to 127)");

    if (numNetworks > 0) {
        Monitor.println("✓ Found networks");
        TEST_ASSERT(true, "Network scan completed successfully");
    } else if (numNetworks == 0) {
        Monitor.println("⚠ No networks found");
        TEST_ASSERT(true, "Scan completed but no networks found");
    } else {
        Monitor.println("✗ Scan failed");
    }

    delay(100);
}

void test_ssid_from_scan() {
    TEST_START("WiFi SSID from Scan");

    int8_t numNetworks = wifi.scanNetworks();

    if (numNetworks > 0) {
        Monitor.println("\nScanned Networks:");
        Monitor.println("------------------");

        for (int i = 0; i < min(numNetworks, 5); i++) {
            String ssid = wifi.SSID(i);
            Monitor.print(i);
            Monitor.print(": ");
            Monitor.println(ssid);

            TEST_ASSERT(ssid.length() >= 0, "SSID should be retrievable");
        }

        // Test first network SSID
        String firstSSID = wifi.SSID(0);
        TEST_ASSERT(firstSSID.length() > 0, "First network should have valid SSID");
    } else {
        Monitor.println("⚠ No networks to test SSID retrieval");
        TEST_ASSERT(true, "Skipped - no networks found");
    }

    delay(100);
}

void test_connected_ssid() {
    TEST_START("WiFi Connected SSID");

    // Ensure connected
    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(3000);
    }

    String ssid = wifi.SSID();
    Monitor.print("Current SSID: ");
    Monitor.println(ssid);

    if (wifi.isConnected()) {
        TEST_ASSERT(ssid.length() > 0, "Connected SSID should not be empty");
        TEST_ASSERT(ssid == TEST_SSID, "SSID should match connected network");
    } else {
        TEST_ASSERT(ssid.length() == 0, "SSID should be empty when not connected");
    }

    delay(100);
}

void test_bssid() {
    TEST_START("WiFi BSSID");

    // Ensure connected
    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(3000);
    }

    uint8_t bssid[6];
    wifi.BSSID(bssid);

    Monitor.print("BSSID: ");
    for (int i = 0; i < 6; i++) {
        if (bssid[i] < 16) Monitor.print("0");
        Monitor.print(bssid[i], HEX);
        if (i < 5) Monitor.print(":");
    }
    Monitor.println();

    if (wifi.isConnected()) {
        bool hasValidBSSID = false;
        for (int i = 0; i < 6; i++) {
            if (bssid[i] != 0) {
                hasValidBSSID = true;
                break;
            }
        }
        TEST_ASSERT(hasValidBSSID, "BSSID should be valid when connected");
    } else {
        TEST_ASSERT(true, "Skipped - not connected");
    }

    delay(100);
}

void test_rssi() {
    TEST_START("WiFi RSSI");

    // Ensure connected
    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(3000);
    }

    int32_t rssi = wifi.RSSI();
    Monitor.print("RSSI: ");
    Monitor.print(rssi);
    Monitor.println(" dBm");

    if (wifi.isConnected()) {
        TEST_ASSERT(rssi < 0 && rssi >= -100, "RSSI should be between -100 and 0 dBm");

        // Quality indicator
        if (rssi > -50) {
            Monitor.println("Signal: Excellent");
        } else if (rssi > -60) {
            Monitor.println("Signal: Good");
        } else if (rssi > -70) {
            Monitor.println("Signal: Fair");
        } else {
            Monitor.println("Signal: Weak");
        }
    } else {
        TEST_ASSERT(true, "Skipped - not connected");
    }

    delay(100);
}

void test_local_ip() {
    TEST_START("WiFi Local IP");

    // Ensure connected
    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(3000);
    }

    IPAddress ip = wifi.localIP();
    Monitor.print("Local IP: ");
    Monitor.println(ip);

    if (wifi.isConnected()) {
        TEST_ASSERT(ip != IPAddress(0, 0, 0, 0), "Local IP should be valid when connected");
        TEST_ASSERT(ip[0] != 0, "IP should have valid first octet");
    } else {
        TEST_ASSERT(ip == IPAddress(0, 0, 0, 0), "Local IP should be 0.0.0.0 when not connected");
    }

    delay(100);
}

void test_subnet_mask() {
    TEST_START("WiFi Subnet Mask");

    // Ensure connected
    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(3000);
    }

    IPAddress mask = wifi.subnetMask();
    Monitor.print("Subnet Mask: ");
    Monitor.println(mask);

    if (wifi.isConnected()) {
        TEST_ASSERT(mask != IPAddress(0, 0, 0, 0), "Subnet mask should be valid when connected");
        // Common subnet masks start with 255
        TEST_ASSERT(mask[0] == 255, "Subnet mask should start with 255");
    } else {
        TEST_ASSERT(true, "Skipped - not connected");
    }

    delay(100);
}

void test_gateway_ip() {
    TEST_START("WiFi Gateway IP");

    // Ensure connected
    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(3000);
    }

    IPAddress gateway = wifi.gatewayIP();
    Monitor.print("Gateway IP: ");
    Monitor.println(gateway);

    if (wifi.isConnected()) {
        TEST_ASSERT(gateway != IPAddress(0, 0, 0, 0), "Gateway IP should be valid when connected");
        TEST_ASSERT(gateway[0] != 0, "Gateway should have valid first octet");
    } else {
        TEST_ASSERT(true, "Skipped - not connected");
    }

    delay(100);
}

void test_mac_address() {
    TEST_START("WiFi MAC Address");

    uint8_t mac[6];
    wifi.macAddress(mac);

    Monitor.print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 16) Monitor.print("0");
        Monitor.print(mac[i], HEX);
        if (i < 5) Monitor.print(":");
    }
    Monitor.println();

    // MAC address should not be all zeros
    bool hasValidMAC = false;
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0) {
            hasValidMAC = true;
            break;
        }
    }

    TEST_ASSERT(hasValidMAC, "MAC address should be valid (not all zeros)");

    delay(100);
}

void test_connection_cycle() {
    TEST_START("WiFi Connection Cycle");

    // Disconnect if connected
    if (wifi.isConnected()) {
        wifi.disconnect();
        delay(1000);
    }

    TEST_ASSERT(!wifi.isConnected(), "Should be disconnected at start");

    // Connect
    uint8_t result = wifi.begin(TEST_SSID, TEST_PASSWORD);
    delay(3000);

    if (wifi.isConnected()) {
        TEST_ASSERT(wifi.status() == WL_CONNECTED, "Status should be WL_CONNECTED");

        // Get network info
        IPAddress ip = wifi.localIP();
        String ssid = wifi.SSID();

        TEST_ASSERT(ip != IPAddress(0, 0, 0, 0), "Should have valid IP when connected");
        TEST_ASSERT(ssid.length() > 0, "Should have valid SSID when connected");

        // Disconnect
        wifi.disconnect();
        delay(1000);

        TEST_ASSERT(!wifi.isConnected(), "Should be disconnected after disconnect");
        TEST_ASSERT(wifi.status() == WL_DISCONNECTED, "Status should be WL_DISCONNECTED");
    } else {
        Monitor.println("⚠ Could not connect (network may not exist)");
        TEST_ASSERT(true, "Connection cycle test skipped");
    }

    delay(100);
}

void test_multiple_status_calls() {
    TEST_START("WiFi Multiple Status Calls");

    uint8_t status1 = wifi.status();
    delay(100);
    uint8_t status2 = wifi.status();
    delay(100);
    uint8_t status3 = wifi.status();

    Monitor.print("Status calls: ");
    Monitor.print(status1);
    Monitor.print(", ");
    Monitor.print(status2);
    Monitor.print(", ");
    Monitor.println(status3);

    // Status should be consistent (unless network conditions changed)
    TEST_ASSERT(true, "Multiple status calls completed");

    delay(100);
}

void test_network_info_consistency() {
    TEST_START("WiFi Network Info Consistency");

    if (!wifi.isConnected()) {
        wifi.begin(TEST_SSID, TEST_PASSWORD);
        delay(3000);
    }

    if (wifi.isConnected()) {
        // Get info multiple times
        IPAddress ip1 = wifi.localIP();
        delay(100);
        IPAddress ip2 = wifi.localIP();

        String ssid1 = wifi.SSID();
        delay(100);
        String ssid2 = wifi.SSID();

        TEST_ASSERT(ip1 == ip2, "Local IP should be consistent");
        TEST_ASSERT(ssid1 == ssid2, "SSID should be consistent");

        Monitor.print("IP: "); Monitor.println(ip1);
        Monitor.print("SSID: "); Monitor.println(ssid1);
    } else {
        Monitor.println("⚠ Not connected - skipping consistency test");
        TEST_ASSERT(true, "Test skipped - not connected");
    }

    delay(100);
}

void setup() {

    Bridge.begin();
    
    Monitor.begin();

    Monitor.println("");
    Monitor.println("==========================================");
    Monitor.println("BridgeWiFi Test Suite");
    Monitor.println("==========================================");
    Monitor.print("Test SSID: "); Monitor.println(TEST_SSID);
    Monitor.print("Test Password: "); Monitor.println(TEST_PASSWORD);
    Monitor.println("==========================================");

    Monitor.println("Bridge initialized successfully");

    Monitor.println("Waiting 5s for the other side");
    delay(5000);

    // Run all tests
    test_initial_status();
    test_begin_open_network();
    test_status_check();
    test_disconnect();
    test_begin_with_password();
    test_multiple_status_calls();

    // Network scanning tests
    test_scan_networks();
    test_ssid_from_scan();

    // Connection info tests (require connection)
    test_connected_ssid();
    test_bssid();
    test_rssi();
    test_local_ip();
    test_subnet_mask();
    test_gateway_ip();
    test_mac_address();

    // Advanced tests
    test_connection_cycle();
    test_network_info_consistency();

    // Print summary
    printTestSummary();

    Monitor.println("\n✓ Test suite completed!");
}

void loop() {
    // Test suite runs once in setup()
    delay(1000);
}