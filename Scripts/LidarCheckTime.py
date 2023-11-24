#!/usr/bin/env python3

import socket
import time
from ctypes import Structure, c_int64
import struct
from multiprocessing import Process

import signal
import sys


class LidarData(Structure):
    _fields_ = [
        ("steadyTimestamp", c_int64),
        ("systemTimestamp", c_int64),
        ("deviceStartTime", c_int64),
    ]


f = open("./times.csv", "a+")
# f = open("/Users/krayzee/Downloads/times.csv", "a+")
f.write("port,system_time,device_time,diff,max_diff,min_diff\n")


class ReadMulticast:
    def __init__(self, grp: str = '239.0.0.230', port: int = 8001, is_all_groups: bool = True):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        if is_all_groups:
            # on this port, receives ALL multicast groups
            print("Binding *:{}".format(port))
            self.sock.bind(('', port))
        else:
            # on this port, listen ONLY to MCAST_GRP
            print("Binding group {}:{}".format(grp, port))
            self.sock.bind((grp, port))
        self.port = port
        self.grp = grp
        mreq = struct.pack("4sl", socket.inet_aton(self.grp), socket.INADDR_ANY)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        self.max_diff = 0
        self.min_diff = 999
        self.read()

    def read(self):
        while True:
            package = self.sock.recv(32)
            lidar_timestamp = (LidarData.from_buffer_copy(package)).deviceStartTime
            system_timestamp = int(round(time.time() * 1000))
            diff = system_timestamp - lidar_timestamp
            print("Port: {}, System: {}, Device: {}, Diff: {} (Max={}, Min={})".format(self.port,
                                                                                       system_timestamp,
                                                                                       lidar_timestamp,
                                                                                       diff,
                                                                                       self.max_diff,
                                                                                       self.min_diff))
            if diff > self.max_diff:
                self.max_diff = diff
            if diff < self.min_diff:
                self.min_diff = diff
            f.write(','.join([str(self.port), str(system_timestamp), str(lidar_timestamp),str(diff), str(self.max_diff), str(self.min_diff)]))
            f.write('\n')
            assert diff < 1000


PORTS = [8001, 8002, 8003, 8004]
lidars = {}
for MCAST_PORT in PORTS:
    lidars.update({MCAST_PORT: Process(target=ReadMulticast, kwargs={'port': MCAST_PORT})})
    lidars[MCAST_PORT].start()

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    for lidar in lidars:
        lidars[lidar].terminate()
    f.close()


signal.signal(signal.SIGINT, signal_handler)
print('Press Ctrl+C')
signal.pause()

