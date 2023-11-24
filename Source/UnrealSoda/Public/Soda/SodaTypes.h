// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SodaTypes.generated.h"

#define G_ACC 9.8337

namespace soda
{
	enum EWidgetMode
	{
		WM_None = -1,
		WM_Translate,
		WM_Rotate,
		WM_Scale,
		WM_Max,
	};

	enum ECoordSystem
	{
		COORD_None = -1,
		COORD_World,
		COORD_Local,
		COORD_Max,
	};
}

/**
 * ESegmObjectLabel 
 */
UENUM(BlueprintType)
enum class ESegmObjectLabel : uint8
{
	/* Carla's types*/
	None = 0,
	Buildings = 1,
	Fences = 2,
	Other = 3,
	Pedestrians = 4,
	Poles = 5,
	RoadLines = 6, //Road line type isn't defined
	Roads = 7,
	Sidewalks = 8,
	TrafficSigns = 12,
	Vegetation = 9,
	Vehicles = 10,
	Walls = 11,

	/* Soda's types*/
	RoadLines_Solid = 32,
	RoadLines_Dashed = 33,
	RoadLines_Crosswalk = 34,
	RoadLines_ZigZag = 35,
	RoadLines_DoubleSide = 36,
	RoadLines_Stop = 37,
	RoadLines_DoubleDashed = 38,
	RoadLines_Nondrivable = 39,
};

/**
 * EVehicleComponentHealth
 */
UENUM(BlueprintType)
enum class EVehicleComponentHealth : uint8
{
	Undefined,
	Disabled,
	Ok,
	Warning,
	Error,
};

/**
 * ESodaSpectatorMode
 */
UENUM(BlueprintType)
enum class ESodaSpectatorMode : uint8
{
	Perspective,
	OrthographicFree,
	Top,
	Bottom,
	Left,
	Right,
	Front,
	Back
};

/**
 * EScenarioStopReason
 */
UENUM(BlueprintType)
enum class EScenarioStopReason : uint8
{
	UserRequest,
	ScenarioStopTrigger,
	QuitApplication,
	InnerError,
};

/**
 * FSubobjectReference 
 */
USTRUCT(BlueprintType)
struct  FSubobjectReference
{
	GENERATED_USTRUCT_BODY()

	FSubobjectReference()
	{}

	FSubobjectReference(const FString & PathToSubobject):
		PathToSubobject(PathToSubobject)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = SubobjectReference)
	FString PathToSubobject;

	template< class T >
	T* GetObject(AActor* OwningActor) const
	{
		if (OwningActor)
		{
			return FindObject<T>(OwningActor, *PathToSubobject);
		}
		return nullptr;
	}

	bool operator== (const FSubobjectReference& Other) const
	{
		return PathToSubobject == Other.PathToSubobject;
	}
};

UENUM(BlueprintType)
enum class EFOVRenderingStrategy: uint8
{
	Never,
	OnSelect,
	Ever
};
