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
UDP_PORT = 3003

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

        cur_timestamp = datetime.now()
        latency = int((cur_timestamp - timestamp).total_seconds() * 1000)

        #assert len(bytes) == 72, "Protocol error"

        s = ConstBitStream(bytes)

        reserved = s.read('uintle:32')
        object_type = s.read('uintle:32') 
        object_id = s.read('uintle:32')
        timestamp = s.read('floatle:64') 
        east = s.read('floatle:64') 
        north = s.read('floatle:64') 
        up = s.read('floatle:64') 
        heading = s.read('floatle:64') 

        timestamp = cur_timestamp

        print("object_id:", object_id, "object_type:", object_type, "heading:", heading)

    print('Finished')
