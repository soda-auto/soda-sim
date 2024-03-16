// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "InputCoreTypes.h"
#include "UnrealClient.h"
#include "SodaSelection.generated.h"

struct HActor;
class AActor;
class FViewport;

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API USodaSelection : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool InputKey(const FInputKeyEventArgs& InEventArgs);
	virtual bool InputAxis(FViewport* Viewport, FInputDeviceId InputDevice, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false);
	virtual void Tick(FViewport* Viewport);

	UFUNCTION(BlueprintCallable, Category=SodaEditor)
	virtual bool SelectActor(const AActor* Actor, const UPrimitiveComponent* PrimComponent);

	UFUNCTION(BlueprintCallable, Category=SodaEditor)
	virtual void UnselectActor();

	UFUNCTION(BlueprintCallable, Category=SodaEditor)
	virtual AActor* GetSelectedActor();

	UFUNCTION(BlueprintCallable, Category=SodaEditor)
	virtual AActor* GetHoveredActor();

	UFUNCTION(BlueprintCallable, Category=SodaEditor)
	void DrawDebugBoxes();

	void RetargetWidgetForSelectedActor();

	void FreeseSelectedActor();
	void UnfreeseSelectedActor();

protected:

	static AActor * GetSodaActorUnderCursor(FViewport* Viewport);

	UPROPERTY()
	const AActor * SelectedActor = nullptr;

	UPROPERTY()
	const AActor* HoveredActor = nullptr;

	//UPROPERTY()
	//const AActor* PressedActor = nullptr;

	//UPROPERTY()
	//UPrimitiveComponent* HoverPrimitive = nullptr;

	bool bIsTraking = false;
	FVector2D MouseDelta;
	bool bIsSelectedActorFreezed = false;
};
