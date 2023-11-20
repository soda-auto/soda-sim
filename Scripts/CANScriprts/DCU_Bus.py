#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan1', bustype='socketcan')

db = cantools.database.load_file('DBC/Arrival_dcu_Bus.dbc')

DCUStatus1_msg = db.get_message_by_name('DCUStatus1')

DCUStatus1_msg_data = DCUStatus1_msg.encode({
    "SuspLvlPosition" : 0,        #Not used 
    "EmergencyBrakingMode" : 0,   #Not used 
    "VCU2DC_ChrgCurSetpoint" : 0, #Not used 
    "VehSpeed" : 0,               #Not used 
    "IsIgnitionOn" : 0,           #Not used 
    "GearShiftPositionLocked" : 0,#Not used 
    "GearShiftPosition" : 0,      #Not used 
    "BrkSwitch" : 0,
    "BrakePedalPos" : 0,          #Not used 
    "AccelPedalPos1" : 0,         #Not used 
    "GearShifterState" : 0,       #Not used 
    "Vehicle_State" : 0           #Not used 

})

can_bus.send(can.Message(arbitration_id=(DCUStatus1_msg.frame_id & 0xFFFFFF00) | 0x64 , data=DCUStatus1_msg_data, is_extended_id=True))


