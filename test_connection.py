#!/usr/bin/env python3
"""
Simple test script to verify the VoIP server is running and accepting connections.
Tests both WebSocket (control) and UDP (voice) connectivity.
"""

import asyncio
import json
import ssl
import sys
import time

try:
    import websockets
except ImportError:
    print("[-] websockets library not installed")
    print("   Install with: pip install websockets")
    sys.exit(1)


async def test_websocket_connection():
    """Test WebSocket connection to control server"""
    uri = "wss://localhost:9000/control"

    print("[*] Testing WebSocket connection...")
    print(f"   Connecting to: {uri}")

    try:
        # Create SSL context that accepts self-signed certificates
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ssl_context.check_hostname = False
        ssl_context.verify_mode = ssl.CERT_NONE

        async with websockets.connect(uri, ssl=ssl_context) as websocket:
            print("[+] WebSocket connected successfully!")

            # Try sending a ping message
            ping_msg = {
                "type": "ping",
                "timestamp": int(time.time() * 1000)  # milliseconds
            }
            await websocket.send(json.dumps(ping_msg))
            print("[>] Sent ping message")

            # Wait for response
            try:
                response = await asyncio.wait_for(websocket.recv(), timeout=5.0)
                data = json.loads(response)
                print(f"[<] Received response: {data.get('type', 'Unknown')}")
                print("[+] WebSocket test PASSED")
                return True
            except asyncio.TimeoutError:
                print("[!] No response received (timeout)")
                print("   Server is accepting connections but not responding")
                return False

    except ConnectionRefusedError:
        print("[-] Connection refused - is the server running?")
        return False
    except Exception as e:
        print(f"[-] WebSocket test failed: {e}")
        return False


async def test_udp_socket():
    """Test UDP socket for voice data"""
    print("\n[*] Testing UDP voice socket...")
    print("   Target: localhost:9001")

    try:
        # Create UDP socket
        loop = asyncio.get_event_loop()
        transport, protocol = await loop.create_datagram_endpoint(
            asyncio.DatagramProtocol,
            remote_addr=('localhost', 9001)
        )

        print("[+] UDP socket created successfully!")

        # Send test packet
        test_data = b"PING"
        transport.sendto(test_data)
        print("[>] Sent test UDP packet")
        print("[+] UDP test PASSED (send only)")

        transport.close()
        return True

    except Exception as e:
        print(f"[-] UDP test failed: {e}")
        return False


async def main():
    """Run all tests"""
    print("=" * 60)
    print("VoIP Server Connection Test")
    print("=" * 60)
    print()

    # Test WebSocket
    ws_result = await test_websocket_connection()

    # Test UDP
    udp_result = await test_udp_socket()

    # Summary
    print()
    print("=" * 60)
    print("Test Summary")
    print("=" * 60)
    print(f"WebSocket (Control): {'[PASS]' if ws_result else '[FAIL]'}")
    print(f"UDP (Voice):         {'[PASS]' if udp_result else '[FAIL]'}")
    print()

    if ws_result and udp_result:
        print("[SUCCESS] All tests passed! Server is ready.")
        return 0
    else:
        print("[WARNING] Some tests failed. Check server logs.")
        return 1


if __name__ == "__main__":
    exit_code = asyncio.run(main())
    sys.exit(exit_code)
