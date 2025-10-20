"""
BridgeWiFi Python Server Implementation

This module provides the server-side RPC methods for testing the BridgeWiFi class.
It simulates WiFi operations including connection, scanning, and network information.
"""

import time
import random
from arduino.app_utils import *


# WiFi status codes (matching Arduino)
WL_NO_SHIELD = 255
WL_IDLE_STATUS = 0
WL_NO_SSID_AVAIL = 1
WL_SCAN_COMPLETED = 2
WL_CONNECTED = 3
WL_CONNECT_FAILED = 4
WL_CONNECTION_LOST = 5
WL_DISCONNECTED = 6

# Global WiFi state
wifi_state = {
    'status': WL_IDLE_STATUS,
    'ssid': '',
    'password': '',
    'connected': False,
    'local_ip': '0.0.0.0',
    'subnet_mask': '0.0.0.0',
    'gateway_ip': '0.0.0.0',
    'mac_address': [0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED],
    'bssid': [0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    'rssi': 0,
    'last_scan': []
}

# Simulated available networks
# Format: (SSID, BSSID, RSSI, encrypted)
SIMULATED_NETWORKS = [
    ("TestNetwork", [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01], -45, True),
    ("OpenNetwork", [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02], -55, False),
    ("Office_WiFi", [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x03], -62, True),
    ("Guest_Network", [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x04], -70, True),
    ("MyHomeWiFi", [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x05], -48, True),
    ("CoffeeShop_Free", [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x06], -75, False),
]

# Valid credentials for testing
VALID_CREDENTIALS = {
    "TestNetwork": "TestPassword123",
    "Office_WiFi": "office2024",
    "MyHomeWiFi": "myhome!23",
}


def wifi_begin(ssid: str, password: str = ""):
    """
    Start WiFi connection
    Returns: status code (WL_CONNECTED, WL_CONNECT_FAILED, WL_NO_SSID_AVAIL)
    """
    global wifi_state

    print(f"WiFi begin: SSID='{ssid}', Password={'***' if password else '(none)'}")

    # Check if SSID exists in simulated networks
    network_found = None
    for net in SIMULATED_NETWORKS:
        if net[0] == ssid:
            network_found = net
            break

    if not network_found:
        print(f"  → SSID '{ssid}' not found")
        wifi_state['status'] = WL_NO_SSID_AVAIL
        wifi_state['connected'] = False
        return WL_NO_SSID_AVAIL

    # Check if network requires password
    network_ssid, network_bssid, network_rssi, is_encrypted = network_found

    if is_encrypted:
        # Check if password is correct
        if ssid in VALID_CREDENTIALS:
            if password == VALID_CREDENTIALS[ssid]:
                # Successful connection
                wifi_state['status'] = WL_CONNECTED
                wifi_state['connected'] = True
                wifi_state['ssid'] = ssid
                wifi_state['password'] = password
                wifi_state['bssid'] = network_bssid
                wifi_state['rssi'] = network_rssi

                # Assign IP address (simulate DHCP)
                wifi_state['local_ip'] = f"192.168.1.{random.randint(100, 200)}"
                wifi_state['subnet_mask'] = "255.255.255.0"
                wifi_state['gateway_ip'] = "192.168.1.1"

                print(f"  → Connected successfully!")
                print(f"     IP: {wifi_state['local_ip']}")
                return WL_CONNECTED
            else:
                print(f"  → Wrong password")
                wifi_state['status'] = WL_CONNECT_FAILED
                wifi_state['connected'] = False
                return WL_CONNECT_FAILED
        else:
            # Network not in our test set
            print(f"  → Network requires password but not configured in test")
            wifi_state['status'] = WL_CONNECT_FAILED
            wifi_state['connected'] = False
            return WL_CONNECT_FAILED
    else:
        # Open network - connect without password
        wifi_state['status'] = WL_CONNECTED
        wifi_state['connected'] = True
        wifi_state['ssid'] = ssid
        wifi_state['password'] = ""
        wifi_state['bssid'] = network_bssid
        wifi_state['rssi'] = network_rssi

        # Assign IP address
        wifi_state['local_ip'] = f"192.168.1.{random.randint(100, 200)}"
        wifi_state['subnet_mask'] = "255.255.255.0"
        wifi_state['gateway_ip'] = "192.168.1.1"

        print(f"  → Connected to open network!")
        print(f"     IP: {wifi_state['local_ip']}")
        return WL_CONNECTED


def wifi_disconnect():
    """
    Disconnect from WiFi network
    Returns: 1 for success, 0 for failure
    """
    global wifi_state

    print("WiFi disconnect")

    if wifi_state['connected']:
        wifi_state['status'] = WL_DISCONNECTED
        wifi_state['connected'] = False
        wifi_state['ssid'] = ''
        wifi_state['password'] = ''
        wifi_state['local_ip'] = '0.0.0.0'
        wifi_state['subnet_mask'] = '0.0.0.0'
        wifi_state['gateway_ip'] = '0.0.0.0'
        wifi_state['bssid'] = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
        wifi_state['rssi'] = 0

        print("  → Disconnected successfully")
        return 1
    else:
        print("  → Already disconnected")
        return 1


def wifi_status():
    """
    Get current WiFi status
    Returns: status code
    """
    status = wifi_state['status']
    status_names = {
        WL_NO_SHIELD: "WL_NO_SHIELD",
        WL_IDLE_STATUS: "WL_IDLE_STATUS",
        WL_NO_SSID_AVAIL: "WL_NO_SSID_AVAIL",
        WL_SCAN_COMPLETED: "WL_SCAN_COMPLETED",
        WL_CONNECTED: "WL_CONNECTED",
        WL_CONNECT_FAILED: "WL_CONNECT_FAILED",
        WL_CONNECTION_LOST: "WL_CONNECTION_LOST",
        WL_DISCONNECTED: "WL_DISCONNECTED",
    }

    print(f"WiFi status: {status} ({status_names.get(status, 'UNKNOWN')})")
    return status


def wifi_scan():
    """
    Scan for available WiFi networks
    Returns: number of networks found
    """
    global wifi_state

    print("WiFi scan started...")

    # Simulate scan delay
    time.sleep(0.5)

    # Store scan results
    wifi_state['last_scan'] = SIMULATED_NETWORKS.copy()

    # Add some random variation to RSSI
    varied_networks = []
    for ssid, bssid, rssi, encrypted in wifi_state['last_scan']:
        varied_rssi = rssi + random.randint(-3, 3)
        varied_networks.append((ssid, bssid, varied_rssi, encrypted))
    wifi_state['last_scan'] = varied_networks

    num_networks = len(wifi_state['last_scan'])

    print(f"  → Found {num_networks} networks:")
    for i, (ssid, bssid, rssi, encrypted) in enumerate(wifi_state['last_scan']):
        security = "WPA2" if encrypted else "Open"
        print(f"     {i}: {ssid} ({rssi} dBm, {security})")

    return num_networks


def wifi_ssid_by_index(network_item: int):
    """
    Get SSID of scanned network by index
    Returns: SSID string
    """
    if 'last_scan' not in wifi_state or len(wifi_state['last_scan']) == 0:
        print(f"WiFi SSID[{network_item}]: No scan results")
        return ""

    if network_item < 0 or network_item >= len(wifi_state['last_scan']):
        print(f"WiFi SSID[{network_item}]: Index out of range")
        return ""

    ssid = wifi_state['last_scan'][network_item][0]
    print(f"WiFi SSID[{network_item}]: {ssid}")
    return ssid


def wifi_bssid():
    """
    Get BSSID of currently connected network
    Returns: array of 6 bytes
    """
    if not wifi_state['connected']:
        print("WiFi BSSID: Not connected")
        return [0x00, 0x00, 0x00, 0x00, 0x00, 0x00]

    bssid = wifi_state['bssid']
    bssid_str = ":".join([f"{b:02X}" for b in bssid])
    print(f"WiFi BSSID: {bssid_str}")
    return bssid


def wifi_rssi():
    """
    Get RSSI (signal strength) of currently connected network
    Returns: RSSI in dBm (negative value)
    """
    if not wifi_state['connected']:
        print("WiFi RSSI: Not connected")
        return 0

    # Add slight variation to simulate real conditions
    rssi = wifi_state['rssi'] + random.randint(-2, 2)
    print(f"WiFi RSSI: {rssi} dBm")
    return rssi


def wifi_local_ip():
    """
    Get local IP address
    Returns: IP address string
    """
    ip = wifi_state['local_ip']
    print(f"WiFi Local IP: {ip}")
    return ip


def wifi_subnet_mask():
    """
    Get subnet mask
    Returns: subnet mask string
    """
    mask = wifi_state['subnet_mask']
    print(f"WiFi Subnet Mask: {mask}")
    return mask


def wifi_gateway_ip():
    """
    Get gateway IP address
    Returns: gateway IP string
    """
    gateway = wifi_state['gateway_ip']
    print(f"WiFi Gateway IP: {gateway}")
    return gateway


def wifi_mac_address():
    """
    Get MAC address of WiFi interface
    Returns: array of 6 bytes
    """
    mac = wifi_state['mac_address']
    mac_str = ":".join([f"{b:02X}" for b in mac])
    print(f"WiFi MAC Address: {mac_str}")
    return mac


# Test helper functions
def add_test_network(ssid: str, password: str = None, rssi: int = -50):
    """
    Add a custom network for testing
    """
    global SIMULATED_NETWORKS, VALID_CREDENTIALS

    bssid = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, random.randint(0x10, 0xFF)]
    encrypted = password is not None

    SIMULATED_NETWORKS.append((ssid, bssid, rssi, encrypted))

    if password:
        VALID_CREDENTIALS[ssid] = password

    print(f"Added test network: {ssid} ({'encrypted' if encrypted else 'open'})")


def remove_test_network(ssid: str):
    """
    Remove a test network
    """
    global SIMULATED_NETWORKS, VALID_CREDENTIALS

    SIMULATED_NETWORKS = [net for net in SIMULATED_NETWORKS if net[0] != ssid]

    if ssid in VALID_CREDENTIALS:
        del VALID_CREDENTIALS[ssid]

    print(f"Removed test network: {ssid}")


def simulate_connection_loss():
    """
    Simulate losing WiFi connection
    """
    global wifi_state

    if wifi_state['connected']:
        print("Simulating connection loss...")
        wifi_state['status'] = WL_CONNECTION_LOST
        wifi_state['connected'] = False
        wifi_state['local_ip'] = '0.0.0.0'
        print("  → Connection lost!")


def set_signal_strength(rssi: int):
    """
    Set the RSSI of current connection
    """
    global wifi_state

    if wifi_state['connected']:
        wifi_state['rssi'] = rssi
        print(f"Set RSSI to {rssi} dBm")
    else:
        print("Cannot set RSSI - not connected")


def get_wifi_info():
    """
    Print complete WiFi state information
    """
    print("\n" + "=" * 50)
    print("WiFi State Information")
    print("=" * 50)
    print(f"Status: {wifi_state['status']}")
    print(f"Connected: {wifi_state['connected']}")
    print(f"SSID: {wifi_state['ssid']}")
    print(f"Local IP: {wifi_state['local_ip']}")
    print(f"Subnet Mask: {wifi_state['subnet_mask']}")
    print(f"Gateway: {wifi_state['gateway_ip']}")

    mac_str = ":".join([f"{b:02X}" for b in wifi_state['mac_address']])
    print(f"MAC Address: {mac_str}")

    if wifi_state['connected']:
        bssid_str = ":".join([f"{b:02X}" for b in wifi_state['bssid']])
        print(f"BSSID: {bssid_str}")
        print(f"RSSI: {wifi_state['rssi']} dBm")

    print("=" * 50 + "\n")


def list_networks():
    """
    List all simulated networks
    """
    print("\n" + "=" * 50)
    print("Available Networks")
    print("=" * 50)

    for i, (ssid, bssid, rssi, encrypted) in enumerate(SIMULATED_NETWORKS):
        security = "WPA2" if encrypted else "Open"
        password_info = ""
        if encrypted and ssid in VALID_CREDENTIALS:
            password_info = f" (Password: {VALID_CREDENTIALS[ssid]})"

        print(f"{i}: {ssid:20} {rssi:4} dBm  {security:8}{password_info}")

    print("=" * 50 + "\n")


if __name__ == "__main__":
    # Register RPC methods
    Bridge.provide("wifi/begin", wifi_begin)
    Bridge.provide("wifi/disconnect", wifi_disconnect)
    Bridge.provide("wifi/status", wifi_status)
    Bridge.provide("wifi/scan", wifi_scan)
    Bridge.provide("wifi/SSID", wifi_ssid_by_index)
    Bridge.provide("wifi/BSSID", wifi_bssid)
    Bridge.provide("wifi/RSSI", wifi_rssi)
    Bridge.provide("wifi/localIP", wifi_local_ip)
    Bridge.provide("wifi/subnetMask", wifi_subnet_mask)
    Bridge.provide("wifi/gatewayIP", wifi_gateway_ip)
    Bridge.provide("wifi/macAddress", wifi_mac_address)

    print("=" * 60)
    print("WiFi Bridge Server ready")
    print("=" * 60)
    print("\nAvailable RPC methods:")
    print("  - wifi/begin")
    print("  - wifi/disconnect")
    print("  - wifi/status")
    print("  - wifi/scan")
    print("  - wifi/SSID")
    print("  - wifi/BSSID")
    print("  - wifi/RSSI")
    print("  - wifi/localIP")
    print("  - wifi/subnetMask")
    print("  - wifi/gatewayIP")
    print("  - wifi/macAddress")
    print()

    # List available test networks
    list_networks()

    print("Helper functions available:")
    print("  - add_test_network(ssid, password, rssi)")
    print("  - remove_test_network(ssid)")
    print("  - simulate_connection_loss()")
    print("  - set_signal_strength(rssi)")
    print("  - get_wifi_info()")
    print("  - list_networks()")
    print()

    App.run()
