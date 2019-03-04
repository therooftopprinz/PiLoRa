#!/usr/bin/python
import socket
import time

ADDR = "127.0.0.1"
RX = 7888
rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

rx.bind((ADDR, RX))

started = False
transferred = 0
packets = 0
while True:
    data, addr = rx.recvfrom(256) # buffer size is 1024 bytes
    if (False == started):
        start = time.time()
        started = True
    transferred += len(data)
    packets += 1
    print "received: ", data
    if ("end"==data):
        elapsed = time.time() - start
        started = False
        print "Elapsed: ", elapsed
        print "transferred: ", transferred
        print "Data Rate: " + str((transferred/elapsed)*8) + " bps"
        print "Packet Rate: " + str(packets/elapsed) + " packets/s"
        transferred = 0
        packets = 0