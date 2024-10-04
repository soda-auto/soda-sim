// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Editor/SodaSelection.h"
#include "EngineUtils.h"
#include "Soda/ISodaActor.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaGameViewportClient.h"
#include "DrawDebugHelpers.h"
#include "Soda/Misc/EditorUtils.h"

USodaSelection::USodaSelection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AActor* USodaSelection::GetSelectedActor() 
{ 
	return const_cast<AActor*>(SelectedActor); 
}

AActor* USodaSelection::GetHoveredActor() 
{ 
	return const_cast<AActor*>(HoveredActor);
}

void USodaSelection::Tick(FViewport* Viewport)
{
	if (!bIsTraking)
	{
		AActor * Actor = GetSodaActorUnderCursor(Viewport);

		if (Actor && SelectedActor != Actor) // Hover
		{
			if (HoveredActor)
			{
				// TODO: HoveredActor->Unhover()
			}
			HoveredActor = Actor;
			// TODO: HoveredActor->Hover()
		}
		else if (HoveredActor && (HoveredActor != Actor)) // Unhover
		{
			// TODO: HoveredActor->Unhover()
			HoveredActor = nullptr;
		}
	}

	if (SelectedActor && !IsValid(SelectedActor))
	{
		SelectedActor = nullptr;
	}

	if (HoveredActor && !IsValid(HoveredActor))
	{
		HoveredActor = nullptr;
	}
}

bool USodaSelection::InputKey(const FInputKeyEventArgs& InEventArgs)
{
	bool bIsLeftMouseButtonDown = InEventArgs.Viewport->KeyState(EKeys::LeftMouseButton);
	bool bIsLeftMouseButtonEvent = InEventArgs.Key == EKeys::LeftMouseButton;

	if (bIsLeftMouseButtonEvent)
	{
		if (InEventArgs.Event == IE_Pressed)
		{
			bIsTraking = true;
			MouseDelta = FVector2D::ZeroVector;
		}
		else if (InEventArgs.Event == IE_Released)
		{
			bIsTraking = false;
			if (MouseDelta.Size() < 0.5)
			{
				AActor* Actor = GetSodaActorUnderCursor(InEventArgs.Viewport);
				if (Actor)
				{
					if (Actor != SelectedActor)
					{
						SelectActor(HoveredActor, nullptr); // Select Actor
					}
					else if (SelectedActor) 	// Unselect Actor
					{
						UnselectActor();
					}
				}
				else if (SelectedActor)
				{
					UnselectActor(); // Unselect Actor
				}
			}
			MouseDelta = FVector2D::ZeroVector;
		}
		//return true;
	}
	return false;
}

bool USodaSelection::InputAxis(FViewport* Viewport, FInputDeviceId InputDevice, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if(bIsTraking)
	{
		MouseDelta += FVector2D(Key == EKeys::MouseX ? Delta : 0, Key == EKeys::MouseY ? Delta : 0);
	}
	return false;
}

AActor * USodaSelection::GetSodaActorUnderCursor(FViewport* Viewport)
{
	check(Viewport);

	UWorld * World = USodaSubsystem::GetChecked()->GetWorld();
	check(World);

	int32 X = Viewport->GetMouseX();
	int32 Y = Viewport->GetMouseY();

	FVector Origin;
	FVector Direction;
	if (FEditorUtils::DeprojectScreenToWorld(World, FVector2D(X, Y), Origin, Direction))
	{
		TArray<FHitResult> Hits;
		FCollisionQueryParams Param(SCENE_QUERY_STAT(WidgetTrace), true);
		const FVector RayEnd = Origin + Direction * HALF_WORLD_MAX;
		if (World->LineTraceMultiByObjectType(Hits, Origin, RayEnd, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), Param))
		{
			float ClosestHitDistanceSqr = TNumericLimits<float>::Max();
			AActor* ClosestActor = nullptr;
			for (const FHitResult& Hit : Hits)
			{
				if (Hit.GetActor() && Hit.GetActor()->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
				{
					const float DistanceToHitSqr = (Hit.ImpactPoint - Origin).SizeSquared();
					if (DistanceToHitSqr < ClosestHitDistanceSqr)
					{
						ClosestHitDistanceSqr = DistanceToHitSqr;
						ClosestActor = Hit.GetActor();
					}
				}
			}

			return ClosestActor;
		}
	}
	return nullptr;
}

bool USodaSelection::SelectActor(const AActor* Actor, const UPrimitiveComponent* PrimComponent)
{
	if (bIsSelectedActorFreezed)
	{
		return false;
	}

	if (!IsValid(Actor))
	{
		return false;
	}

	if (!Actor->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
	{
		return false;
	}

	if (SelectedActor == Actor)
	{
		return true;
	}

	UnselectActor();

	SelectedActor = Actor;
	if (SelectedActor == HoveredActor)
	{
		HoveredActor = nullptr;
	}

	ISodaActor::Execute_OnSelect(const_cast<AActor*>(Actor), PrimComponent);

	RetargetWidgetForSelectedActor();

	return true;
}

void USodaSelection::UnselectActor()
{
	if (IsValid(SelectedActor) && bIsSelectedActorFreezed)
	{
		return;
	}

	if (IsValid(SelectedActor))
	{
		if (SelectedActor->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
		{
			ISodaActor::Execute_OnUnselect(const_cast<AActor*>(SelectedActor));
		}

		if (USodaGameViewportClient* ViewportClient = Cast<USodaGameViewportClient>(GetWorld()->GetGameViewport()))
		{
			ViewportClient->SetWidgetTarget(nullptr);
		}
	}

	SelectedActor = nullptr;
	UnfreeseSelectedActor();
}

void USodaSelection::RetargetWidgetForSelectedActor()
{
	if (IsValid(SelectedActor))
	{
		if (USodaGameViewportClient* ViewportClient = Cast<USodaGameViewportClient>(GetWorld()->GetGameViewport()))
		{
			if (USodaSubsystem::GetChecked()->GetSodaActorDescriptor(SelectedActor->GetClass()).bAllowTransform)
			{
				ViewportClient->SetWidgetTarget(SelectedActor->GetRootComponent());
			}
		}
	}
}

void USodaSelection::DrawDebugBoxes()
{
	if (IsValid(SelectedActor))
	{
		FVector BoundOrigin, BoundExtent;
		SelectedActor->GetActorBounds(true, BoundOrigin, BoundExtent);
		DrawDebugBox(GetWorld(), BoundOrigin, BoundExtent, FColor(0, 255, 0), false, 0, 0, 5);
	}

	if (IsValid(HoveredActor))
	{
		FVector BoundOrigin, BoundExtent;
		HoveredActor->GetActorBounds(true, BoundOrigin, BoundExtent);
		DrawDebugBox(GetWorld(), BoundOrigin, BoundExtent, FColor(255, 255, 0), false, 0, 0, 5);
	}
}

void USodaSelection::FreeseSelectedActor()
{
	if (IsValid(SelectedActor))
	{
		bIsSelectedActorFreezed = true;
	}
}

void USodaSelection::UnfreeseSelectedActor()
{
	bIsSelectedActorFreezed = false;
}