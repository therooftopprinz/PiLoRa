#!/usr/bin/python
import socket
import time

ADDR = "127.0.0.1"
LORA_RX = 7899
sx1278_tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

i=0
while True:
    data = "TEST MESSAGE " + str(i)
    sx1278_tx.sendto(data, (ADDR, LORA_RX))
    print "sent: ", data
    time.sleep(0.015)
    i += 1