#! /usr/bin/env python
import json
import os
import re
import socket
import struct
import sys


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


def get_data_size():
    ctrl_path = sys.argv[1]
    ctrl_file = open(ctrl_path, "r")
    ctrl_info = json.loads(ctrl_file.read())

    discretization = int(ctrl_info["discretization(Hz)"])
    length = ctrl_info["rt_batch_length"]
    length = float(re.match(r"(\d*\.?\d*)s", length).group(1))
    size = int(length * discretization) // 8
    return size


def get_data_file_path():
    ctrl_path = sys.argv[1]
    station = sys.argv[2]
    root = os.path.dirname(os.path.dirname(ctrl_path))
    data_dir = root + "/data/"
    files = os.listdir(data_dir)
    for file in files:
        for ch in file.split("_"):
            if ch.upper() == station.upper():
                return data_dir + file
    return ""


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: dataSender.py [path_to_ctrl_file] [station]")
        exit(0)

    size = get_data_size()
    datafile = open(get_data_file_path(), "rb")
    datafile.seek(60)

    server = "localhost"
    port = 5678

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((server, port))
    print("Bound to {}:{}".format(server, port))

    s.listen(1)
    conn, addr = s.accept()
    print("{} has connected.".format(addr))

    try:
        data = datafile.read(size)
        while data != "":
            send_message(conn, data)
            receive_message(conn)
            data = datafile.read(size)
    except KeyboardInterrupt:
        print()
        sys.exit()

    print("{} has disconnected.".format(addr))
    datafile.close()
    conn.close()
