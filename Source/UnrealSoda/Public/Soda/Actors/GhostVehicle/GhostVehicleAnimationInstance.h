// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "GhostVehicleAnimationInstance.generated.h"

class AGhostVehicle;

struct FGhostWheelAnimationData
{
	FName BoneName;
	FRotator RotOffset;
	FVector LocOffset;
};

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct UNREALSODA_API FGhostVehicleAnimationInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGhostVehicleAnimationInstanceProxy()
		: FAnimInstanceProxy()
		, WheelSpokeCount(0)
		, MaxAngularVelocity(256.f)
		, ShutterSpeed(30.f)
		, StageCoachBlend(730.f)
	{
	}

	FGhostVehicleAnimationInstanceProxy(UAnimInstance* Instance)
		: FAnimInstanceProxy(Instance)
		, WheelSpokeCount(0)
		, MaxAngularVelocity(256.f)
		, ShutterSpeed(30.f)
		, StageCoachBlend(730.f)
	{
	}

public:

	void SetVehicle(const AGhostVehicle * GhostVehicle);

	/** FAnimInstanceProxy interface begin*/
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	/** FAnimInstanceProxy interface end*/

	const TArray<FGhostWheelAnimationData>& GetWheelAnimData() const
	{
		return WheelInstances;
	}

	void SetStageCoachEffectParams(int InWheelSpokeCount, float InMaxAngularVelocity, float InShutterSpeed, float InStageCoachBlend)
	{
		WheelSpokeCount = InWheelSpokeCount;
		MaxAngularVelocity = InMaxAngularVelocity;
		ShutterSpeed = InShutterSpeed;
		StageCoachBlend = InStageCoachBlend;
	}

private:
	TArray<FGhostWheelAnimationData> WheelInstances;

	int WheelSpokeCount;			// Number of spokes visible on wheel
	float MaxAngularVelocity;		// Wheel max rotation speed in degrees/second
	float ShutterSpeed;				// Camera shutter speed in frames/second
	float StageCoachBlend;			// Blend effect degrees/second

};

UCLASS(transient)
class UNREALSODA_API UGhostVehicleAnimationInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category = "Animation")
	const AGhostVehicle* GetVehicle() const;

public:
	TArray<TArray<FGhostWheelAnimationData>> WheelData;

public:
	void SetVehicle(const AGhostVehicle* InGhostVehicle)
	{
		GhostVehicle = InGhostVehicle;
		AnimInstanceProxy.SetVehicle(InGhostVehicle);
	}


private:
	/** UAnimInstance interface begin*/
	virtual void NativeInitializeAnimation() override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
	/** UAnimInstance interface end*/

	FGhostVehicleAnimationInstanceProxy AnimInstanceProxy;

	UPROPERTY(transient)
	TObjectPtr<const AGhostVehicle> GhostVehicle;
};


