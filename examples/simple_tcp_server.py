#!/usr/bin/env python3
"""
Simple TCP server for testing the TeeStream socket example.
This server listens on a specified port and prints all received data.
"""

import socket
import argparse
import sys
import signal

def signal_handler(sig, frame):
    print("\nShutting down server...")
    sys.exit(0)

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Simple TCP server for testing TeeStream')
    parser.add_argument('--port', type=int, default=12345, help='Port to listen on (default: 12345)')
    parser.add_argument('--host', type=str, default='0.0.0.0', help='Host to bind to (default: 0.0.0.0)')
    args = parser.parse_args()

    # Register signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)

    # Create a TCP/IP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # Allow reuse of the address
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # Bind the socket to the port
    server_address = (args.host, args.port)
    print(f'Starting server on {args.host}:{args.port}')
    server_socket.bind(server_address)
    
    # Listen for incoming connections
    server_socket.listen(1)
    
    print('Waiting for a connection...')
    print('Press Ctrl+C to exit')
    
    try:
        # Wait for a connection
        client_socket, client_address = server_socket.accept()
        print(f'Connection from {client_address[0]}:{client_address[1]}')
        
        # Receive and print data
        while True:
            data = client_socket.recv(4096)
            if not data:
                print('Client disconnected')
                break
            
            # Print received data
            print(f'Received: {data.decode("utf-8")}', end='')
            
    except Exception as e:
        print(f'Error: {e}')
    finally:
        # Clean up the connection
        try:
            client_socket.close()
        except:
            pass
        server_socket.close()
        print('Server shut down')

if __name__ == '__main__':
    main() 