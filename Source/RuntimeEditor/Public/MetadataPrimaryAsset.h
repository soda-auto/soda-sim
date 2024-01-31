// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/AssetManagerTypes.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataAsset.h"
#include "IEditableObject.h"
#include "MetadataPrimaryAsset.generated.h"

/**
 * UMetadataPrimaryAsset saved metadata for all CPP classes, structures, enums, properties and functuins during cooking of an uproject. 
 * To access the metadata use the appropriate methods in FRuntimeMetaData. 
 * TODO: Add filter to find metadata to save by path
 */
UCLASS(BlueprintType, Blueprintable)
class RUNTIMEEDITOR_API UMetadataPrimaryAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "MetadataPrimaryAsset")
	bool bSaveAllCPPMetadata = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual void Serialize(FArchive& Ar) override;
	//virtual void Serialize(FStructuredArchive::FRecord Record) override;
	virtual void PostLoad() override;

	virtual const TMap<FString, FRuntimeMetadataObject>& GetMetadataScope() const { return MetadataGlobalScope; }

protected:
#if WITH_EDITOR
	virtual bool AddStructToMetadataGlobalScope(const UStruct* Struct);
	virtual void UpdateMetadataGlobalScope();
#endif

	UPROPERTY()
	TMap<FString, FRuntimeMetadataObject> MetadataGlobalScope;
};