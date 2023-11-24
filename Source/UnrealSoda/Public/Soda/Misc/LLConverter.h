// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "LLConverter.generated.h"


#define DEFAULT_LAT 59.995
#define DEFAULT_LON 30.13
#define DEFAULT_ALT 10.0

USTRUCT(BlueprintType)
struct UNREALSODA_API FLLConverter
{
	GENERATED_USTRUCT_BODY()

public:
	FLLConverter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LLConverter, SaveGame, meta = (EditInRuntime))
	double OrignLat = DEFAULT_LAT; // y

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LLConverter, SaveGame, meta = (EditInRuntime))
	double OrignLon = DEFAULT_LON; // x

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LLConverter, SaveGame, meta = (EditInRuntime))
	double OrignAltitude = DEFAULT_ALT; //z

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LLConverter, SaveGame, meta = (EditInRuntime))
	FVector OrignShift {0, 0, 0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LLConverter, SaveGame, meta = (EditInRuntime))
	float OrignDYaw = 0;

	void Init();

	void LLA2UE(FVector & Postion, double Lon, double Lat, double Alt) const;
	void UE2LLA(const FVector & Postion, double & Lon, double & Lat, double & Alt) const;

	FRotator ConvertRotationForward(const FRotator & InRot) const;
	FRotator ConvertRotationBackward(const FRotator& InRot) const;

	FVector ConvertDirForward(const FVector& InVec) const;
	FVector ConvertDirBackward(const FVector& InVec) const;

protected:
	double orig_lambda;
	double orig_phi;
	double orig_s;
	double orig_N;

	double orig_sin_lambda;
	double orig_cos_lambda;
	double orig_cos_phi;
	double orig_sin_phi;

	double x0;
	double y0;
	double z0;

	/* 
	Converts WGS-84 Geodetic point (lat, lon, h) to the Earth-Centered Earth-Fixed (ECEF) coordinates (x, y, z).
	*/
	void GeodeticToEcef(double lat, double lon, double h, double & x, double & y, double & z) const;

	/*
	Converts the Earth-Centered Earth-Fixed (ECEF) coordinates (x, y, z) to (WGS-84) Geodetic point (lat, lon, h).
	*/
	void EcefToGeodetic(double x, double y, double z, double & lat, double & lon, double & h) const;

	/* 
	Converts the Earth-Centered Earth-Fixed (ECEF) coordinates (x, y, z) to East-North-Up coordinates in a 
	Local Tangent Plane that is centered at the  (WGS-84) Geodetic point (orign_lat, orign_lon, orign_altitude). 
	*/
	void EcefToEnu(double x, double y, double z, double & xEast, double & yNorth, double & zUp) const;

	/*
	Inverse of EcefToEnu. Converts East-North-Up coordinates (xEast, yNorth, zUp) in a Local Tangent Plane 
	that is centered at the (WGS-84) Geodetic point (orign_lat, orign_lon, orign_altitude) 
	to the Earth-Centered Earth-Fixed (ECEF) coordinates (x, y, z).
	*/
	void EnuToEcef(double xEast, double yNorth, double zUp, double & x, double & y, double & z) const;
};

