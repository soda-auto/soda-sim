// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/UltrasonicSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaStatics.h"
#include "DrawDebugHelpers.h"
#include "Soda/Misc/SodaPhysicsInterface.h"
#include "Soda/Misc/MeshGenerationUtils.h"
#include "DynamicMeshBuilder.h"

DECLARE_STATS_GROUP(TEXT("UltrasonicSensorHub"), STATGROUP_UltrasonicSensorHub, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("TickComponent"), STAT_UltrasonicTickComponent, STATGROUP_UltrasonicSensorHub);
DECLARE_CYCLE_STAT(TEXT("batch execute"), STAT_UltrasonicBatchExecute, STATGROUP_UltrasonicSensorHub);
DECLARE_CYCLE_STAT(TEXT("Add to batch execute"), STAT_UltrasonicAddToBatch, STATGROUP_UltrasonicSensorHub);
DECLARE_CYCLE_STAT(TEXT("Process query results"), STAT_UltrasonicProcessQueryResults, STATGROUP_UltrasonicSensorHub);
DECLARE_CYCLE_STAT(TEXT("Publish results"), STAT_UltrasonicPublishResults, STATGROUP_UltrasonicSensorHub);
DECLARE_CYCLE_STAT(TEXT("Add Echo"), STAT_UltrasonicAddEcho, STATGROUP_UltrasonicSensorHub);

UUltrasonicSensorComponent::UUltrasonicSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other Sensors");
	GUI.ComponentNameOverride = TEXT("Generic Ultrasonic");
	GUI.IcanName = TEXT("SodaIcons.Ultrasonic");
	GUI.bIsPresentInAddMenu = true;

	FOVSetup.Color = FLinearColor(0.1, 0.1, 1.0, 1.0);
	FOVSetup.MaxViewDistance = 100;
}

bool UUltrasonicSensorComponent::GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes)
{

	TArray<FVector> Cloud;
	static const  int RingsSteps = 20;
	static const int RadStep = 20;

	float L0 = std::cos(FOV_Horizont / 180 * M_PI / 2);
	float R0 = std::sin(FOV_Horizont / 180 * M_PI / 2);
	float ZScale = FOV_Vertical / FOV_Horizont;
	float XScale = 0.5;

	for (int j = 0; j < RingsSteps; j++)
	{
		float A = float(j) / float(RingsSteps) * M_PI / 2;
		float R1 = std::cos(A) * R0;
		float L1 = std::sin(A) * XScale + L0;

		for (int i = 0; i < RadStep; i++)
		{
			float A1 = float(i) / float(RadStep) * M_PI * 2;
			Cloud.Add(FVector(L1, std::cos(A1) * R1, std::sin(A1) * R1 * ZScale) * FOVSetup.MaxViewDistance);
		}
	}
	Cloud.Add(FVector(1, 0, 0) * FOVSetup.MaxViewDistance);
	Cloud.Add(FVector(0));

	FSensorFOVMesh MeshData;
	if (FMeshGenerationUtils::GenerateConvexHullMesh3D(Cloud, MeshData.Vertices, MeshData.Indices))
	{
		for (auto& it : MeshData.Vertices) it.Color = FOVSetup.Color.ToFColor(true);
		Meshes.Add(MoveTempIfPossible(MeshData));
		return true;
	}
	else
	{
		return false;
	}

	return false;
}

bool UUltrasonicSensorComponent::NeedRenderSensorFOV() const
{
	return (FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::Ever) ||
		(FOVSetup.FOVRenderingStrategy == EFOVRenderingStrategy::OnSelect && IsVehicleComponentSelected());
	return false;
}

FBoxSphereBounds UUltrasonicSensorComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FMeshGenerationUtils::CalcFOVBounds(FOV_Horizont, FOV_Horizont, FOVSetup.MaxViewDistance)).TransformBy(LocalToWorld);
}

//////////////////////////////////////////////////////////////////////////////

UUltrasonicSensorHubComponent::UUltrasonicSensorHubComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other Sensors");
	GUI.ComponentNameOverride = TEXT("Generic Ultrasonic Hub");
	GUI.IcanName = TEXT("SodaIcons.Ultrasonic");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}


void UUltrasonicSensorHubComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;
	
	if (!PointCloudPublisher.IsAdvertised() || Sensors.Num() == 0) return;

	for (FUltrasonicEchos &Echo : EchoCollections) Echo.Clear();

	check(Sensors.Num() > CurrentTransmitter);

	Scan.device_timestamp = soda::RawTimestamp<std::chrono::milliseconds>(SodaApp.GetSimulationTimestamp());

	UUltrasonicSensorComponent * Sensor = Sensors[CurrentTransmitter];

	FVector Loc = Sensor->GetComponentLocation();
	FRotator Rot = Sensor->GetComponentRotation();
	FTransform RelTrans = Sensor->GetRelativeTransform();

	BatchStart.SetNum(0, false);
	BatchEnd.SetNum(0, false);

	if (bLogTick)
	{
		UE_LOG(LogSoda, Warning, TEXT("UltrasonicHub name: %s, transmitter %d, timestamp: %s"),
			*GetFName().ToString(),
			CurrentTransmitter,
			*USodaStatics::Int64ToString(Scan.device_timestamp));
	}

	if (DrawTracedRays)
	{
		DrawDebugLine(GetWorld(), Loc + Rot.RotateVector(FVector(1.0, 0.0, 0.0)) * Sensor->DistanseMin, Loc + Rot.RotateVector(FVector(1.0, 0.0, 0.0)) * Sensor->DistanseMax, FColor(0, 0, 255), false, -1.f, 0, 2.f);
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_UltrasonicAddToBatch);
		for (int i = 0; i < Sensor->Rows; i++)
		{
			float VerticalAng = Sensor->FOV_Vertical * -0.5f + Sensor->FOV_Vertical / (float)Sensor->Rows * (float)i;
			for (int j = 0; j < Sensor->Step; j++)
			{
				float HorizontAng = Sensor->FOV_Horizont * -0.5f + Sensor->FOV_Horizont / (float)Sensor->Step * (float)j;
				FVector RayNorm = Rot.RotateVector(FRotator(VerticalAng, HorizontAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0)));
				BatchStart.Add(Loc + RayNorm * Sensor->DistanseMin);
				BatchEnd.Add(Loc + RayNorm * Sensor->DistanseMax);
			}
		}
	}

	TArray<FHitResult> OutHits;

	{
		SCOPE_CYCLE_COUNTER(STAT_UltrasonicBatchExecute);
		FSodaPhysicsInterface::RaycastSingleScope(
			GetWorld(), OutHits, BatchStart, BatchEnd,
			ECollisionChannel::ECC_Visibility,
			FCollisionQueryParams(NAME_None, false, GetOwner()),
			FCollisionResponseParams::DefaultResponseParam,
			FCollisionObjectQueryParams::DefaultObjectQueryParam);
	}

	check(OutHits.Num() == BatchStart.Num());

	{
		SCOPE_CYCLE_COUNTER(STAT_UltrasonicProcessQueryResults);

		for (int i = 0; i < Sensor->Rows; i++)
		{
			float VerticalAng = Sensor->FOV_Vertical * -0.5f + Sensor->FOV_Vertical / (float)Sensor->Rows * (float)i;
			for (int j = 0; j < Sensor->Step; j++)
			{
				FHitResult& Hit = OutHits[i * Sensor->Step + j];
				bool Traced = Hit.bBlockingHit;

				if (Traced)
				{
					if (EnabledGroundFilter)
					{
						if ((Loc.Z - Hit.Location.Z) > DistanceToGround)
						{
							Traced = false;
						}
					}
				}

				if (Traced)
				{
					for (size_t k = 0; k < EchoCollections.Num(); ++k)
					{
						FVector CurLoc = Sensors[k]->GetComponentLocation();
						FRotator CurRot = Sensors[k]->GetComponentRotation();
						FVector TracedVector = CurRot.UnrotateVector(Hit.Location - CurLoc);
						FVector2D VHor(TracedVector.X, TracedVector.Y);
						float CosHor = VHor.X / VHor.Size();
						FVector2D VVert(TracedVector.X, TracedVector.Z);
						float CosVert = VVert.X / VVert.Size();
						float CurDistance = (Hit.Location - CurLoc).Size();
						FVector MirrorPerpendicular = ((CurLoc - Hit.Location) + (Loc - Hit.Location)) / 2;
						float Dot = FVector::DotProduct(Hit.Normal.GetSafeNormal(), MirrorPerpendicular.GetSafeNormal());
						float Power = powf(Dot / (CurDistance / 100.f), 2);
						if (CosHor < Sensor->CosFovHorizontal || CosVert < Sensor->CosFovVertical || CurDistance > Sensor->DistanseMax || Power < SensorThreshold)
							continue;

						EchoCollections[k].AddHit(&Hit, (k == CurrentTransmitter) ? CurDistance : CurDistance + (Hit.Location - Sensors[CurrentTransmitter]->GetComponentLocation()).Size(), Power, MinDistGap);
					}
				}

				if (DrawTracedRays || DrawNotTracedRays)
				{
					float HorizontAng = Sensor->FOV_Horizont * -0.5f + Sensor->FOV_Horizont / (float)Sensor->Step * (float)j;
					FVector RayNorm = Rot.RotateVector(FRotator(VerticalAng, HorizontAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0)));

					if (DrawTracedRays && Traced)
					{
						DrawDebugLine(GetWorld(), Loc + RayNorm * Sensor->DistanseMin, Hit.Location, FColor(255, 0, 0), false, -1.f, 0, 2.f);
					}
					else if (DrawNotTracedRays && !Traced)
					{
						DrawDebugLine(GetWorld(), Loc + RayNorm * Sensor->DistanseMin, Loc + RayNorm * Sensor->DistanseMax, FColor(0, 255, 0), false, -1.f, 0, 2.f);
					}
				}
			}
		}

		for (size_t i = 0; i < EchoCollections.Num(); ++i)
		{
			EchoCollections[i].RemoveExcessEchos();
			EchoCollections[i].Echos.Sort(FUltrasonicEcho::DistancePredicate);

			Scan.ultrasonics[i].properties = (i == CurrentTransmitter) ? soda::Ultrasonic::Properties::transmitter : soda::Ultrasonic::Properties::none;
			Scan.ultrasonics[i].echoes.resize(EchoCollections[i].Echos.Num(), 0.f);

			for (size_t j = 0; j < EchoCollections[i].Echos.Num(); ++j)
			{
				Scan.ultrasonics[i].echoes[j] = EchoCollections[i].Echos[j].BeginDistance / 100.f;
				if (DrawEchos)
				{
					FColor Color = (i == CurrentTransmitter) ? FColor(255, 255, 0) : FColor(120, 120, 255);
					DrawDebugLine(GetWorld(), Sensors[i]->GetComponentLocation(), EchoCollections[i].Echos[j].EchoPosition, Color, false, -1.f, 0, 2.f);
					DrawDebugPoint(GetWorld(), EchoCollections[i].Echos[j].EchoPosition, 10.f, Color, false, -1.f, 0);
				}
				if (bLogEchos)
					UE_LOG(LogSoda, Log, TEXT("ULTRASONIC Echo power = %f, BeginDistance = %f, EndDistance = %f, IsTransmitter = %d"), EchoCollections[i].Echos[j].ReturnPower, EchoCollections[i].Echos[j].BeginDistance, EchoCollections[i].Echos[j].EndDistance, (int)(i == CurrentTransmitter));
			}
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_UltrasonicPublishResults);
		PointCloudPublisher.Publish(Scan);
	}

	CurrentTransmitter = (CurrentTransmitter + 1) % Sensors.Num();
}

bool UUltrasonicSensorHubComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (AActor* Owner = GetOwner())
	{
		Owner->GetComponents(Sensors, false);
	}

	EchoCollections.SetNum(Sensors.Num());
	Scan.ultrasonics.resize(Sensors.Num());
	for (int i = 0; i< Sensors.Num(); ++i) 
	{
		Scan.ultrasonics[i].hfov = Sensors[i]->FOV_Horizont;
		Scan.ultrasonics[i].vfov = Sensors[i]->FOV_Vertical;

		Sensors[i]->CosFovVertical = cos(Sensors[i]->FOV_Vertical / 180.0 * M_PI / 2);
		Sensors[i]->CosFovHorizontal = cos(Sensors[i]->FOV_Horizont / 180.0 * M_PI / 2);
	}

	PointCloudPublisher.Advertise();
	if (!PointCloudPublisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't advertise publisher"));
		return false;
	}

	return true;
}

void UUltrasonicSensorHubComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PointCloudPublisher.Shutdown();
}

void UUltrasonicSensorHubComponent::GetRemark(FString& Info) const
{
	Info = PointCloudPublisher.Address + ":" + FString::FromInt(PointCloudPublisher.Port);
}

void FUltrasonicEchos::AddHit(FHitResult* NewHit, float Distance, float Power, float MinDistGap)
{
	{
		SCOPE_CYCLE_COUNTER(STAT_UltrasonicAddEcho);
		

		FUltrasonicEcho* EchoToAddHit = Echos.FindByPredicate([this, NewHit, Distance, MinDistGap](FUltrasonicEcho EchoToCheck) { return (Distance > EchoToCheck.BeginDistance - MinDistGap) && (Distance < EchoToCheck.EndDistance + MinDistGap); });
		if (EchoToAddHit == 0)
		{
			Echos.Add(FUltrasonicEcho());
			Echos.Last().Hits.Add(NewHit);
			Echos.Last().BeginDistance = Distance;
			Echos.Last().EndDistance = Distance;
			Echos.Last().ReturnPower = Power;
			Echos.Last().EchoPosition = NewHit->Location;
		}
		else
		{
			if (Distance < EchoToAddHit->BeginDistance)
			{
				EchoToAddHit->EchoPosition = NewHit->Location;
			}
			EchoToAddHit->BeginDistance = (std::min)(EchoToAddHit->BeginDistance, Distance);
			EchoToAddHit->EndDistance = (std::max)(EchoToAddHit->EndDistance, Distance);
			EchoToAddHit->ReturnPower = (std::max)(EchoToAddHit->ReturnPower, Power);
			
			EchoToAddHit->Hits.Add(NewHit);
		}
	}
}


void FUltrasonicEchos::RemoveExcessEchos()
{
	if (Echos.Num() <= EchosMaxNum)
		return;

	Echos.Sort(FUltrasonicEcho::ReturnPowerPredicate);
	Echos.SetNum(EchosMaxNum);
}


