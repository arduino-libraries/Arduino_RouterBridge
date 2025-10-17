/*
    This file is part of the Arduino_RouterBridge library.

    Copyright (c) 2025 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#pragma once

#ifndef WIFI_BRIDGE_H
#define WIFI_BRIDGE_H

#define WIFI_BEGIN_METHOD           "wifi/begin"
#define WIFI_DISCONNECT_METHOD      "wifi/disconnect"
#define WIFI_STATUS_METHOD          "wifi/status"
#define WIFI_SCAN_METHOD            "wifi/scan"
#define WIFI_SSID_METHOD            "wifi/SSID"
#define WIFI_BSSID_METHOD           "wifi/BSSID"
#define WIFI_RSSI_METHOD            "wifi/RSSI"
#define WIFI_LOCAL_IP_METHOD        "wifi/localIP"
#define WIFI_SUBNET_MASK_METHOD     "wifi/subnetMask"
#define WIFI_GATEWAY_IP_METHOD      "wifi/gatewayIP"
#define WIFI_MAC_ADDRESS_METHOD     "wifi/macAddress"

#include <Arduino.h>
#include <IPAddress.h>


// WiFi status codes (matching Arduino WiFi library)
typedef enum {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1,
    WL_SCAN_COMPLETED = 2,
    WL_CONNECTED = 3,
    WL_CONNECT_FAILED = 4,
    WL_CONNECTION_LOST = 5,
    WL_DISCONNECTED = 6
} wl_status_t;

// WiFi encryption types. Not used yet
// typedef enum {
//     ENC_TYPE_WEP = 5,
//     ENC_TYPE_TKIP = 2,
//     ENC_TYPE_CCMP = 4,
//     ENC_TYPE_NONE = 7,
//     ENC_TYPE_AUTO = 8
// } wl_enc_type;

class BridgeWiFi {

    BridgeClass* bridge;
    struct k_mutex wifi_mutex{};
    wl_status_t _status = WL_IDLE_STATUS;
    String _ssid{};
    IPAddress _localIP{};
    IPAddress _subnetMask{};
    IPAddress _gatewayIP{};
    uint8_t _bssid[6]{};
    int32_t _rssi = 0;

public:

    explicit BridgeWiFi(BridgeClass& bridge): bridge(&bridge) {}

    /**
     * Start WiFi connection for OPEN networks
     * @param ssid: SSID of the network
     */
    uint8_t begin(const char* ssid) {
        return begin(ssid, nullptr);
    }

    /**
     * Start WiFi connection with WPA/WPA2 encryption
     * @param ssid: SSID of the network
     * @param passphrase: Passphrase (max 63 chars)
     */
    uint8_t begin(const char* ssid, const char* passphrase) {

        if (!init()) {
            _status = WL_NO_SHIELD;
            return _status;
        }

        k_mutex_lock(&wifi_mutex, K_FOREVER);

        String ssid_str = ssid;
        String pass_str = passphrase ? passphrase : "";

        uint8_t result;
        const bool ok = bridge->call(WIFI_BEGIN_METHOD, ssid_str, pass_str).result(result);

        if (ok && result == WL_CONNECTED) {
            _status = WL_CONNECTED;
            _ssid = ssid_str;
            updateConnectionInfo();
        } else if (ok) {
            _status = static_cast<wl_status_t>(result);
        } else {
            _status = WL_CONNECT_FAILED;
        }

        k_mutex_unlock(&wifi_mutex);

        return result;
    }

    /**
     * Disconnect from the network
     */
    uint8_t disconnect() {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        bool success = false;
        if (_status == WL_CONNECTED) {
            int32_t result;
            success = bridge->call(WIFI_DISCONNECT_METHOD).result(result);
            if (success) {
                _status = WL_DISCONNECTED;
                _ssid = "";
                _localIP = IPAddress(0, 0, 0, 0);
            }
        }

        k_mutex_unlock(&wifi_mutex);

        return success ? WL_DISCONNECTED : _status;
    }

    /**
     * Get the connection status
     */
    uint8_t status() {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        int result;
        const bool ok = bridge->call(WIFI_STATUS_METHOD).result(result);

        if (ok) {
            _status = static_cast<wl_status_t>(result);
        }

        const wl_status_t current_status = _status;
        k_mutex_unlock(&wifi_mutex);

        return current_status;
    }

    /**
     * Start scan WiFi networks available
     * @return Number of discovered networks
     */
    int8_t scanNetworks() {
        if (!init()) {
            return -1;
        }

        k_mutex_lock(&wifi_mutex, K_FOREVER);

        int networks;
        const bool ok = bridge->call(WIFI_SCAN_METHOD).result(networks);

        if (networks > 127) networks=127;
        if (networks < -1) networks=-1;

        k_mutex_unlock(&wifi_mutex);

        return ok ? static_cast<int8_t>(networks) : -1;
    }

    /**
     * Return the SSID discovered during the network scan
     * @param networkItem: specify from which network item want to get the information
     */
    String SSID(uint8_t networkItem) {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        String ssid;
        bridge->call(WIFI_SSID_METHOD, networkItem).result(ssid);

        k_mutex_unlock(&wifi_mutex);

        return ssid;
    }

    /**
     * Return the current SSID associated with the network
     */
    String SSID() {
        k_mutex_lock(&wifi_mutex, K_FOREVER);
        const String ssid = _ssid;
        k_mutex_unlock(&wifi_mutex);
        return ssid;
    }

    /**
     * Return the current BSSID associated with the network
     * @param bssid: array to store BSSID (6 bytes)
     */
    uint8_t* BSSID(uint8_t* bssid) {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        MsgPack::arr_t<uint8_t> bssid_arr;
        const bool ok = bridge->call(WIFI_BSSID_METHOD).result(bssid_arr);

        if (ok && bssid_arr.size() >= 6) {
            for (size_t i = 0; i < 6; ++i) {
                bssid[i] = bssid_arr[i];
                _bssid[i] = bssid_arr[i];
            }
        }

        k_mutex_unlock(&wifi_mutex);

        return bssid;
    }

    /**
     * Return the current RSSI (Received Signal Strength in dBm)
     */
    int32_t RSSI() {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        int32_t rssi;
        const bool ok = bridge->call(WIFI_RSSI_METHOD).result(rssi);

        if (ok) {
            _rssi = rssi;
        }

        k_mutex_unlock(&wifi_mutex);

        return rssi;
    }

    /**
     * Get the interface IP address
     */
    IPAddress localIP() {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        String ip_str;
        const bool ok = bridge->call(WIFI_LOCAL_IP_METHOD).result(ip_str);

        if (ok) {
            _localIP.fromString(ip_str);
        }

        const IPAddress ip = _localIP;
        k_mutex_unlock(&wifi_mutex);

        return ip;
    }

    /**
     * Get the interface subnet mask address
     */
    IPAddress subnetMask() {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        String mask_str;
        const bool ok = bridge->call(WIFI_SUBNET_MASK_METHOD).result(mask_str);

        if (ok) {
            _subnetMask.fromString(mask_str);
        }

        const IPAddress mask = _subnetMask;
        k_mutex_unlock(&wifi_mutex);

        return mask;
    }

    /**
     * Get the gateway IP address
     */
    IPAddress gatewayIP() {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        String gateway_str;
        const bool ok = bridge->call(WIFI_GATEWAY_IP_METHOD).result(gateway_str);

        if (ok) {
            _gatewayIP.fromString(gateway_str);
        }

        const IPAddress gateway = _gatewayIP;
        k_mutex_unlock(&wifi_mutex);

        return gateway;
    }

    /**
     * Get the interface MAC address
     * @param mac: array to store MAC address (6 bytes)
     */
    uint8_t* macAddress(uint8_t* mac) {
        k_mutex_lock(&wifi_mutex, K_FOREVER);

        MsgPack::arr_t<uint8_t> mac_arr;
        const bool ok = bridge->call(WIFI_MAC_ADDRESS_METHOD).result(mac_arr);

        if (ok && mac_arr.size() >= 6) {
            for (size_t i = 0; i < 6; ++i) {
                mac[i] = mac_arr[i];
            }
        }

        k_mutex_unlock(&wifi_mutex);

        return mac;
    }

    /**
     * Check if WiFi is connected
     */
    bool isConnected() {
        return status() == WL_CONNECTED;
    }

private:

    bool init() {
        k_mutex_init(&wifi_mutex);
        if (!(*bridge)) {
            return bridge->begin();
        }
        return true;
    }

    void updateConnectionInfo() {
        // Update local IP and other connection info
        String ip_str;
        if (bridge->call(WIFI_LOCAL_IP_METHOD).result(ip_str)) {
            _localIP.fromString(ip_str);
        }

        String mask_str;
        if (bridge->call(WIFI_SUBNET_MASK_METHOD).result(mask_str)) {
            _subnetMask.fromString(mask_str);
        }

        String gateway_str;
        if (bridge->call(WIFI_GATEWAY_IP_METHOD).result(gateway_str)) {
            _gatewayIP.fromString(gateway_str);
        }
    }
};

#endif //WIFI_BRIDGE_H