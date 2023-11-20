#!/usr/bin/env python3

import cantools
import can

can_bus = can.interface.Bus('vcan0', bustype='socketcan')

db = cantools.database.load_file('DBC/VehicleAPI_V24.dbc')

RMC_DriverControlSimple_msg = db.get_message_by_name('RMC_DriverControlSimple')

RMC_DriverControlSimple_data = RMC_DriverControlSimple_msg.encode({
    "RMC_RoadWheelAngleRequest" : 3.14/6,
    "RMC_WatchDog" : 0,                  #Not used
    "RMC_TractionForceRequest" : 5000, 
    "RMC_GearRequest" : 1,
    "RMC_DriverControlMode" : 0,         #Not used
    "RMC_DriverState" : 0                #Not used

})

can_bus.send(can.Message(arbitration_id=RMC_DriverControlSimple_msg.frame_id, data=RMC_DriverControlSimple_data, is_extended_id=False))


