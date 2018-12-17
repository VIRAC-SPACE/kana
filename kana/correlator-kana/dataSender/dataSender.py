#! /usr/bin/env python
import json
import sys
import re
import socket
import os
import struct


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


def getDataSize():
    ctrlpath = sys.argv[1]
    ctrlfile = open(ctrlpath, "r")
    ctrlinfo = json.loads(ctrlfile.read())

    discretization = int(ctrlinfo["discretization(Hz)"])
    length = ctrlinfo["rt_batch_length"]
    length = float(re.match(r"(\d*\.?\d*)s", length).group(1))
    size = int(length * discretization) // 8
    return size


def getDataFilePath():
    ctrlpath = sys.argv[1]
    station = sys.argv[2]
    root = os.path.dirname(os.path.dirname(ctrlpath))
    dataDir = root + "/data/"
    files = os.listdir(dataDir)
    for file in files:
        for ch in file.split("_"):
            if ch.upper() == station.upper():
                return dataDir + file
    return ""


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: dataSender.py [path_to_ctrl_file] [station]")
        exit(0)

    size = getDataSize()
    datafile = open(getDataFilePath(), "rb")
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
            sendMsg(conn, data)
            recvMsg(conn)
            data = datafile.read(size)
    except KeyboardInterrupt:
        print()
        sys.exit()

    print("{} has disconnected.".format(addr))
    datafile.close()
    conn.close()
