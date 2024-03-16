#!/usr/bin/env python3

import numpy as np
import socket
import signal
import struct
import copy
import math
import threading
from bitstring import ConstBitStream
from datetime import datetime


UDP_IP = ''
UDP_PORT = 8000

is_running = True

earth_a = 6378137.0   
RefPoint = [30.13, 59.995]
    
def signal_handler(signum, frame):
    import sys
    global is_running
    if (is_running):
        is_running = False
    else:
        sys.exit(0)

if __name__ == '__main__':

    print('Started')
    signal.signal(signal.SIGINT, signal_handler)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # For Linux only
    # sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    
    sock.bind((UDP_IP, UDP_PORT))

    print("Binding: ", UDP_IP, ":", UDP_PORT);

    timestamp = datetime.now()

    while is_running:

        bytes, addr = sock.recvfrom(1024)

        cur_timestamp = datetime.now()
        latency = int((cur_timestamp - timestamp).total_seconds() * 1000)

        assert len(bytes) == 72, "Protocol error"

        s = ConstBitStream(bytes)

        s.read('uint:8') #sync
        s.read('uintle:16') #imeatamp

        s.read('intle:24') / 10000 #acc_x
        s.read('intle:24') / 10000 #acc_y
        s.read('intle:24') / 10000 #acc_z

        s.read('intle:24') / 100000 #ang_vel_x
        s.read('intle:24') / 100000 #ang_vel_y
        s.read('intle:24') / 100000 #ang_vel_z

        s.read('uint:8') #nav_status
        s.read('uint:8') #chksum1
        lat = s.read('floatle:64') / math.pi * 180.0 #latitude
        lon = s.read('floatle:64') / math.pi * 180.0 #longitude
        s.read('floatle:32') #altitude
        s.read('intle:24') #vel_north
        s.read('intle:24') #vel_east
        s.read('intle:24') #vel_down

        s.read('intle:24') / 1000000 #yaw
        s.read('intle:24') / 1000000 #pitch
        s.read('intle:24') / 1000000 #roll

        s.read('uint:8') #chksum2
        s.read('uint:8') #channel

        timestamp = cur_timestamp

        print("latency: ", latency)
        
        #dX = (lon - RefPoint[0]) / 180 * earth_a / math.cos(lat / 180 * math.pi)
        #dY = (lat - RefPoint[1]) / 90 * earth_a / math.cos(lat / 180 * math.pi)
        #print("X:", dX, "Y:", dY)

    print('Finished')
