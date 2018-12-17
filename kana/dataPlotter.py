#! /usr/bin/env python
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import struct
import socket
import _thread


def sendMsg(sock, msg):
    msg = struct.pack("q", len(msg)) + msg
    sock.sendall(msg)


def recvMsg(sock):
    rawMsgLen = recvall(sock, 8)
    if not rawMsgLen:
        return None
    msgLen = struct.unpack("q", rawMsgLen)[0]
    return recvall(sock, msgLen)


def recvall(sock, n):
    data = b""
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data += packet
    return data


fig = plt.figure()
ax1 = fig.add_subplot(1, 1, 1)

server = "localhost"
port = 7890

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((server, port))
print("Bound to {}:{}".format(server, port))

s.listen(1)
conn, addr = s.accept()
print("{} has connected.".format(addr))

window = recvMsg(conn)
window = struct.unpack("q", window)[0]

xs, ys = [], []


def updatePlot(args):
    while True:
        msg = recvMsg(conn)
        fmt = "{}f".format(len(msg) // 4)
        data = struct.unpack(fmt, msg)

        step = window / len(data)
        xs.clear()
        ys.clear()
        for i in range(len(data)):
            xs.append(step * i)
            ys.append(data[i])


def animate(i):
    ax1.clear()
    ax1.ticklabel_format(style="sci", axis="x", scilimits=(0, 0))
    ax1.plot(xs, ys, linewidth=2.0)


ani = animation.FuncAnimation(fig, animate, interval=500)
_thread.start_new_thread(updatePlot, (0,))
plt.show()

print("{} has disconnected.".format(addr))
conn.close()
