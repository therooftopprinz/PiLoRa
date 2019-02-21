#!/usr/bin/python
import socket

ADDR = "127.0.0.1"
RX = 7888
rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

rx.bind((ADDR, RX))

while True:
    data, addr = rx.recvfrom(256) # buffer size is 1024 bytes
    print "received: ", data
