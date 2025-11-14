"""
BridgeUDP Python Server Implementation

This module provides the server-side RPC methods for testing the BridgeUDP class.
It simulates UDP operations and handles packet transmission/reception.
"""

import socket
import struct
import threading
import time
from arduino.app_utils import *


# Global state for UDP connections
udp_connections = {}
connection_counter = 0
connection_lock = threading.Lock()

# Simulated incoming packets buffer (connection_id -> list of packets)
incoming_packets = {}


class UDPConnection:
    """Represents a UDP connection"""

    def __init__(self, connection_id, host, port, is_multicast=False):
        self.connection_id = connection_id
        self.host = host
        self.port = port
        self.is_multicast = is_multicast
        self.socket = None
        self.running = False
        self.receive_thread = None

        # Create and bind socket
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        if is_multicast:
            # Multicast setup
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket.bind(('', port))

            # Join multicast group
            mreq = struct.pack("4sl", socket.inet_aton(host), socket.INADDR_ANY)
            self.socket.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        else:
            # Regular UDP
            self.socket.bind((host, port))

        # Set socket to non-blocking for receive thread
        self.socket.settimeout(0.5)

        # Start receive thread
        self.running = True
        self.receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self.receive_thread.start()

        print(f"UDP Connection {connection_id} created: {host}:{port} (multicast={is_multicast})")

    def _receive_loop(self):
        """Background thread to receive UDP packets"""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(4096)
                if data:
                    # Format: [IP (4 bytes), Port (2 bytes), Length (2 bytes), Data]
                    ip_bytes = socket.inet_aton(addr[0])
                    port_bytes = struct.pack('>H', addr[1])
                    length_bytes = struct.pack('>H', len(data))

                    packet = list(ip_bytes) + list(port_bytes) + list(length_bytes) + list(data)

                    with connection_lock:
                        if self.connection_id not in incoming_packets:
                            incoming_packets[self.connection_id] = []
                        incoming_packets[self.connection_id].append(packet)

                    print(f"Received {len(data)} bytes from {addr[0]}:{addr[1]} on connection {self.connection_id}")
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"Error receiving on connection {self.connection_id}: {e}")
                break

    def send(self, target_host, target_port, data):
        """Send data to target"""
        try:
            self.socket.sendto(bytes(data), (target_host, target_port))
            return len(data)
        except Exception as e:
            print(f"Error sending on connection {self.connection_id}: {e}")
            return 0

    def close(self):
        """Close the connection"""
        self.running = False
        if self.receive_thread:
            self.receive_thread.join(timeout=1.0)
        if self.socket:
            self.socket.close()
        print(f"UDP Connection {self.connection_id} closed")


def udp_connect(hostname: str, port: int):
    """
    Create a UDP connection
    Returns: connection_id
    """
    global connection_counter

    with connection_lock:
        connection_counter += 1
        conn_id = connection_counter

        try:
            conn = UDPConnection(conn_id, hostname, port, is_multicast=False)
            udp_connections[conn_id] = conn
            incoming_packets[conn_id] = []

            print(f"UDP connect: {hostname}:{port} -> connection_id={conn_id}")
            return conn_id
        except Exception as e:
            print(f"Error in udp_connect: {e}")
            return 0


def udp_connect_multicast(hostname: str, port: int):
    """
    Create a UDP multicast connection
    Returns: connection_id
    """
    global connection_counter

    with connection_lock:
        connection_counter += 1
        conn_id = connection_counter

        try:
            conn = UDPConnection(conn_id, hostname, port, is_multicast=True)
            udp_connections[conn_id] = conn
            incoming_packets[conn_id] = []

            print(f"UDP connect multicast: {hostname}:{port} -> connection_id={conn_id}")
            return conn_id
        except Exception as e:
            print(f"Error in udp_connect_multicast: {e}")
            return 0


def udp_close(connection_id: int):
    """
    Close a UDP connection
    Returns: status message
    """
    with connection_lock:
        if connection_id in udp_connections:
            conn = udp_connections[connection_id]
            conn.close()
            del udp_connections[connection_id]

            if connection_id in incoming_packets:
                del incoming_packets[connection_id]

            print(f"UDP close: connection_id={connection_id}")
            return "closed"
        else:
            print(f"UDP close: connection_id={connection_id} not found")
            return "not found"


def udp_write(connection_id: int, target_host: str, target_port: int, data: list):
    """
    Write data to a UDP connection
    Returns: number of bytes written
    """
    with connection_lock:
        if connection_id not in udp_connections:
            print(f"UDP write: connection_id={connection_id} not found")
            return 0

        conn = udp_connections[connection_id]

    # Send outside lock to avoid blocking
    written = conn.send(target_host, target_port, data)
    print(f"UDP write: connection_id={connection_id}, target={target_host}:{target_port}, bytes={written}")
    return written


def udp_read(connection_id: int, size: int):
    """
    Read data from a UDP connection
    Returns: array of bytes (includes packet header)
    """
    with connection_lock:
        if connection_id not in incoming_packets:
            return []

        packets = incoming_packets[connection_id]
        if not packets:
            return []

        # Get available bytes from first packet
        available = packets[0]

        if size >= len(available):
            # Return entire packet and remove from queue
            result = packets.pop(0)
            print(f"UDP read: connection_id={connection_id}, size={len(result)} (complete packet)")
            return result
        else:
            # Return partial data
            result = available[:size]
            incoming_packets[connection_id][0] = available[size:]
            print(f"UDP read: connection_id={connection_id}, size={size} (partial)")
            return result


# Test helper functions
def send_test_packet(target_host: str, target_port: int, message: str):
    """
    Send a test UDP packet to the specified host:port
    Useful for testing receive functionality
    """
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(message.encode(), (target_host, target_port))
        sock.close()
        print(f"Sent test packet to {target_host}:{target_port}: {message}")
        return True
    except Exception as e:
        print(f"Error sending test packet: {e}")
        return False


def send_test_packets_loop(target_host: str, target_port: int, count: int = 10, interval: float = 1.0):
    """
    Send multiple test packets in a loop
    """
    for i in range(count):
        message = f"Test packet {i+1}/{count}"
        send_test_packet(target_host, target_port, message)
        time.sleep(interval)


def echo_server(listen_port: int):
    """
    Start a simple UDP echo server for testing
    """
    def echo_loop():
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(('0.0.0.0', listen_port))
        sock.settimeout(0.5)

        print(f"UDP Echo server started on port {listen_port}")

        while True:
            try:
                data, addr = sock.recvfrom(4096)
                if data:
                    # Echo back with "ECHO: " prefix
                    response = b"ECHO: " + data
                    sock.sendto(response, addr)
                    print(f"Echo server: received {len(data)} bytes, sent {len(response)} bytes to {addr}")
            except socket.timeout:
                continue
            except Exception as e:
                print(f"Echo server error: {e}")
                break

    thread = threading.Thread(target=echo_loop, daemon=True)
    thread.start()


if __name__ == "__main__":
    # Register RPC methods
    Bridge.provide("udp/connect", udp_connect)
    Bridge.provide("udp/connectMulticast", udp_connect_multicast)
    Bridge.provide("udp/close", udp_close)
    Bridge.provide("udp/write", udp_write)
    Bridge.provide("udp/read", udp_read)

    # Start echo server for testing
    echo_server(5000)

    # Example: Send test packets periodically
    # Uncomment to enable automatic packet sending for testing
    # threading.Thread(target=lambda: send_test_packets_loop("192.168.1.100", 8888), daemon=True).start()

    print("UDP Bridge Server ready")
    print("Available methods:")
    print("  - udp/connect")
    print("  - udp/connectMulticast")
    print("  - udp/close")
    print("  - udp/write")
    print("  - udp/read")
    print("")
    print("Echo server running on port 5000")

    App.run()