// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/LLConverter.h"
#include "Soda/Misc/MathUtils.hpp"
#include "Math/RotationMatrix.h"

#define _DEG2RAD(a) ((a) / (180.0 / M_PI))
#define _RAD2DEG(a) ((a) * (180.0 / M_PI))

// WGS-84 geodetic constants
static const double earth_a = 6378137.0;                      // WGS-84 Earth semimajor axis (m)
static const double earth_b = 6356752.314245;                 // Derived Earth semiminor axis (m)
static const double earth_f = (earth_a - earth_b) / earth_a;  // Ellipsoid Flatness
static const double earth_e_sq = earth_f * (2 - earth_f);     // Square of Eccentricity

FLLConverter::FLLConverter()
{
	Init();
}

void FLLConverter::Init()
{
	orig_lambda = _DEG2RAD(OrignLat);
	orig_phi = _DEG2RAD(OrignLon);
	orig_s = std::sin(orig_lambda);
	orig_N = earth_a / std::sqrt(1 - earth_e_sq * orig_s * orig_s);

	orig_sin_lambda = std::sin(orig_lambda);
	orig_cos_lambda = std::cos(orig_lambda);
	orig_cos_phi = std::cos(orig_phi);
	orig_sin_phi = std::sin(orig_phi);

	x0 = (OrignAltitude + orig_N) * orig_cos_lambda * orig_cos_phi;
	y0 = (OrignAltitude + orig_N) * orig_cos_lambda * orig_sin_phi;
	z0 = (OrignAltitude + (1 - earth_e_sq) * orig_N) * orig_sin_lambda;

}

void FLLConverter::LLA2UE(FVector & Postion, double Lon, double Lat, double Alt) const
{
	double x, y, z;
	GeodeticToEcef(Lat, Lon, Alt, x, y, z);

	double xEast, yNorth, zUp;
	EcefToEnu(x, y, z, xEast, yNorth, zUp);
	
	Postion = FVector(-xEast, yNorth, zUp) * 100.f;

	//if (bInvertX) Postion.X = -Postion.X;
	//if (bInvertY) Postion.Y = -Postion.Y;
	if (OrignDYaw != 0) Postion = Postion.RotateAngleAxis(OrignDYaw, FVector(0, 0, 1)); 
	Postion += OrignShift;
}

void FLLConverter::UE2LLA(const FVector & InPostion, double & Lon, double & Lat, double & Alt) const
{
	FVector Postion = InPostion - OrignShift;
	if(OrignDYaw != 0) Postion = Postion.RotateAngleAxis(-OrignDYaw, FVector(0, 0, 1)); 
	//if (bInvertX) Postion.X = -Postion.X;
	//if (bInvertY) Postion.Y = -Postion.Y;
	double xEast = -Postion.X / 100.f;
	double yNorth = Postion.Y / 100.f;
	double zUp = Postion.Z / 100.f;
	double x, y, z;
	EnuToEcef(xEast, yNorth, zUp, x, y, z);
	EcefToGeodetic(x, y, z, Lat, Lon, Alt);
}

FRotator FLLConverter::ConvertRotationForward(const FRotator& InRot) const
{
	FRotator OutRot = InRot;
	OutRot.Yaw -= OrignDYaw; 
	/*
	if (bInvertX)
	{
		OutRot.Yaw = 180 + OutRot.Yaw;
		OutRot.Pitch = -OutRot.Pitch;
	}
	if (bInvertY)
	{
		OutRot.Yaw = -OutRot.Yaw;
		OutRot.Roll = -OutRot.Roll;
	}
	*/
	OutRot.Yaw = NormAngDeg(OutRot.Yaw);
	return OutRot;
}

FRotator FLLConverter::ConvertRotationBackward(const FRotator& InRot) const
{
	FRotator OutRot = InRot;
	/*
	if (bInvertY)
	{
		OutRot.Yaw = -OutRot.Yaw;
		OutRot.Roll = -OutRot.Roll;
	}
	if (bInvertX)
	{
		OutRot.Yaw =  OutRot.Yaw - 180;
		OutRot.Pitch = -OutRot.Pitch;
	}
	*/
	OutRot.Yaw += OrignDYaw;
	OutRot.Yaw = NormAngDeg(OutRot.Yaw);
	return OutRot;
}

FVector FLLConverter::ConvertDirForward(const FVector& InVec) const
{
	FVector Out = InVec.RotateAngleAxis(-OrignDYaw, FVector(0, 0, 1)); 
	//if (bInvertX) Out.X = -Out.X;
	//if (bInvertY) Out.Y = -Out.Y;
	return Out;
}

FVector FLLConverter::ConvertDirBackward(const FVector& InVec) const
{
	FVector Out = InVec;
	//if (bInvertX) Out.X = -Out.X;
	//if (bInvertY) Out.Y = -Out.Y;
	if (OrignDYaw != 0) Out = Out.RotateAngleAxis(OrignDYaw, FVector(0, 0, 1)); 
	return Out;
}

void FLLConverter::GeodeticToEcef(double lat, double lon, double h, double & x, double & y, double & z) const
{
	double lambda = _DEG2RAD(lat);
	double phi = _DEG2RAD(lon);
	double s = std::sin(lambda);
	double N = earth_a / std::sqrt(1 - earth_e_sq * s * s);

	double sin_lambda = std::sin(lambda);
	double cos_lambda = std::cos(lambda);
	double cos_phi = std::cos(phi);
	double sin_phi = std::sin(phi);

	x = (h + N) * cos_lambda * cos_phi;
	y = (h + N) * cos_lambda * sin_phi;
	z = (h + (1 - earth_e_sq) * N) * sin_lambda;
}

void FLLConverter::EcefToGeodetic(double x, double y, double z, double & lat, double & lon, double & h) const
{
	double eps = earth_e_sq / (1.0 - earth_e_sq);
	double p = std::sqrt(x * x + y * y);
	double q = std::atan2((z * earth_a), (p * earth_b));
	double sin_q = std::sin(q);
	double cos_q = std::cos(q);
	double sin_q_3 = sin_q * sin_q * sin_q;
	double cos_q_3 = cos_q * cos_q * cos_q;
	double phi = std::atan2((z + eps * earth_b * sin_q_3), (p - earth_e_sq * earth_a * cos_q_3));
	double lambda = std::atan2(y, x);
	double v = earth_a / std::sqrt(1.0 - earth_e_sq * std::sin(phi) * std::sin(phi));
	h = (p / std::cos(phi)) - v;

	lat = _RAD2DEG(phi);
	lon = _RAD2DEG(lambda);
}

void FLLConverter::EcefToEnu(double x, double y, double z, double & xEast, double & yNorth, double & zUp) const
{
	double xd, yd, zd;
	xd = x - x0;
	yd = y - y0;
	zd = z - z0;

	xEast = -orig_sin_phi * xd + orig_cos_phi * yd;
	yNorth = -orig_cos_phi * orig_sin_lambda * xd - orig_sin_lambda * orig_sin_phi * yd + orig_cos_lambda * zd;
	zUp = orig_cos_lambda * orig_cos_phi * xd + orig_cos_lambda * orig_sin_phi * yd + orig_sin_lambda * zd;
}

void FLLConverter::EnuToEcef(double xEast, double yNorth, double zUp, double & x, double & y, double & z) const
{
	double xd = -orig_sin_phi * xEast - orig_cos_phi * orig_sin_lambda * yNorth + orig_cos_lambda * orig_cos_phi * zUp;
	double yd = orig_cos_phi * xEast - orig_sin_lambda * orig_sin_phi * yNorth + orig_cos_lambda * orig_sin_phi * zUp;
	double zd = orig_cos_lambda * yNorth + orig_sin_lambda * zUp;

	x = xd + x0;
	y = yd + y0;
	z = zd + z0;
}
