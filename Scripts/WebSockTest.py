#!/usr/bin/env python3

import asyncio
import websockets
import sys

Requests = {
	0 : '{"jsonrpc": "2.0", "method": "open_level",           "params": {"level_name": "SimpleTown"}, "id": 1}',
	1 : '{"jsonrpc": "2.0", "method": "get_level",            "params": {}, "id": 1}',
	2 : '{"jsonrpc": "2.0", "method": "get_levels",           "params": {}, "id": 1}',
	3 : '{"jsonrpc": "2.0", "method": "reset_level",          "params": {}, "id": 1}',
	4 : '{"jsonrpc": "2.0", "method": "get_level_vehicles",   "params": {}, "id": 1}',
	5 : '{"jsonrpc": "2.0", "method": "get_vehicle_classes",  "params": {}, "id": 1}',
	6 : '{"jsonrpc": "2.0", "method": "get_vehicle_state",    "params": {"veh_id" : 0}, "id": 1}',
	7 : '{"jsonrpc": "2.0", "method": "set_vehicle_inputs",   "params": {"veh_id" : 0, "throttle" : 0, "steering" : 0, "brake" : 0, "gear" : 0}, "id": 1}',
	8 : '{"jsonrpc": "2.0", "method": "get_vehicle_params",   "params": {"veh_id" : 0}, "id": 1}',
	9 : '{"jsonrpc": "2.0", "method": "get_vehicle_param",    "params": {"veh_id" : 0, "parameter_name" : "VehicleMovementClass"}, "id": 1}',
	10 :  '''
	{
		"jsonrpc": "2.0", 
		"method": "set_vehicle_params",   
		"params": 
		{
			"veh_id" : 0, 
			"parameters" : [{"name" : "VehicleMovementClass", "value" : "2WD"}]
		}, "id": 1
	}''',
	11 : '''
	{
		"jsonrpc": "2.0", 
		"method": "set_vehicle_param",    
		"params": {"veh_id" : 0, "parameter_name" : "VehicleMovementClass", "value" : "2WD"}, 
		"id": 1
	}''',
	12 : '{"jsonrpc": "2.0", "method": "change_vehicle_class", "params": {"veh_id" : 0, "vehicle_class_name" : "RobocarDefSensors_C"}, "id": 1}',
	13 : '''
	{
		"jsonrpc": "2.0", 
		"method": "set_vehicle_position", 
		"params": 
		{
			"veh_id" : 0, 
			"location" : {"x" : 706, "y" : -5580, "z" : 265},
			"rotation" : {"yaw" : 0, "pitch" : 0, "roll" : 0}
		}, 
		"id": 1
	}''',
	14 : '{"jsonrpc": "2.0", "method": "reset_default_vehicle_params", "params": {"veh_id" : 0}, "id": 1}',
	15 : '{"jsonrpc": "2.0", "method": "get_sensors_params", "params": {"veh_id" : 0}, "id": 1}',
	16 : '{"jsonrpc": "2.0", "method": "get_sensor_params",  "params": {"veh_id" : 0, "sensor_name" : "IMU"}, "id": 1}',
	17 : '{"jsonrpc": "2.0", "method": "get_sensor_param",   "params": {"veh_id" : 0, "sensor_name" : "IMU", "parameter_name": "Port"}, "id": 1}',
	18 : '''
	{
		"jsonrpc": "2.0", 
		"method": "set_sensors_params", 
		"params": 
		{
			"veh_id" : 0,
			"sensors": 
			[
				{
					"sensor_name" : "IMU",
					"parameters" : 
					[
						{"name" : "Port", "value" : 8001}
					]
				}
			]
		}, 
		"id": 1
	}''',
	19 : '''
	{
		"jsonrpc": "2.0", 
		"method": "set_sensor_params", 
		"params": 
		{
			"veh_id" : 0,
			"sensor_name": "IMU",
			"parameters": 
			[
				{"name" : "Port", "value" : 8002}
			]
		}, 
		"id": 1
	}''',
	20 : '{"jsonrpc": "2.0", "method": "set_sensor_param", "params": {"veh_id" : 0, "sensor_name" : "IMU", "parameter_name" : "Port", "value" : 8003}, "id": 1}',
	21 : '{"jsonrpc": "2.0", "method": "enable_sensor",    "params": {"veh_id" : 0, "sensor_name" : "IMU"}, "id": 1}',
	22 : '{"jsonrpc": "2.0", "method": "disable_sensor",   "params": {"veh_id" : 0, "sensor_name" : "IMU"}, "id": 1}'
}


if len(sys.argv) < 2: 
	print ("Use python3 WebSockTest.py <request_id>")
	sys.exit(-1)

async def hello():
	uri = "ws://localhost:16000"
	async with websockets.connect(uri) as websocket:
		await websocket.send(Requests[int(sys.argv[1])])
		data = await websocket.recv()
		print(data)
asyncio.get_event_loop().run_until_complete(hello())

