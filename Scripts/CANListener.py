#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan0', bustype='socketcan')
dbc = cantools.database.load_file('DBC/Honeywell_TARS-HCASS.dbc')

while 1:
    message = can_bus.recv()
    try:
        decoded_messaged = dbc.decode_message(message.arbitration_id, message.data)
        print(decoded_messaged)
    except:
        print("Can't decode msg id:", message.arbitration_id)

