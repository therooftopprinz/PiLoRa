#!/usr/bin/python
import socket

ADDR = "127.0.0.1"
RX = 3000
LORA_RX = 8000
lorarx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

rx.bind((ADDR, RX))

for i in range(0,2000):
    lorarx.sendto("TEST MESSAGE " + str(i), (ADDR, LORA_RX))
    data, addr = rx.recvfrom(256) # buffer size is 1024 bytes
    print "transfered: ", data
