// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extent.generated.h"

/**
 * FExtent 
 * 3D object extent struct. Similarly FBox struct
 */
USTRUCT( BlueprintType)
struct FExtent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Extent, SaveGame)
	float Forward = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Extent, SaveGame)
	float Backward = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Extent, SaveGame)
	float Left = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Extent, SaveGame)
	float Right = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Extent, SaveGame)
	float Up = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Extent, SaveGame)
	float Down = 0;

	FExtent() {};
	inline FExtent(const FBox& Box)
	{
		if (Box.IsValid)
		{
			Forward = Box.Max.X;
			Backward = -Box.Min.X;
			Left = -Box.Min.Y;
			Right = Box.Max.Y;
			Up = Box.Max.Z;
			Down = -Box.Min.Z;
		}
	}

	inline FBox ToBox() const
	{
		FBox Box;
		Box.Max.X = Forward;
		Box.Min.X = -Backward;
		Box.Min.Y = -Left;
		Box.Max.Y = Right;
		Box.Max.Z = Up;
		Box.Min.Z = -Down;
		return Box;
	}

	inline FBoxSphereBounds ToBoxSphereBounds() const
	{
		FBox Box = ToBox();
		FBoxSphereBounds SphereBounds;
		Box.GetCenterAndExtents(SphereBounds.Origin, SphereBounds.BoxExtent);
		SphereBounds.SphereRadius = SphereBounds.BoxExtent.Size();
		return SphereBounds;
	}
};