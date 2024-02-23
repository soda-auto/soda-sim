#!/usr/bin/env python3

import socket
import sys
import threading
import struct
import time
import signal
from pynput import keyboard

send_addr = ('127.0.0.1', 7077)
sens_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("Starting sending udp://{}:{}".format(send_addr[0], send_addr[1]))
print("Use A,W,S,D,Q,E,1,2,3,4 to control the vehicle")

class GenericVehicleControlMode1:
    def __init__(self):
        self.steer_req = 0
        self.drive_effort_req = 0
        self.target_speed_req = 0
        self.gear_state_req = 0
        self.gear_num_req = 0 
        self.steer_req_mode = 0 # By Ratio
        self.drive_effort_req_mode = 0 # By Ratio
        
    def print(self):
        print("Steer: ", self.steer_req, "; DriveEffort: ", self.drive_effort_req, "; GearState: ", self.gear_state_req, "; GearNum:", self.gear_num_req, ";")
        
    def pack(self):
        return struct.pack('=fffbbbb', 
            self.steer_req, 
            self.drive_effort_req, 
            self.target_speed_req, 
            self.gear_state_req, 
            self.gear_num_req,
            self.steer_req_mode,
            self.drive_effort_req_mode)

global do_work
global listener
global control

do_work = True;
control = GenericVehicleControlMode1();
 
def send_func():
    while(do_work):
        sens_sock.sendto(control.pack(), send_addr)
        time.sleep(0.05)

def on_press(key):
    global control
    
    try:
        key.char
    except AttributeError:
        return

    if key.char == 'a': 
        control.steer_req -= 0.1
        control.print()
    elif key.char == 'd': 
        control.steer_req += 0.1
        control.print()
    elif key.char == 'w': 
        control.drive_effort_req += 0.1
        control.print()
    elif key.char == 's': 
        control.drive_effort_req -= 0.1
        control.print()
    elif key.char == '1': 
        control.gear_state_req = 0
        control.gear_num_req = 0
        control.print()
    elif key.char == '2': 
        control.gear_state_req = 1
        control.gear_num_req = 0
        control.print()
    elif key.char == '3': 
        control.gear_state_req = 2
        control.gear_num_req = 0
        control.print()
    elif key.char == '4': 
        control.gear_state_req = 3
        control.gear_num_req = 0
        control.print()
    elif key.char == 'q':
        control.gear_num_req -= 1
        control.print()
    elif key.char == 'e':
        control.gear_num_req += 1
        control.print()

     
def signal_handler(sig, frame):
    global listener
    global do_work
    do_work = False
    listener.stop()
    sens_sock.close()

listener = keyboard.Listener(on_press=on_press) 
listener.start()

send_thread = threading.Thread(target=send_func)
send_thread.start()

signal.signal(signal.SIGINT, signal_handler)

while send_thread.is_alive():
    send_thread.join(timeout=0.1)
