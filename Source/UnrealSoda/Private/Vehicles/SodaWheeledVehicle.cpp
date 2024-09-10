// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/UnrealSoda.h"
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/MemoryReader.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DisplayDebugHelpers.h"
#include "SodaJoystick.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleBrakeSystemComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleEngineComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleHandBrakeComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleSteeringComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleDifferentialComponent.h"
#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "VehicleUtility.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/DBGateway.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"

/******************************************************************************************
 * ASodaWheeledVehicle
 * ****************************************************************************************/

ASodaWheeledVehicle::ASodaWheeledVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = Mesh = CreateDefaultSubobject< USkeletalMeshComponent >(TEXT("VehicleMesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->SetSimulatePhysics(false);
	//Mesh->bBlendPhysics = true;
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;


	SpringArm = CreateDefaultSubobject< USpringArmComponent >(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Mesh);
	SpringArm->TargetArmLength = 700;

	FollowCamera = CreateDefaultSubobject< UCameraComponent >(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ASodaWheeledVehicle::SetDefaultMeshProfile()
{
	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->SetUseCCD(true);
	//Mesh->bBlendPhysics = true;
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCollisionResponseToChannels(DefCollisionResponseToChannel);
	Mesh->SetCollisionObjectType(DefCollisionObjectType);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ASodaWheeledVehicle::BeginPlay() 
{
	Super::BeginPlay();

	DefCollisionResponseToChannel = Mesh->GetCollisionResponseToChannels();
	DefCollisionObjectType = Mesh->GetCollisionObjectType();

	class UCustomSpringArmComponent : public USpringArmComponent
	{
		public:
			void ResetLag()
			{
				UpdateDesiredArmLocation(false, false, false, 0.0);
			}
	};

	((UCustomSpringArmComponent*)SpringArm)->ResetLag();

	SavedSpringArmLength = SpringArm->TargetArmLength;

}

bool ASodaWheeledVehicle::DrawDebug(class UCanvas* Canvas, float& YL, float& YPos)
{
	if (!Super::DrawDebug(Canvas, YL, YPos))
	{
		return false;
	}

	UFont* RenderFont = GEngine->GetSmallFont();
	static FColor SubCaption(170, 170, 170);

	TArray<UActorComponent*> VehicleComponents = GetComponentsByInterface(USodaVehicleComponent::StaticClass());
	for (auto& It : VehicleComponents)
	{
		if (ISodaVehicleComponent* Component = Cast<ISodaVehicleComponent>(It))
		{
			Component->DrawDebug(Canvas, YL, YPos);
		}
	}

	return true;
}

const FVehicleSimData& ASodaWheeledVehicle::GetSimData() const
{
	return GetWheeledComponentInterface() ? GetWheeledComponentInterface()->GetSimData() : FVehicleSimData::Zero; 
}

USodaVehicleWheelComponent* ASodaWheeledVehicle::GetWheel4WD(E4WDWheelIndex Ind) const
{ 
	if (Is4WDVehicle() && Ind != E4WDWheelIndex::None)
	{
		return Wheels4WD[int(Ind)];
	}
	else
	{
		return nullptr;
	}
}

void ASodaWheeledVehicle::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PlayerController = Cast< APlayerController >(GetController());

	if (ActiveVehicleInput && ActiveVehicleInput->HealthIsWorkable() && GetWheeledComponentInterface())
	{
		ActiveVehicleInput->UpdateInputStates(DeltaTime, Chaos::CmSToKmH(GetWheeledComponentInterface()->GetSimData().VehicleKinematic.GetForwardSpeed()), PlayerController);
		/*
		UE_LOG(LogSoda, Log, TEXT("%sms; Steering: %.1f; Throttle: %.1f, Brake: %.1f"), 
			*soda::ToString<std::chrono::milliseconds>(std::chrono::system_clock::now()),
			ActiveVehicleInput->GetSteeringInput() * 100,
			ActiveVehicleInput->GetThrottleInput() * 100,
			ActiveVehicleInput->GetBrakeInput() * 100
		);
		*/
	}

	if (PlayerController)
	{
		USodaUserSettings* Settings = SodaApp.GetSodaUserSettings();

		bool bIsLeftCtrlDown = PlayerController->IsInputKeyDown(EKeys::LeftControl);

		if (bIsLeftCtrlDown && PlayerController->WasInputKeyJustPressed(Settings->ZoomUpKeyInput))
		{
			++Zoom;
		}

		if (bIsLeftCtrlDown && PlayerController->WasInputKeyJustPressed(Settings->ZoomDownKeyInput))
		{
			if (Zoom > 1) --Zoom;
		}

		const float MouseSpeed = 50;
		const float KeySpeed = 50;

		float X, Y;
		PlayerController->GetInputMouseDelta(X, Y);
		float Yaw = SpringArm->GetRelativeRotation().Yaw + X * DeltaTime * MouseSpeed;
		float Pitch = FMath::Clamp(SpringArm->GetRelativeRotation().Pitch + Y * DeltaTime * MouseSpeed, -60.f, 10.f);

		if (PlayerController->IsInputKeyDown(EKeys::Up))
		{
			Pitch += DeltaTime * KeySpeed;
		}

		if (PlayerController->IsInputKeyDown(EKeys::Down))
		{
			Pitch -= DeltaTime * KeySpeed;
		}

		if (PlayerController->IsInputKeyDown(EKeys::Left))
		{
			Yaw -= DeltaTime * KeySpeed;
		}

		if (PlayerController->IsInputKeyDown(EKeys::Right))
		{
			Yaw += DeltaTime * KeySpeed;
		}

		SpringArm->SetRelativeRotation(FRotator(Pitch, Yaw, 0));
		SpringArm->TargetArmLength = SavedSpringArmLength + Zoom * Zoom * 100;
	}
}

UPawnMovementComponent* ASodaWheeledVehicle::GetMovementComponent() const
{
	return Cast<UPawnMovementComponent>(WheeledComponentInterface);
}

float ASodaWheeledVehicle::GetForwardSpeed() const
{
	if (HasActorBegunPlay() && GetWheeledComponentInterface())
	{
		return GetWheeledComponentInterface()->GetSimData().VehicleKinematic.GetForwardSpeed();
	}
	else
	{
		return 0;
	}
}

void ASodaWheeledVehicle::ReRegistreVehicleComponents()
{
	Super::ReRegistreVehicleComponents();

	GetComponents<USodaVehicleWheelComponent>(Wheels);
	GetComponents<UVehicleInputComponent>(VehicleInputs);
	GetComponents<UVehicleEngineBaseComponent>(VehicleEngines);
	GetComponents<UVehicleDriverComponent>(VehicleDrivers);
	GetComponents<UVehicleGearBoxBaseComponent>(GearBoxes);
	GetComponents<UVehicleSteeringRackBaseComponent>(VehicleSteeringRacks);
	GetComponents<UVehicleHandBrakeBaseComponent>(VehicleHandBrakes);
	GetComponents<UVehicleDifferentialBaseComponent>(VehicleDifferentials);

	Wheels4WD.Empty();
	if (Wheels.Num() == 4)
	{
		auto Add4WDWheel = [this](E4WDWheelIndex Index)
		{
			if (USodaVehicleWheelComponent** Wheel = Wheels.FindByPredicate([this, Index](const USodaVehicleWheelComponent* Wheel) { return Wheel->WheelIndex4WD == Index; }))
			{
				Wheels4WD.Add(*Wheel);
			}
		};

		Add4WDWheel(E4WDWheelIndex::FL);
		Add4WDWheel(E4WDWheelIndex::FR);
		Add4WDWheel(E4WDWheelIndex::RL);
		Add4WDWheel(E4WDWheelIndex::RR);
		if (Wheels4WD.Num() != 4)
		{
			Wheels4WD.Empty();
		}
	}

	if (Mesh)
	{
		for (auto& It : Wheels)
		{
			int Ind = Mesh->GetBoneIndex(It->BoneName);
			if (Ind != INDEX_NONE)
			{
				It->RestingLocation = Mesh->GetBoneLocation(It->BoneName, EBoneSpaces::ComponentSpace);
				It->RestingRotation = Mesh->GetBoneQuaternion(It->BoneName, EBoneSpaces::ComponentSpace).Rotator();
			}
			else
			{
				UE_LOG(LogSoda, Error, TEXT("ASodaWheeledVehicle::ReRegistreVehicleComponents(); Can't find vehicle bone name: %s"), *It->BoneName.ToString());
			}
		}
	}

	SetDefaultMeshProfile();

	WheeledComponentInterface = nullptr;
}

void ASodaWheeledVehicle::SetActiveVehicleInput(UVehicleInputComponent* InVehicleInput)
{
	if (InVehicleInput)
	{
		InVehicleInput->ActivateVehicleComponent();
	}
}

UVehicleInputComponent* ASodaWheeledVehicle::SetActiveVehicleInputByName(FName VehicleInputName)
{
	for (auto& Input : VehicleInputs)
	{
		if (Input->GetFName() == VehicleInputName)
		{
			SetActiveVehicleInput(Input);
			return Input;
		}
	}

	return nullptr;
}

UVehicleInputComponent* ASodaWheeledVehicle::SetActiveVehicleInputByType(EVehicleInputType Type)
{
	for (auto& Input : VehicleInputs)
	{
		if (Input->InputType == Type)
		{
			SetActiveVehicleInput(Input);
			return Input;
		}
	}

	return nullptr;
}

float ASodaWheeledVehicle::GetEnginesLoad() const
{
	float Load = 0;
	for (size_t i = 0; i < VehicleEngines.Num(); ++i)
	{
		Load += VehicleEngines[i]->GetEngineLoad();
	}
	return Load / VehicleEngines.Num();
}

float ASodaWheeledVehicle::GetReqSteer() const
{
	if (Is4WDVehicle())
	{
		return (GetWheel4WD(E4WDWheelIndex::FL)->ReqSteer + GetWheel4WD(E4WDWheelIndex::FR)->ReqSteer) / 2.0;
	}
	else
	{
		return 0; // TODO
	}
}

float ASodaWheeledVehicle::GetSteer() const
{
	if (Is4WDVehicle())
	{
		return (GetWheel4WD(E4WDWheelIndex::FL)->Steer + GetWheel4WD(E4WDWheelIndex::FR)->Steer) / 2.0;
	}
	else
	{
		return 0; // TODO
	}
}

FWheeledVehicleState ASodaWheeledVehicle::GetVehicleState() const
{
	FWheeledVehicleState Ret;

	if (GetWheeledComponentInterface())
	{
		const FPhysBodyKinematic& VehicleKinematic = GetSimData().VehicleKinematic;

		Ret.Position = VehicleKinematic.Curr.GlobalPose.GetTranslation();
		Ret.Rotation = VehicleKinematic.Curr.GlobalPose.Rotator();
		Ret.Velocity = VehicleKinematic.Curr.GetLocalVelocity();
		Ret.AngularVelocity = VehicleKinematic.Curr.AngularVelocity;
		Ret.Acc = VehicleKinematic.GetLocalAcc();

		if (Is4WDVehicle())
		{
			auto WheelDataConv = [](const USodaVehicleWheelComponent& In, FWheeledVehicleWheelState& Out)
			{
				Out.AngularVelocity = In.AngularVelocity;
				Out.Torq = In.ReqTorq;
				Out.BrakeTorq = In.ReqBrakeTorque;
				Out.Steer = In.Steer;
			};

			WheelDataConv(*GetWheel4WD(E4WDWheelIndex::FL), Ret.WheelsFL);
			WheelDataConv(*GetWheel4WD(E4WDWheelIndex::FR), Ret.WheelsFR);
			WheelDataConv(*GetWheel4WD(E4WDWheelIndex::RL), Ret.WheelsRL);
			WheelDataConv(*GetWheel4WD(E4WDWheelIndex::RR), Ret.WheelsRR);
		}
	}



	return Ret;
}

void ASodaWheeledVehicle::ScenarioBegin()
{
	Super::ScenarioBegin();
}

void ASodaWheeledVehicle::ScenarioEnd() 
{
	Super::ScenarioEnd();
}

const FSodaActorDescriptor* ASodaWheeledVehicle::GenerateActorDescriptor() const
{
	return nullptr;
}

void ASodaWheeledVehicle::OnPushDataset(soda::FActorDatasetData& InDataset) const
{
	Super::OnPushDataset(InDataset);

	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	auto Doc = InDataset.GetRowDoc() << "WheeledVehicleData" << open_document;

	//Doc << "EngineLoad" << GetEnginesLoad();

	if (UVehicleInputComponent* Input = GetActiveVehicleInput())
	{
		Doc << "Inputs" << open_document
			<< "Break" << Input->GetInputState().Brake
			<< "Steer" << Input->GetInputState().Steering
			<< "Throttle" << Input->GetInputState().Throttle
			<< "Gear" << int(Input->GetInputState().GearState)
		<< close_document;
	}

	if (GetWheeledComponentInterface())
	{
		bsoncxx::builder::stream::array WheelsArray;
		for (int i = 0; i < Wheels.Num(); ++i)
		{
			const auto& Wheel = Wheels[i];
			WheelsArray << open_document
				<< "4wdInd" << int(Wheel->WheelIndex4WD)
				<< "AngVel" << Wheel->ResolveAngularVelocity()
				<< "ReqTorq" << Wheel->ReqTorq
				<< "ReqBrakeTorq" << Wheel->ReqBrakeTorque
				<< "Steer" << Wheel->Steer
				<< "Sus" << Wheel->SuspensionOffset
				<< "LongSlip" << Wheel->Slip.X
				<< "LatSlip" << Wheel->Slip.Y
			<< close_document;
		}

		Doc << "Wheels" << WheelsArray;
	}

	Doc << close_document;
}

void ASodaWheeledVehicle::GenerateDatasetDescription(soda::FBsonDocument& Doc) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	if (GetWheeledComponentInterface())
	{
		(*Doc)
			<< "TrackWidth" << GetWheeledComponentInterface()->GetTrackWidth()
			<< "WheelBaseWidth" << GetWheeledComponentInterface()->GetWheelBaseWidth()
			<< "Mass" << GetWheeledComponentInterface()->GetVehicleMass();

		bsoncxx::builder::stream::array WheelsArray;
		for (int i = 0; i < Wheels.Num(); ++i)
		{
			const auto& Wheel = Wheels[i];
			WheelsArray << open_document
				<< "4wdInd" << int(Wheel->WheelIndex4WD)
				<< "Radius" << Wheel->Radius
				<< "Location" << open_array << Wheel->RestingLocation.X << Wheel->RestingLocation.Y << Wheel->RestingLocation.Z << close_array
			<< close_document;
		}
		(*Doc) << "Wheels" << WheelsArray;
	}
}

FVector ASodaWheeledVehicle::GetVelocity() const
{
	if (WheeledComponentInterface)
	{
		return WheeledComponentInterface->GetSimData().VehicleKinematic.Curr.GlobalVelocityOfCenterMass;
	}
	else
	{
		return Super::GetVelocity();
	}
}