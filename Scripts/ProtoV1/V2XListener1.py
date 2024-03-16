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
UDP_PORT = 5000

is_running = True

    
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
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    sock.bind((UDP_IP, UDP_PORT))

    print("Binding: ", UDP_IP, ":", UDP_PORT);

    timestamp = datetime.now()

    while is_running:

        bytes, addr = sock.recvfrom(1024)

        #cur_timestamp = datetime.now()
        #latency = int((cur_timestamp - timestamp).total_seconds() * 1000)

        #assert len(bytes) == 72, "Protocol error"

        s = ConstBitStream(bytes)

        ID = s.read('uintle:16')
        Timestamp = s.read('uintle:64')

        BoundingBox0_X = s.read('floatle:64') 
        BoundingBox0_Y = s.read('floatle:64') 
        BoundingBox1_X = s.read('floatle:64') 
        BoundingBox1_Y = s.read('floatle:64') 
        BoundingBox2_X = s.read('floatle:64') 
        BoundingBox2_Y = s.read('floatle:64') 
        BoundingBox3_X = s.read('floatle:64') 
        BoundingBox3_Y = s.read('floatle:64') 

        BoundingBox3D0_X = s.read('floatle:64') 
        BoundingBox3D0_Y = s.read('floatle:64') 
        BoundingBox3D0_Z = s.read('floatle:64') 
        BoundingBox3D1_X = s.read('floatle:64') 
        BoundingBox3D1_Y = s.read('floatle:64') 
        BoundingBox3D1_Z = s.read('floatle:64') 
        BoundingBox3D2_X = s.read('floatle:64') 
        BoundingBox3D2_Y = s.read('floatle:64') 
        BoundingBox3D2_Z = s.read('floatle:64') 
        BoundingBox3D3_X = s.read('floatle:64') 
        BoundingBox3D3_Y = s.read('floatle:64') 
        BoundingBox3D3_Z = s.read('floatle:64') 
        BoundingBox3D4_X = s.read('floatle:64') 
        BoundingBox3D4_Y = s.read('floatle:64') 
        BoundingBox3D4_Z = s.read('floatle:64') 
        BoundingBox3D5_X = s.read('floatle:64') 
        BoundingBox3D5_Y = s.read('floatle:64') 
        BoundingBox3D5_Z = s.read('floatle:64') 
        BoundingBox3D6_X = s.read('floatle:64') 
        BoundingBox3D6_Y = s.read('floatle:64') 
        BoundingBox3D6_Z = s.read('floatle:64') 
        BoundingBox3D7_X = s.read('floatle:64') 
        BoundingBox3D7_Y = s.read('floatle:64') 
        BoundingBox3D7_Z = s.read('floatle:64') 

        Longitude = s.read('floatle:64') 
        Latitude = s.read('floatle:64') 
        Altitude = s.read('floatle:64') 
        Roll = s.read('floatle:64') 
        Pitch = s.read('floatle:64') 
        Yaw = s.read('floatle:64') 

        AngVelRoll = s.read('floatle:64') 
        AngVelPitch = s.read('floatle:64') 
        AngVelYaw = s.read('floatle:64') 

        VelForward = s.read('floatle:64') 
        VelLateral = s.read('floatle:64') 
        VelUp = s.read('floatle:64') 


        #timestamp = cur_timestamp

        print("ID:", ID, "Longitude:", math.degrees(Longitude), "Latitude:", math.degrees(Latitude), "Altitude:", Altitude)

    print('Finished')
