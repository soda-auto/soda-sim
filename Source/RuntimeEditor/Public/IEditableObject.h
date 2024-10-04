// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Widgets/SWidget.h"
#include "Templates/SharedPointer.h"
#include "IEditableObject.generated.h"

USTRUCT(BlueprintType)
struct RUNTIMEEDITOR_API FRuntimeMetadataField
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	TMap<FString, FString> MetadataMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	FString ToolTip_Depricated;

};

FArchive& operator<<(FArchive& Ar, FRuntimeMetadataField& This);
void operator << (FStructuredArchive::FSlot Slot, FRuntimeMetadataField& This);

USTRUCT(BlueprintType)
struct RUNTIMEEDITOR_API FRuntimeMetadataObject
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	TMap<FString, FRuntimeMetadataField> Fields;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	TMap<FString, FString> MetadataMap;
};

FArchive& operator<<(FArchive& Ar, FRuntimeMetadataObject& This);
void operator << (FStructuredArchive::FSlot Slot, FRuntimeMetadataObject& This);

/**
 * FPropertyChangedEventBP
 * Just a wrapper for FPropertyChangedEvent and FPropertyChangedChainEvent for use them in BP
 */
USTRUCT(BlueprintType)
struct FPropertyChangedEventBP
{
	GENERATED_BODY()

	FPropertyChangedEventBP()
	{}

	FPropertyChangedEventBP(FPropertyChangedEvent* InPropertyChangedEvent) :
		PropertyChangedEvent(InPropertyChangedEvent)
	{}

	FPropertyChangedEventBP(FPropertyChangedChainEvent* InPropertyChangedChainEvent) :
		PropertyChangedChainEvent(InPropertyChangedChainEvent)
	{}

	FPropertyChangedEvent* PropertyChangedEvent = nullptr;
	FPropertyChangedChainEvent * PropertyChangedChainEvent = nullptr;
};

UINTERFACE(BlueprintType)
class RUNTIMEEDITOR_API UEditableObject: public UInterface
{
	GENERATED_BODY()
};

/**
 * IEditableObject
 * The purpose of this interface is to "bring back" some functions from Object that are only available in editor mode to runtime mode.
 * All "brougt back" functions have the prefix "Runtime". For example PostEditChangeProperty -> RuntimePostEditChangeProperty
 */
class RUNTIMEEDITOR_API IEditableObject
{
	GENERATED_BODY()

public:
	/**
	 * Same as UObject::RuntimePostEditChangeProperty(FPropertyChangedEvent& ) but for the runtime
	 */
	virtual void RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	/**
	 * Same as UObject::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& ) but for the runtime
	 */
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent);

	/**
	 * Same as UObject::RuntimePreEditChange(FProperty& ) but for the runtime
	 */
	virtual void RuntimePreEditChange(FProperty* PropertyAboutToChange);

	/**
	 * Same as UObject::CanEditChange(FEditPropertyChain* ) but for the runtime
	 */
	virtual void RuntimePreEditChange(FEditPropertyChain& PropertyAboutToChange);

	/**
	 * Same as UObject::CanEditChange( const FProperty* ) but for the runtime
	 */
	virtual bool RuntimeCanEditChange(const FProperty* InProperty) const;

	/**
	 * Same as UObject::CanEditChange( const FEditPropertyChain* ) but for the runtime
	 */
	virtual bool RuntimeCanEditChange(const FEditPropertyChain& PropertyChain) const;

	/**
	 * Generate additional widget in the head in soda::IDetailsView
	 */
	virtual TSharedPtr<SWidget> GenerateToolBar();

	/** Add addition metadata for object */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Editable Object")
	void GenerateMetadata(FRuntimeMetadataObject & Metadata);

	/** 
	 * BP version of RuntimePostEditChangeProperty and RuntimePostEditChangeChainProperty. 
	 * Avalible only if this object inherited from C++ IEditableObject  
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Editable Object", meta=(DisplayName = "PostEditChangeProperty"))
	void ReceivePostEditChangeProperty(const FPropertyChangedEventBP & PropertyChangedEven);
};

