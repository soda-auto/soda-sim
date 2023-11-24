// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/WheeledVehicleMovements/SodaChaosWheeledVehicleMovement.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "DisplayDebugHelpers.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Engine/CollisionProfile.h"
#include "Components/SkeletalMeshComponent.h"
#include "ChaosVehicleWheel.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "SimpleVehicle.h"
#include "VehicleUtility.h"
#include "ChaosVehicleManager.h"

/**********************************************************************************
 * USodaChaosWheeledVehicleSimulation 
 **********************************************************************************/

USodaChaosWheeledVehicleSimulation::USodaChaosWheeledVehicleSimulation(ASodaWheeledVehicle* WheeledVehicleIn, USodaChaosWheeledVehicleMovementComponent* InWheeledVehicleComponent) :
	WheeledVehicle(WheeledVehicleIn),
	WheeledVehicleComponent(InWheeledVehicleComponent)
{
	check(WheeledVehicleIn);
	check(InWheeledVehicleComponent);
}

void USodaChaosWheeledVehicleSimulation::Init(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicleIn)
{
	UChaosWheeledVehicleSimulation::Init(PVehicleIn);

	VehicleSimData.VehicleKinematic.Push(0.1);
	VehicleSimData.VehicleKinematic.Curr.GlobalPose = VehicleState.VehicleWorldTransform;
	VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass = VehicleState.VehicleWorldVelocity;
	VehicleSimData.VehicleKinematic.Curr.AngularVelocity = VehicleState.VehicleWorldAngularVelocity;
	VehicleSimData.VehicleKinematic.Curr.CenterOfMassLocal = VehicleState.VehicleWorldCOM - VehicleState.VehicleWorldTransform.GetTranslation();
}

void USodaChaosWheeledVehicleSimulation::UpdateSimulation(float DeltaTime, const FChaosVehicleAsyncInput& InputData, Chaos::FRigidBodyHandle_Internal* Handle)
{
	UChaosWheeledVehicleSimulation::UpdateSimulation(DeltaTime, InputData, Handle);

	//TODO: Add realtime indicator
	//auto Diff = soda::Now() - VehicleSimData.SimulatedTimestamp;
	//UE_LOG(LogSoda, Warning, TEXT("*** %f %f"), std::chrono::duration_cast<float>(Diff).count(), DeltaTime);

	FScopeLock ScopeLock(&WheeledVehicle->PhysicMutex);

	const auto& ChaosWheels = PVehicle->Wheels;
	const auto& SodaChaosWheelSetup = WheeledVehicleComponent->SodaWheelSetups;

	check(SodaChaosWheelSetup.Num() == ChaosWheels.Num());

	/*
	 *    Post Simulation
	 */
	
	VehicleSimData.VehicleKinematic.Push(DeltaTime);
	VehicleSimData.VehicleKinematic.Curr.GlobalPose = VehicleState.VehicleWorldTransform;
	VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass = VehicleState.VehicleWorldVelocity;
	//VehicleSimData.VehicleKinematic.Curr.AngularVelocity = VehicleState.VehicleWorldAngularVelocity;
	VehicleSimData.VehicleKinematic.Curr.CenterOfMassLocal = VehicleState.VehicleWorldCOM; // VehicleWorldCOM is local space (not world). Is it bug of UE source? 
	VehicleSimData.VehicleKinematic.ComputeAngularVelocity();
	//VehicleSimData.VehicleKinematic.ComputeGlobalVelocityOfCenterMass(); 
	VehicleSimData.VehicleKinematic.ComputeGlobalAcceleration();
	++VehicleSimData.SimulatedStep;
	VehicleSimData.SimulatedTimestamp = /*bSynchronousMode ? SodaApp.GetSimulationTimestamp() :*/ soda::Now();

	
	for (int i = 0; i < ChaosWheels.Num(); ++i)
	{
		
		SodaChaosWheelSetup[i].SodaWheel->Pitch = -ChaosWheels[i].GetAngularPosition();
		SodaChaosWheelSetup[i].SodaWheel->Steer = ChaosWheels[i].GetSteeringAngle() / 180 * M_PI;
		SodaChaosWheelSetup[i].SodaWheel->AngularVelocity = -ChaosWheels[i].GetAngularVelocity();
		SodaChaosWheelSetup[i].SodaWheel->Slip = FVector2D(std::cos(ChaosWheels[i].GetSlipAngle()), std::sin(ChaosWheels[i].GetSlipAngle())) * ChaosWheels[i].GetSlipMagnitude();
		SodaChaosWheelSetup[i].SodaWheel->SuspensionOffset = PVehicle->Suspension[i].GetSuspensionOffset();
	}

	WheeledVehicleComponent->PostPhysicSimulation(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);
	WheeledVehicleComponent->PostPhysicSimulationDeferred(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);
	
	/*
	 *   Pre Simulation
	 */
	
	WheeledVehicleComponent->PrePhysicSimulation(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);

	for (int32 i = 0; i < ChaosWheels.Num(); i++)
	{
		PVehicle->Wheels[i].SetDriveTorque(Chaos::TorqueMToCm(SodaChaosWheelSetup[i].SodaWheel->ReqTorq));
		PVehicle->Wheels[i].SetSteeringAngle(SodaChaosWheelSetup[i].SodaWheel->ReqSteer / M_PI * 180);
		PVehicle->Wheels[i].SetBrakeTorque(Chaos::TorqueMToCm(SodaChaosWheelSetup[i].SodaWheel->ReqBrakeTorque));
	}
	
}

/**********************************************************************************
 * USodaChaosWheeledVehicleMovementComponent
 **********************************************************************************/
USodaChaosWheeledVehicleMovementComponent::USodaChaosWheeledVehicleMovementComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Physic");
	GUI.IcanName = TEXT("SodaIcons.Tire");
	GUI.ComponentNameOverride = TEXT("Chaos Physic");
	GUI.bIsPresentInAddMenu = true;
	Common.UniqueVehiceComponentClass = UWheeledVehicleMovementInterface::StaticClass();
	Common.bIsTopologyComponent = true;

	WheelSetups.SetNum(4);
	bWantsInitializeComponent = true;

	bMechanicalSimEnabled = false;
}

void USodaChaosWheeledVehicleMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void USodaChaosWheeledVehicleMovementComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void USodaChaosWheeledVehicleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	WheeledVehicle = Cast<ASodaWheeledVehicle>(GetOwner());
}

void USodaChaosWheeledVehicleMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void USodaChaosWheeledVehicleMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!HealthIsWorkable()) return;

	FVehicleSimData& VehicleSimData = static_cast<USodaChaosWheeledVehicleSimulation*>(VehicleSimulationPT.Get())->VehicleSimData;

	VehicleSimData.RenderStep = VehicleSimData.SimulatedStep;
	VehicleSimData.RenderTimestamp = VehicleSimData.SimulatedTimestamp;

	USkinnedMeshComponent *Mesh = Cast< USkinnedMeshComponent >(UpdatedPrimitive);
	if (!Mesh) return;	
}

void USodaChaosWheeledVehicleMovementComponent::DrawDebug(UCanvas *Canvas, float &YL, float &YPos)
{
	ISodaVehicleComponent::DrawDebug(Canvas, YL, YPos);

	/*
	if (PVehicle == NULL) return;

	UFont* RenderFont = GEngine->GetSmallFont();

	Canvas->SetDrawColor(FColor::Green);
	YPos += Canvas->DrawText(RenderFont, TEXT("PhysX Vehicle:"), 4, YPos + 4);

	Canvas->SetDrawColor(FColor::White);
	//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Speed (km/h): %.1f"), CmSToKmH(ForwardSpeed)), 4, YPos);
	//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Acc (m/s^2): %.1f"), FowardAcc / 100.0), 4, YPos);

	FPhysXVehicleManager *MyVehicleManager = FPhysXVehicleManager::GetVehicleManagerFromScene(GetWorld()->GetPhysicsScene());
	MyVehicleManager->SetRecordTelemetry(this, true);


	SCOPED_SCENE_READ_LOCK(MyVehicleManager->GetScene());
	PxWheelQueryResult *WheelsStates = MyVehicleManager->GetWheelsStates_AssumesLocked(this);
	check(WheelsStates);

	float XPos = 4.f;
	auto GetXPos = [&XPos](float Amount) -> float {
		float Ret = XPos;
		XPos += Amount;
		return Ret;
	};

	if (bShowWheelSates)
	{
		for (uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w)
		{
			XPos = 4.f;
			const PxMaterial *ContactSurface = WheelsStates[w].tireSurfaceMaterial;
			const PxReal TireFriction = WheelsStates[w].tireFriction;
			const PxReal LatSlip = WheelsStates[w].lateralSlip;
			const PxReal LongSlip = WheelsStates[w].longitudinalSlip;
			const PxReal WheelRPM = OmegaToRPM(PVehicle->mWheelsDynData.getWheelRotationSpeed(w));

			UPhysicalMaterial *ContactSurfaceMaterial = ContactSurface
															? FPhysxUserData::Get<UPhysicalMaterial>(ContactSurface->userData)
															: NULL;
			const FString ContactSurfaceString = ContactSurfaceMaterial ? ContactSurfaceMaterial->GetName() : FString(TEXT("NONE"));

			Canvas->SetDrawColor(FColor::White);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("[%d]"), w), GetXPos(20.f), YPos);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("RPM: %.1f"), WheelRPM), GetXPos(80.f), YPos);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("Slip Ratio: %.2f"), LongSlip), GetXPos(100.f), YPos);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("Slip Angle (degrees): %.1f"), FMath::RadiansToDegrees(LatSlip)), GetXPos(180.f), YPos);
			Canvas->DrawText(RenderFont, FString::Printf(TEXT("Contact Surface: %s"), *ContactSurfaceString), GetXPos(200.f), YPos);

			YPos += YL;
			XPos = 24.f;
			if ((int32)w < Wheels.Num())
			{
				UVehicleWheel *Wheel = Wheels[w];
				Canvas->DrawText(RenderFont, FString::Printf(TEXT("Normalized Load: %.1f"), Wheel->DebugNormalizedTireLoad), GetXPos(150.f), YPos);
				Canvas->DrawText(RenderFont, FString::Printf(TEXT("Torque (Nm): %.1f"), Cm2ToM2(Wheel->DebugWheelTorque)), GetXPos(150.f), YPos);
				Canvas->DrawText(RenderFont, FString::Printf(TEXT("Long Force: %.1fN (%.1f%%)"), Wheel->DebugLongForce / 100.f, 100.f * Wheel->DebugLongForce / Wheel->DebugTireLoad), GetXPos(200.f), YPos);
				Canvas->DrawText(RenderFont, FString::Printf(TEXT("Lat Force: %.1fN (%.1f%%)"), Wheel->DebugLatForce / 100.f, 100.f * Wheel->DebugLatForce / Wheel->DebugTireLoad), GetXPos(200.f), YPos);
			}
			else
			{
				Canvas->DrawText(RenderFont, TEXT("Wheels array insufficiently sized!"), YL * 50, YPos);
			}

			YPos += YL * 1.2f;
		}
	}

	if (bShowWheelGraph)
	{
		PxVehicleTelemetryData *TelemetryData = MyVehicleManager->GetTelemetryData_AssumesLocked();

		if (TelemetryData)
		{
			const float GraphWidth(100.0f), GraphHeight(100.0f);

			int GraphChannels[] = {PxVehicleWheelGraphChannel::eWHEEL_OMEGA,
								   PxVehicleWheelGraphChannel::eSUSPFORCE,
								   PxVehicleWheelGraphChannel::eTIRE_LONG_SLIP,
								   PxVehicleWheelGraphChannel::eNORM_TIRE_LONG_FORCE,
								   PxVehicleWheelGraphChannel::eTIRE_LAT_SLIP,
								   PxVehicleWheelGraphChannel::eNORM_TIRE_LAT_FORCE,
								   PxVehicleWheelGraphChannel::eNORMALIZED_TIRELOAD,
								   PxVehicleWheelGraphChannel::eTIRE_FRICTION};

			for (uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w)
			{
				float CurX = 4;
				for (uint32 i = 0; i < UE_ARRAY_COUNT(GraphChannels); ++i)
				{
					float OutX = GraphWidth;
					DrawTelemetryGraph(GraphChannels[i], TelemetryData->getWheelGraph(w), Canvas, CurX, YPos, GraphWidth, GraphHeight, OutX);
					CurX += OutX + 10.f;
				}

				YPos += GraphHeight + 10.f;
				YPos += YL;
			}
		}
	}
	

	if (bDrawDebugLines)
	{
		DrawDebugLines();
	}
*/
}

TUniquePtr<Chaos::FSimpleWheeledVehicle> USodaChaosWheeledVehicleMovementComponent::CreatePhysicsVehicle() 
{
	WheeledVehicle = Cast<ASodaWheeledVehicle>(GetOwner());
	VehicleSimulationPT = MakeUnique<USodaChaosWheeledVehicleSimulation>(WheeledVehicle, this);
	return UChaosVehicleMovementComponent::CreatePhysicsVehicle();
}

void USodaChaosWheeledVehicleMovementComponent::ProcessSleeping(const FControlInputs& ControlInputs)
{
	// Do not sleep

	FBodyInstance* TargetInstance = GetBodyInstance();
	if (TargetInstance)
	{
		bool PrevSleeping = VehicleState.bSleeping;
		VehicleState.bSleeping = !TargetInstance->IsInstanceAwake();

		// The physics system has woken vehicle up due to a collision or something
		if (PrevSleeping && !VehicleState.bSleeping)
		{
			VehicleState.SleepCounter = 0;
		}

		//if ((VehicleState.bSleeping && bControlInputPressed) || GVehicleDebugParams.DisableVehicleSleep)
		{
			VehicleState.bSleeping = false;
			VehicleState.SleepCounter = 0;
			SetSleeping(false);
		}

	}
}

bool USodaChaosWheeledVehicleMovementComponent::SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation)
{
	/*
	if (IsValid(GetWheeledVehicle()))
	{
		WheeledVehicle->SetActorLocation(NewLocation, false, nullptr, ETeleportType::ResetPhysics);
		WheeledVehicle->SetActorRotation(NewRotation, ETeleportType::ResetPhysics);
		RecreatePhysicsState();
		return true;
	}
	*/
	return false;
}

const FVehicleSimData& USodaChaosWheeledVehicleMovementComponent::GetSimData() const
{
	if (VehicleSimulationPT)
	{
		return static_cast<USodaChaosWheeledVehicleSimulation*>(VehicleSimulationPT.Get())->VehicleSimData;
	}
	else
	{
		return FVehicleSimData::Zero;
	}
}

void USodaChaosWheeledVehicleMovementComponent::OnPreActivateVehicleComponent()
{
	//Super::OnPreActivateVehicleComponent();

	OnSetActiveMovement();
}

bool USodaChaosWheeledVehicleMovementComponent::OnActivateVehicleComponent()
{
	if (!ISodaVehicleComponent::OnActivateVehicleComponent())
	{
		return false;
	}

	if (PVehicleOutput.IsValid())
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("USodaChaosWheeledVehicleMovementComponent::OnActivateVehicleComponent(); PVehicleOutput already is exist"));
		return false;
	}

	if (!CanCreateVehicle())
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("USodaChaosWheeledVehicleMovementComponent::OnActivateVehicleComponent(); CanCreateVehicle()==false"));
		return false;
	}

	if (GetWheeledVehicle()->GetWheels().Num() != SodaWheelSetups.Num())
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("USodaChaosWheeledVehicleMovementComponent::OnActivateVehicleComponent(); Mismatch count of SodaWheels[%i] and SodaWheelSetups[%i]"), GetWheeledVehicle()->GetWheels().Num(), SodaWheelSetups.Num());
		return false;
	}

	if (GetWheeledVehicle()->GetWheels().Num() == 0)
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("USodaChaosWheeledVehicleMovementComponent::OnActivateVehicleComponent(); wheels num == 0"));
		return false;
	}

	for (auto& It : SodaWheelSetups)
	{

		USodaVehicleWheelComponent* SodaWheel = It.ConnectedSodaWheel.GetObject<USodaVehicleWheelComponent>(GetOwner());
		if (SodaWheel)
		{
			It.SodaWheel = SodaWheel;
		}
		else
		{
			SetHealth(EVehicleComponentHealth::Error);
			UE_LOG(LogSoda, Error, TEXT("USodaChaosWheeledVehicleMovementComponent::OnActivateVehicleComponent(); Can't find USodaVehicleWheelComponent with name \"%s\""), *It.ConnectedSodaWheel.PathToSubobject);
			return false;
		}

		//(*SodaWheel)->Radius = It->WheelRadius;
	}

	if (UpdatedPrimitive)
	{
		UpdatedPrimitive->SetSimulatePhysics(true);
	}

	/***    Activate chaos vehicle   ***/
	bAllowCreatePhysicsState = true;
	CreatePhysicsState(true);


	if (USkeletalMeshComponent* SkeletalMesh = GetSkeletalMesh())
	{
		SkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		SkeletalMesh->InitAnim(true);
	}

	USkinnedMeshComponent* Mesh = Cast< USkinnedMeshComponent >(UpdatedPrimitive);
	if (!Mesh)
	{
		SetHealth(EVehicleComponentHealth::Error);
		return false;
	}

	bSynchronousMode = SodaApp.IsSynchronousMode();

	return true;
}

void USodaChaosWheeledVehicleMovementComponent::OnDeactivateVehicleComponent()
{
	ISodaVehicleComponent::OnDeactivateVehicleComponent();

	DestroyPhysicsState();
	bAllowCreatePhysicsState = false;
}

bool USodaChaosWheeledVehicleMovementComponent::ShouldCreatePhysicsState() const
{
	return bAllowCreatePhysicsState && Super::ShouldCreatePhysicsState();
}

#if WITH_EDITOR

void USodaChaosWheeledVehicleMovementComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void USodaChaosWheeledVehicleMovementComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if ((PropertyChangedEvent.Property != nullptr) && (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UChaosWheeledVehicleMovementComponent, WheelSetups)))
	{
		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
		{
			//const int32 AddedAtIndex = PropertyChangedEvent.GetArrayIndex(PropertyChangedEvent.Property->GetFName().ToString());
			//check(AddedAtIndex != INDEX_NONE);
			SodaWheelSetups.Add({});
		}
		else if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayRemove)
		{
			SodaWheelSetups.RemoveAt(SodaWheelSetups.Num() - 1);
		}

		if (SodaWheelSetups.Num() != WheelSetups.Num())
		{
			SodaWheelSetups.SetNum(WheelSetups.Num());
		}
	}
}

void USodaChaosWheeledVehicleMovementComponent::PostEditUndo()
{
	Super::PostEditUndo();
}

void USodaChaosWheeledVehicleMovementComponent::PostInitProperties()
{
	Super::PostInitProperties();
	if (SodaWheelSetups.Num() != WheelSetups.Num())
	{
		SodaWheelSetups.SetNum(WheelSetups.Num());
	}
}

#endif