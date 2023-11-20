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
from sys import platform

#camera_addr = "ipc:///tmp/ue4_imu"

UDP_IP = ''
UDP_PORT = 8000
period = 300


is_running = True

global pitch
global roll
global yaw
global acc_x
global acc_y
global acc_z
global ang_vel_x
global ang_vel_y
global ang_vel_z
global latency
    
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

    
    #ctx = zmq.Context()
    #sock = ctx.socket(zmq.SUB)
    #sock.setsockopt(zmq.SUBSCRIBE, b'')
    #sock.connect(camera_addr)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if platform == "linux" or platform == "linux2":
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    sock.bind((UDP_IP, UDP_PORT))

    print("Binding: ", UDP_IP, ":", UDP_PORT);

    xs = []
    for i in range(period): xs.append(i)
    pitch = [0] * period
    roll = [0] * period
    yaw = [0] * period
    acc_x = [0] * period
    acc_y = [0] * period
    acc_z = [0] * period
    ang_vel_x = [0] * period
    ang_vel_y = [0] * period
    ang_vel_z = [0] * period
    latency = [0] * period

    lock = threading.Lock()

    fig = plt.figure("IMU")
    #fig_lat = plt.figure("Latency")

    ax01 = plt.subplot2grid((2, 2), (0, 0))
    ax02 = plt.subplot2grid((2, 2), (0, 1))
    ax03 = plt.subplot2grid((2, 2), (1, 0))
    ax04 = plt.subplot2grid((2, 2), (1, 1))

    ax01.set_title('Linear Acceleration')
    ax02.set_title('Angular Velocity')
    ax03.set_title('Angels')
    ax04.set_title('Latency')

    ax01.set_ylim(-20 ,20)
    ax02.set_ylim(-1, 1)
    ax03.set_ylim(-3.5, 3.5)
    ax04.set_ylim(-10, 50)

    ax01.set_ylabel('m/c^2')
    ax02.set_ylabel('rad/c')
    ax03.set_ylabel('rad')
    ax04.set_ylabel('msec')

    p00, = ax01.plot(xs, acc_x, 'r-', label="x")
    p01, = ax01.plot(xs, acc_y, 'g-', label="y")
    p02, = ax01.plot(xs, acc_z, 'b-', label="z")

    p10, = ax02.plot(xs, ang_vel_x, 'r-', label="x")
    p11, = ax02.plot(xs, ang_vel_y, 'g-', label="y")
    p12, = ax02.plot(xs, ang_vel_z, 'b-', label="z")

    p20, = ax03.plot(xs, pitch, 'r-', label="pitch")
    p21, = ax03.plot(xs, roll, 'g-', label="roll")
    p22, = ax03.plot(xs, yaw, 'b-', label="yaw")

    p30, = ax04.plot(xs, latency, 'b-', label="latency")

    ax01.legend([p00, p01, p02], [p00.get_label(), p01.get_label(), p02.get_label()])
    ax02.legend([p10, p11, p12], [p10.get_label(), p11.get_label(), p12.get_label()])
    ax03.legend([p20, p21, p22], [p20.get_label(), p21.get_label(), p22.get_label()])
    ax03.legend([p30], [p30.get_label()])

    ax01.grid(True)
    ax02.grid(True)
    ax03.grid(True)
    ax04.grid(True)

    def anim_func(i):
        lock.acquire()
        p00.set_data(xs, copy.copy(acc_x))
        p01.set_data(xs, copy.copy(acc_y))
        p02.set_data(xs, copy.copy(acc_z))
        p10.set_data(xs, copy.copy(ang_vel_x))
        p11.set_data(xs, copy.copy(ang_vel_y))
        p12.set_data(xs, copy.copy(ang_vel_z))
        p20.set_data(xs, copy.copy(pitch))
        p21.set_data(xs, copy.copy(roll))
        p22.set_data(xs, copy.copy(yaw))
        p30.set_data(xs, copy.copy(latency))
        lock.release()
        return p00, p01, p02, p10, p11, p12, p20, p21, p22, p30

    anim = FuncAnimation(fig, anim_func, interval=33)
    

    #first_stamp = 0

    def loop():
        global pitch
        global roll
        global yaw
        global acc_x
        global acc_y
        global acc_z
        global ang_vel_x
        global ang_vel_y
        global ang_vel_z
        global latency

        index = 0
        timestamp = datetime.now()

        while is_running:

            bytes, addr = sock.recvfrom(1024)

            cur_timestamp = datetime.now()
            latency.append(int((cur_timestamp - timestamp).total_seconds() * 1000))

            assert len(bytes) == 72, "Protocol error len = %s" % len(bytes)

            s = ConstBitStream(bytes)

            lock.acquire()

            s.read('uint:8') #sync
            device_time = s.read('uintle:16')

            acc_x.append(s.read('intle:24') / 10000)
            acc_y.append(s.read('intle:24') / 10000)
            acc_z.append(s.read('intle:24') / 10000)

            ang_vel_x.append(s.read('intle:24') / 100000)
            ang_vel_y.append(s.read('intle:24') / 100000)
            ang_vel_z.append(s.read('intle:24') / 100000)

            s.read('uint:8') #nav_status
            s.read('uint:8') #chksum1
            lat = s.read('floatle:64') / math.pi * 180.0 #latitude
            lon = s.read('floatle:64') / math.pi * 180.0  #longitude
            s.read('floatle:32') #altitude
            s.read('intle:24') #vel_north
            s.read('intle:24') #vel_east
            s.read('intle:24') #vel_down

            r_yaw = s.read('intle:24') / 1000000
            r_pitch = s.read('intle:24') / 1000000
            r_roll = s.read('intle:24')  / 1000000       

            yaw.append(r_yaw)
            pitch.append(r_pitch)
            roll.append(r_roll)

            s.read('uint:8') #chksum2
            channel = s.read('uint:8')

            if channel == 0:
                gps_minutes = s.read('uintle:32')
                num_sats = s.read('uint:8')
                position_mode = s.read('uint:8') 
                velocity_mode = s.read('uint:8') 
                orientation_mode = s.read('uint:8')
                print('gps_minutes:', gps_minutes, "; device_time ", device_time)
            else: 
                s.read('intle:64') #placeholder

            s.read('uint:8') #chksum3


            #xs.append(msg[10] - first_stamp)
            pitch = pitch[-period:]
            roll = roll[-period:]
            yaw = yaw[-period:]
            acc_x = acc_x[-period:] 
            acc_y = acc_y[-period:]
            acc_z = acc_z[-period:]
            ang_vel_x = ang_vel_x[-period:]
            ang_vel_y = ang_vel_y[-period:]
            ang_vel_z = ang_vel_z[-period:]
            latency = latency[-period:]
            #xs = xs[-period:]
            lock.release()

            

            timestamp = cur_timestamp

            print('lat:', lat, "; lon ", lon, "; yaw ", r_yaw,  "; pitch ", r_pitch,  "; roll ", r_roll)


    thread = threading.Thread(target=loop)
    thread.start()
    plt.show()
    print('Finished')

