// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionEventLibrary.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/Actors/ScenarioTriggerActors.h"
#include "Components/ShapeComponent.h"
#include "Soda/SodaStatics.h"

//-------------------------------------------------------------------------------------------------
UEventAtScenarioBegin::UEventAtScenarioBegin(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("At Scenario Begin"));
}

//-------------------------------------------------------------------------------------------------
UEventAtScenarioEnd::UEventAtScenarioEnd(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("At Scenario End"));
}

//-------------------------------------------------------------------------------------------------
UEventAtScenarioTick::UEventAtScenarioTick(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("At Scenario Tick"));
}

//-------------------------------------------------------------------------------------------------
UEventAtScenarioTime::UEventAtScenarioTime(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("At Scenario Time"));
}

void UEventAtScenarioTime::ScenarioBegin()
{
	CurrentTime = 0;
}

void UEventAtScenarioTime::Tick(float DeltaTime)
{
	CurrentTime += DeltaTime;
	if (CurrentTime >= ScenarioTime)
	{
		EventDelegate.ExecuteIfBound();
	}
}

//-------------------------------------------------------------------------------------------------
UEventOverlapTrigger::UEventOverlapTrigger(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("Reach Position"));
}

void UEventOverlapTrigger::Tick(float DeltaTime)
{
	if (Actor.Get() && Trigger.Get())
	{
		if (Trigger->GetCollisionComponent()->IsOverlappingActor(Actor.Get()))
		{
			EventDelegate.ExecuteIfBound();
		}
	}
}

//-------------------------------------------------------------------------------------------------
UEventRelativeDistance::UEventRelativeDistance(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("Relative Distance"));
}

void UEventRelativeDistance::ScenarioBegin()
{
	if (FromActor.IsValid() && ToActor.IsValid())
	{
		FromActorExtent = USodaStatics::CalculateActorExtent(FromActor.Get());
		ToActorExtent = USodaStatics::CalculateActorExtent(ToActor.Get());
	}
}

void UEventRelativeDistance::Tick(float DeltaTime)
{
	if (FromActor.IsValid() && ToActor.IsValid())
	{
		float CurDistance = MAX_FLT;
		if (OverlapEstimation == EScenarioEventOverlapEstimation::Position)
		{
			if (DistanceEstimation == EScenarioEventDistanceEstimation::Longotude)
			{
				CurDistance = FromActor.Get()->GetTransform().InverseTransformPositionNoScale(ToActor.Get()->GetActorLocation()).X;
			}
			else // EScenarioEventDistanceEstimation::Euclidean
			{
				CurDistance = (FromActor.Get()->GetActorLocation() - ToActor.Get()->GetActorLocation()).Size();
			}
		}
		else // EScenarioEventOverlapEstimation::ClosesPoint
		{
			const FTransform& ToActorTransform = ToActor->GetTransform();
			const FTransform& FromActorTransform = FromActor->GetTransform();

			FVector Vertices[8];
			ToActorExtent.GetVertices(Vertices);

			FBox NewBox(ForceInit);
			for (int32 VertexIndex = 0; VertexIndex < UE_ARRAY_COUNT(Vertices); VertexIndex++)
			{
				FVector ProjectedVertex = FromActorTransform.InverseTransformPosition(ToActorTransform.TransformPosition(Vertices[VertexIndex]));
				NewBox += ProjectedVertex;
			}

			//DrawDebugBox(GetWorld(), FromActorExtent.GetCenter(), FromActorExtent.GetExtent(), FQuat::Identity, FColor::Red, false, -1.f, 0, 1.f);
			//DrawDebugBox(GetWorld(), NewBox.GetCenter(), NewBox.GetExtent(), FQuat::Identity, FColor::Green, false, -1.f, 0, 1.f);

			if (DistanceEstimation == EScenarioEventDistanceEstimation::Longotude)
			{
				CurDistance = NewBox.Min.X - FromActorExtent.Max.X;
			}
			else // EScenarioEventDistanceEstimation::Euclidean
			{
				CurDistance = FMath::Sqrt(FromActorExtent.ComputeSquaredDistanceToBox(NewBox));
			}
		}

		//UE_LOG(LogSoda, Warning, TEXT("*** %f"), CurDistance);

		switch (Conditional)
		{
		case EScenarioEventConditional::LessThan:
			if (CurDistance < Distance)
			{
				EventDelegate.ExecuteIfBound();
			}
			break;
		case EScenarioEventConditional::GreaterThan:
			if (CurDistance > Distance)
			{
				EventDelegate.ExecuteIfBound();
			}
			break;
		}
	}
}

//-------------------------------------------------------------------------------------------------
UEventRelativeSpeed::UEventRelativeSpeed(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("Relative Speed"));
}

void UEventRelativeSpeed::Tick(float DeltaTime)
{
	if (FromActor.IsValid() && ToActor.IsValid())
	{
		const FVector FromActorVelocity = FromActor->GetTransform().InverseTransformVectorNoScale(FromActor->GetVelocity());
		const FVector ToActorVelocity = FromActor->GetTransform().InverseTransformVectorNoScale(ToActor->GetVelocity());
		const float CurrVel = FromActor->GetTransform().InverseTransformVectorNoScale(FromActor->GetVelocity() - ToActor->GetVelocity()).X;

		switch (Conditional)
		{
		case EScenarioEventConditional::LessThan:
			if (CurrVel < Speed)
			{
				EventDelegate.ExecuteIfBound();
			}
			break;
		case EScenarioEventConditional::GreaterThan:
			if (CurrVel > Speed)
			{
				EventDelegate.ExecuteIfBound();
			}
			break;
		}

		UE_LOG(LogSoda, Warning, TEXT("V %f"), CurrVel);
	}
}

//-------------------------------------------------------------------------------------------------
UEventStandStill::UEventStandStill(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("Stand Still"));
}

void UEventStandStill::Tick(float DeltaTime)
{
	//if ()
	{
		//EventDelegate.ExecuteIfBound();
	}
}
