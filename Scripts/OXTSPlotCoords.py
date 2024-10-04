#!/usr/bin/env python3

import numpy as np
#import zmq
import socket
import signal
import struct
import copy
import math
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import threading
from bitstring import ConstBitStream
from datetime import datetime


#camera_addr = "ipc:///tmp/ue4_imu"

UDP_IP = ''
UDP_PORT = 8000
period = 10000


is_running = True

global x
global y


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

    x = [] 
    y = [] 

    lock = threading.Lock()

    fig = plt.figure("IMU")

    ax01 = fig.add_subplot(1,1,1)
    ax01.grid(True)

    def anim_func(i):
        lock.acquire()
        ax01.clear()
        ax01.plot(copy.copy(x), copy.copy(y))
        ax01.plot(x[-1], y[-1], 'go')
        lock.release()

    anim = FuncAnimation(fig, anim_func, interval=33)

    def loop():
        global x
        global y

        while is_running:

            bytes, addr = sock.recvfrom(1024)

            assert len(bytes) == 72, "Protocol error"
            s = ConstBitStream(bytes)

            #if(first_stamp == 0) : first_stamp = msg[10]
            lock.acquire()

            s.read('uint:8') #sync
            s.read('uintle:16')

            s.read('intle:24')
            s.read('intle:24') 
            s.read('intle:24')

            s.read('intle:24') 
            s.read('intle:24') 
            s.read('intle:24')

            s.read('uint:8') #nav_status
            s.read('uint:8') #chksum1

            y.append(s.read('floatle:64') / math.pi * 180.0) #latitude
            x.append(s.read('floatle:64') / math.pi * 180.0)  #longitude

            s.read('floatle:32') #altitude
            s.read('intle:24') #vel_north
            s.read('intle:24') #vel_east
            s.read('intle:24') #vel_down

            s.read('intle:24')
            s.read('intle:24') 
            s.read('intle:24')

            s.read('uint:8') #chksum2
            s.read('uint:8')
            s.read('intle:64')

            x = x[-period:]
            y = y[-period:]

            lock.release()


    thread = threading.Thread(target=loop)
    thread.start()
    plt.show()
    print('Finished')

