// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/Base/ImuGnssSensor.h"
#include "Soda/Misc/ExtraWindow.h"
#include "Soda/Misc/TelemetryGraph.h"
#include "OdometryTest.generated.h"

class UCanvasRenderTarget2D;

USTRUCT(BlueprintType)
struct UNREALSODA_API FOdometryData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OdometryData)
	FVector OdometryPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OdometryData)
	FRotator OdometryRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OdometryData)
	FVector OdometryVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OdometryData)
	FVector ActualPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OdometryData)
	FRotator ActualRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OdometryData)
	FVector ActualVelocity;
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UOdometryTestComponent : public UImuGnssSensor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OdometryTest, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bInitPositionFromZero = false;

protected:
	UPROPERTY(BlueprintReadOnly, Category = OdometryTest)
	UExtraWindow* GraphWindow = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = OdometryTest)
	UCanvasRenderTarget2D* CanvasRenderTarget2D = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = OdometryTest)
	FOdometryData GetOdometry() const;

	UFUNCTION(BlueprintCallable, Category = Debug, meta = (CallInRuntime))
	virtual void ShowGraphs();

public:
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual FString GetRemark() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

public:
	UOdometryTestComponent();

private:
	FTelemetryGraphGrid TelemetryGrid;

	bool bIsFirstTick = false;
	FVector P;
	FQuat Q;
	FVector V;

	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro;
};
