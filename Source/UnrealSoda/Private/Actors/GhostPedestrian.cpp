// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/GhostPedestrian.h"
#include "Components/SkeletalMeshComponent.h"
#include "Soda/Actors/NavigationRoute.h"
#include "Soda/UnrealSoda.h"
#include "Components/BoxComponent.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaApp.h"
#include "Soda/Misc/EditorUtils.h"

AGhostPedestrian::AGhostPedestrian()
{
	Mesh = CreateDefaultSubobject< USkeletalMeshComponent >(TEXT("PedestrianMesh"));
	//Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);

	RootComponent = Mesh;
	PrimaryActorTick.bCanEverTick = true;
	USodaStatics::TagActor(this, true, EActorTagMethod::ForceCustomLabel, ESegmObjectLabel::Pedestrians);
}

void AGhostPedestrian::BeginPlay()
{
	Super::BeginPlay();

	InitTransform = GetActorTransform();

	TrajectoryPlaner.Init(GetWorld());
}

void AGhostPedestrian::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	TrajectoryPlaner.Deinit();
}

FVector AGhostPedestrian::GetVelocity() const
{
	if (bIsMoving && TrajectoryPlaner.GetWayPointsNum())
	{
		return GetActorTransform().TransformVectorNoScale(FVector(CurrentVelocity, 0, 0));
	}
	else
	{
		return FVector::ZeroVector;
	}
}

void AGhostPedestrian::InputWidgetDelta(const USceneComponent* WidgetTargetComponent, FTransform& NewWidgetTransform)
{
	if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
	{
		if (bJoinToInitialRoute)
		{
			const float Key = RoutePlanner->Spline->FindInputKeyClosestToWorldLocation(NewWidgetTransform.GetTranslation());
			NewWidgetTransform = RoutePlanner->Spline->GetTransformAtSplineInputKey(Key, ESplineCoordinateSpace::World);
			InitialRouteOffset = RoutePlanner->Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
		}
		else
		{
			UpdateJoinCurve();
		}
	}
}

void AGhostPedestrian::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	check(PlayerController);

	USodaSubsystem * SodaSubsystem  = USodaSubsystem::GetChecked();

	if (bIsMoving)
	{

		if (TrajectoryPlaner.UpdateTarjectory(GetWorld(), GetTransform(), 10000))
		{
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
		}

		if (CurrentSplineOffset > TrajectoryPlaner.GetRouteSpline()->GetSplineLength())
		{
			CurrentVelocity = 0;
		}

		if (TrajectoryPlaner.GetWayPointsNum())
		{
			if (bCheckCollision && CheckCollision(TrajectoryPlaner.GetRouteSpline()->GetTransformAtSplineInputKey(TrajectoryPlaner.GetCurrentSplineKey(), ESplineCoordinateSpace::World)))
			{
				CurrentVelocity = 0;
			}
			else
			{
				CurrentSplineOffset += DeltaTime * CurrentVelocity;
				CurrentVelocity = FMath::Clamp(CurrentVelocity + Acceleration * DeltaTime, 0.f, Velocity);
				TrajectoryPlaner.SetSplineOffset(CurrentSplineOffset);
				SetActorTransform(AdjustTransform(TrajectoryPlaner.GetRouteSpline()->GetTransformAtSplineInputKey(TrajectoryPlaner.GetCurrentSplineKey(), ESplineCoordinateSpace::World)));
			}
		}
	}
	else
	{
		if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
		{
			if (RoutePlanner->Spline->GetSplineGuid() != SplineGuid)
			{
				if (bJoinToInitialRoute)
				{
					SetActorTransform(AdjustTransform(RoutePlanner->Spline->GetTransformAtDistanceAlongSpline(InitialRouteOffset, ESplineCoordinateSpace::World)));
				}
				else
				{
					UpdateJoinCurve();
				}
				SplineGuid = RoutePlanner->Spline->GetSplineGuid();
			}
		}
	}

	if (!SodaSubsystem->IsScenarioRunning() && !bJoinToInitialRoute)
	{
		for (int i = 1; i < JoiningCurvePoints.Num(); ++i)
		{
			const FVector & P0 = JoiningCurvePoints[i - 1];
			const FVector & P1 = JoiningCurvePoints[i];
			DrawDebugLine(GetWorld(), P0, P1, FColor(180, 180, 255), false, -1.f, 0, 2);
		}
	}

	if (bDrawDebugRoute)
	{
		TrajectoryPlaner.DrawRoute(this);
	}

	if (SodaSubsystem->IsScenarioRunning())
	{
		SyncDataset();
	}
}

static AActor* FindActorByName(const FString& ActorNameStr, const UWorld* InWorld)
{
	for (ULevel const* Level : InWorld->GetLevels())
	{
		if (Level)
		{
			for (AActor* Actor : Level->Actors)
			{
				if (Actor)
				{
					if (Actor->GetName() == ActorNameStr)
					{
						return Actor;
					}
				}
			}
		}
	}

	return nullptr;
}


void AGhostPedestrian::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsSaveGame() && Ar.IsLoading())
	{
		if (ANavigationRouteEditable* FoundActor = Cast<ANavigationRouteEditable>(FEditorUtils::FindActorByName(InitialRoute.ToSoftObjectPath(), GetWorld())))
		{
			InitialRoute = FoundActor;
		}
	}
}

void AGhostPedestrian::GoToLeftRoute(float StaticOffset, float VelocityToOffset)
{
	if (bIsMoving)
	{
		if (TrajectoryPlaner.GoToLeftRoute(GetWorld(), GetActorTransform(), StaticOffset + CurrentVelocity * VelocityToOffset))
		{
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
		}
	}
}

void AGhostPedestrian::GoToRightRoute(float StaticOffset, float VelocityToOffset)
{
	if (bIsMoving)
	{
		if (TrajectoryPlaner.GoToRightRoute(GetWorld(), GetActorTransform(), StaticOffset + CurrentVelocity * VelocityToOffset))
		{
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
		}
	}
}

void AGhostPedestrian::GoToRoute(TSoftObjectPtr<ANavigationRoute> Route, float StaticOffset, float VelocityToOffset)
{
	if (bIsMoving && Route.Get())
	{
		if (TrajectoryPlaner.GoToRoute(GetWorld(), GetActorTransform(), StaticOffset + CurrentVelocity * VelocityToOffset, Route.Get()))
		{
			CurrentSplineOffset = TrajectoryPlaner.GetDistanceAlongSpline();
		}
	}
}

void AGhostPedestrian::StartMoving()
{
	if (!bIsMoving)
	{
		CurrentVelocity = 0;
		CurrentSplineOffset = 0;
		TrajectoryPlaner.Reset();
		InitTransform = GetActorTransform();
		bIsMoving = true;

		if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
		{
			if (!bJoinToInitialRoute)
			{
				UpdateJoinCurve();
				auto Poits = JoiningCurvePoints;
				Poits.Pop(false);
				TrajectoryPlaner.AddRouteByWaypoints(Poits, RoutePlanner, false, false, false);
			}
			TrajectoryPlaner.AddRouteBySpline(RoutePlanner->Spline, InitialRouteOffset, RoutePlanner, false, false, true);
		}

		ReceiveStartMoving();
	}
}

void AGhostPedestrian::StopMoving()
{
	if (bIsMoving)
	{
		SetActorTransform(InitTransform);
		bIsMoving = false;
		CurrentVelocity = 0;
		CurrentSplineOffset = 0;
		TrajectoryPlaner.Reset();

		ReceiveStopMoving();
	}
}

bool AGhostPedestrian::IsLinkedToRoute() const
{
	return !TrajectoryPlaner.IsEnded();
}

void AGhostPedestrian::ScenarioBegin()
{
	ISodaActor::ScenarioBegin();
	StartMoving();
}

void AGhostPedestrian::ScenarioEnd()
{
	ISodaActor::ScenarioEnd();
	StopMoving();
}

void AGhostPedestrian::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	IEditableObject::RuntimePostEditChangeProperty(PropertyChangedEvent);
}

void AGhostPedestrian::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) 
{
	IEditableObject::RuntimePostEditChangeChainProperty(PropertyChangedEvent);
	SplineGuid = FGuid::NewGuid();
}

void AGhostPedestrian::UpdateJoinCurve()
{
	JoiningCurvePoints.Reset();

	if (ANavigationRoute* RoutePlanner = InitialRoute.Get())
	{
		const FTransform StartTransform = GetActorTransform();
		const FTransform EndTransform = RoutePlanner->Spline->GetTransformAtDistanceAlongSpline(InitialRouteOffset, ESplineCoordinateSpace::World, false);
		const float TangentSize = (StartTransform.GetTranslation() - EndTransform.GetTranslation()).Size();

		FInterpCurveVector JoiningCurve;
		JoiningCurve.Points.Add(FInterpCurvePoint<FVector>(
			0,
			StartTransform.GetTranslation(),
			FVector::ZeroVector,
			StartTransform.GetUnitAxis(EAxis::X) * TangentSize,
			CIM_CurveUser
		));
		JoiningCurve.Points.Add(FInterpCurvePoint<FVector>(
			1,
			EndTransform.GetTranslation(),
			EndTransform.GetUnitAxis(EAxis::X) * TangentSize,
			FVector::ZeroVector,
			CIM_CurveUser
		));

		const int Steps = 20;
		for (int i = 0; i <= Steps; ++i)
		{
			JoiningCurvePoints.Add(JoiningCurve.Eval(i / float(Steps)));
		}
	}
}

FTransform AGhostPedestrian::AdjustTransform(const FTransform& InTransform)
{
	FTransform Transform = InTransform;
	Transform.SetRotation(InTransform.GetRotation() * RotationOffset.Quaternion());
	return Transform;
}

bool AGhostPedestrian::CheckCollision(const FTransform & Transform)
{
	//FVector GetActorLocation();
	//FQuat Rotation = TrajectoryPlaner.RouteSpline->GetQuaternionAtSplineInputKey(TrajectoryPlaner.CurrentSplineKey, ESplineCoordinateSpace::World);

	FCollisionQueryParams Params(TEXT("PedestrianCheckCollision"), true, this);
	//Params.bFindInitialOverlaps = false;
	//FCollisionObjectQueryParams ObjectParams{ ECollisionChannel::ECC_Visibility };

	FHitResult OutHit;
	GetWorld()->SweepSingleByChannel(OutHit, Transform.GetLocation(), Transform.GetLocation() + Transform.GetRotation().GetForwardVector() * TraceLookAhead + Transform.GetRotation().GetUpVector() * TraceUpOffes, FQuat::Identity, ECollisionChannel::ECC_WorldDynamic, FCollisionShape::MakeCapsule(TraceCapsuleRadius, TraceCapsuleHalfHeigh), Params);
	
	return IsValid(OutHit.GetActor());
}

const FSodaActorDescriptor* AGhostPedestrian::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Pedestrian"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("SodaIcons.Walking"), /*Icon*/
		true, /*bAllowTransform*/
		false, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}