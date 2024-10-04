// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Platform.h"
#include "UObject/WeakInterfacePtr.h"
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"

class UGameViewportClient;
class USodaGameViewportClient;
class FSceneView;
class ULocalPlayer;

namespace FEditorUtils
{

struct FActorPositionTraceResult
{
	/** Enum representing the state of this result */
	enum ResultState
	{
		/** The trace found a valid hit target */
		HitSuccess,
		/** The trace found no valid targets, so chose a default position */
		Default,
		/** The trace failed entirely */
		Failed,
	};

	/** Constructor */
	FActorPositionTraceResult() : State(Failed), Location(0.f), SurfaceNormal(0.f, 0.f, 1.f) {}

	/** The state of this result */
	ResultState	State;

	/** The location of the preferred trace hit */
	FVector		Location;

	/** The surface normal of the trace hit */
	FVector		SurfaceNormal;

	/** Pointer to the actor that was hit, if any. nullptr otherwise */
	TWeakObjectPtr<AActor>	HitActor;
};

struct UNREALSODA_API FViewportCursorLocation
{
public:
	FViewportCursorLocation(const FSceneView* View, USodaGameViewportClient* InViewportClient, int32 X, int32 Y);
	virtual ~FViewportCursorLocation() {}

	const FVector& GetOrigin()			const { return Origin; }
	const FVector& GetDirection()		const { return Direction; }
	const FIntPoint& GetCursorPos()		const { return CursorPos; }
	//ELevelViewportType	GetViewportType()	const { return LVT_Perspective; }
	USodaGameViewportClient* GetViewportClient()	const { return ViewportClient; }

private:
	FVector	Origin;
	FVector	Direction;
	FIntPoint CursorPos;
	USodaGameViewportClient* ViewportClient;
};

struct UNREALSODA_API FViewportClick : public FViewportCursorLocation
{
public:
	FViewportClick(const FSceneView* View, USodaGameViewportClient* ViewportClient, FKey InKey, EInputEvent InEvent, int32 X, int32 Y)
		: FViewportCursorLocation(View, ViewportClient, X, Y)
		, Key(InKey), Event(InEvent)
	{
		//ControlDown = ViewportClient->IsCtrlPressed();
		//ShiftDown = ViewportClient->IsShiftPressed();
		//AltDown = ViewportClient->IsAltPressed();
	}
	virtual ~FViewportClick() {}

	/** @return The 2D screenspace cursor position of the mouse when it was clicked. */
	const FIntPoint& GetClickPos()	const { return GetCursorPos(); }
	const FKey& GetKey() const { return Key; }
	EInputEvent	GetEvent() const { return Event; }

	//virtual bool	IsControlDown()	const { return ControlDown; }
	//virtual bool	IsShiftDown()	const { return ShiftDown; }
	//virtual bool	IsAltDown()		const { return AltDown; }

private:
	FKey Key;
	EInputEvent	Event;
	//bool ControlDown, ShiftDown, AltDown;
};

bool DeprojectScreenToWorld(const ULocalPlayer* LocalPlayer, const FVector2D& ScreenPos, FVector& out_WorldOrigin, FVector& out_WorldDirection);

void DeprojectScreenToWorld(const FVector2D& ScreenPos, const FIntRect& ViewRect, const FMatrix& InvViewProjMatrix, FVector& out_WorldOrigin, FVector& out_WorldDirection);

bool DeprojectScreenToWorld(const UWorld* InWorld, const FVector2D& ScreenPos, FVector& out_WorldOrigin, FVector& out_WorldDirection);

FActorPositionTraceResult TraceWorldForPosition(const UWorld* InWorld, const FVector& RayStart, const FVector& RayEnd, const TArray<AActor*>* IgnoreActors = nullptr);

FActorPositionTraceResult TraceWorldForPosition(const UWorld* InWorld, const FVector2D& ScreenPos, const TArray<AActor*>* IgnoreActors = nullptr);

bool AbsoluteToScreenPosition(const UGameViewportClient* ViewportClient, const FVector2D& AbsoluteCoordinate, FVector2D& OutScreenPos);

FName MakeUniqueObjectName(UObject* Parent, const UClass* Class, FName InBaseName = NAME_None);

AActor* FindOuterActor(const UObject* InObject);

AActor* FindActorByName(const FString& ActorNameStr, const UWorld* InWorld);

FString GetOwningActorOf(const FSoftObjectPath& SoftObjectPath);

AActor* FindActorByName(const FSoftObjectPath& SoftObjectPath, const UWorld* InWorld);

} // FEditorUtils
