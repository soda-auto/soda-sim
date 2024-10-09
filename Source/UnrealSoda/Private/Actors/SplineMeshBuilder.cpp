// Fill out your copyright notice in the Description page of Project Settings.


#include "Soda/Actors/SplineMeshBuilder.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ASplineMeshBuilder::ASplineMeshBuilder()
{

	PrimaryActorTick.bCanEverTick = false;

	SplineRoadBackComponent = CreateDefaultSubobject<USplineComponent>("RoadBack");

	if (SplineRoadBackComponent)
	{
		SetRootComponent(SplineRoadBackComponent);
	}

}

void ASplineMeshBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	bClosedLoop = SplineRoadBackComponent->IsClosedLoop();

	UpdateUpVectors();
	PlaceMeshesAlongSpline();
}


void ASplineMeshBuilder::UpdateUpVectors()
{
	for (int SplineCount = 0; SplineCount < (SplineRoadBackComponent->GetNumberOfSplinePoints()); SplineCount++)
	{
		FRotator RotnMid = SplineRoadBackComponent->GetRotationAtSplinePoint(SplineCount, ESplineCoordinateSpace::World);
		FVector NewUp = UKismetMathLibrary::GetUpVector(RotnMid);

		SplineRoadBackComponent->SetUpVectorAtSplinePoint(SplineCount, NewUp, ESplineCoordinateSpace::World, true);
	}
}

void ASplineMeshBuilder::PlaceMeshesAlongSpline()
{
	SplineMeshComponentArray.Empty();

	if (!MeshOnSpline)
	{
		return;
	}

	int32 NumberOfMeshes = FMath::TruncToInt((SplineRoadBackComponent->GetSplineLength() / (LengthOfTheSegment))) - (bClosedLoop ? int32(0) : int32(1));

	for (int SplineCount = 0; SplineCount <= NumberOfMeshes; SplineCount++)
	{
		float CurDst = SplineCount * LengthOfTheSegment;
		float NextDst = FMath::Min((SplineCount + 1) * LengthOfTheSegment, SplineRoadBackComponent->GetSplineLength());
		float MidDst = 0.5 * (CurDst + NextDst);
		float TileLength = NextDst - CurDst;


		USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass());

		bool bSetAlternativeMesh = false;
		int32 IdxOfReplacementMesh = 0;

		int32 k = 0;

		for (const auto& Elem : ReplacementsMeshes)
		{
			if (MidDst > Elem.DistanceOnSplineToCover.Min && MidDst < Elem.DistanceOnSplineToCover.Max && Elem.MeshOnSplineAlternative)
			{
				bSetAlternativeMesh = true;
				IdxOfReplacementMesh = k;				
			}
			k = k + 1;
		}

		if (bUseReplacementsMeshes && bSetAlternativeMesh)
		{
			SplineMeshComponent->SetStaticMesh(ReplacementsMeshes[IdxOfReplacementMesh].MeshOnSplineAlternative);
		}
		else
		{
			SplineMeshComponent->SetStaticMesh(MeshOnSpline);
		}

		SplineMeshComponent->SetMobility(EComponentMobility::Static);
		SplineMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		SplineMeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		SplineMeshComponent->RegisterComponentWithWorld(GetWorld());
		SplineMeshComponent->AttachToComponent(SplineRoadBackComponent, FAttachmentTransformRules::KeepRelativeTransform);


		FVector StartPoint = SplineRoadBackComponent->GetLocationAtDistanceAlongSpline(CurDst, ESplineCoordinateSpace::Local);
		FVector StartTangent = SplineRoadBackComponent->GetTangentAtDistanceAlongSpline(CurDst, ESplineCoordinateSpace::Local);

		FVector EndPoint = SplineRoadBackComponent->GetLocationAtDistanceAlongSpline(NextDst, ESplineCoordinateSpace::Local);
		FVector EndTangent = SplineRoadBackComponent->GetTangentAtDistanceAlongSpline(NextDst, ESplineCoordinateSpace::Local);

		StartTangent = StartTangent.GetSafeNormal(0.0001) * FMath::Min(StartTangent.Size(), TileLength);
		EndTangent = EndTangent.GetSafeNormal(0.0001) * FMath::Min(EndTangent.Size(), TileLength);

		if (OverrideMaterial)
		{	
			SplineMeshComponent->SetMaterial(0, OverrideMaterial);
		}

		FRotator RotnMid = SplineRoadBackComponent->GetRotationAtDistanceAlongSpline(MidDst, ESplineCoordinateSpace::Local);
		FVector NewUp = UKismetMathLibrary::GetUpVector(RotnMid);

		SplineMeshComponent->SetSplineUpDir(NewUp, false);
		SplineMeshComponent->SetStartAndEnd(StartPoint, StartTangent, EndPoint, EndTangent, false);

		SplineMeshComponent->SetStartRoll(GetRoll(CurDst, SplineRoadBackComponent->GetRotationAtDistanceAlongSpline(MidDst, ESplineCoordinateSpace::Local)), false);
		SplineMeshComponent->SetEndRoll(GetRoll(NextDst, SplineRoadBackComponent->GetRotationAtDistanceAlongSpline(MidDst, ESplineCoordinateSpace::Local)), true);

		SplineMeshComponentArray.Add(SplineMeshComponent);


	}
}


float ASplineMeshBuilder::GetRoll(float Dst, FRotator RelativeRotation)
{

	FVector X1 = SplineRoadBackComponent->GetDirectionAtDistanceAlongSpline(Dst, ESplineCoordinateSpace::Local);
	FVector X2 = SplineRoadBackComponent->GetRightVectorAtDistanceAlongSpline(Dst, ESplineCoordinateSpace::Local);
	FVector X3 = SplineRoadBackComponent->GetUpVectorAtDistanceAlongSpline(Dst, ESplineCoordinateSpace::Local);

	X1 = RelativeRotation.UnrotateVector(X1);
	X2 = RelativeRotation.UnrotateVector(X2);
	X3 = RelativeRotation.UnrotateVector(X3);

	return UKismetMathLibrary::MakeRotationFromAxes(X1, X2, X3).Roll * PI / 180;
}



