// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IEditableObject.h"
#include "ISodaActor.generated.h"

namespace FEditorUtils
{
	struct FViewportClick;
}

class SWidget;
class USodaGameViewportClient;
class FSceneView;
class PDI;

/**
 * FSodaActorDescriptor
 */

USTRUCT()
struct FSodaActorDescriptor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Soda Actor Descriptor")
	FString DisplayName;

	UPROPERTY(EditAnywhere, Category = "Soda Actor Descriptor")
	FString Category = TEXT("Default");

	UPROPERTY(EditAnywhere, Category = "Soda Actor Descriptor")
	FString SubCategory;

	UPROPERTY(EditAnywhere, Category = "Soda Actor Descriptor")
	FName Icon = TEXT("ClassIcon.Actor");

	UPROPERTY(EditAnywhere, Category = "Soda Actor Descriptor")
	bool bAllowTransform = false;

	UPROPERTY(EditAnywhere, Category = "Soda Actor Descriptor")
	bool bAllowSpawn = false;

	UPROPERTY(EditAnywhere, Category = "Soda Actor Descriptor")
	FVector SpawnOffset;
};

/**
 * USodaActor
 * The "Pinned Actor" means the actor SaveGame serialization will be carried over between different levels
 */
UINTERFACE(BlueprintType, Blueprintable, meta=(RuntimeMetaData))
class UNREALSODA_API USodaActor
	: public UEditableObject
{
	GENERATED_BODY()
};

class UNREALSODA_API ISodaActor
	: public IEditableObject
{
	GENERATED_BODY()

public:
	/** Called  when the actor was selected in the viewport */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Soda Actor")
	void OnSelect(const UPrimitiveComponent* PrimComponent);

	/** Called  when the actor was unselected in the viewport */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Soda Actor")
	void OnUnselect();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Soda Actor")
	void OnHover();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Soda Actor")
	void OnUnhover();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Soda Actor")
	void OnComponentHover(const UPrimitiveComponent* PrimComponent);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Soda Actor")
	void OnComponentUnhover(const UPrimitiveComponent* PrimComponent);

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "ScenarioBegin"))
	void ReceiveScenarioBegin();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "ScenarioEnd"))
	void ReceiveScenarioEnd();

	/** Get actor descriptor for viewport. Called from the CDO object */
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const { return nullptr; }

	virtual bool CanBePinned() const { return false; }

	virtual bool Unpin() { return false; }

	virtual bool SaveToSlot(const FString& Lable, const FString& Description, const FGuid& Guid = FGuid(), bool bRebase = true) { return false; }

	/** Spawn actor from Slot. Called from the CDO object */
	virtual AActor* SpawnActorFromSlot(UWorld* World, const FGuid& Slot, const FTransform& Transform, FName DesireName = NAME_None) const { return nullptr; }

	virtual const FGuid& GetSlotGuid() const { static FGuid Dummy{}; return Dummy; }

	virtual FString GetSlotLable() const { return ""; }

	virtual bool Resave() { return false; }

	/** Actor has been modified and needs to be saved */
	virtual bool IsDirty() { return bIsDirty; }

	/** Set the "dirty flag" */
	virtual void MarkAsDirty();

	/** Clear the "dirty flag" */
	virtual void ClearDirty() { bIsDirty = false; }

	/** Called when the scenario begun */
	virtual void ScenarioBegin();

	/** Called when the scenario ended */
	virtual void ScenarioEnd();

	virtual AActor* AsActor();
	virtual const AActor* AsActor() const;

	virtual bool ShowSelectBox() const { return true; }

	virtual void SetActorHiddenInScenario(bool bHiddenInScenario) {}
	virtual bool GetActorHiddenInScenario() const { return false; }

	// TODO: may by add these in the future

	//virtual bool HandleClick(USodaGameViewportClient* ViewportClient, UActorComponent* ActorComponent, const FEditorUtils::FViewportClick& Click) { return false; }
	//virtual bool GetWidgetLocation(const USodaGameViewportClient* ViewportClient, FVector& OutLocation) const { return false; }
	//virtual bool GetCustomInputCoordinateSystem(const USodaGameViewportClient* ViewportClient, FMatrix& OutMatrix) const { return false; }
	//virtual bool HandleInputDelta(USodaGameViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) { return false; }
	//virtual bool HandleInputKey(USodaGameViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) { return false; }
	//virtual void StartEditing() {}
	//virtual void EndEditing() {}
	//virtual TSharedPtr<SWidget> GenerateContextMenu() const { return SNullWidget::NullWidget; }
	virtual void InputWidgetDelta(const USceneComponent* WidgetTargetComponent, FTransform & NewWidgetTransform) {}
	virtual void DrawVisualization(USodaGameViewportClient* ViewportClient, const FSceneView* View, FPrimitiveDrawInterface* PDI) {}

public:
	/* Override from IEditableObject */
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	//virtual void RuntimePreEditChange(FProperty* PropertyAboutToChange) override;
	//virtual void RuntimePreEditChange(FEditPropertyChain& PropertyAboutToChange) override;

protected:
	bool bIsDirty = false;

};

