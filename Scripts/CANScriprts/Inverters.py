#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan0', bustype='socketcan')

db = cantools.database.load_file('DBC/20160525_RMS_PM_CAN_DB.dbc')

M192_Command_Message_msg = db.get_message_by_name('M192_Command_Message')

M192_Command_Message_data_1 = M192_Command_Message_msg.encode({
    "Inverter_Enable" : 0,      #Not used 
    "Direction_Command" : 0,
    "Speed_Command" : 0,        #Not used 
    "Torque_Command" : 100,
    "Inverter_Discharge" : 0,   #Not used 
    "Torque_Limit_Command" : 0, #Not used 
    "Speed_Mode_Enable" : 0     #Not used 
})

M192_Command_Message_data_2 = M192_Command_Message_msg.encode({
    "Inverter_Enable" : 0,      #Not used 
    "Direction_Command" : 0,
    "Speed_Command" : 0,        #Not used 
    "Torque_Command" : 150,
    "Inverter_Discharge" : 0,   #Not used 
    "Torque_Limit_Command" : 0, #Not used 
    "Speed_Mode_Enable" : 0     #Not used 
})

can_bus.send(can.Message(arbitration_id=M192_Command_Message_msg.frame_id + 112, data=M192_Command_Message_data_1, is_extended_id=False))
can_bus.send(can.Message(arbitration_id=M192_Command_Message_msg.frame_id + 147, data=M192_Command_Message_data_2, is_extended_id=False))

