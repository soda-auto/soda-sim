// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/FileDatabaseManager.h"
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
#include "Soda/SodaCommonSettings.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Images/SImage.h"
#include "Styling/StyleColors.h"
#include "UI/Wnds/SSlotActorManagerWindow.h"
#include "UI/Wnds/SExportVehicleWindow.h"
#include "UI/ToolBoxes/SVehicleComponentsToolBox.h"
#include "SodaStyleSet.h"
#include "GlobalRenderResources.h"
#include "GameFramework/PlayerController.h"
#include "RuntimeEditorUtils.h"

#define PARCE_VEC(V, Mul) V.X * Mul, V.Y * Mul, V.Z * Mul

FVehicleSimData FVehicleSimData::Zero{};

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

ASodaVehicle * ASodaVehicle::RespawnVehcile(FVector Location, FRotator Rotation, bool IsLocalCoordinateSpace)
{
	UWorld * World = GetWorld();
	check(World);

	FTransform SpawnTransform = IsLocalCoordinateSpace ?
		FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f) + Rotation, GetActorLocation() + Location, FVector(1.0f, 1.0f, 1.0f)) :
		FTransform(Rotation, Location, FVector(1.0f, 1.0f, 1.0f));

	ASodaVehicle * NewVehicle = World->SpawnActorDeferred<ASodaVehicle>(
		GetClass(),
		SpawnTransform, 
		nullptr, nullptr, 
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if(NewVehicle == nullptr)
	{
		return nullptr;
	}

	FActorRecord SavedActorRecord;
	SavedActorRecord.SerializeActor(this, false);
	SavedActorRecord.DeserializeActor(NewVehicle, false);
	NewVehicle->MarkAsDirty(); //NewVehicle->bIsDirty = IsDirty();
	
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	if (ASodaActorFactory* ActorFactory = SodaSubsystem->GetActorFactory())
	{
		ActorFactory->ReplaceActor(NewVehicle, this, false);
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

ASodaVehicle* ASodaVehicle::SpawnVehicleFormSlot(const UObject* WorldContextObject, const FGuid& SlotGuid, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
{
	UWorld* World = USodaStatics::GetGameWorld(WorldContextObject);
	check(World);

	soda::FFileDatabaseSlotInfo SlotInfo;
	if (!SodaApp.GetFileDatabaseManager().GetSlot(SlotGuid, SlotInfo))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFormSlot(); Can't find slot "));
		return nullptr;
	}

	TArray<uint8> SlotData;
	if (!SodaApp.GetFileDatabaseManager().GetSlotData(SlotGuid, SlotData))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFormSlot(); Can't GetSlotData() "));
		return nullptr;
	}

	TSharedPtr<FJsonActorArchive> Ar = MakeShared<FJsonActorArchive>();
	if (!Ar->LoadFromString(FString(SlotData.Num() / sizeof(FString::ElementType), (FString::ElementType*)(SlotData.GetData()))))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFormSlot(); Can't FJsonActorArchive::LoadFromString() "));
		return nullptr;
	}

	if (ASodaVehicle* Vehicle = SpawnVehicleFromJsonArchive(World, Ar, Location, Rotation, Posses, DesireName, bApplyOffset))
	{
		Vehicle->SlotGuid = SlotGuid;
		Vehicle->SlotLabel = SlotInfo.Label;
		return Vehicle;
	}
	
	return nullptr;
}


bool ASodaVehicle::DrawDebug(class UCanvas* Canvas, float& YL, float& YPos)
{
	YPos += 30;

	FCanvasTileItem TileItem(FVector2D(0, 0), GWhiteTexture, FVector2D(GetDefault<USodaCommonSettings>()->VehicleDebugAreaWidth, Canvas->SizeY), FLinearColor(0.0f, 0.0f, 0.0f, 0.3f));
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);

	UFont* RenderFont = GEngine->GetSmallFont();
	static FColor SubCaption(170, 170, 170);

	Canvas->SetDrawColor(FColor(51, 102, 204));
	YPos += Canvas->DrawText(
		UEngine::GetMediumFont(), 
		FString::Printf(TEXT("Slot: %s %s"), 
			SlotGuid.IsValid() ? *SlotLabel : TEXT("-"),
			IsDirty() ? TEXT("*") : TEXT("")),
		4, YPos + 4);

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
		checkf(true, TEXT("ASodaVehicle::ReActivateVehicleComponents() can be called only after BeginPlay()"));
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

	SyncDataset();
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

bool ASodaVehicle::SaveToJsonFile(const FString& FileName)
{
	FJsonActorArchive Ar;
	if (Ar.SerializeActor(this, true))
	{
		if (Ar.SaveToFile(FileName))
		{
			ClearDirty();
			UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SaveToJsonFile(). Save vehicle to: %s"), *FileName);
			return true;
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToJsonFile(). Can't save vehicle to: %s"), *FileName);
			return false;
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToJsonFile(). Serialize vehicle failed"));
		return false;
	}
}


bool ASodaVehicle::SaveToBinFile(const FString& FileName)
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
			ClearDirty();
			UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SaveToBinFile(). Save vehicle to: %s"), *FileName);
			return true;
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToBinFile(). Can't save vehicle to: %s"), *FileName);
			return false;
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToBinFile(). Faild to save vehicle to memory"), *FileName);
		return false;
	}
	
}

bool ASodaVehicle::SaveToSlot(const FString& Label, const FString& Description, const FGuid& CustomGuid, bool bRebase)
{
	FJsonActorArchive Ar;
	if (!Ar.SerializeActor(this, true))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToSlot(); Can't SerializeActor() "));
		return false;
	}

	FString JsonString;
	if (!Ar.SaveToString(JsonString))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToSlot(); Can't SaveToString() "));
		return false;
	}

	TArray<uint8> SlotData;
	SlotData.SetNum(JsonString.Len() * sizeof(FString::ElementType));
	FMemory::Memcpy((void*)SlotData.GetData(), (void*)JsonString.GetCharArray().GetData(), SlotData.Num());

	soda::FFileDatabaseSlotInfo SlotInfo{};
	SlotInfo.GUID = CustomGuid.IsValid() ? CustomGuid : FGuid::NewGuid();
	SlotInfo.Type = soda::EFileSlotType::Vehicle;
	SlotInfo.Label = Label;
	SlotInfo.Description = Description;
	SlotInfo.DataClass = GetClass();

	if (!SodaApp.GetFileDatabaseManager().AddOrUpdateSlotInfo(SlotInfo))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToSlot(); Can't AddOrUpdateSlotInfo() "));
		return false;
	}

	if (!SodaApp.GetFileDatabaseManager().UpdateSlotData(SlotInfo.GUID, SlotData))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SaveToSlot(); Can't UpdateSlotData() "));
		return false;
	}

	if (bRebase)
	{
		if (SlotGuid != SlotInfo.GUID)
		{
			ALevelState::GetChecked()->MarkAsDirty();
		}
		SlotGuid = SlotInfo.GUID;
		SlotLabel = Label;
	}

	ClearDirty();

	return true;
}

bool ASodaVehicle::Resave()
{
	if (!SlotGuid.IsValid())
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::Resave(); There is no slot for save"));
		return false;
	}

	soda::FFileDatabaseSlotInfo Slot;
	if (!SodaApp.GetFileDatabaseManager().GetSlot(SlotGuid, Slot))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::Resave(); Can't find slot "));
		return false;
	}

	if (!SaveToSlot(Slot.Label, Slot.Description, Slot.GUID, false))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::Resave(); SaveToSlot() faild "));
		return false;
	}

	ClearDirty();

	return true;
}

ASodaVehicle* ASodaVehicle::SpawnVehicleFromJsonArchive(UWorld* World, const TSharedPtr<FJsonActorArchive> & Ar, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
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

	NewVehicle->FinishSpawning(SpawnTransform);

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
	
	ASodaVehicle* NewVehicle = SpawnVehicleFromJsonArchive(World, Ar, Location, Rotation, Posses, DesireName, bApplyOffset);

	if (NewVehicle)
	{
		UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SpawnVehicleFromJsonFile(); Spawned vehicle from JSON: %s"), *FileName);
	}

	return NewVehicle;
}

ASodaVehicle* ASodaVehicle::SpawnVehicleFromBinFile(const UObject* WorldContextObject, const FString& FileName, const FVector& Location, const FRotator& Rotation, bool Posses, FName DesireName, bool bApplyOffset)
{
	UWorld* World = USodaStatics::GetGameWorld(WorldContextObject);
	check(World);

	USaveGameActor* SaveGameData = nullptr;
	TArray<uint8> ObjectBytes;
	if (FFileHelper::LoadFileToArray(ObjectBytes, *FileName))
	{
		SaveGameData = Cast<USaveGameActor>(UGameplayStatics::LoadGameFromMemory(ObjectBytes));
	}
	

	if (!SaveGameData)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBinFile(): Can't open %s"), *FileName);
		return nullptr;
	}

	UClass* Class = SaveGameData->GetActorRecord().Class.Get();
	if (!Class)
	{
		Class = LoadObject<UClass>(nullptr, *SaveGameData->GetActorRecord().Class.ToString());
	}

	if (!Class)
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBinFile(): Can't find class %s"), *SaveGameData->GetActorRecord().Class.ToString());
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
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBinFile(); Can't spawn new vehicle"));
		return nullptr;
	}


	if (!SaveGameData->DeserializeActor(NewVehicle, false))
	{
		NewVehicle->Destroy();
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::SpawnVehicleFromBinFile(); Can't deserialize vehicle"));
		return nullptr;
	}

	NewVehicle->FinishSpawning(SpawnTransform);

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController != nullptr && Posses)
	{
		PlayerController->Possess(NewVehicle);
	}

	UE_LOG(LogSoda, Log, TEXT("ASodaVehicle::SpawnVehicleFromBinFile(); Spawned vehicle from bin: %s"), *FileName);

	return NewVehicle;
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

FString ASodaVehicle::ExportTo(FName ExporterName)
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
			UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::ExportTo(); Faild to export to ExporterName: \"%s\""), *ExporterName.ToString());
			return "";
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::ExportTo(); Can't find ExporterName: \"%s\""), *ExporterName.ToString());
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

bool ASodaVehicle::Unpin()
{
	SlotGuid.Invalidate();

	MarkAsDirty();
	USodaSubsystem::GetChecked()->LevelState->MarkAsDirty();

	return true;
}

FString ASodaVehicle::GetSlotLabel() const
{
	return SlotLabel;
}

AActor* ASodaVehicle::SpawnActorFromSlot(UWorld* World, const FGuid& Slot, const FTransform& Transform, FName DesireName) const
{
	return SpawnVehicleFormSlot(World, Slot, Transform.GetTranslation(), Transform.GetRotation().Rotator(), false, DesireName, false);
}

void ASodaVehicle::ScenarioBegin()
{
	ISodaActor::ScenarioBegin();

	FScopeLock ScopeLock(&PhysicMutex);

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
					SodaSubsystem->OpenWindow("Vehcile Manager", SNew(soda::SSlotActorManagerWindow, soda::EFileSlotType::Vehicle, this));
				}
			})),
		NAME_None, 
		FText::FromString("Manager"),
		FText::FromString("Open the Vehicled Manager"), 
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
				RespawnVehcile(FVector(0, 0, 20), FRotator::ZeroRotator, true);
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
			FRuntimeEditorUtils::MakeWidget_HackTooltip(ToolbarBuilder)
		];
}

void ASodaVehicle::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
}

void ASodaVehicle::DrawVisualization(USodaGameViewportClient* ViewportClient, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	for (auto& Component : GetVehicleComponents())
	{
		Component->DrawVisualization(ViewportClient, View, PDI);
	}
}