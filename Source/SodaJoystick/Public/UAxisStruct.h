#pragma once

#include "CoreMinimal.h"
#include "UAxisStruct.generated.h"

USTRUCT(BlueprintType)
struct SODAJOYSTICK_API FAxisStruct
{
public:
	GENERATED_USTRUCT_BODY()

    FAxisStruct() : 
		Name("DefaultAxisName"),
        InMin(-32767),
	    InMax(32767),
		OutMin(-1.f),
		OutMax(1.f)
	{};

   
	UPROPERTY(EditAnywhere, Category = UAxisStruct)
	FString Name;
	UPROPERTY(EditAnywhere, Category = UAxisStruct)
	int32 InMin;
	UPROPERTY(EditAnywhere, Category = UAxisStruct)
	int32 InMax;
	UPROPERTY(EditAnywhere, Category = UAxisStruct)
	float OutMin;
	UPROPERTY(EditAnywhere, Category = UAxisStruct)
	float OutMax;

	float GetScaledValue(int RawValue)
	{
  		return (RawValue - InMin) * (OutMax - OutMin) / (InMax - InMin) + OutMin;
	};
};
