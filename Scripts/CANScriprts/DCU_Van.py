#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan1', bustype='socketcan')

db = cantools.database.load_file('DBC/Arrival_dcu_T6.dbc')

DCUStatus1_msg = db.get_message_by_name('DCUStatus1')

DCUStatus1_msg_data = DCUStatus1_msg.encode({
    "SuspensionLevelPosition" : 0,        #Not used 
    "EmergencyBrakingMode" : 0,           #Not used 
    "VCU2DC_Charge_current_setpoint" : 0, #Not used 
    "VehSpeed" : 0,                       #Not used 
    "IsIgnitionOn" : 0,                   #Not used 
    "GearShiftPositionLocked" : 0,        #Not used 
    "GearShiftPosition" : 0,              #Not used 
    "BrakeSwitch" : 1,                 
    "BrakePedalPos" : 0,                  #Not used 
    "AccelPedalPos1" : 0,                 #Not used 
    "GearShifterState" : 0,               #Not used   
    "Vehicle_State" : 0                   #Not used 
})

can_bus.send(can.Message(arbitration_id=(DCUStatus1_msg.frame_id & 0xFFFFFF00) | 0x64 , data=DCUStatus1_msg_data, is_extended_id=True))


