#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan0', bustype='socketcan')

db = cantools.database.load_file('DBC/ADSSafetyLayer_v6.dbc')

ADS1_msg = db.get_message_by_name('ADS1')
ADS2_msg = db.get_message_by_name('ADS2')
ADS3_msg = db.get_message_by_name('ADS3')

ADS1_data = ADS1_msg.encode({
    "PMotorTotalMax_kW" : 0,          #Not used 
    "NStabilisationStatus" : 0,       #Not used 
    "NPlanningStatus" : 0,            #Not used 
    "NPerceptionStatus" : 0,          #Not used 
    "NMappingLocalisationStatus" : 0, #Not used 
    "NGearRequest" : 1,
    "NControlModeActive" : 0,
    "NADSStatus" : 0,                 #Not used 
    "NWatchdogADS" : 0                #Not used 
})

ADS2_data = ADS2_msg.encode({
    "MMotorFRRequest_Nm" : 100,
    "MMotorFLRequest_Nm" : 100,
    "gLongRequest_g" : 1,
    "rCurvatureRequest_1m" : 0.1
})

ADS3_data = ADS3_msg.encode({
    "MMotorRLRequest_Nm" : 100,
    "MMotorRRRequest_Nm" : 100,
    "pBrakeRRequest_bar" : 100,
    "pBrakeFRequest_bar" : 100,
    "aWheelFMeanRequest_deg" : 15
})

can_bus.send(can.Message(arbitration_id=ADS1_msg.frame_id, data=ADS1_data, is_extended_id=False))
can_bus.send(can.Message(arbitration_id=ADS2_msg.frame_id, data=ADS2_data, is_extended_id=False))
can_bus.send(can.Message(arbitration_id=ADS3_msg.frame_id, data=ADS3_data, is_extended_id=False))

