// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "IEditableObject.h"
#include "RuntimeMetaDataStatic.generated.h"

USTRUCT(BlueprintType)
struct RUNTIMEEDITOR_API FRuntimeMetadataPropertyBuilder
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	FString ToolTip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	bool bIsInitialize;
};

USTRUCT(BlueprintType)
struct RUNTIMEEDITOR_API FRuntimeMetadataButtonBuilder
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	FString ToolTip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Metadata)
	bool bIsInitialize;
};

UCLASS(Category = "Soda")
class RUNTIMEEDITOR_API URuntimeMetaDataStatic: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "RuntimeMetaData")
	static void MakeRuntimeMetadataProperties(const FString& CategoryName, const TMap<FString, FRuntimeMetadataPropertyBuilder>& In, FRuntimeMetadataObject& Out);

	UFUNCTION(BlueprintPure, Category = "RuntimeMetaData")
	static void MakeRuntimeMetadataButtons(const FString& CategoryName, const TMap<FString, FRuntimeMetadataButtonBuilder>& In, FRuntimeMetadataObject& Out);

	UFUNCTION(BlueprintPure, Category = "RuntimeMetaData")
	static void MergeRuntimeMetadata(const TArray<FRuntimeMetadataObject> & In, FRuntimeMetadataObject& Out);
};
