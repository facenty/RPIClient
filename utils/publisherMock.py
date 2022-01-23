import socket
import sys
import json
from datetime import datetime
import time

HOST, PORT = "localhost", 9999

# Create a socket (SOCK_STREAM means a TCP socket)
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    # Connect to server and send data
    sock.connect((HOST, PORT))
    print(json.dumps({"ClientType": "PUBLISHER"}))
    sock.sendall(bytes(json.dumps({"ClientType": "PUBLISHER"}), "ascii"))
    if not str(sock.recv(1024).strip(), "ascii") == "OK":
        sys.exit("Handshake error")

    while True:
        try:
            sock.sendall(bytes(str(datetime.now()), "ascii"))
            time.sleep(2)
        except:
            sys.exit("Failed to send data")
