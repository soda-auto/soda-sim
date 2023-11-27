// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/RefPoint.h"
#include "Soda/UnrealSoda.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "Soda/LevelState.h"
#include "UObject/ConstructorHelpers.h"
#include "SceneView.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Engine/GameViewportClient.h"
#include <cmath>

namespace soda
{

void BuildCylinderVerts(const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, float Radius, float HalfHeight, uint32 Sides, TArray<FVector>& OutVerts, TArray<int32>& OutIndices)
{
	const float	AngleDelta = 2.0f * PI / Sides;
	FVector	LastVertex = Base + XAxis * Radius;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	float TCStep = 1.0f / Sides;

	FVector TopOffset = HalfHeight * ZAxis;

	int32 BaseVertIndex = OutVerts.Num();

	//Compute vertices for base circle.
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		OutVerts.Add(Vertex - TopOffset); //Add bottom vertex
		LastVertex = Vertex;
		TC.X += TCStep;
	}

	LastVertex = Base + XAxis * Radius;
	TC = FVector2D(0.0f, 1.0f);

	//Compute vertices for the top circle
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		OutVerts.Add(Vertex + TopOffset); //Add top vertex
		LastVertex = Vertex;
		TC.X += TCStep;
	}

	//Add top/bottom triangles, in the style of a fan.
	//Note if we wanted nice rendering of the caps then we need to duplicate the vertices and modify
	//texture/tangent coordinates.
	for (uint32 SideIndex = 1; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex;
		int32 V1 = BaseVertIndex + SideIndex;
		int32 V2 = BaseVertIndex + ((SideIndex + 1) % Sides);

		//bottom
		OutIndices.Add(V2);
		OutIndices.Add(V1);
		OutIndices.Add(V0);

		// top
		OutIndices.Add(Sides + V0);
		OutIndices.Add(Sides + V1);
		OutIndices.Add(Sides + V2);
	}

	//Add sides.

	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex + SideIndex;
		int32 V1 = BaseVertIndex + ((SideIndex + 1) % Sides);
		int32 V2 = V0 + Sides;
		int32 V3 = V1 + Sides;

		OutIndices.Add(V1);
		OutIndices.Add(V2);
		OutIndices.Add(V0);

		OutIndices.Add(V1);
		OutIndices.Add(V3);
		OutIndices.Add(V2);
	}
}

}

ARefPoint::ARefPoint()
{
	static ConstructorHelpers::FObjectFinder< UMaterial > AxesMatDense(TEXT("/SodaSim/Assets/CPP/AxesMatDense"));
	static ConstructorHelpers::FObjectFinder< UMaterial > AxesMatOpacity(TEXT("/SodaSim/Assets/CPP/AxesMatOpacity"));
	static ConstructorHelpers::FObjectFinder< UMaterial > TextMat(TEXT("/SodaSim/Assets/CPP/UnlitText"));

	Shaft = CreateDefaultSubobject< UProceduralMeshComponent >(TEXT("Shaft"));
	RootComponent = Shaft;

	if (AxesMatOpacity.Succeeded()) MatOpacity  = AxesMatOpacity.Object;
	if (AxesMatDense.Succeeded())  MatDense = AxesMatDense.Object;
	if (TextMat.Succeeded())  MatText = TextMat.Object;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ARefPoint::BeginPlay()
{
	Super::BeginPlay();

	Shaft->ClearAllMeshSections();

	const float TotalLength = 2000000;
	const float ShaftRadius = 3;

	TArray < FVector > Vertices;
	TArray < int32 >Triangles;
	TArray<FVector2D> UV0;
	TArray<FVector> Normals;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	soda::BuildCylinderVerts(FVector(0, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0), FVector(1, 0, 0), ShaftRadius, 0.5f * TotalLength, 16, Vertices, Triangles);
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);
	Shaft->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);
	Shaft->CreateMeshSection(1, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);

	Vertices.Empty();
	Triangles.Empty();
	UV0.Empty();
	Normals.Empty();
	Colors.Empty();
	Tangents.Empty();

	soda::BuildCylinderVerts(FVector(0, 0, 0), FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0), ShaftRadius, 0.5f * TotalLength, 16, Vertices, Triangles);
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);
	Shaft->CreateMeshSection(2, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);
	Shaft->CreateMeshSection(3, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);

	Vertices.Empty();
	Triangles.Empty();
	UV0.Empty();
	Normals.Empty();
	Colors.Empty();
	Tangents.Empty();

	soda::BuildCylinderVerts(FVector(0, 0, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), ShaftRadius, 0.5f * TotalLength, 16, Vertices, Triangles);
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);
	Shaft->CreateMeshSection(4, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);
	Shaft->CreateMeshSection(5, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);

	if (IsValid(MatDense))
	{
		UMaterialInstanceDynamic* MatDenseDyn = UMaterialInstanceDynamic::Create(MatDense, this);
		Shaft->SetMaterial(0, MatDenseDyn);
		MatDenseDyn->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1, 0, 0));
	}

	if (IsValid(MatOpacity))
	{
		UMaterialInstanceDynamic* MatOpacityDyn = UMaterialInstanceDynamic::Create(MatOpacity, this);
		Shaft->SetMaterial(1, MatOpacityDyn);
		MatOpacityDyn->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1, 0, 0));
	}

	if (IsValid(MatDense))
	{
		UMaterialInstanceDynamic* MatDenseDyn = UMaterialInstanceDynamic::Create(MatDense, this);
		Shaft->SetMaterial(2, MatDenseDyn);
		MatDenseDyn->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0, 1, 0));
	}

	if (IsValid(MatOpacity))
	{
		UMaterialInstanceDynamic* MatOpacityDyn = UMaterialInstanceDynamic::Create(MatOpacity, this);
		Shaft->SetMaterial(3, MatOpacityDyn);
		MatOpacityDyn->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0, 1, 0));
	}

	if (IsValid(MatDense))
	{
		UMaterialInstanceDynamic* MatDenseDyn = UMaterialInstanceDynamic::Create(MatDense, this);
		Shaft->SetMaterial(4, MatDenseDyn);
		MatDenseDyn->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0, 0, 1));
	}

	if (IsValid(MatOpacity))
	{
		UMaterialInstanceDynamic* MatOpacityDyn = UMaterialInstanceDynamic::Create(MatOpacity, this);
		Shaft->SetMaterial(5, MatOpacityDyn);
		MatOpacityDyn->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0, 0, 1));
	}

	TextRenderNorth = NewObject<UTextRenderComponent>(this);
	TextRenderNorth->SetupAttachment(RootComponent);
	TextRenderNorth->RegisterComponent();
	TextRenderNorth->SetText(FText::FromString(TEXT("N")));
	TextRenderNorth->SetRelativeRotation(FRotator(90, -90, 0));
	TextRenderNorth->SetWorldSize(50);
	TextRenderNorth->SetTextRenderColor(FColor(0, 255, 0));
	TextRenderNorth->SetVisibility(false);

	TextRenderSouth = NewObject<UTextRenderComponent>(this);
	TextRenderSouth->SetupAttachment(RootComponent);
	TextRenderSouth->RegisterComponent();
	TextRenderSouth->SetText(FText::FromString(TEXT("S")));
	TextRenderSouth->SetRelativeRotation(FRotator(90, -90, 0));
	TextRenderSouth->SetWorldSize(50);
	TextRenderSouth->SetTextRenderColor(FColor(0, 255, 0));
	TextRenderSouth->SetVisibility(false);

	TextRenderWest = NewObject<UTextRenderComponent>(this);
	TextRenderWest->SetupAttachment(RootComponent);
	TextRenderWest->RegisterComponent();
	TextRenderWest->SetText(FText::FromString(TEXT("W")));
	TextRenderWest->SetRelativeRotation(FRotator(90, -90, 0));
	TextRenderWest->SetWorldSize(50);
	TextRenderWest->SetTextRenderColor(FColor(255, 0, 0));
	TextRenderWest->SetVisibility(false);

	TextRenderEast = NewObject<UTextRenderComponent>(this);
	TextRenderEast->SetupAttachment(RootComponent);
	TextRenderEast->RegisterComponent();
	TextRenderEast->SetText(FText::FromString(TEXT("E")));
	TextRenderEast->SetRelativeRotation(FRotator(90, -90, 0));
	TextRenderEast->SetWorldSize(50);
	TextRenderEast->SetTextRenderColor(FColor(255, 0, 0));
	TextRenderEast->SetVisibility(false);

	if (MatText)
	{
		TextRenderNorth->SetTextMaterial(MatText);
		TextRenderSouth->SetTextMaterial(MatText);
		TextRenderWest->SetTextMaterial(MatText);
		TextRenderEast->SetTextMaterial(MatText);
	}

	const auto & LLConverter = ALevelState::GetChecked()->GetLLConverter();
	SetActorLocation(LLConverter.OrignShift);
	SetActorRotation(FRotator(0, LLConverter.OrignDYaw, 0));
	Longitude = LLConverter.OrignLon;
	Latitude = LLConverter.OrignLat;
	Altitude = LLConverter.OrignAltitude;
}

void ARefPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARefPoint::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	check(World);

	Shaft->SetMeshSectionVisible(0, bDrawAxis);
	Shaft->SetMeshSectionVisible(1, bDrawAxis && bDrawOverTop);
	Shaft->SetMeshSectionVisible(2, bDrawAxis);
	Shaft->SetMeshSectionVisible(3, bDrawAxis && bDrawOverTop);
	Shaft->SetMeshSectionVisible(4, bDrawAxis);
	Shaft->SetMeshSectionVisible(5, bDrawAxis && bDrawOverTop);

	TextRenderNorth->SetVisibility(bDrawAxis);
	TextRenderSouth->SetVisibility(bDrawAxis);
	TextRenderWest->SetVisibility(bDrawAxis);
	TextRenderEast->SetVisibility(bDrawAxis);
	
	if (bDrawAxis)
	{
		float FlipX = 1; //bInvertX ? -1 : 1;
		float FlipY = 1; //bInvertY ? -1 : 1;
		TextRenderNorth->SetRelativeLocation(FVector(-10, 130 * FlipY, 0));
		TextRenderSouth->SetRelativeLocation(FVector(-10, -130 * FlipY, 0));
		TextRenderWest->SetRelativeLocation(FVector(130 * FlipX, 0, 0));
		TextRenderEast->SetRelativeLocation(FVector(-130 * FlipX, 0, 0));

		if (bScreenAutoScale)
		{
			if (APlayerController* PlayerController = World->GetFirstPlayerController())
			{
				const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
				if (LocalPlayer && LocalPlayer->ViewportClient)
				{
					FSceneViewProjectionData ProjectionData;
					if (LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
					{
						const float ZoomFactor = FMath::Min<float>(ProjectionData.ProjectionMatrix.M[0][0], ProjectionData.ProjectionMatrix.M[1][1]);
						if (ZoomFactor != 0.0f)
						{
							const float Radius = (ProjectionData.ViewOrigin - GetActorLocation()).Size() * (ScreenSize / ZoomFactor);
							SetActorScale3D(FVector(std::fabsf(Radius)));
						}
					}
				}
			}
		}
	}
}

void ARefPoint::UpdateGlobalRefPoint()
{
	if (HasActorBegunPlay())
	{
		ALevelState::GetChecked()->SetGeoReference(Latitude, Longitude, Altitude, GetActorLocation(), GetActorRotation().Yaw);
	}
	else
	{
		TActorIterator<ALevelState> It(GetWorld());
		if (It)
		{
			It->SetGeoReference(Latitude, Longitude, Altitude, GetActorLocation(), GetActorRotation().Yaw);
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ARefPoint::UpdateGlobalRefPoint(); Add ALavelState actor to the level first"));
		}
	}
}

const FSodaActorDescriptor* ARefPoint::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Reference Point"), /*DisplayName*/
		TEXT("Tools"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("SodaIcons.Compass"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 0), /*SpawnOffset*/
	};
	return &Desc;
}
