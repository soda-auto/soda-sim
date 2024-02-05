#!/usr/bin/env python3

import socket
import sys
import threading
import struct
import time
from pynput.keyboard import Key, Listener, KeyCode

send_addr = ('127.0.0.1', 7077)
sens_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

class GenericVehicleControlMode1:
    def __init__(self):
        self.steer_req = 0
        self.acc_deacc_req = 0
        self.gear_state_req = 0
        self.gear_num_req = 0
        
    def print(self):
        print("Steer: ", self.steer_req, "; Acc: ", self.acc_deacc_req, "; GearState: ", self.gear_state_req, "; GearNum:", self.gear_num_req, ";")

global do_work
global listener
global control

do_work = True;
control = GenericVehicleControlMode1();
 
def send_func():
    while(do_work):
        bytes = struct.pack('=ffbb', control.steer_req, control.acc_deacc_req, control.gear_state_req, control.gear_num_req)
        sens_sock.sendto(bytes, send_addr)
        time.sleep(0.05)

send_thread = threading.Thread(target=send_func)
send_thread.start()

def on_press(key):
    global listener
    global do_work
    global control

    try:
        if key.char == 'z':
            do_work = False
            listener.stop()
        elif key.char == 'a': 
            control.steer_req += 0.1
            control.print()
        elif key.char == 'd': 
            control.steer_req -= 0.1
            control.print()
        elif key.char == 'w': 
            control.acc_deacc_req += 0.1
            control.print()
        elif key.char == 's': 
            control.acc_deacc_req -= 0.1
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
    except AttributeError:
        pass

with Listener(on_press=on_press) as listener:
    listener.join()

sens_sock.close()
