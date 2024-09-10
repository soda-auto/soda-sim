// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/UI/SMessageBox.h"
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/MemoryReader.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DisplayDebugHelpers.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaActorFactory.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "Soda/LevelState.h"
#include "DesktopPlatformModule.h"
#include "CanvasItem.h"
#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "RuntimeEditorModule.h"
#include "RuntimeMetaData.h"
#include "Soda/VehicleComponents/CANBus.h"
#include "Soda/DBGateway.h"
#include "Misc/Paths.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Images/SImage.h"
#include "Styling/StyleColors.h"
#include "UI/Wnds/SVehcileManagerWindow.h"
#include "UI/Wnds/SVehcileManagerWindow.h"
#include "UI/Wnds/SSaveVehicleRequestWindow.h"
#include "UI/Wnds/SExportVehicleWindow.h"
#include "UI/ToolBoxes/SVehicleComponentsToolBox.h"
#include "SodaStyleSet.h"
#include "GlobalRenderResources.h"
#include "GameFramework/PlayerController.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

#define PARCE_VEC(V, Mul) V.X * Mul, V.Y * Mul, V.Z * Mul

FVehicleSimData FVehicleSimData::Zero{};

FString FVechicleSaveAddress::ToVehicleName() const
{
	switch (Source)
	{
	case EVehicleSaveSource::NoSave:
		return TEXT("(New Vehicle)");

	case EVehicleSaveSource::BinExternal:
		return FPaths::GetBaseFilename(Location);

	case EVehicleSaveSource::BinLocal:
		return Location;

	case EVehicleSaveSource::Slot:
		return Location;

	case EVehicleSaveSource::BinLevel:
		return TEXT("(Level Vehicle)");

	case EVehicleSaveSource::JsonLocal:
		return Location;

	case EVehicleSaveSource::JsonExternal:
		return FPaths::GetBaseFilename(Location);

	case EVehicleSaveSource::DB:
		return Location;

	default:
		return TEXT("wtf?");
	}
}

FString FVechicleSaveAddress::ToUrl() const
{
	switch (Source)
	{
	case EVehicleSaveSource::NoSave:
		return TEXT("");

	case EVehicleSaveSource::BinExternal:
		return FString(TEXT("binext://")) + Location;

	case EVehicleSaveSource::BinLocal:
		return FString(TEXT("binloc://")) + Location;

	case EVehicleSaveSource::Slot:
		return FString(TEXT("slot://")) + Location;

	case EVehicleSaveSource::BinLevel:
		return FString(TEXT("level://")) + Location;

	case EVehicleSaveSource::JsonLocal:
		return FString(TEXT("jsonloc://")) + Location;

	case EVehicleSaveSource::JsonExternal:
		return FString(TEXT("jsonext://")) + Location;

	case EVehicleSaveSource::DB:
		return FString(TEXT("mongodb://")) + Location;

	default:
		return TEXT("");
	}
}

bool FVechicleSaveAddress::SetFromUrl(const FString& Url)
{
	FString SourceStr, TmpLocation;
	if (!Url.Split(TEXT("://"), &SourceStr, &TmpLocation))
	{
		return false;
	}

	if (SourceStr == TEXT("binext"))
	{
		Source = EVehicleSaveSource::BinExternal;
		Location = TmpLocation;
		return true;
	}
	else if (SourceStr == TEXT("binloc"))
	{
		Source = EVehicleSaveSource::BinLocal;
		Location = TmpLocation;
		return true;
	}
	else if (SourceStr == TEXT("slot"))
	{
		Source = EVehicleSaveSource::Slot;
		Location = TmpLocation;
		return true;
	}
	else if (SourceStr == TEXT("level"))
	{
		Source = EVehicleSaveSource::BinLevel;
		Location = TmpLocation;
		return true;
	}
	else if (SourceStr == TEXT("jsonloc"))
	{
		Source = EVehicleSaveSource::JsonLocal;
		Location = TmpLocation;
		return true;
	}
	else if (SourceStr == TEXT("jsonext"))
	{
		Source = EVehicleSaveSource::JsonExternal;
		Location = TmpLocation;
		return true;
	}
	else if (SourceStr == TEXT("mongodb"))
	{
		Source = EVehicleSaveSource::DB;
		Location = TmpLocation;
		return true;
	}
	else
	{
		return false;
	}
}

ASodaVehicle::ASodaVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASodaVehicle::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	TArray<UActorComponent*> ActorComponents;
	GetComponents<UActorComponent>(ActorComponents);

	/* Deserialize components from Bin */
	if (ComponentRecords.Num())
	{
		for (auto* Component : ActorComponents)
		{
			if (FComponentRecord* Record = ComponentRecords.FindByKey(Component))
			{
				Record->DeserializeComponent(Component);
			}
			else
			{
				if (ISodaVehicleComponent* SodaComponent = Cast<ISodaVehicleComponent>(Component))
				{
					//Component->DestroyComponent();
					SodaComponent->GetVehicleComponentGUI().bIsDeleted = true;
				}
			}
		}

		for (auto& ComponentRecord : ComponentRecords)
		{
			if (ComponentRecord.Class->ImplementsInterface(USodaVehicleComponent::StaticClass()))
			{
				if (!FindVehicleComponentByName(ComponentRecord.Name.ToString()))
				{
					UClass* Class = ComponentRecord.Class.Get();
					if (!Class)
					{
						Class = LoadObject<UClass>(nullptr, *ComponentRecord.Class.ToString());
					}
					if (Class)
					{
						UActorComponent* Component = AddVehicleComponent(Class, ComponentRecord.Name);
						check(Component);
						ComponentRecord.DeserializeComponent(Component);
					}
				}
			}
		}
	}

	/* Deserialize components from Json */
	if (JsonAr)
	{
		for (auto* Component : ActorComponents)
		{
			if (!JsonAr->DeserializeComponent(Component))
			{
				if (ISodaVehicleComponent * SodaComponent =  Cast<ISodaVehicleComponent>(Component))
				{
					//Component->DestroyComponent();
					SodaComponent->GetVehicleComponentGUI().bIsDeleted = true;
				}
			}
		}

		for (auto& It : JsonAr->Components->Values)
		{
			TSharedPtr<FJsonObject> ComponentObject = It.Value->AsObject();
			if (!ComponentObject)
			{
				continue;
			}
			UClass* Class = JsonAr->GetObjectClass(ComponentObject);
			
			if (Class)
			{
				if (!FindVehicleComponentByName(It.Key) && Class->ImplementsInterface(USodaVehicleComponent::StaticClass()))
				{
					UActorComponent* Component = AddVehicleComponent(Class, FName(*It.Key));
					check(Component);
					JsonAr->JsonObjectToUObject(ComponentObject.ToSharedRef(), Component, true);
				}
			}
			else
			{
				UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::PreInitializeComponents(); Can't find component class \"%s\""), *ComponentObject->GetStringField(TEXT("__class__")));
			}
		}
	}

	for (auto* Component : ActorComponents)
	{
		if (ISodaVehicleComponent* SodaComponent = Cast<ISodaVehicleComponent>(Component))
		{
			if (SodaComponent->GetVehicleComponentGUI().bIsDeleted)
			{
				SodaComponent->GetVehicleComponentGUI().Category = "DELETED";
				SodaComponent->GetVehicleComponentCommon().Activation = EVehicleComponentActivation::None;
			}
		}
	}
}

void ASodaVehicle::BeginPlay()
{
	Super::BeginPlay();

	check(USodaSubsystem::Get());

	VehicelExtent = USodaStatics::CalculateActorExtent(this);
	UpdateProperties();

	PhysicMutex.Lock();

	ReRegistreVehicleComponents();
	
	for (auto& Component : GetVehicleComponents())
	{
		if ((Component->GetVehicleComponentCommon().Activation == EVehicleComponentActivation::OnBeginPlay) && !Component->IsVehicleComponentActiveted())
		{
			Component->OnPreActivateVehicleComponent();
		}
	}

	for (auto& Component : GetVehicleComponents())
	{
		if ((Component->GetVehicleComponentCommon().Activation == EVehicleComponentActivation::OnBeginPlay) && !Component->IsVehicleComponentActiveted())
		{
			Component->OnActivateVehicleComponent();
		}
	}

	for (auto& Component : GetVehicleComponents())
	{
		if (Component->IsVehicleComponentActiveted())
		{
			Component->OnPostActivateVehicleComponent();
		}
	}

	PhysicMutex.Unlock();
}

void ASodaVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason) 
{
	for (auto& Component : GetVehicleComponents())
	{
		if (Component->IsVehicleComponentActiveted())
		{
			Component->OnPreDeactivateVehicleComponent();
		}
	}

	TArray<ISodaVehicleComponent*> ComponentsToPostDeactivateVehicle;
	for (auto& Component : GetVehicleComponents())
	{
		if (Component->IsVehicleComponentActiveted())
		{
			Component->OnDeactivateVehicleComponent();
			ComponentsToPostDeactivateVehicle.Add(Component);
		}
	}

	for (auto& Component : ComponentsToPostDeactivateVehicle)
	{
		if (Component->IsVehicleComponentActiveted())
		{
			Component->OnPostDeactivateVehicleComponent();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ASodaVehicle::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (IsValid(VehicleWidget1) && Cast<APlayerController>(NewController))
	{
		VehicleWidget1->AddToViewport(0);
	}

	if (IsValid(VehicleWidget2) && Cast<APlayerController>(NewController))
	{
		VehicleWidget2->AddToViewport(0);
	}
}

void ASodaVehicle::UnPossessed()
{
	Super::UnPossessed();

	if (IsValid(VehicleWidget1))
	{
		VehicleWidget1->RemoveFromParent();
	}

	if (IsValid(VehicleWidget2))
	{
		VehicleWidget2->RemoveFromParent();
	}

	USodaSubsystem::GetChecked()->UnpossesVehicle = this;
}

void ASodaVehicle::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	ComponentRecords.Empty();
	const TSet<UActorComponent*>& Components = GetComponents();

	if (Ar.IsSaveGame())
	{
		if (Ar.IsSaving())
		{
			for (auto* Component : Components)
			{
				FComponentRecord& ComponentRecord = ComponentRecords.Add_GetRef(FComponentRecord());
				ComponentRecord.SerializeComponent(Component);
			}
			Ar << ComponentRecords;
		}
		else if (Ar.IsLoading())
		{
			Ar << ComponentRecords;
		}
	}
}

UActorComponent* ASodaVehicle::FindVehicleComponentByName(const FString & ComponentName) const
{
	TArray<UActorComponent*> Components = GetComponentsByInterface(USodaVehicleComponent::StaticClass());
	for(auto & Component: Components)
	{
		if(Component->GetName() == ComponentName)
		{
			return Component;
		}
	}
	return nullptr;
}

UActorComponent* ASodaVehicle::FindComponentByName(const FString& ComponentName) const
{
	const TSet<UActorComponent*>& Components = GetComponents();

	for (auto& Component : Components)
	{
		if (Component->GetName() == ComponentName)
		{
			return Component;
		}
	}

	return nullptr;
}

UActorComponent* ASodaVehicle::AddVehicleComponent(TSubclassOf<UActorComponent> Class, FName Name)
{
	FScopeLock ScopeLock(&PhysicMutex);

	if (Class.Get() == nullptr)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::AddVehicleComponent(); Null class cannot be created "));
		return nullptr;
	}

	if (!Class->ImplementsInterface(USodaVehicleComponent::StaticClass()))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::AddVehicleComponent(); The class isn't the ISodaVehicleComponent"));
		return nullptr;
	}

	/* Check component name*/
	FString ComponentName;
	if (Name.IsNone())
	{
		Name = USodaStatics::GetSensorShortName(Class->GetFName());
	}
	ComponentName = Name.ToString();
	
	TArray<UActorComponent*> ActorComponents;
	GetComponents<UActorComponent>(ActorComponents);
	int i = 0;
	bool isOk = false;
	while(!isOk)
	{
		isOk = true;
		for (auto& Component : ActorComponents)
		{
			if (Component->GetName() == ComponentName)
			{
				ComponentName = Name.ToString() + FString::FromInt(++i);
				isOk = false;
				break;
			}
		}
	}

	UActorComponent* NewComponent = NewObject<UActorComponent>(this, Class, FName(*ComponentName));
	if (!NewComponent)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::AddVehicleComponent(); Can't create non-default component; Name: '%s', Class: '%s'"), *ComponentName, *Class->GetFName().ToString());
		return nullptr;
	}

	if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(NewComponent))
	{
		int MaxOrder = -1;
		for (auto& Component : GetVehicleComponents())
		{
			MaxOrder = std::max(MaxOrder, Component->GetVehicleComponentGUI().Order);
		}
		VehicleComponent->GetVehicleComponentGUI().Order = MaxOrder + 1;
		if (HasActorBegunPlay())
		{
			VehicleComponent->GetVehicleComponentCommon().Activation = EVehicleComponentActivation::None;
		}
	}

	if (USceneComponent* SceneComponent = Cast<USceneComponent>(NewComponent))
	{
		SceneComponent->SetupAttachment(RootComponent);
	}

	NewComponent->RegisterComponent();

	UE_LOG(LogSoda, Log, TEXT("Create non-default component; Name: '%s', Class: '%s'"), *ComponentName, *Class->GetFName().ToString());

	if (HasActorBegunPlay())
	{
		ReActivateVehicleComponents(true);
	}

	return NewComponent;
}

bool ASodaVehicle::RemoveVehicleComponentByName(FName Name)
{
	FScopeLock ScopeLock(&PhysicMutex);

	ISodaVehicleComponent* Component = Cast<ISodaVehicleComponent>(FindVehicleComponentByName(Name.ToString()));

	if (Component == nullptr)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::RemoveVehicleComponentByName(); Can't find component '%s'"), *Name.ToString());
		return false;
	}

	Component->RemoveVehicleComponent();

	return true;
}

TArray<ISodaVehicleComponent*> ASodaVehicle::GetVehicleComponentsSorted(const FString& GUICategory, bool bNormalizeIfNecessary )
{
	TArray<ISodaVehicleComponent*> Components = GetVehicleComponents();

	if (bNormalizeIfNecessary)
	{
		int Ind = 0;
		for (auto& Component : GetVehicleComponentsSorted("", false))
		{
			Component->GetVehicleComponentGUI().Order = Ind++;
		}
	}

	if (GUICategory.Len() > 0)
	{
		Components.RemoveAll([GUICategory](const auto& El) { return El->GetVehicleComponentGUI().Category != GUICategory; });
		Components.Sort([](const auto& A, const auto& B) { return A.GetVehicleComponentGUI().Order < B.GetVehicleComponentGUI().Order; });
	}
	else
	{
		Components.Sort([](const auto& A, const auto& B)
		{
			int Ret = A.GetVehicleComponentGUI().Category.Compare(B.GetVehicleComponentGUI().Category, ESearchCase::IgnoreCase);
			if (Ret == 0)
			{
				return A.GetVehicleComponentGUI().Order < B.GetVehicleComponentGUI().Order;
			}
			else if (Ret < 0)
			{
				return true;
			}
			else // Ret > 0
			{
				return false;
			}
		});
	}

	return Components;
}

ASodaVehicle * ASodaVehicle::RespawnVehcile(FVector Location, FRotator Rotation, bool IsOffset, const UClass* NewVehicleClass)
{
	UWorld * World = GetWorld();
	check(World);

	FTransform SpawnTransform = IsOffset ? 
		FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f) + Rotation, GetActorLocation() + Location, FVector(1.0f, 1.0f, 1.0f)) :
		FTransform(Rotation, Location, FVector(1.0f, 1.0f, 1.0f));

	const UClass* Class = (NewVehicleClass != nullptr) ? NewVehicleClass : GetClass();

	ASodaVehicle * NewVehicle = World->SpawnActorDeferred<ASodaVehicle>(
		const_cast<UClass*>(Class),
		SpawnTransform, 
		nullptr, nullptr, 
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if(NewVehicle == nullptr)
	{
		return nullptr;
	}

	if (NewVehicleClass == nullptr)
	{
		FActorRecord SavedActorRecord;
		SavedActorRecord.SerializeActor(this, false);
		SavedActorRecord.DeserializeActor(NewVehicle, false);
		NewVehicle->bIsDirty = IsDirty();
	}

	if (NewVehicleClass == nullptr || GetClass() == NewVehicleClass)
	{
		NewVehicle->SaveAddress = SaveAddress;
	}

	if (GetClass() == NewVehicleClass)
	{
		NewVehicle->MarkAsDirty();
	}

	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	SodaSubsystem->NotifyLevelIsChanged();

	if (SodaSubsystem->LevelState && SodaSubsystem->LevelState->ActorFactory)
	{
		SodaSubsystem->LevelState->ActorFactory->ReplaceActor(NewVehicle, this, false);
	}
	if(SodaSubsystem->UnpossesVehicle == this) SodaSubsystem->UnpossesVehicle = NewVehicle;
	

	bool NeedPosses = IsPlayerControlled();

	if (IsValid(VehicleWidget1))
		VehicleWidget1->Vehcile = nullptr;

	if (IsValid(VehicleWidget2))
		VehicleWidget2->Vehcile = nullptr;
	Destroy();

	NewVehicle->FinishSpawning(SpawnTransform);

	APlayerController *PlayerController = World->GetFirstPlayerController();
	if ( PlayerController != nullptr && NeedPosses )
	{
		PlayerController->Possess(NewVehicle);
	}

	return NewVehicle;
}

ASodaVehicle* ASodaVehicle::RespawnVehcileFromAddress(const FVechicleSaveAddress& Address, FVector Location, FRotator Rotation, bool IsOffset)
{
	UWorld* World = GetWorld();
	check(World);

	FTransform SpawnTransform = IsOffset ?
		FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f) + Rotation, GetActorLocation() + Location, FVector(1.0f, 1.0f, 1.0f)) :
		FTransform(Rotation, Location, FVector(1.0f, 1.0f, 1.0f));

	bool NeedPosses = IsPlayerControlled();

	Destroy();

	ASodaVehicle* NewVehicle = ASodaVehicle::SpawnVehicleFormAddress(World, Address, SpawnTransform.GetTranslation(), SpawnTransform.GetRotation().Rotator(), NeedPosses);

	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	SodaSubsystem->NotifyLevelIsChanged();
	if (SodaSubsystem->LevelState && SodaSubsystem->LevelState->ActorFactory)
	{
		SodaSubsystem->LevelState->ActorFactory->ReplaceActor(NewVehicle, this, false);
	}
	if (SodaSubsystem->UnpossesVehicle == this) SodaSubsystem->UnpossesVehicle = NewVehicle;
	
	return NewVehicle;
}


bool ASodaVehicle::DrawDebug(class UCanvas* Canvas, float& YL, float& YPos)
{
	YPos += 30;

	FCanvasTileItem TileItem(FVector2D(0, 0), GWhiteTexture, FVector2D(SodaApp.GetSodaUserSettings()->VehicleDebugAreaWidth, Canvas->SizeY), FLinearColor(0.0f, 0.0f, 0.0f, 0.3f));
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);

	UFont* RenderFont = GEngine->GetSmallFont();
	static FColor SubCaption(170, 170, 170);

	Canvas->SetDrawColor(FColor(51, 102, 204));
	YPos += Canvas->DrawText(UEngine::GetMediumFont(), TEXT("Vehicle: ") + SaveAddress.ToVehicleName() + (IsDirty() ? TEXT("*") : TEXT("")), 4, YPos + 4);
	YPos += Canvas->DrawText(UEngine::GetMediumFont(), TEXT("Source: ") + UEnum::GetValueAsString(SaveAddress.Source), 4, YPos + 4);

	if (bShowPhysBodyKinematic)
	{
		Canvas->SetDrawColor(SubCaption);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("PhysBodyKinematic:")), 16, YPos);
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Acc: [%.1f, %.1f, %.1f] m/s^2"),          PARCE_VEC(PhysBodyKinematicCashed.Curr.GetLocalAcceleration(), 0.01)), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ang Velocity: [%.1f, %.1f, %.1f] rad/s"), PARCE_VEC(PhysBodyKinematicCashed.Curr.AngularVelocity, 1)), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Vel : [%.1f, %.1f, %.1f] m/s"),           PARCE_VEC(PhysBodyKinematicCashed.Curr.GetLocalVelocity(), 0.01)), 16, YPos);
	}

	return true;
}

void ASodaVehicle::ReRegistreVehicleComponents()
{
	PreTickedVehicleComponens.Empty();
	PostTickedVehicleComponens.Empty();
	PostTickedDeferredVehicleComponens.Empty();

	for (auto& It : GetVehicleComponents())
	{
		if (IVehicleTickablObject* Component = Cast<IVehicleTickablObject>(It))
		{
			if (Component->GetVehicleComponentTick().bAllowVehiclePrePhysTick)
			{
				PreTickedVehicleComponens.Add(Component);
			}
			if (Component->GetVehicleComponentTick().bAllowVehiclePostPhysTick)
			{
				PostTickedVehicleComponens.Add(Component);
			}
			if (Component->GetVehicleComponentTick().bAllowVehiclePostDeferredPhysTick)
			{
				PostTickedDeferredVehicleComponens.Add(Component);
			}
		}

		It->OnRegistreVehicleComponent();
	}

	PreTickedVehicleComponens.Sort([](const auto& LHS, const auto& RHS) { return LHS.GetVehicleComponentTick().PrePhysTickGroup > RHS.GetVehicleComponentTick().PrePhysTickGroup; });
	PostTickedVehicleComponens.Sort([](const auto& LHS, const auto& RHS) { return LHS.GetVehicleComponentTick().PostPhysTickGroup > RHS.GetVehicleComponentTick().PostPhysTickGroup; });
	PostTickedDeferredVehicleComponens.Sort([](const auto& LHS, const auto& RHS) {return LHS.GetVehicleComponentTick().PostPhysDeferredTickGroup > RHS.GetVehicleComponentTick().PostPhysDeferredTickGroup; });

	GetComponents<UCANBusComponent>(CANDevs);
}

void ASodaVehicle::ReActivateVehicleComponents(bool bOnlyTopologyComponents)
{
	if (!HasActorBegunPlay())
	{
		checkf(true, TEXT("ReActivateVehicleComponents() can be called only after BeginPlay()"));
		return;
	}

	FScopeLock ScopeLock(&PhysicMutex);

	TArray<ISodaVehicleComponent*> Components = GetVehicleComponents();

	TArray<TTuple< ISodaVehicleComponent*, bool>> ComponentsActive;
	ComponentsActive.Reserve(Components.Num());
	for (auto& It : Components)
	{
		if (!bOnlyTopologyComponents || It->GetVehicleComponentCommon().bIsTopologyComponent)
		{
			const bool bNeedActivate =
				(It->IsVehicleComponentActiveted() && It->GetDeferredTask() != EVehicleComponentDeferredTask::Deactivate) ||
				It->GetDeferredTask() == EVehicleComponentDeferredTask::Activate;
			ComponentsActive.Add(MakeTuple(It, bNeedActivate));
			if (It->IsVehicleComponentActiveted())
			{
				It->OnPreDeactivateVehicleComponent();
				It->OnDeactivateVehicleComponent();
				It->OnPostDeactivateVehicleComponent();
			}
		}
	}

	ReRegistreVehicleComponents();
	
	for (auto& It : ComponentsActive)
	{
		if (It.Get<1>()) It.Get<0>()->OnPreActivateVehicleComponent();
	}

	for (auto& It : ComponentsActive)
	{
		if (It.Get<1>()) It.Get<0>()->OnActivateVehicleComponent();
	}

	for (auto& It : ComponentsActive)
	{
		if (It.Get<1>()) It.Get<0>()->OnPostActivateVehicleComponent();
	}
}


void ASodaVehicle::UpdateProperties()
{
	if (VehicleWidget1)
	{
		VehicleWidget1->RemoveFromParent();
		VehicleWidget1 = nullptr;
	}

	if (VehicleWidget2)
	{
		VehicleWidget2->RemoveFromParent();
		VehicleWidget2 = nullptr;
	}

	const bool bIsControledByPlayer = !!Cast<APlayerController>(GetController());

	if (bShowVehicleWidget1 && VehicleWidgetClass1)
	{
		VehicleWidget1 = NewObject<UVehicleWidget>(this, VehicleWidgetClass1);
		VehicleWidget1->Vehcile = this;
		if(bIsControledByPlayer) VehicleWidget1->AddToViewport(0);
	}

	if (bShowVehicleWidget2 && VehicleWidgetClass2)
	{
		VehicleWidget2 = NewObject<UVehicleWidget>(this, VehicleWidgetClass2);
		VehicleWidget2->Vehcile = this;
		if(bIsControledByPlayer) VehicleWidget1->AddToViewport(0);
	}

	USodaStatics::TagActor(this, true, EActorTagMethod::ForceCustomLabel, SegmObjectLabel);
}

void ASodaVehicle::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp)
{
	for (auto& Component : PreTickedVehicleComponens)
	{
		if (Component->IsTickAllowed())
		{
			Component->PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);
		}
	}
}

void ASodaVehicle::PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	for (auto& Component : PostTickedVehicleComponens)
	{
		if (Component->IsTickAllowed())
		{
			Component->PostPhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);
		}
	}
	PhysBodyKinematicCashed = VehicleKinematic;

	if (Dataset)
	{
		Dataset->BeginRow();
		for (auto& Component : VehicleComponensForDataset)
		{
			Component->OnPushDataset(*Dataset);
		}
		OnPushDataset(*Dataset);
		Dataset->EndRow();
	}
}

void ASodaVehicle::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	for (auto& Component : PostTickedDeferredVehicleComponens)
	{
		if (Component->IsTickAllowed())
		{
			Component->PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);
		}
	}
}

bool ASodaVehicle::SaveToJson(const FString& FileName, bool bRebase)
{
	FJsonActorArchive Ar;
	if (Ar.SerializeActor(this, true))
	{
		if (Ar.SaveToFile(FileName))
		{
			if (bRebase)
			{
				if (FPaths::IsSamePath(FPaths::GetPath(FileName), GetDefaultVehiclesFolder()))
				{
					SaveAddress.Set(EVehicleSaveSource::JsonLocal, FPaths::GetBaseFilename(FileName));
				}
				else
				{
					SaveAddress.Set(EVehicleSaveSource::JsonExternal, FileName);
				}
			}
			ClearDirty();
			UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SaveToJson(). Save vehicle to: %s"), *FileName);
			return true;
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToJson(). Can't save vehicle to: %s"), *FileName);
			return false;
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToJson(). Serialize vehicle failed"));
		return false;
	}
}

bool ASodaVehicle::SaveToDB(const FString& VehicleName, bool bRebase)
{
	FJsonActorArchive Ar;
	FString JsonString;
	if (Ar.SerializeActor(this, true))
	{
		if (Ar.SaveToString(JsonString))
		{
			UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SaveToDB(). Save vehicle"));
			
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToDB(). Can't save vehicle"));
			return false;
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToDB(). Serialize vehicle failed"));
		return false;
	}

	if (soda::FDBGateway::Instance().SaveVehicleData(VehicleName, bsoncxx::from_json(bsoncxx::stdx::string_view{ TCHAR_TO_UTF8(*JsonString) })))
	{
		ClearDirty();

		if (bRebase)
		{
			SaveAddress.Set(EVehicleSaveSource::DB, VehicleName);
		}
		return true;
	}
	else
	{
		return false;
	}

}

bool ASodaVehicle::SaveToBin(const FString& FileName, bool bRebase)
{
	USaveGameActor* SaveGameData = NewObject< USaveGameActor >();
	if (!SaveGameData->SerializeActor(const_cast<ASodaVehicle*>(this), false))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToBin(). Serialize vehicle failed"));
		return false;
	}

	TArray<uint8> BytesData;
	if (UGameplayStatics::SaveGameToMemory(SaveGameData, BytesData))
	{
		if (FFileHelper::SaveArrayToFile(BytesData, *FileName))
		{
			if (bRebase)
			{
				if (FPaths::IsSamePath(FPaths::GetPath(FileName), GetDefaultVehiclesFolder()))
				{
					SaveAddress.Set(EVehicleSaveSource::BinLocal, FPaths::GetBaseFilename(FileName));
				}
				else
				{
					SaveAddress.Set(EVehicleSaveSource::BinExternal, FileName);
				}
			}
			ClearDirty();
			UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SaveToBin(). Save vehicle to: %s"), *FileName);
			return true;
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToBin(). Can't save vehicle to: %s"), *FileName);
			return false;
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToBin(). Faild to save vehicle to memory"), *FileName);
		return false;
	}
	
}

bool ASodaVehicle::SaveToSlot(const FString& SlotName, bool bRebase)
{
	USaveGameActor* SaveGameData = NewObject< USaveGameActor >();
	if (!SaveGameData->SerializeActor(const_cast<ASodaVehicle*>(this), false))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToSlot(). Serialize vehicle failed"));
		return false;
	}

	if (UGameplayStatics::SaveGameToSlot(SaveGameData, SlotName, 0))
	{
		if (bRebase)
		{
			SaveAddress.Set(EVehicleSaveSource::Slot, SlotName);
		}
		ClearDirty();
		UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SaveToSlot(). Save vehicle to: %s"), *SlotName);
		return true;
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToSlot(). Can't save vehicle to: %s"), *SlotName);
		return false;
	}
}

bool ASodaVehicle::SaveToAddress(const FVechicleSaveAddress& Address, bool bRebase)
{
	if (Address.Location.Len() == 0)
	{
		return false;
	}

	switch (Address.Source)
	{
	case EVehicleSaveSource::BinExternal:
		return SaveToBin(Address.Location, bRebase);

	case EVehicleSaveSource::BinLocal:
		return SaveToBin(GetDefaultVehiclesFolder() / FPaths::GetBaseFilename(Address.Location) + TEXT(".bin"), bRebase);

	case EVehicleSaveSource::Slot:
		return SaveToSlot(Address.Location, bRebase);

	case EVehicleSaveSource::JsonLocal:
		return SaveToJson(GetDefaultVehiclesFolder() / FPaths::GetBaseFilename(Address.Location) + TEXT(".json"), bRebase);

	case EVehicleSaveSource::JsonExternal:
		return SaveToJson(Address.Location, bRebase);

	case EVehicleSaveSource::DB:
		return SaveToDB(Address.Location, bRebase);

	case EVehicleSaveSource::NoSave:
	case EVehicleSaveSource::BinLevel:
	default:
		return false;
	}
}

bool ASodaVehicle::Resave()
{
	return SaveToAddress(SaveAddress, false);
}

FString ASodaVehicle::GetDefaultVehiclesFolder()
{
	//return IPluginManager::Get().FindPlugin(TEXT("UnrealSoda"))->GetBaseDir() / TEXT("Vehicles");
	return FPaths::ProjectSavedDir() / TEXT("SodaVehicles");
}

bool ASodaVehicle::DeleteVehicleSave(const FVechicleSaveAddress& Address)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

	switch (Address.Source)
	{
	case EVehicleSaveSource::JsonExternal:
	case EVehicleSaveSource::BinExternal:
		return FileManager.DeleteFile(*Address.Location);

	case EVehicleSaveSource::BinLocal:
		return FileManager.DeleteFile(*(GetDefaultVehiclesFolder() / Address.Location + TEXT(".bin")));

	case EVehicleSaveSource::JsonLocal:
		return FileManager.DeleteFile(*(GetDefaultVehiclesFolder() / Address.Location + TEXT(".json")));

	case EVehicleSaveSource::Slot:
		return UGameplayStatics::DeleteGameInSlot(Address.Location, 0);

	case EVehicleSaveSource::DB:
		return soda::FDBGateway::Instance().DeleteVehicleData(Address.Location);

	case EVehicleSaveSource::NoSave:
	case EVehicleSaveSource::BinLevel:
	default:
		return false;
	}
}

void ASodaVehicle::GetSavedVehiclesLocal(TArray<FVechicleSaveAddress>& Addresses)
{
	FString VehiclesDir = GetDefaultVehiclesFolder();

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *VehiclesDir);

	for (auto& File : Files)
	{
		FString Ext = FPaths::GetExtension(File, false).ToLower();
		Addresses.Add(FVechicleSaveAddress(EVehicleSaveSource::JsonLocal, FPaths::GetBaseFilename(File)));
	}
}

bool ASodaVehicle::GetSavedVehiclesDB(TArray<FVechicleSaveAddress>& Addresses)
{
	return soda::FDBGateway::Instance().LoadVehiclesList(Addresses);
}

ASodaVehicle* ASodaVehicle::FindVehicleByAddress(const UWorld* World, const FVechicleSaveAddress& Address)
{
	check(World);
	for (TActorIterator<ASodaVehicle> It(World); It; ++It)
	{
		if (It->SaveAddress == Address) return *It;
	}
	return nullptr;
}

ASodaVehicle* ASodaVehicle::SpawnVehicleFromJsonArchive(UWorld* World, const TSharedPtr<FJsonActorArchive> & Ar, const FVechicleSaveAddress& Address, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
{
	UClass* VahicleClass = Ar->GetActorClass();

	if (!VahicleClass || !VahicleClass->IsChildOf<ASodaVehicle>())
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromJsonArchive(); Vehicle class faild"));
		return nullptr;
	}

	FVector Offset = FVector::ZeroVector;
	if (bApplyOffset)
	{
		const FSodaActorDescriptor& Desc = USodaSubsystem::Get()->GetSodaActorDescriptor(VahicleClass);
		Offset = Desc.SpawnOffset;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.bDeferConstruction = true;
	SpawnInfo.Name = DesireName.IsNone() ? NAME_None : FEditorUtils::MakeUniqueObjectName(World->GetCurrentLevel(), VahicleClass, DesireName);
	FTransform SpawnTransform(Rotation, Location + Offset, FVector(1.0f, 1.0f, 1.0f));
	ASodaVehicle* NewVehicle = Cast<ASodaVehicle>(World->SpawnActor(VahicleClass, &SpawnTransform, SpawnInfo));

	NewVehicle->JsonAr = Ar;

	if (NewVehicle == nullptr)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromJsonArchive(); Can't spawn new vehicle"));
		return nullptr;
	}

	if (!Ar->DeserializeActor(NewVehicle, false))
	{
		NewVehicle->Destroy();
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromJsonArchive(); Can't deserialize vehicle"));
		return nullptr;
	}

	NewVehicle->SetSaveAddress(Address);
	NewVehicle->FinishSpawning(SpawnTransform);

	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		SodaSubsystem->NotifyLevelIsChanged();
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController != nullptr && Posses)
	{
		PlayerController->Possess(NewVehicle);
	}

	return NewVehicle;
}

ASodaVehicle* ASodaVehicle::SpawnVehicleFromJsonFile(const UObject* WorldContextObject, const FString& FileName, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
{
	UWorld* World = USodaStatics::GetGameWorld(WorldContextObject);
	check(World);

	TSharedPtr<FJsonActorArchive> Ar = MakeShared<FJsonActorArchive>();
	if (!Ar->LoadFromFile(FileName))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromJsonFile(); Can't read file "));
		return nullptr;
	}

	FVechicleSaveAddress Address = 
		FPaths::IsSamePath(FPaths::GetPath(FileName), GetDefaultVehiclesFolder()) ?
		FVechicleSaveAddress(EVehicleSaveSource::JsonLocal, FPaths::GetBaseFilename(FileName)) :
		FVechicleSaveAddress(EVehicleSaveSource::JsonExternal, FileName);
	
	ASodaVehicle* NewVehicle = SpawnVehicleFromJsonArchive(World, Ar, Address, Location, Rotation, Posses, DesireName, bApplyOffset);

	if (NewVehicle)
	{
		UE_LOG(LogSoda, Log, TEXT("Spawned vehicle from JSON: %s"), *FileName);
	}

	return NewVehicle;
}

ASodaVehicle* ASodaVehicle::SpawnVehicleFromDB(const UObject* WorldContextObject, const FString& VehicleName, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
{
	bsoncxx::document::view Data = soda::FDBGateway::Instance().LoadVehicleData(VehicleName);
	if (Data.empty())
	{
		return nullptr;
	}
	
	FString JsonString = FString(bsoncxx::to_json(Data).c_str());
	
	UWorld* World = USodaStatics::GetGameWorld(WorldContextObject);
	check(World);

	TSharedPtr<FJsonActorArchive> Ar = MakeShared<FJsonActorArchive>();
	if (!Ar->LoadFromString(JsonString))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromDB(); Can't read file "));
		return nullptr;
	}

	ASodaVehicle* NewVehicle = SpawnVehicleFromJsonArchive(World, Ar, FVechicleSaveAddress(EVehicleSaveSource::DB, VehicleName), Location, Rotation, Posses, DesireName, bApplyOffset);

	if (NewVehicle)
	{
		UE_LOG(LogSoda, Log, TEXT("Spawned vehicle from DB: %s"), *VehicleName);
	}

	return NewVehicle;
}

ASodaVehicle* ASodaVehicle::SpawnVehicleFromBin(const UObject* WorldContextObject, const FString& SlotOrFileName, bool IsSlot, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
{
	UWorld* World = USodaStatics::GetGameWorld(WorldContextObject);
	check(World);

	USaveGameActor* SaveGameData = nullptr;
	if (IsSlot)
	{
		SaveGameData = Cast<USaveGameActor>(UGameplayStatics::LoadGameFromSlot(SlotOrFileName, 0));
	}
	else
	{
		TArray<uint8> ObjectBytes;
		if (FFileHelper::LoadFileToArray(ObjectBytes, *SlotOrFileName))
		{
			SaveGameData = Cast<USaveGameActor>(UGameplayStatics::LoadGameFromMemory(ObjectBytes));
		}
	}

	if (!SaveGameData)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBin(): Can't open %s"), *SlotOrFileName);
		return nullptr;
	}

	UClass* Class = SaveGameData->GetActorRecord().Class.Get();
	if (!Class)
	{
		Class = LoadObject<UClass>(nullptr, *SaveGameData->GetActorRecord().Class.ToString());
	}

	if (!Class)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBin(): Can't find class %s"), *SaveGameData->GetActorRecord().Class.ToString());
		return nullptr;
	}

	FVector Offset = FVector::ZeroVector;
	if (bApplyOffset)
	{
		const FSodaActorDescriptor& Desc = USodaSubsystem::Get()->GetSodaActorDescriptor(Class);
		Offset = Desc.SpawnOffset;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.bDeferConstruction = true;
	SpawnInfo.Name = DesireName.IsNone() ? NAME_None : FEditorUtils::MakeUniqueObjectName(World->GetCurrentLevel(), Class, DesireName);
	FTransform SpawnTransform(Rotation, Location + Offset, FVector(1.0f, 1.0f, 1.0f));
	ASodaVehicle* NewVehicle = Cast<ASodaVehicle>(World->SpawnActor(Class, &SpawnTransform, SpawnInfo));

	if (NewVehicle == nullptr)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBin(); Can't spawn new vehicle"));
		return nullptr;
	}


	if (!SaveGameData->DeserializeActor(NewVehicle, false))
	{
		NewVehicle->Destroy();
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBin(); Can't deserialize vehicle"));
		return nullptr;
	}

	if (IsSlot)
	{
		NewVehicle->SetSaveAddress(FVechicleSaveAddress(EVehicleSaveSource::Slot, SlotOrFileName));
	}
	else
	{
		if (FPaths::IsSamePath(FPaths::GetPath(SlotOrFileName), GetDefaultVehiclesFolder()))
		{
			NewVehicle->SetSaveAddress(FVechicleSaveAddress(EVehicleSaveSource::BinLocal, FPaths::GetBaseFilename(SlotOrFileName)));
		}
		else
		{
			NewVehicle->SetSaveAddress(FVechicleSaveAddress(EVehicleSaveSource::BinExternal, SlotOrFileName));
		}
		
	}

	NewVehicle->FinishSpawning(SpawnTransform);

	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		SodaSubsystem->NotifyLevelIsChanged();
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController != nullptr && Posses)
	{
		PlayerController->Possess(NewVehicle);
	}

	UE_LOG(LogSoda, Log, TEXT("Spawned vehicle from bin: %s"), *SlotOrFileName);

	return NewVehicle;
}

ASodaVehicle* ASodaVehicle::SpawnVehicleFormAddress(const UObject* WorldContextObject, const FVechicleSaveAddress& Address, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
{
	if (Address.Location.Len() == 0)
	{
		return nullptr;
	}

	switch (Address.Source)
	{
	case EVehicleSaveSource::BinExternal:
		return SpawnVehicleFromBin(WorldContextObject, Address.Location, false, Location, Rotation, Posses, DesireName, bApplyOffset);

	case EVehicleSaveSource::BinLocal:
		return SpawnVehicleFromBin(WorldContextObject, GetDefaultVehiclesFolder() / Address.Location + TEXT(".bin"), false, Location, Rotation, Posses, DesireName, bApplyOffset);

	case EVehicleSaveSource::Slot:
		return SpawnVehicleFromBin(WorldContextObject, Address.Location, true, Location, Rotation, Posses, DesireName, bApplyOffset);

	case EVehicleSaveSource::JsonExternal:
		return SpawnVehicleFromJsonFile(WorldContextObject, Address.Location, Location, Rotation, Posses, DesireName, bApplyOffset);

	case EVehicleSaveSource::JsonLocal:
		return SpawnVehicleFromJsonFile(WorldContextObject, GetDefaultVehiclesFolder() / Address.Location + TEXT(".json"), Location, Rotation, Posses, DesireName, bApplyOffset);

	case EVehicleSaveSource::DB:
		return SpawnVehicleFromDB(WorldContextObject, Address.Location, Location, Rotation, Posses, DesireName, bApplyOffset);

	case EVehicleSaveSource::NoSave:
	case EVehicleSaveSource::BinLevel:
	default:
		return nullptr;
	}
}

TArray<ISodaVehicleComponent*> ASodaVehicle::GetVehicleComponents() const
{
	TArray<ISodaVehicleComponent*> Components;
	for (UActorComponent* Component : GetComponents())
	{
		if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(Component))
		{
			Components.Add(VehicleComponent);
		}
	}
	
	return Components;
}

FString ASodaVehicle::ExportTo(const FString& ExporterName)
{
	if (auto Exporter = SodaApp.GetVehicleExporters().Find(ExporterName))
	{
		FString Ret;
		if (Exporter->Get()->ExportToString(this, Ret))
		{
			return Ret;
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::ExportTo(); Faild to export to ExporterName: \"%s\""), *ExporterName);
			return "";
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::ExportTo(); Can't find ExporterName: \"%s\""), *ExporterName);
		return "";
	}
}

void ASodaVehicle::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	ISodaActor::RuntimePostEditChangeProperty(PropertyChangedEvent);

	if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, TEXT("ReactivateActor")))
	{
		UpdateProperties();
		ReActivateVehicleComponents(true);
	}
}

void ASodaVehicle::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	ISodaActor::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, TEXT("ReactivateActor")))
	{
		UpdateProperties();
		ReActivateVehicleComponents(true);
	}
}

bool ASodaVehicle::OnSetPinnedActor(bool bIsPinnedActor)
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	if (SodaSubsystem->LevelState && SodaSubsystem->LevelState->ActorFactory && SodaSubsystem->LevelState->ActorFactory->CheckActorIsExist(this))
	{
		if (bIsPinnedActor)
		{
			SodaSubsystem->OpenWindow(FString::Printf(TEXT("Pin \"%s\" Vehicle"), *GetName()), SNew(soda::SVehcileManagerWindow, this));
		}
		else
		{
			SaveAddress.Reset(); 
		}
	}
	return true;
}

bool ASodaVehicle::IsPinnedActor() const
{
	return SaveAddress.Source == EVehicleSaveSource::JsonLocal || SaveAddress.Source == EVehicleSaveSource::DB;
}

FString ASodaVehicle::GetPinnedActorName() const
{
	return SaveAddress.ToVehicleName();
}

bool ASodaVehicle::SavePinnedActor()
{
	if (SaveAddress.Source == EVehicleSaveSource::JsonLocal || SaveAddress.Source == EVehicleSaveSource::DB)
	{
		return Resave();
	}
	return false;
}

AActor* ASodaVehicle::LoadPinnedActor(UWorld* World, const FTransform& Transform, const FString& SlotName, bool bForceCreate, FName DesireName) const
{
	FVechicleSaveAddress Addr;
	if (Addr.SetFromUrl(SlotName))
	{
		return SpawnVehicleFormAddress(World, Addr, Transform.GetTranslation(), Transform.GetRotation().Rotator(), false, DesireName, false);
	}
	return nullptr;
}

FString ASodaVehicle::GetPinnedActorSlotName() const
{
	return SaveAddress.ToUrl();
}

void ASodaVehicle::ScenarioBegin()
{
	ISodaActor::ScenarioBegin();

	FScopeLock ScopeLock(&PhysicMutex);

	VehicleComponensForDataset.Empty();

	/*
	for (auto& Component : GetVehicleComponents())
	{
		if ((Component->GetVehicleComponentCommon().Activation == EVehicleComponentActivation::OnStartScenario) && !Component->IsVehicleComponentActiveted())
		{
			Component->ActivateVehicleComponent();
		}
	}*/

	for (auto& Component : GetVehicleComponents())
	{
		if ((Component->GetVehicleComponentCommon().Activation == EVehicleComponentActivation::OnStartScenario) && !Component->IsVehicleComponentActiveted())
		{
			Component->SetDeferredTask(EVehicleComponentDeferredTask::Activate);
		}
	}
	ReActivateVehicleComponents(false);


	if (bRecordDataset && soda::FDBGateway::Instance().GetStatus() == soda::EDBGatewayStatus::Connected && soda::FDBGateway::Instance().IsDatasetRecording())
	{
		soda::FBsonDocument Doc;
		GenerateDatasetDescription(Doc);

		for (auto& Component : GetVehicleComponents())
		{
			if ((Component->GetVehicleComponentCommon().Activation == EVehicleComponentActivation::OnStartScenario) && !Component->IsVehicleComponentActiveted())
			{
				Component->GenerateDatasetDescription(Doc);
			}
		}

		Dataset = soda::FDBGateway::Instance().CreateActorDataset(GetName(), "ego", GetClass()->GetName(), *Doc);
		if (Dataset)
		{
			for (auto& Component : GetVehicleComponents())
			{
				if (Component->GetVehicleComponentCommon().bWriteDataset)
				{
					VehicleComponensForDataset.Add(Component);
				}
			}
		}
		else
		{
			SodaApp.GetSodaSubsystemChecked()->ScenarioStop(EScenarioStopReason::InnerError, EScenarioStopMode::RestartLevel, "Can't create dataset for \"" + GetName() + "\"");
		}
	}

	for (auto& Component : GetVehicleComponents())
	{
		Component->ScenarioBegin();
	}

	if (bPossesWhenScarioPlay)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->Possess(this);
		}
	}
}

void ASodaVehicle::ScenarioEnd()
{
	ISodaActor::ScenarioEnd();

	for (auto& Component : GetVehicleComponents())
	{
		if ((Component->GetVehicleComponentCommon().Activation == EVehicleComponentActivation::OnStartScenario) && Component->IsVehicleComponentActiveted())
		{
			Component->SetDeferredTask(EVehicleComponentDeferredTask::Deactivate);
		}
		Component->ScenarioEnd();
	}
	ReActivateVehicleComponents(false);

	if (Dataset)
	{
		Dataset->PushAsync();
		Dataset.Reset();
	}
	VehicleComponensForDataset.Empty();
}

#if WITH_EDITOR
void ASodaVehicle::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!HasActorBegunPlay())
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
}

void ASodaVehicle::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (!HasActorBegunPlay())
	{
		Super::PostEditChangeChainProperty(PropertyChangedEvent);
	}
}
#endif


void ASodaVehicle::OnPushDataset(soda::FActorDatasetData& InDataset) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	const FVehicleSimData& SimData = GetSimData();

	FVector Location = SimData.VehicleKinematic.Curr.GlobalPose.GetLocation();
	FRotator Rotation = SimData.VehicleKinematic.Curr.GlobalPose.GetRotation().Rotator();
	FVector Vel = SimData.VehicleKinematic.Curr.GetLocalVelocity();
	FVector Acc = SimData.VehicleKinematic.Curr.GetLocalAcceleration();
	FVector AngVel = SimData.VehicleKinematic.Curr.AngularVelocity;

	try
	{
		InDataset.GetRowDoc()
			<< "VehicleData" << open_document
			<< "SimTsUs" << std::int64_t(soda::RawTimestamp<std::chrono::microseconds>(SimData.SimulatedTimestamp))
			<< "RenderTsUs" << std::int64_t(soda::RawTimestamp<std::chrono::microseconds>(SimData.RenderTimestamp))
			<< "Step" << SimData.SimulatedStep
			<< "Loc" << open_array << Location.X << Location.Y << Location.Z << close_array
			<< "Rot" << open_array << Rotation.Roll << Rotation.Pitch << Rotation.Yaw << close_array
			<< "Bel" << open_array << Vel.X << Vel.Y << Vel.Z << close_array
			<< "Acc" << open_array << Acc.X << Acc.Y << Acc.Z << close_array
			<< "AngVel" << open_array << AngVel.X << AngVel.Y << AngVel.Z << close_array
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}

void ASodaVehicle::GenerateDatasetDescription(soda::FBsonDocument& Doc) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	const FExtent& Extent = GetVehicleExtent();
	*Doc
		<< "Extents" << open_document
		<< "Forward" << Extent.Forward
		<< "Backward" << Extent.Backward
		<< "Left" << Extent.Left
		<< "Right" << Extent.Right
		<< "Up" << Extent.Up
		<< "Down" << Extent.Down
		<< close_document;
}

TSharedPtr<SWidget> ASodaVehicle::GenerateToolBar()
{
	FUniformToolBarBuilder ToolbarBuilder(TSharedPtr<const FUICommandList>(), FMultiBoxCustomization::None);
	ToolbarBuilder.SetStyle(&FSodaStyle::Get(), "PaletteToolBar");
	ToolbarBuilder.BeginSection(NAME_None);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this] 
			{
				if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
				{
					if (SaveAddress.Source != EVehicleSaveSource::NoSave &&
						SaveAddress.Source == EVehicleSaveSource::BinLevel)
					{
						SodaSubsystem->OpenWindow("Save Vehicle As", SNew(soda::SSaveVehicleRequestWindow, this));
					}
					else
					{
						SodaSubsystem->OpenWindow("Save New Vehicle As", SNew(soda::SVehcileManagerWindow, this));
					}
				}
			})),
		NAME_None, 
		FText::FromString("Save"),
		FText::FromString("Save vehicle to JSON format"), 
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaVehicleBar.Save"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this] 
			{
				if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
				{
					SodaSubsystem->OpenWindow("Export Vehicle As", SNew(soda::SExportVehicleWindow, this));
				}
			})),
		NAME_None, 
		FText::FromString("Export"),
		FText::FromString("Export vehicle to custom formats"), 
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaVehicleBar.Export"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this] 
			{
				IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
				if (!DesktopPlatform)
				{
					UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::ImportFromJSONAs() Can't get the IDesktopPlatform ref"));
					return;
				}

				const FString FileTypes = TEXT("Soda Vehicles (*.json)|*.json");

				TArray<FString> OpenFilenames;
				int32 FilterIndex = -1;
				if (!DesktopPlatform->OpenFileDialog(nullptr, TEXT("Import Vehicle from JSON"), GetDefaultVehiclesFolder(), TEXT(""), FileTypes, EFileDialogFlags::None, OpenFilenames, FilterIndex) || OpenFilenames.Num() <= 0)
				{
					return;
				}

				RespawnVehcileFromAddress(FVechicleSaveAddress(EVehicleSaveSource::JsonExternal, OpenFilenames[0]), GetActorLocation() + FVector(0, 0, 50), FRotator(0.f, GetActorRotation().Yaw, 0.f), true);
			})),
		NAME_None, 
		FText::FromString("Import"),
		FText::FromString("Import Vehicle from JSON"), 
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaVehicleBar.Import"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this] 
			{
				if(USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
				{
					SodaSubsystem->PushToolBox(SNew(soda::SVehicleComponentsToolBox, this));
				}
			})),
		NAME_None, 
		FText::FromString("Components"),
		FText::FromString("Open Vehicle Components"), 
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaVehicleBar.Components"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this] 
			{
				RespawnVehcile(FVector(0, 0, 20), FRotator::ZeroRotator, true, GetClass());
			})),
		NAME_None, 
		FText::FromString("Reset"),
		FText::FromString("Recreate vehicle"), 
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaVehicleBar.Reset"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.EndSection();

	return
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FStyleColors::Panel)
		.Padding(3)
		[
			ToolbarBuilder.MakeWidget()
		];
}

void ASodaVehicle::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
}