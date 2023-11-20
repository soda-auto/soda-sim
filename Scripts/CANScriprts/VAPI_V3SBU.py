#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan0', bustype='socketcan')

db = cantools.database.load_file('DBC/VAPI3_SBU_SCU.dbc')

ADS1_msg = db.get_message_by_name('SBU_ADS1')
ADS2_msg = db.get_message_by_name('SBU_ADS2')
SCU_Control_msg = db.get_message_by_name('SCU_Control')
SBU_Control = db.get_message_by_name('SBU_Control')

ADS1_data = ADS1_msg.encode({
    "PMotorTotalMax_kW" : 0,           #Not used
    "NStabilisationStatus" : 0,        #Not used
    "NPlanningStatus" : 0,             #Not used
    "NPerceptionStatus" : 0,           #Not used
    "NMappingLocalisationStatus" : 0,  #Not used
    "NGearRequest" : 1,
    "NControlModeActive" : 0,
    "NADSStatus" : 0,                  #Not used
    "NWatchdogADS" : 0                 #Not used
})

ADS2_data = ADS2_msg.encode({
    "gLongRequest_g" : 1,
    "rCurvatureRequest_1m" : 0.1
})

SCU_Control_data = SCU_Control_msg.encode({
    "SCU_RWA_Req" : 1
})

SBU_Control_data = SBU_Control.encode({
    "SBU_BrTqReqRR_Nm" : 100,
    "SBU_BrTqReqFL_Nm" : 100,
    "SBU_BrTqReqRL_Nm" : 100,
    "SBU_BrTqReqFR_Nm" : 100
})

can_bus.send(can.Message(arbitration_id=ADS1_msg.frame_id, data=ADS1_data, is_extended_id=False))
can_bus.send(can.Message(arbitration_id=ADS2_msg.frame_id, data=ADS2_data, is_extended_id=False))
can_bus.send(can.Message(arbitration_id=SCU_Control_msg.frame_id, data=SCU_Control_data, is_extended_id=False))
can_bus.send(can.Message(arbitration_id=SBU_Control.frame_id, data=SBU_Control_data, is_extended_id=False))
