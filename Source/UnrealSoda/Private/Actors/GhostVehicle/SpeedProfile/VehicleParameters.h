// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

namespace SpeedProfile
{

struct FVehicleShapeParameters 
{
	double length = 4.81;
	double width = 2.0;
	double base_to_back = 0.96;
	double wheel_base = 2.9;
	double rear_wheel_to_gravity_center = 1.168;
	double gravity_center_height = 0.4;
};

struct FVehicleDynamicParams 
{
	double max_velocity = 60;
	double r_wheel = 0.35;
	double max_power = 270E3;
	double max_torque = 2.5E3;
	double car_mass = 1196;
	double omega_nul_torque = 201.6;
	double drag_force_coeff = 1.0;
	double rolling_resistance = 600;
	double omega_max_power = 192.68;
};

constexpr auto defaultVanDynamics = FVehicleDynamicParams
{
	40., /*.max_velocity*/
	0.336, /*.r_wheel*/
	270000, /*.max_power*/
	2500, /*.max_torque*/
	2000.0, /*.car_mass*/
	201.6, /*.omega_nul_torque*/
	1.0, /*.drag_force_coeff*/
	600., /*.rolling_resistance*/
	192.68 /*.omega_max_power*/
};

constexpr auto defaultVanShape = FVehicleShapeParameters
{
	5.3, /*length*/
	2.1, /*width*/
	1.2, /*base_to_back*/
	2.8, /*wheel_base*/
	1.4, /*rear_wheel_to_gravity_center*/
	0.4 /*gravity_center_height*/
};

} // SpeedProfile

