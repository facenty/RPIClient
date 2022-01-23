import socket
import json
import sys

HOST, PORT = "localhost", 9999

# Create a socket (SOCK_STREAM means a TCP socket)
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    # Connect to server and send data
    sock.connect((HOST, PORT))
    sock.sendall(bytes(json.dumps({"ClientType": "SUBSCRIBER"}), "ascii"))
    if not str(sock.recv(1024).strip(), "ascii") == "OK":
        sys.exit("Handshake error")
    while True:
        data = sock.recv(1024)
        if len(data) == 0:
            break
        print(str(data, "ascii"))
