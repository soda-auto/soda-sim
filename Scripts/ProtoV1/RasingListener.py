#!/usr/bin/env python3

import socket
import sys
import threading
import struct
import time
import signal
import math

recv_addr = ('', 5010)

print("Starting listening udp://{}:{}".format(recv_addr[0], recv_addr[1]))

IN_MSG_HEADER_FMT = '=fffiffbbQQQ'
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
        print('left_border_offset [m]: {}'.format(in_data[0]))
        print('right_border_offset [m]: {}'.format(in_data[1]))
        print('center_line_yaw [deg]: {}'.format(math.degrees(in_data[2])))
        print('lap_caunter: {}'.format(in_data[3]))
        print('covered_distance_current_lap [m]: {}'.format(in_data[4]))
        print('covered_distance_full [m]: {}'.format(in_data[5]))
        print('border_is_valid: {}'.format(in_data[6]))
        print('lap_counter_is_valid: {}'.format(in_data[7]))
        print('timestemp [ns]: {}'.format(in_data[8]))
        print('start_timestemp [ns]: {}'.format(in_data[9]))
        print('lap_timestemp [ns]: {}'.format(in_data[10]))

recv_thread = threading.Thread(target=recv_func)
recv_thread.start()

def signal_handler(sig, frame):
    global do_work
    do_work = False
signal.signal(signal.SIGINT, signal_handler)

while recv_thread.is_alive():
    recv_thread.join(timeout=0.1)

recv_sock.close()
