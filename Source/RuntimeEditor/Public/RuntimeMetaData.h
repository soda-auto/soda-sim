// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IEditableObject.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMetaData, Log, All);

class UMetadataPrimaryAsset; 

class RUNTIMEEDITOR_API FRuntimeMetaData
{
public:
	/* FField & UField */
	static const FString* FindMetaData(const FField* Field, const TCHAR* Key);
	static const FString* FindMetaData(const FField* Field, const FName& Key);
	static const FString* FindMetaData(const UField* Field, const TCHAR* Key);
	static const FString* FindMetaData(const UField* Field, const FName& Key);
	static bool HasMetaData(const UField* Field, const TCHAR* Key);
	static bool HasMetaData(const UField* Field, const FName& Key);
	static bool HasMetaData(const FField* Field, const TCHAR* Key);
	static bool HasMetaData(const FField* Field, const FName& Key);
	static const FString& GetMetaData(const UField* Field, const TCHAR* Key);
	static const FString& GetMetaData(const FField* Field, const TCHAR* Key);
	static const FString& GetMetaData(const UField* Field, const FName& Key);
	static const FString& GetMetaData(const FField* Field, const FName& Key);
	static FText GetDisplayNameText(const FField* Field);
	static FText GetDisplayNameText(const UField* Field);
	static FText GetToolTipText(const UField* Field, bool bShortTooltip);
	static FText GetToolTipText(const FField* Field, bool bShortTooltip);
	static bool GetBoolMetaData(const UField* Field, const TCHAR* Key);
	static bool GetBoolMetaData(const UField* Field, const FName& Key);
	static bool GetBoolMetaData(const FField* Field, const TCHAR* Key);
	static bool GetBoolMetaData(const FField* Field, const FName& Key);
	static UClass* GetClassMetaData(const FField* Field, const TCHAR* Key);
	static UClass* GetClassMetaData(const FField* Field, const FName& Key);
	static float GetFloatMetaData(const FField* Field, const TCHAR* Key);
	static float GetFloatMetaData(const FField* Field, const FName& Key);
	static int32 GetIntMetaData(const FField* Field, const TCHAR* Key);
	static int32 GetIntMetaData(const FField* Field, const FName& Key);

	/* UClass */
	static bool IsAutoExpandCategory(const UClass* Class, const TCHAR* InCategory);
	static bool IsAutoCollapseCategory(const UClass* Class, const TCHAR* InCategory);

	/* UEnum */
	static bool HasMetaData(const UEnum *Enum, const TCHAR* Key, int32 NameIndex = INDEX_NONE);
	static FString GetMetaData(const UEnum* Enum, const TCHAR* Key, int32 NameIndex = INDEX_NONE, bool bAllowRemap = true);
	static FText GetToolTipTextByIndex(const UEnum* Enum, int32 NameIndex);
	static FText GetDisplayNameTextByIndex(const UEnum* Enum, int32 NameIndex);

	/**  Add custom metadata generated from IEditableObject::GenerateMetadata() to the MetadataGlobalScope */
	static bool AddGeneratedMetadataToGlobalScope(UClass* Class);

	/** Find all UMetadataPrimaryAsset and add metadata to MetadataGlobalScope */
	static void RegisterMetadataPrimaryAssets(); 

private:
	static const FString * TryGetMetaData(const UField* Field, const TCHAR* Key);
	static const FString * TryGetMetaData(const FField* Field, const TCHAR* Key);

	static const FRuntimeMetadataField* FindField(const UField* Field);
	static const FRuntimeMetadataField* FindField(const FField* Field);

	//static void PrimaryAssetsLoaded(FPrimaryAssetId AssetId);

	static TMap<FString, FRuntimeMetadataObject> MetadataGlobalScope;
};
