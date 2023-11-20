#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan0', bustype='socketcan')

db = cantools.database.load_file('DBC/20160525_RMS_PM_CAN_DB.dbc')


while 1:
    message = can_bus.recv()
    if((message.arbitration_id == 165 + 112) or (message.arbitration_id == 165 + 147)):
        try:
            decoded_messaged = db.decode_message(165, message.data)
            print("Offset :", message.arbitration_id - 165, decoded_messaged)
        except:
            print("Can't decode msg id:", message.arbitration_id)
    else:
        print("Can't decode msg id:", message.arbitration_id)

