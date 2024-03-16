// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include <mutex>
#include <deque>
#include "GroundScaner.generated.h"

UCLASS()
class UNREALSODA_API UGroundScaner : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	float WheelBase = 338.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	float TrackWidth = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	float Border = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	float ScanHeight = 400.f;

	// [Cm]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	int MeshStepX = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	int MeshStepY = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	int RenderTimeHistorySize = 30;

	// [Seconds]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	float RenderTimeExtra = 0.01;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GroundScaner, SaveGame, meta = (EditInRuntime))
	bool bDrawDebug = false;

	ECollisionChannel CollisionChannel = ECollisionChannel::ECC_Visibility;
	FCollisionQueryParams CollisionQueryParams {};
	FCollisionResponseParams CollisionResponseParams = FCollisionResponseParams::DefaultResponseParam;
	FCollisionObjectQueryParams CollisionObjectQueryParams = FCollisionObjectQueryParams::DefaultObjectQueryParam;

public:
	virtual void BeginDestroy() override;
	struct FMapEl
	{
		FVector Value;
		bool Valid = false;
	};

	bool Scan(const FPhysBodyKinematic & VehicleKinematic, float DeltaTime);
	bool Scan(const FVector& Position, const FVector& Size, float Yaw);
	bool GetHeight(float X, float Y, float & Height);
	const TArray<FMapEl>& GetMap() const { return  Map; }

private:
	TArray<FMapEl> Map;
	TArray<FMapEl> TmpMap;
	int X0;
	int Y0;
	int Width;
	int Length;
	float Timestamp;
	std::deque<float> DtStat;
	std::mutex Mutex;
};
