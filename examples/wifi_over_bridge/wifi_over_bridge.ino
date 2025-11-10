/*
  WiFi Bridge Example

  This example demonstrates how to use the BridgeWiFi class to:
  - Scan for available networks
  - Connect to a WiFi network
  - Display connection information

  Created 2025
*/

#include "Arduino_RouterBridge.h"

const char* ssid = "YourNetworkName";
const char* password = "YourPassword";

BridgeWiFi wifi(Bridge);


void printConnectionInfo() {
  Monitor.println("Connection Information:");
  Monitor.println("----------------------");

  Monitor.print("SSID: ");
  Monitor.println(wifi.SSID());

  Monitor.print("IP Address: ");
  Monitor.println(wifi.localIP());

  Monitor.print("Subnet Mask: ");
  Monitor.println(wifi.subnetMask());

  Monitor.print("Gateway IP: ");
  Monitor.println(wifi.gatewayIP());

  Monitor.print("Signal Strength (RSSI): ");
  Monitor.print(wifi.RSSI());
  Monitor.println(" dBm");

  uint8_t mac[6];
  wifi.macAddress(mac);
  Monitor.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Monitor.print("0");
    Monitor.print(mac[i], HEX);
    if (i < 5) Monitor.print(":");
  }
  Monitor.println("\n");

  uint8_t bssid[6];
  wifi.BSSID(bssid);
  Monitor.print("BSSID: ");
  for (int i = 0; i < 6; i++) {
    if (bssid[i] < 0x10) Monitor.print("0");
    Monitor.print(bssid[i], HEX);
    if (i < 5) Monitor.print(":");
  }
  Monitor.println("\n");
}

void printStatus(uint8_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      Monitor.println("Idle");
      break;
    case WL_NO_SSID_AVAIL:
      Monitor.println("No SSID available");
      break;
    case WL_SCAN_COMPLETED:
      Monitor.println("Scan completed");
      break;
    case WL_CONNECTED:
      Monitor.println("Connected");
      break;
    case WL_CONNECT_FAILED:
      Monitor.println("Connection failed");
      break;
    case WL_CONNECTION_LOST:
      Monitor.println("Connection lost");
      break;
    case WL_DISCONNECTED:
      Monitor.println("Disconnected");
      break;
    case WL_NO_SHIELD:
      Monitor.println("No WiFi adapter/shield");
      break;
    default:
      Monitor.print("Unknown status: ");
      Monitor.println(status);
      break;
  }
}

void setup() {

  Bridge.begin();

  Monitor.begin();

  Monitor.println("\n=== WiFi Bridge Example ===\n");

  // Scan for available networks
  Monitor.println("Scanning for WiFi networks...");
  int8_t numNetworks = wifi.scanNetworks();
  
  if (numNetworks == -1) {
    Monitor.println("Error: Failed to scan networks");
  } else if (numNetworks == 0) {
    Monitor.println("No networks found");
  } else {
    Monitor.print("Found ");
    Monitor.print(numNetworks);
    Monitor.println(" networks:\n");
    
    for (int i = 0; i < numNetworks; i++) {
      Monitor.print(i + 1);
      Monitor.print(": ");
      Monitor.println(wifi.SSID(i));
    }
  }

  Monitor.println("\n----------------------------\n");

  // Connect to WiFi
  Monitor.print("Connecting to ");
  Monitor.print(ssid);
  Monitor.println("...");

  uint8_t status = wifi.begin(ssid, password);

  if (status == WL_CONNECTED) {
    Monitor.println("Connected successfully!\n");
    printConnectionInfo();
  } else {
    Monitor.print("Connection failed with status: ");
    Monitor.println(status);
    printStatus(status);
  }
}

void loop() {
  // Check connection status every 10 seconds
  static unsigned long lastCheck = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastCheck >= 10000) {
    lastCheck = currentTime;

    if (wifi.isConnected()) {
      Monitor.println("WiFi still connected");
      Monitor.print("Signal strength (RSSI): ");
      Monitor.print(wifi.RSSI());
      Monitor.println(" dBm");
    } else {
      Monitor.println("WiFi disconnected!");
      Monitor.print("Status: ");
      printStatus(wifi.status());
      
      // Attempt to reconnect
      Monitor.println("Attempting to reconnect...");
      wifi.begin(ssid, password);
    }
    Monitor.println();
  }
}
