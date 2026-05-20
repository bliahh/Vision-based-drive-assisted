import socket

s = socket.socket()
s.bind(("0.0.0.0", 5005))
s.listen(1)

conn, addr = s.accept()
print("Connected:", addr)

buffer = ""

while True:
    data = conn.recv(4096).decode()
    if not data:
        break

    buffer += data

    while "\n" in buffer:
        msg, buffer = buffer.split("\n", 1)
        print(msg)