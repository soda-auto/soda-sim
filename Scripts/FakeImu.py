#!/usr/bin/env python3


from socket import *
import sys
import time

port = 3000

s = socket(AF_INET, SOCK_DGRAM)
s.bind(('', 0))
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
data = bytearray([0]*72)

while 1:
    s.sendto(data, ('<broadcast>', port))
    time.sleep(0.01)
    # print ("Sent")
