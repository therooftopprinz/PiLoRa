#!/usr/bin/python
import socket

ADDR = "127.0.0.1"
TX_PORT = 3000
LORA_TX_PORT = 8001

txsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
loratxsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

loratxsock.bind((ADDR, LORA_TX_PORT))

for i in range(0,1024*64):
    txsock.sendto("TEST MESSAGE " + str(i), (ADDR, TX_PORT))
    data, addr = loratxsock.recvfrom(256) # buffer size is 1024 bytes
    print "transfered: ", data
