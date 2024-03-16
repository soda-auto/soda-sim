#!/usr/bin/env python3

import socket
import sys
import threading
import struct
import time
import signal

recv_addr = ('', 7078)

print("Starting listening udp://{}:{}".format(recv_addr[0], recv_addr[1]))

IN_MSG_HEADER_FMT = '=fffffffffffffbbbQ'
IN_MSG_HEADER_LENGTH = struct.calcsize(IN_MSG_HEADER_FMT)

recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.settimeout(0.1)
#recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
recv_sock.bind(recv_addr)

global do_work
do_work = True

def recv_func():
    while(do_work):
        bytes = None
        try:
            bytes, addr = recv_sock.recvfrom(1024)
        except socket.timeout:
            continue
        assert len(bytes) == IN_MSG_HEADER_LENGTH, "Protocol error, got {} bytes, excepted {}".format(len(bytes), IN_MSG_HEADER_LENGTH)
        in_data = struct.unpack(IN_MSG_HEADER_FMT, bytes)
        
        print('---------------------------------------------------')
        print('Wheel_FL (ang vel, brake_torq, torq): {}rad/s; {}N/m; {}N/m;'.format(in_data[0], in_data[1], in_data[2]))
        print('Wheel_FR (ang vel, brake_torq, torq): {}rad/s; {}N/m; {}N/m;'.format(in_data[3], in_data[4], in_data[5]))
        print('Wheel_RL (ang vel, brake_torq, torq): {}rad/s; {}N/m; {}N/m;'.format(in_data[6], in_data[7], in_data[8]))
        print('Wheel_RR (ang vel, brake_torq, torq): {}rad/s; {}N/m; {}N/m;'.format(in_data[9], in_data[10], in_data[11]))
        print('Steer: {}rad'.format(in_data[12]))
        print('Gear state: {}'.format(in_data[13]))
        print('Gear num: {}'.format(in_data[14]))
        print('Mode: {}'.format(in_data[15]))
        print('Timestamp [ns]: {};'.format(in_data[16]))
        
recv_thread = threading.Thread(target=recv_func)
recv_thread.start()

def signal_handler(sig, frame):
    global do_work
    do_work = False
signal.signal(signal.SIGINT, signal_handler)

while recv_thread.is_alive():
    recv_thread.join(timeout=0.1)

recv_sock.close()
