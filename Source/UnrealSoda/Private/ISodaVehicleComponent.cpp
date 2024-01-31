// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ISodaVehicleComponent.h"
#include "Engine/Canvas.h"
#include "Soda/SodaApp.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/UnrealSoda.h"
#include "RuntimeMetaData.h"
#include "Soda/SodaGameViewportClient.h"
#include "Engine/Engine.h"
#include "SceneView.h"

bool ISodaVehicleComponent::OnActivateVehicleComponent()
{
	checkf(AsActorComponent()->HasBegunPlay(), TEXT("ISodaVehicleComponent::OnActivateVehicleComponent(); Can be called only after BeginPlay()"));

	DeferredTask = EVehicleComponentDeferredTask::None;
	DebugCanvasErrorMessages.Empty();
	DebugCanvasWarningMessages.Empty();

	TickDivider = SodaApp.IsSynchronousMode() ? GetVehicleComponentCommon().NonRealtimeTickDivider : GetVehicleComponentCommon().RealtimeTickDivider;
	TickDivider = FMath::Clamp(TickDivider, 1, 100);

	if (bIsVehicleComponentActiveted)
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("ISodaVehicleComponent::OnActivateVehicleComponent(); Deactivated component befor call OnActivateVehicleComponent()"));
		return false;
	}

	bIsVehicleComponentActiveted = true;

	if (!AsActorComponent()->HasBegunPlay())
	{
		SetHealth(EVehicleComponentHealth::Error);
		return false;
	}

	if (!GetVehicle() && !GetVehicleComponentCommon().bAnyParentAllowed)
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("ISodaVehicleComponent::OnActivateVehicleComponent(); Parent actor must be ASodaVehicle"));
		return false;
	}

	if (GetVehicle() && GetVehicleComponentCommon().bIsTopologyComponent)
	{
		GetVehicle()->NotifyNeedReActivateComponents();
	}
	
	SetHealth(EVehicleComponentHealth::Ok);
	return true;
}

void ISodaVehicleComponent::OnDeactivateVehicleComponent()
{
	SetHealth(EVehicleComponentHealth::Disabled);
	bIsVehicleComponentActiveted = false;
	DeferredTask = EVehicleComponentDeferredTask::None;
	DebugCanvasErrorMessages.Empty();
	DebugCanvasWarningMessages.Empty();

	if (GetVehicle() && GetVehicleComponentCommon().bIsTopologyComponent)
	{
		GetVehicle()->NotifyNeedReActivateComponents();
	}
}

void ISodaVehicleComponent::ActivateVehicleComponent()
{
	if (!bIsVehicleComponentActiveted)
	{

		if (GetVehicle())
		{
			if (GetVehicleComponentCommon().UniqueVehiceComponentClass)
			{
				UClass* Class = GetVehicleComponentCommon().UniqueVehiceComponentClass;
				bool bIsInterfcae = Class->IsChildOf(UInterface::StaticClass());

				for (auto& It : GetVehicle()->GetComponentsByInterface(USodaVehicleComponent::StaticClass()))
				{
					if (ISodaVehicleComponent* Component = Cast<ISodaVehicleComponent>(It))
					{
						if (Component->IsVehicleComponentActiveted() &&
							((bIsInterfcae && It->GetClass()->ImplementsInterface(Class)) || 
							(!bIsInterfcae && It->GetClass()->IsChildOf(GetVehicleComponentCommon().UniqueVehiceComponentClass))))
						{
							// Don't need activate components after register
							Component->DeferredTask = EVehicleComponentDeferredTask::Deactivate;
							GetVehicle()->NotifyNeedReActivateComponents();
						}
					}
				}
			}

			if (GetVehicleComponentCommon().bIsTopologyComponent || GetVehicle()->DoNeedReActivateComponents())
			{
				// Activate vehicle components in scope
				DeferredTask = EVehicleComponentDeferredTask::Activate;
				GetVehicle()->ReActivateVehicleComponents(true);
			}
			else
			{
				FScopeLock ScopeLock(&GetVehicle()->PhysicMutex);
				OnPreActivateVehicleComponent();
				OnActivateVehicleComponent();
			}
		}
		else
		{
			OnPreActivateVehicleComponent();
			OnActivateVehicleComponent();
		}
	}
}

void ISodaVehicleComponent::DeactivateVehicleComponent()
{
	if (bIsVehicleComponentActiveted)
	{
		if (GetVehicle())
		{
			FScopeLock ScopeLock(&GetVehicle()->PhysicMutex);
			OnDeactivateVehicleComponent();
			GetVehicle()->ReActivateVehicleComponentsIfNeeded();
		}
		else
		{
			OnDeactivateVehicleComponent();
		}
	}
}

void ISodaVehicleComponent::Toggle()
{
	if (IsVehicleComponentActiveted())
	{
		DeactivateVehicleComponent();
	}
	else 
	{
		ActivateVehicleComponent(); 
	}
}

void ISodaVehicleComponent::SetHealth(EVehicleComponentHealth NewHealth, const FString& AddMessage) 
{ 
	Health = NewHealth; 
	if (AddMessage.Len())
	{
		switch (Health)
		{
			case EVehicleComponentHealth::Undefined:
				break;
			case EVehicleComponentHealth::Disabled:
				break;
			case EVehicleComponentHealth::Ok:
				break;
			case EVehicleComponentHealth::Warning:
				DebugCanvasWarningMessages.Add(AddMessage);
				break;
			case EVehicleComponentHealth::Error:
				DebugCanvasErrorMessages.Add(AddMessage);
				break;
		}
	}
}

void ISodaVehicleComponent::AddDebugMessage(EVehicleComponentHealth MessageHealth, const FString& Message)
{
	switch (MessageHealth)
	{
	case EVehicleComponentHealth::Undefined:
		break;
	case EVehicleComponentHealth::Disabled:
		break;
	case EVehicleComponentHealth::Ok:
		break;
	case EVehicleComponentHealth::Warning:
		DebugCanvasWarningMessages.Add(Message);
		break;
	case EVehicleComponentHealth::Error:
		DebugCanvasErrorMessages.Add(Message);
		break;
	}
}

bool ISodaVehicleComponent::IsTickOnCurrentFrame() const
{
	return (SodaApp.GetFrameIndex() % TickDivider) == 0;
}

void ISodaVehicleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	UFont* RenderFont = GEngine->GetSmallFont();
	FString ComponentName = AsActorComponent()->GetName();

	switch (GetHealth())
	{
	case EVehicleComponentHealth::Undefined:
		Canvas->SetDrawColor(FColor::Silver);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("%s : Undefined"), *ComponentName), 4, YPos + 4);
		break;
	case EVehicleComponentHealth::Disabled:
		if (GetVehicleComponentCommon().bDrawDebugCanvas)
		{
			Canvas->SetDrawColor(FColor::Silver);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("%s : Disabled"), *ComponentName), 4, YPos + 4);
		}
		break;
	case EVehicleComponentHealth::Ok:
		if (GetVehicleComponentCommon().bDrawDebugCanvas)
		{
			Canvas->SetDrawColor(FColor::Green);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("%s : OK"), *ComponentName), 4, YPos + 4);
		}
		break;
	case EVehicleComponentHealth::Warning:
		Canvas->SetDrawColor(FColor::Yellow);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("%s : Warning"), *ComponentName), 4, YPos + 4);
		break;
	case EVehicleComponentHealth::Error:
		Canvas->SetDrawColor(FColor::Red);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("%s : Error"), *ComponentName), 4, YPos + 4);
		break;
	}

	if (GetVehicleComponentCommon().bDrawDebugCanvas || GetHealth() == EVehicleComponentHealth::Error || GetHealth() == EVehicleComponentHealth::Warning)
	{
		if (DebugCanvasErrorMessages.Num())
		{
			Canvas->SetDrawColor(FColor::Red);
			for (auto& Str : DebugCanvasErrorMessages)
			{
				YPos += Canvas->DrawText(RenderFont, Str, 16, YPos);
			}
		}

		if (DebugCanvasWarningMessages.Num())
		{
			Canvas->SetDrawColor(FColor::Yellow);
			for (auto& Str : DebugCanvasWarningMessages)
			{
				YPos += Canvas->DrawText(RenderFont, Str, 16, YPos);
			}
		}
	}
}

bool ISodaVehicleComponent::RenameVehicleComponent(const FString& NewName)
{
	for (auto& Component : AsActorComponent()->GetOwner()->GetComponents())
	{
		if (Component->GetName() == NewName) return false;
	}

	MarkAsDirty();

	bool bResult = AsActorComponent()->Rename(*NewName);
	if (bResult)
	{
		if (GetVehicle() && GetVehicleComponentCommon().bIsTopologyComponent)
		{
			GetVehicle()->ReActivateVehicleComponents(true);
		}
	}

	return bResult;
}

void ISodaVehicleComponent::RemoveVehicleComponent()
{
	ASodaVehicle* Vehicle = GetVehicle();
	AsActorComponent()->DestroyComponent();
	if (Vehicle && GetVehicleComponentCommon().bIsTopologyComponent)
	{
		Vehicle->ReActivateVehicleComponents(true);
	}
}

void ISodaVehicleComponent::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	IEditableObject::RuntimePostEditChangeProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;
	//const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	static FName RelativeLocationName("RelativeLocation");
	static FName RelativeRotationName("RelativeRotation");
	static FName RelativeScale3DName("RelativeScale3D");

	if (PropertyChangedEvent.Property->HasAllPropertyFlags(CPF_SaveGame) || PropertyName == RelativeLocationName || PropertyName == RelativeRotationName || PropertyName == RelativeScale3DName)
	{
		MarkAsDirty();
	}

	if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, TEXT("ReactivateComponent")))
	{
		if (IsVehicleComponentActiveted())
		{
			DeactivateVehicleComponent();
			ActivateVehicleComponent();
		}
	}
	else if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, TEXT("ReactivateActor")))
	{
		GetVehicle()->ReActivateVehicleComponents(true);
	}

	//UE_LOG(LogSoda, Error, TEXT("RuntimePostEditChangeProperty() %s"), *PropertyChangedEvent.GetPropertyName().ToString());
}

void ISodaVehicleComponent::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	IEditableObject::RuntimePostEditChangeProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;
	//const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	static FName RelativeLocationName("RelativeLocation");
	static FName RelativeRotationName("RelativeRotation");
	static FName RelativeScale3DName("RelativeScale3D");

	static FName ReactivateComponent("ReactivateComponent");
	static FName ReactivateActor("ReactivateActor");

	if (PropertyChangedEvent.Property->HasAllPropertyFlags(CPF_SaveGame) || PropertyName == RelativeLocationName || PropertyName == RelativeRotationName || PropertyName == RelativeScale3DName)
	{
		MarkAsDirty();
	}

	auto & TailProperty = PropertyChangedEvent.PropertyChain.GetTail()->GetValue();

	if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, ReactivateComponent) || FRuntimeMetaData::HasMetaData(TailProperty, ReactivateComponent) )
	{
		if (IsVehicleComponentActiveted())
		{
			DeactivateVehicleComponent();
			ActivateVehicleComponent();
		}
	}
	else if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, ReactivateActor) || FRuntimeMetaData::HasMetaData(TailProperty, ReactivateActor))
	{
		GetVehicle()->ReActivateVehicleComponents(true);
	}
	//UE_LOG(LogSoda, Error, TEXT("RuntimePostEditChangeProperty() %s"), *PropertyChangedEvent.GetPropertyName().ToString());
}

/*
bool ISodaVehicleComponent::DoesVehicleSupport_Implementation(TSubclassOf<ASodaVehicle> VehicleClass)
{
}
*/

void ISodaVehicleComponent::MarkAsDirty()
{
	GetVehicle()->MarkAsDirty();
}

void ISodaVehicleComponent::DrawSelection(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (USceneComponent* SceneComponent = Cast<USceneComponent>(AsActorComponent()))
	{
		const FTransform& LocalToWorld = SceneComponent->GetComponentTransform();
		const float UniformScale = View->WorldToScreen(LocalToWorld.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);
		const FVector BoxExtents(8, 8, 8);
		const float ArrowLength = 60;
		const float ArrowSize = 5;
		const FColor Color = FColor::Turquoise;
		DrawOrientedWireBox(PDI, LocalToWorld.GetLocation(), LocalToWorld.GetScaledAxis(EAxis::X), LocalToWorld.GetScaledAxis(EAxis::Y), LocalToWorld.GetScaledAxis(EAxis::Z), BoxExtents * UniformScale, Color, SDPG_Foreground);
		DrawDirectionalArrow(PDI, LocalToWorld.ToMatrixWithScale(), Color, ArrowLength * UniformScale, ArrowSize * UniformScale, SDPG_Foreground);
	}
}

bool ISodaVehicleComponent::IsDefaultComponent() 
{
	return AsActorComponent()->IsDefaultSubobject(); 
}

bool ISodaVehicleComponent::IsVehicleComponentSelected() const
{
	if (USodaGameViewportClient* GameViewportClient = Cast<USodaGameViewportClient>(Cast<UObject>(this)->GetWorld()->GetGameViewport()))
	{
		return GameViewportClient->GetWidgetTarget() && GameViewportClient->GetWidgetTarget() == Cast<UObject>(this);
	}
	return false;
}