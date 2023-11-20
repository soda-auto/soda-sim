#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan0', bustype='socketcan')

#dbc = cantools.database.load_file('DBC/20160525_RMS_PM_CAN_DB.dbc')
#dbc = cantools.database.load_file('DBC/ADSMeasurementsTelemetry_v7.dbc')
#dbc = cantools.database.load_file('DBC/Arrival_dcu_Bus.dbc')
#dbc = cantools.database.load_file('DBC/Arrival_dcu_T6.dbc')
#dbc = cantools.database.load_file('DBC/VehicleAPI_V24.dbc')
#dbc = cantools.database.load_file('DBC/ADSSafetyLayer_v3.dbc')
#dbc = cantools.database.load_file('DBC/VAPI3_SBU_SCU.dbc')
#dbc = cantools.database.load_file('DBC/ADSSafetyLayer_v6.dbc')
dbc = cantools.database.load_file('DBC/Honeywell_TARS-HCASS.dbc')

while 1:
    message = can_bus.recv()
    try:
        decoded_messaged = dbc.decode_message(message.arbitration_id, message.data)
        print(decoded_messaged)
    except:
        print("Can't decode msg id:", message.arbitration_id)

