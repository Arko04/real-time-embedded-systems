import pyautogui
import socket
import os

print(socket.gethostbyname_ex(socket.gethostname()))

hostname = socket.gethostname()
localIP = socket.gethostbyname_ex(socket.gethostname())[2][1]

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind((localIP, 8081))

print(f"Listening on {localIP}: {s.getsockname()[1]}")

try:
    while True:
        data, addr = s.recvfrom(1024)
        command = data.decode()
        print(f"Received from {addr}: {command}")
        
        if command[0] in ['C', 'S']:
            ack_message = "ACK_RECEIVED"
            s.sendto(ack_message.encode(), addr)
except KeyboardInterrupt:
    print("\nServer stopped by user")
finally:
    s.close()