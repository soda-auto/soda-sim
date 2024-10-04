// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/OdometryTest.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Misc/Utils.h"
#include "Soda/SodaTypes.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Widgets/SWindow.h"

UOdometryTestComponent::UOdometryTestComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Odometry Test");
	GUI.Category = TEXT("Other");
	GUI.bIsPresentInAddMenu = true;

	TelemetryGrid.InitGrid(1, 3, 300, 300, 500);	
	TelemetryGrid.InitCell(0, 0, -50, 50, "", TEXT("Velocity [cm/s]"), { {255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128} });
	TelemetryGrid.InitCell(0, 1, -0.5, 0.5, "", TEXT("Orientation [deg]"), { {255, 0, 0, 128}, {0, 255, 0}, {0, 0, 255, 128} });
	TelemetryGrid.InitCell(0, 2, -5, 5, "", TEXT("Pose [m]"), { {255, 0, 0, 128}, {0, 255, 0, 128}, {0, 0, 255, 128} });
}

void UOdometryTestComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UOdometryTestComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

bool UOdometryTestComponent::OnActivateVehicleComponent()
{
	return Super::OnActivateVehicleComponent();
}

void UOdometryTestComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (IsValid(GraphWindow))
	{
		GraphWindow->Close();
	}
}

FString UOdometryTestComponent::GetRemark() const
{
	return "";
}

void UOdometryTestComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if(Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();

		Canvas->SetDrawColor(FColor::White);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OXTS mode: %d"), OXTS_PositionMode), 16, YPos);
	}

	if (IsValid(GraphWindow) && GraphWindow->ExtraWindow && IsValid(CanvasRenderTarget2D))
	{
		const int Width = int(GraphWindow->ExtraWindow->GetClientSizeInScreen().X);
		const int Height = int(GraphWindow->ExtraWindow->GetClientSizeInScreen().Y);
		if (CanvasRenderTarget2D->GetSurfaceWidth() != Width || CanvasRenderTarget2D->GetSurfaceHeight() != Height)
		{
			CanvasRenderTarget2D->ResizeTarget(Width, Height);
		}
		CanvasRenderTarget2D->UpdateResource();

		FDrawToRenderTargetContext Context;
		UCanvas* Canvas2 = nullptr;
		FVector2D CanvasSize;
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, CanvasRenderTarget2D, Canvas2, CanvasSize, Context);
		float XPos = 10;
		TelemetryGrid.Draw(Canvas2, XPos, 10);
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);

	}
}

void UOdometryTestComponent::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);

	const static FVector G { 0, 0, -980.f };

	if (!HealthIsWorkable()) return;

	VehicleKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro, G.Z);

	if (bIsFirstTick)
	{
		bIsFirstTick = false;
		if (bInitPositionFromZero)
		{
			P = { 0, 0, 0 };
			Q = FQuat(EForceInit::ForceInit);
		}
		else
		{
			P = WorldPose.GetTranslation();
			Q = WorldPose.GetRotation();
		}
		V = WorldVel;
	}

	/* Compute odometry */

	const FVector Acc = Q.RotateVector(-LocalAcc) + G;
	P += (V * DeltaTime + (Acc * DeltaTime * DeltaTime * 0.5));
	V += Acc * DeltaTime;
	Q *= QExp(Gyro * DeltaTime);
	Q.Normalize();

	if (IsValid(GraphWindow) && GraphWindow->ExtraWindow && IsValid(CanvasRenderTarget2D))
	{
		FVector dV = (V - WorldVel);
		FVector dP = (P - WorldPose.GetTranslation()) / 100;
		FRotator dR = Q.Rotator() - WorldPose.GetRotation().Rotator();

		TelemetryGrid.AddPoint(0, 0, dV.X, 0);
		TelemetryGrid.AddPoint(0, 0, dV.Y, 1);
		TelemetryGrid.AddPoint(0, 0, dV.Z, 2);
		TelemetryGrid.AddPoint(0, 1, dR.Roll , 0);
		TelemetryGrid.AddPoint(0, 1, dR.Pitch, 1);
		TelemetryGrid.AddPoint(0, 1, dR.Yaw, 2);
		TelemetryGrid.AddPoint(0, 2, dP.X, 0);
		TelemetryGrid.AddPoint(0, 2, dP.Y, 1);
		TelemetryGrid.AddPoint(0, 2, dP.Z, 2);
	}

	//UE_LOG(LogSoda, Log, TEXT("%s | %s"),* Q.Rotator().ToString(), *WorldPose.GetRotation().Rotator().ToString());
	//UE_LOG(LogSoda, Log, TEXT("%s | %s"), *V.ToString(), *WorldVel.ToString());
	//UE_LOG(LogSoda, Log, TEXT("%s | %s"), *P.ToString(), *WorldPose.GetTranslation().ToString());
}

FOdometryData UOdometryTestComponent::GetOdometry() const
{
	FScopeLock ScopeLock(&GetVehicle()->PhysicMutex);

	return FOdometryData {
		P, Q.Rotator(), V,
		WorldPose.GetTranslation(),  WorldPose.GetRotation().Rotator(), WorldVel
	};
}

void UOdometryTestComponent::ShowGraphs()
{
	if (!IsValid(GraphWindow))
	{
		GraphWindow = NewObject<UExtraWindow>();
		check(GraphWindow);
	}

	if (!IsValid(CanvasRenderTarget2D))
	{
		CanvasRenderTarget2D = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this, UCanvasRenderTarget2D::StaticClass(), 1050, 500); 
		check(CanvasRenderTarget2D);
	}

	GraphWindow->OpenCameraWindow(CanvasRenderTarget2D);
}
