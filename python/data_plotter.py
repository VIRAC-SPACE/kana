#! /usr/bin/env python3

import sys
import _thread
import socket
import struct

import matplotlib.animation as animation
import matplotlib.pyplot as plt


def send_message(sock, msg):
    msg = struct.pack("q", len(msg)) + msg
    sock.sendall(msg)


def receive_message(sock):
    raw_msg_len = receive_value(sock, 8)
    if not raw_msg_len:
        return None
    msg_len = struct.unpack("q", raw_msg_len)[0]
    return receive_value(sock, msg_len)


def receive_value(sock, n):
    data = b""
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data += packet
    return data


def main():
    fig = plt.figure()
    ax1 = fig.add_subplot(1, 1, 1)

    server = sys.argv[1]
    port = sys.argv[2]

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((server, port))
    print("Bound to {}:{}".format(server, port))

    s.listen(1)
    conn, addr = s.accept()
    print("{} has connected.".format(addr))

    window = receive_message(conn)
    window = struct.unpack("q", window)[0]
    xs, ys = [], []

    def update_plot(args):
        while True:
            msg = receive_message(conn)
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
    _thread.start_new_thread(update_plot, (0,))
    plt.show()

    print("{} has disconnected.".format(addr))
    conn.close()


if __name__ == "__main__":
    main()
    sys.exit(0)
