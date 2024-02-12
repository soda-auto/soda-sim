// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once
#include "ComponentReregisterContext.h"
#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include <random>
#include "NavSensor.generated.h"


/**
 * IMU noise generation helper
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FNoiser
{
	GENERATED_BODY()

	/** Standard deviation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime))
	float StdDev = 0;

	/** Constan bias */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime))
	float ConstBias = 0;

	/** Gauss-Markov bias standard deviation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime))
	float GMBiasStdDev = 0;

	/** Period for the Gauss-Markov process [s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime))
	float DeltaTime = 0.01;

	/** Time constant for Gauss-Markov process [s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime))
	float TimeConstant = 400;

	FNoiser()
		: Generator(std::random_device{}()),
		Dist(0.f, 1.f) 
	{};

	/** Initialize inner constants */
	void Init()
	{
		float Mu = 1.f / TimeConstant;
		F = std::exp(-Mu * DeltaTime);
		SqrtDeltaTime = std::sqrt(DeltaTime);
		Z = 0;
	}

	/** Setep and get accumulative noize */
	float Step()
	{
		Z = F * Z + Dist(Generator) * GMBiasStdDev * SqrtDeltaTime;
		return ConstBias + StdDev * Dist(Generator) + Z;
	}

	float GetAccuracy() const
	{
		return ConstBias + StdDev + GMBiasStdDev;
	}

protected:
	float Z = 0;
	float F = 0;
	float SqrtDeltaTime = 0;
	std::default_random_engine Generator;
	std::normal_distribution< float > Dist;
};

/**
 * IMU noise generation helper
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FNoiserVector
{
	GENERATED_BODY()

	/** Standard deviation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FVector StdDev;

	/** Constan bias */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FVector ConstBias;

	/** Gauss-Markov bias standard deviation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FVector GMBiasStdDev;

	/** Period for the Gauss-Markov process [s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float DeltaTime = 0.01;

	/** Time constant for Gauss-Markov process */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noiser, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float TimeConstant = 400;

	FNoiserVector()
		: Generator(std::random_device{}()),
		Dist(0.f, 1.f)
	{};

	/** Update inner constants */
	void UpdateParameters()
	{
		float Mu = 1.f / TimeConstant;
		F = std::exp(-Mu * DeltaTime);
		SqrtDeltaTime = std::sqrt(DeltaTime);
		Z = FVector(0);
	}

	/** Setep and get accumulative noize */
	FVector Step()
	{
		Z = F * Z + FVector(Dist(Generator), Dist(Generator), Dist(Generator)) * GMBiasStdDev * SqrtDeltaTime;
		return ConstBias + StdDev * FVector(Dist(Generator), Dist(Generator), Dist(Generator)) + Z;
	}

	FVector GetAccuracy() const
	{
		return ConstBias + StdDev + GMBiasStdDev;
	}

protected:
	FVector Z;
	float F;
	float SqrtDeltaTime = 0;
	std::default_random_engine Generator;
	std::normal_distribution< float > Dist;
};

/**
 * FImuNoiseParams
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FImuNoiseParams
{
	GENERATED_BODY()

	/** [m] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImuNoiseParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FNoiserVector Location;

	/** Y - Pitch, Z - Yaw, RotNoize.X - Roll, [deg] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImuNoiseParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FNoiserVector Rotation;

	/** [m/s^2] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImuNoiseParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FNoiserVector Acceleration;

	/** [rad/s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImuNoiseParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FNoiserVector Gyro;

	/** [m/s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImuNoiseParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FNoiserVector Velocity;

	FImuNoiseParams()
	{}

	void UpdateParameters()
	{
		Location.UpdateParameters();
		Rotation.UpdateParameters();
		Acceleration.UpdateParameters();
		Gyro.UpdateParameters();
		Velocity.UpdateParameters();
	}
};

/**
 * Base abstract class for all IMU sensors
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UNavSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Enabling IMU niose simulation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool bImuNoiseEnabled = false;

	/** Base IMU noise params */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noise, SaveGame, meta = (EditInRuntime))
	FImuNoiseParams NoiseParams;

public:
	UFUNCTION(BlueprintCallable, Category = Sensor)
	void SetImuNoiseParams(const FImuNoiseParams & NewImuNoiseParams);

	UFUNCTION(BlueprintCallable, Category = Sensor)
	void RestoreBaseImuNoiseParams();

protected:
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FTransform & RelativeTransform, const FPhysBodyKinematic& VehicleKinematic) { return false; }

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

protected:
	FImuNoiseParams StoredParams;
	bool bIsStoredParams = false;
};
