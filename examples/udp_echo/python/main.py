#!/usr/bin/env python3
"""
Simple UDP Echo Server

This server listens for UDP packets and echoes them back to the sender.
It can be used independently or with the Bridge server for testing.

Usage:
    python main.py [--port PORT] [--prefix PREFIX]

Examples:
    python main.py
    python main.py --port 5000
    python main.py --port 5000 --prefix "ECHO: "
"""

import socket
import sys
import argparse
import time
from datetime import datetime


def log(msg):
    with open("log.log", 'a') as f:
        f.write(f"{msg}\n")


class UDPEchoServer:
    """Simple UDP echo server"""

    def __init__(self, port=5000, prefix="ECHO: ", buffer_size=4096):
        self.port = port
        self.prefix = prefix
        self.buffer_size = buffer_size
        self.socket = None
        self.running = False

        # Statistics
        self.packets_received = 0
        self.packets_sent = 0
        self.bytes_received = 0
        self.bytes_sent = 0
        self.start_time = None

    def start(self):
        """Start the echo server"""
        try:
            # Create UDP socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            # Bind to all interfaces
            self.socket.bind(('127.0.0.1', self.port))

            log("=" * 60)
            log("UDP Echo Server")
            log("=" * 60)
            log(f"Listening on: 127.0.0.1:{self.port}")
            log(f"Echo prefix: \"{self.prefix}\"")
            log(f"Buffer size: {self.buffer_size} bytes")
            log("=" * 60)
            log("Press Ctrl+C to stop\n")

            self.running = True
            self.start_time = time.time()

            self.run()

        except PermissionError:
            log(f"ERROR: Permission denied. Port {self.port} may require sudo/admin.")
            sys.exit(1)
        except OSError as e:
            log(f"ERROR: Cannot bind to port {self.port}: {e}")
            sys.exit(1)
        except KeyboardInterrupt:
            log("\n\nShutting down...")
            self.stop()
        except Exception as e:
            log(f"ERROR: {e}")
            self.stop()
            sys.exit(1)

    def run(self):
        """Main server loop"""
        while self.running:
            try:
                # Receive data
                data, addr = self.socket.recvfrom(self.buffer_size)
                log(f"Attempt to read incoming messages")

                self.packets_received += 1
                self.bytes_received += len(data)

                # Print received packet info
                timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                log(f"[{timestamp}] Received {len(data)} bytes from {addr[0]}:{addr[1]}")

                # Decode and print message (if printable)
                try:
                    message = data.decode('utf-8', errors='ignore')
                    if message.isprintable() or message.strip():
                        log(f"  Message: \"{message}\"")
                except:
                    log(f"  Data: {data[:50]}{'...' if len(data) > 50 else ''}")

                # Prepare echo response
                if self.prefix:
                    response = self.prefix.encode() + data
                else:
                    response = data

                # Send echo back
                sent = self.socket.sendto(response, addr)

                self.packets_sent += 1
                self.bytes_sent += sent

                log(f"  Echoed: {sent} bytes\n")

            except socket.timeout:
                log("UDP socket timeout")
                continue
            except KeyboardInterrupt:
                log("Keyboard interrupt")
                raise
            except Exception as e:
                log(f"Error handling packet: {e}\n")

    def stop(self):
        """Stop the server"""
        self.running = False

        if self.socket:
            self.socket.close()

        # Print statistics
        if self.start_time:
            runtime = time.time() - self.start_time

            log("\n" + "=" * 60)
            log("Server Statistics")
            log("=" * 60)
            log(f"Runtime: {runtime:.1f} seconds")
            log(f"Packets received: {self.packets_received}")
            log(f"Packets sent: {self.packets_sent}")
            log(f"Bytes received: {self.bytes_received}")
            log(f"Bytes sent: {self.bytes_sent}")

            if runtime > 0:
                log(f"Packets/sec: {self.packets_received / runtime:.2f}")
                log(f"Throughput: {self.bytes_received / runtime:.2f} bytes/sec")

            log("=" * 60)

        log("Server stopped")


def test_server(host='127.0.0.1', port=5000):
    """Test the echo server by sending test packets"""
    log("\n" + "=" * 60)
    log("Testing Echo Server")
    log("=" * 60)
    log(f"Target: {host}:{port}\n")

    test_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    test_sock.settimeout(2.0)

    test_messages = [
        "Hello, Echo!",
        "Test message 1",
        "This is a longer test message with more content",
        "12345",
        "Special chars: !@#$%^&*()",
    ]

    passed = 0
    failed = 0

    for i, msg in enumerate(test_messages, 1):
        log(f"[Test {i}/{len(test_messages)}] Sending: \"{msg}\"")

        try:
            # Send
            test_sock.sendto(msg.encode(), (host, port))

            # Receive
            data, addr = test_sock.recvfrom(4096)
            response = data.decode('utf-8', errors='ignore')

            log(f"  Received: \"{response}\"")

            # Check if echo contains original message
            if msg in response:
                log("  ✓ PASS\n")
                passed += 1
            else:
                log("  ✗ FAIL - Response doesn't contain original message\n")
                failed += 1

        except socket.timeout:
            log("  ✗ FAIL - Timeout (no response)\n")
            failed += 1
        except Exception as e:
            log(f"  ✗ FAIL - Error: {e}\n")
            failed += 1

    test_sock.close()

    log("=" * 60)
    log(f"Test Results: {passed} passed, {failed} failed")
    log("=" * 60)


def main():
    parser = argparse.ArgumentParser(
        description='Simple UDP Echo Server',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                              Start server on default port 5000
  %(prog)s --port 8888                  Start server on port 8888
  %(prog)s --prefix "REPLY: "           Use custom echo prefix
  %(prog)s --no-prefix                  Echo without prefix
  %(prog)s --test                       Test the server
        """
    )

    parser.add_argument(
        '-p', '--port',
        type=int,
        default=5000,
        help='UDP port to listen on (default: 5000)'
    )

    parser.add_argument(
        '--prefix',
        type=str,
        default='ECHO: ',
        help='Prefix to add to echoed messages (default: "ECHO: ")'
    )

    parser.add_argument(
        '--no-prefix',
        action='store_true',
        help='Echo without any prefix'
    )

    parser.add_argument(
        '-b', '--buffer-size',
        type=int,
        default=4096,
        help='Buffer size in bytes (default: 4096)'
    )

    parser.add_argument(
        '--test',
        action='store_true',
        help='Test the echo server by sending test packets'
    )

    parser.add_argument(
        '--test-host',
        type=str,
        default='127.0.0.1',
        help='Host to test (default: 127.0.0.1)'
    )

    args = parser.parse_args()

    # Handle test mode
    if args.test:
        test_server(args.test_host, args.port)
        return

    # Handle no-prefix option
    prefix = '' if args.no_prefix else args.prefix

    # Start server
    server = UDPEchoServer(
        port=args.port,
        prefix=prefix,
        buffer_size=args.buffer_size
    )

    server.start()


if __name__ == "__main__":
    main()