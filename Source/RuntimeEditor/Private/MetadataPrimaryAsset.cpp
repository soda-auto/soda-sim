// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "MetadataPrimaryAsset.h"
#include "UObject/ObjectSaveContext.h"
#include "RuntimeMetaData.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

FPrimaryAssetId UMetadataPrimaryAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FName("MetadataPrimaryAsset"), FPackageName::GetShortFName(GetOutermost()->GetName()));
}

#if WITH_EDITOR
bool UMetadataPrimaryAsset::AddStructToMetadataGlobalScope(const UStruct* Struct)
{
	check(Struct);
	UPackage* Package = Struct->GetOutermost();
	check(Package);
	UMetaData* MetaData = Package->GetMetaData();
	check(MetaData);

	// Check store object
	bool StoreObject = Struct->HasMetaData(TEXT("RuntimeMetaData"));
	if (!StoreObject)
	{
		for (TFieldIterator<FField> FieldIt(Struct); FieldIt; ++FieldIt)
		{
			FField* Field = *FieldIt;
			if (const TMap<FName, FString>* FieldValues = Field->GetMetaDataMap())
			{
				bool StoreField = false;
				for (const auto& MetaDataIt : *FieldValues)
				{
					if (MetaDataIt.Key.ToString() == TEXT("EditInRuntime") || MetaDataIt.Key.ToString() == TEXT("CallInRuntime"))
					{
						StoreObject = true;
						break;
					}
				}
				if (StoreObject) break;
			}
		}
	}

	if (!StoreObject)
	{
		for (TFieldIterator<UField> FieldIt(Struct); FieldIt; ++FieldIt)
		{
			UField* Field = *FieldIt;
			if (TMap<FName, FString>* FieldValues = MetaData->ObjectMetaDataMap.Find(Field))
			{
				for (const auto& MetaDataIt : *FieldValues)
				{
					if (MetaDataIt.Key.ToString() == TEXT("EditInRuntime") || MetaDataIt.Key.ToString() == TEXT("CallInRuntime"))
					{
						StoreObject = true;
						break;
					}
				}
			}
			if (StoreObject) break;
		}
	}

	if (!StoreObject)
	{
		return false;
	}

	// Store metadata
	FRuntimeMetadataObject& MetadataObject = MetadataGlobalScope.FindOrAdd(Struct->GetName());
	for (TFieldIterator<FField> FieldIt(Struct); FieldIt; ++FieldIt)
	{
		FField* Field = *FieldIt;
		if (const TMap<FName, FString>* FieldValues = Field->GetMetaDataMap())
		{
			FRuntimeMetadataField& MetadataField = MetadataObject.Fields.FindOrAdd(Field->GetName());
			MetadataField.DisplayName = Field->GetDisplayNameText().ToString();
			//MetadataField.ToolTip = Field->GetToolTipText().ToString();
			for (const auto& MetaDataIt : *FieldValues)
			{
				MetadataField.MetadataMap.Add(MetaDataIt.Key.ToString(), MetaDataIt.Value);
			}
		}
	}
	for (TFieldIterator<UField> FieldIt(Struct); FieldIt; ++FieldIt)
	{
		UField* Field = *FieldIt;
		if (TMap<FName, FString>* FieldValues = MetaData->ObjectMetaDataMap.Find(Field))
		{
			FRuntimeMetadataField& MetadataField = MetadataObject.Fields.FindOrAdd(Field->GetName());
			MetadataField.DisplayName = Field->GetDisplayNameText().ToString();
			//MetadataField.ToolTip = Field->GetToolTipText().ToString();
			for (const auto& MetaDataIt : *FieldValues)
			{
				MetadataField.MetadataMap.Add(MetaDataIt.Key.ToString(), MetaDataIt.Value);
			}
		}
	}
	return true;
}

void UMetadataPrimaryAsset::UpdateMetadataGlobalScope()
{
	MetadataGlobalScope.Empty();

	UE_LOG(LogMetaData, Display, TEXT("UMetadataPrimaryAsset::UpdateMetadataGlobalScope(): "));

	for (TObjectIterator<UStruct> It; It; ++It)
	{
		if (bSaveAllCPPMetadata)
		{
			if (!It->IsChildOf(UBlueprintGeneratedClass::StaticClass()))
			{
				if (AddStructToMetadataGlobalScope(*It))
				{
					UE_LOG(LogMetaData, Display, TEXT("   \"%s\""), *It->GetName());
				}
			}
		}
	}
}

#endif

void UMetadataPrimaryAsset::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);
	MetadataGlobalScope.Empty();
}

void UMetadataPrimaryAsset::PostLoad()
{
	Super::PostLoad();

	UE_LOG(LogMetaData, Display, TEXT("UMetadataPrimaryAsset::PostLoad();"));

#if WITH_EDITOR
	UpdateMetadataGlobalScope();
#endif

	
	for (auto & It : MetadataGlobalScope)
	{
		UE_LOG(LogMetaData, Display, TEXT("   \"%s\""), *It.Key);
	}
	
}

void UMetadataPrimaryAsset::Serialize(FArchive& Ar)
{
#if WITH_EDITOR
	if (Ar.IsCooking())
	{
		UE_LOG(LogMetaData, Display, TEXT("UMetadataPrimaryAsset::Serialize(FArchive); Cooking..."));
		UpdateMetadataGlobalScope();
	}
#endif

	Super::Serialize(Ar);
}