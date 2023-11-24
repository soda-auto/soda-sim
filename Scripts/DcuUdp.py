#!/usr/bin/env python3

import socket
import sys
import threading
import struct
import time
from pynput.keyboard import Key, Listener, KeyCode

recv_addr = ('', 7078)
send_addr = ('127.0.0.1', 7077)

OUT_MSG_HEADER_FMT = '=ffb'
OUT_MSG_HEADER_LENGTH = struct.calcsize(OUT_MSG_HEADER_FMT)
IN_MSG_HEADER_FMT = '=fbbffffffffffffdddffffffffffff'
IN_MSG_HEADER_LENGTH = struct.calcsize(IN_MSG_HEADER_FMT)

recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
recv_sock.bind(recv_addr)
sens_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

global do_work
global steer_req
global acc_deacc_req
global gear_req

do_work = True
steer_req = 0
acc_deacc_req = 0
gear_req = 3


def unix_getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())          # Raw read
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def recv_func():
    while(do_work):
        bytes, addr = recv_sock.recvfrom(1024)
        assert len(bytes) == IN_MSG_HEADER_LENGTH, "Protocol error, got {} bytes, excepted {}".format(len(bytes), IN_MSG_HEADER_LENGTH)
        in_data = struct.unpack(IN_MSG_HEADER_FMT, bytes)
        print('Steer: ', in_data[0])
        print('Gear: ', in_data[1])
        print('Mode: ', in_data[2])
        print('Velocity (X, Y, Z): ', in_data[3], in_data[4], in_data[5])
        print('Acc (X, Y, Z): ', in_data[6], in_data[7], in_data[8])
        print('AngVelocity (X, Y, Z): ', in_data[9], in_data[10], in_data[11])
        print('Yaw, Pitch, Roll: ', in_data[12], in_data[13], in_data[14])
        print('Coord: ', in_data[15], in_data[16], in_data[17])
        print('RPM: ', in_data[18], in_data[19], in_data[20], in_data[21])
        print('Brake: ', in_data[22], in_data[23], in_data[24], in_data[25])
        print('Torq: ', in_data[26], in_data[27], in_data[28], in_data[29])

def send_func():
    while(do_work):
        bytes = struct.pack(OUT_MSG_HEADER_FMT, steer_req, acc_deacc_req, gear_req)
        sens_sock.sendto(bytes, send_addr)
        time.sleep(0.05)

recv_thread = threading.Thread(target=recv_func)
recv_thread.start()

send_thread = threading.Thread(target=send_func)
send_thread.start()

global listener

def on_press(key):
    global listener
    global do_work
    global steer_req
    global acc_deacc_req
    global gear_req
    try:
        if key.char == 'q': 
            do_work = False
            listener.stop()
        elif key.char == 'a': steer_req += 0.1
        elif key.char == 'd': steer_req -= 0.1
        elif key.char == 'w': acc_deacc_req += 0.1
        elif key.char == 's': acc_deacc_req -= 0.1
        elif key.char == 'g': gear_req = (gear_req + 1) % 3

    except AttributeError:
        pass

with Listener(on_press=on_press) as listener:
    listener.join()


recv_sock.close()
sens_sock.close()
