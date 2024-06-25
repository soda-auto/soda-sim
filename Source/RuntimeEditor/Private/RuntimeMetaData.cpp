// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimeMetaData.h"
#include "MetadataPrimaryAsset.h"
#include "Engine/AssetManager.h"
#include "RuntimeEditorUtils.h"

DEFINE_LOG_CATEGORY(LogMetaData);

//#define RUNTIME_METADATA_DEBUG

TMap<FString, FRuntimeMetadataObject> FRuntimeMetaData::MetadataGlobalScope;

FRuntimeMetadataField* FRuntimeMetaData::FindField(const UField* Field)
{
	UObject* Owner = Field->GetOuter();
	if (FRuntimeMetadataObject* Metadata = MetadataGlobalScope.Find(Owner->GetName()))
	{
		return Metadata->Fields.Find(Field->GetName());
	}
	return nullptr;
}

FRuntimeMetadataField* FRuntimeMetaData::FindField(const FField* Field)
{
	UObject* Owner = Field->GetOwnerUObject();
	if (FRuntimeMetadataObject* Metadata = MetadataGlobalScope.Find(Owner->GetName()))
	{
		return Metadata->Fields.Find(Field->GetName());
	}
	return nullptr;
}

bool FRuntimeMetaData::AddGeneratedMetadataToGlobalScope(UClass* Class)
{
	if (!Class->ImplementsInterface(UEditableObject::StaticClass()))
	{
		return false;
	}

	FRuntimeMetadataObject DummyMetadataObject;

	while(Class && Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		if (Class->ImplementsInterface(UEditableObject::StaticClass()))
		{
			FRuntimeMetadataObject MetadataObject;
			IEditableObject::Execute_GenerateMetadata(Class->GetDefaultObject(), MetadataObject);

			MetadataObject.Fields.Append(DummyMetadataObject.Fields);
			MetadataGlobalScope.FindOrAdd(Class->GetName()) = MetadataObject;
			DummyMetadataObject.Fields = MetadataObject.Fields;
		}
		else
		{
			MetadataGlobalScope.FindOrAdd(Class->GetName()) = DummyMetadataObject;
		}

#ifdef RUNTIME_METADATA_DEBUG
		for (TFieldIterator<FField> FieldIt(Class); FieldIt; ++FieldIt)
		{
			FField* Field = *FieldIt;
			if (FRuntimeMetadataField* MetadataField = MetadataObject.Fields.Find(Field->GetName()))
			{
				for (auto& It : MetadataField->MetadataMap)
				{
					Field->SetMetaData(*It.Key, *It.Value);
				}
			}
		}
		for (TFieldIterator<UField> FieldIt(Class); FieldIt; ++FieldIt)
		{
			UField* Field = *FieldIt;
			if (FRuntimeMetadataField* MetadataField = MetadataObject.Fields.Find(Field->GetName()))
			{
				for (auto& It : MetadataField->MetadataMap)
				{
					Field->SetMetaData(*It.Key, *It.Value);
				}
			}
		}
#endif // RUNTIME_METADATA_DEBUG

		Class = Cast<UClass>(Class->GetInheritanceSuper());
	}

	return true;
}

const FString * FRuntimeMetaData::TryGetMetaData(const UField* Field, const TCHAR* Key)
{
	if (const FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		return MetadataField->MetadataMap.Find(Key);
	}
	return nullptr;
}

const FString* FRuntimeMetaData::TryGetMetaData(const FField* Field, const TCHAR* Key)
{
	if (const FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		return MetadataField->MetadataMap.Find(Key);
	}
	return nullptr;
}

void FRuntimeMetaData::RegisterMetadataPrimaryAssets()
{
	MetadataGlobalScope.Empty();

	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> PrimaryAssetIdList;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType("MetadataPrimaryAsset"), PrimaryAssetIdList);
	TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAssets(PrimaryAssetIdList);

	TArray<UObject*> LoadedAssets;
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
		Handle->GetLoadedAssets(LoadedAssets);
	}
	else
	{
		for (const auto& It : PrimaryAssetIdList)
		{
			if (UObject* Obj = AssetManager.GetPrimaryAssetObject(It))
			{
				LoadedAssets.Add(Obj);
			}
		}
	}

	for (auto& LoadedAsset : LoadedAssets)
	{
		if (UClass* MetadataPrimaryAsset = Cast<UClass>(LoadedAsset))
		{
			UE_LOG(LogMetaData, Log, TEXT("FRuntimeMetaData::RegisterMetadataPrimaryAssets(); Loaded asset %s"), *MetadataPrimaryAsset->GetName());
			MetadataGlobalScope.Append(CastChecked<UMetadataPrimaryAsset>(MetadataPrimaryAsset->GetDefaultObject())->GetMetadataScope());
		}
	}
}

/*
void FRuntimeMetaData::PrimaryAssetsLoaded(FPrimaryAssetId AssetId)
{
	UAssetManager& AssetManager = UAssetManager::Get();
	UClass* AssetObjectClass = AssetManager.GetPrimaryAssetObjectClass<UMetadataPrimaryAsset>(AssetId);
	if (AssetObjectClass)
	{
		UE_LOG(LogMetaData, Log, TEXT("FRuntimeMetaData::RegisterMetadataPrimaryAssets(); Loaded asset %s"), *AssetObjectClass->GetName());
		MetadataGlobalScope.Append(CastChecked<UMetadataPrimaryAsset>(AssetObjectClass->GetDefaultObject())->GetMetadataScope());
	}
	else
	{
		UE_LOG(LogMetaData, Error, TEXT("FRuntimeMetaData::RegisterMetadataPrimaryAssets(); Primary asset \"%s\" load faild"), *AssetId.ToString());
	}
}
*/

void FRuntimeMetaData::CopyMetaData(const FField* InSourceField, FField* InDestField)
{
	const FRuntimeMetadataField* InMetadataSourceField = FindField(InSourceField);
	FRuntimeMetadataField* InMetadataDestField = FindField(InDestField);

	if (InMetadataSourceField && InMetadataDestField)
	{
		*InMetadataDestField = *InMetadataDestField;
	}
}

#ifdef RUNTIME_METADATA_DEBUG

const FString* FRuntimeMetaData::FindMetaData(const FField* Field, const TCHAR* Key) { return Field->FindMetaData(Key); }
const FString* FRuntimeMetaData::FindMetaData(const FField* Field, const FName& Key) { return Field->FindMetaData(Key); }
const FString* FRuntimeMetaData::FindMetaData(const UField* Field, const TCHAR* Key) { return Field->FindMetaData(Key); }
const FString* FRuntimeMetaData::FindMetaData(const UField* Field, const FName& Key) { return Field->FindMetaData(Key); }
bool FRuntimeMetaData::HasMetaData(const UField* Field, const TCHAR* Key) { return Field->HasMetaData(Key); }
bool FRuntimeMetaData::HasMetaData(const UField* Field, const FName& Key) { return Field->HasMetaData(Key); }
bool FRuntimeMetaData::HasMetaData(const FField* Field, const TCHAR* Key) { return Field->HasMetaData(Key); }
bool FRuntimeMetaData::HasMetaData(const FField* Field, const FName& Key) { return Field->HasMetaData(Key); }
const FString& FRuntimeMetaData::GetMetaData(const UField* Field, const TCHAR* Key) { return Field->GetMetaData(Key); }
const FString& FRuntimeMetaData::GetMetaData(const FField* Field, const TCHAR* Key) { return Field->GetMetaData(Key); }
const FString& FRuntimeMetaData::GetMetaData(const UField* Field, const FName& Key) { return Field->GetMetaData(Key); }
const FString& FRuntimeMetaData::GetMetaData(const FField* Field, const FName& Key) { return Field->GetMetaData(Key); }
void FRuntimeMetaData::SetMetaData(FField* Field, const TCHAR* Key, const TCHAR* InValue) { FField->SetMetaData(Key, InValue); }
void FRuntimeMetaData::SetMetaData(FField* Field, const FName& Key, const TCHAR* InValue) { FField->SetMetaData(Key, InValue); }
void FRuntimeMetaData::SetMetaData(FField* Field, const TCHAR* Key, FString&& InValue) { FField->SetMetaData(Key, InValue); }
void FRuntimeMetaData::SetMetaData(FField* Field, const FName& Key, FString&& InValue) { FField->SetMetaData(Key, InValue); }
void FRuntimeMetaData::SetMetaData(UField* Field, const TCHAR* Key, const TCHAR* InValue) { FField->SetMetaData(Key, InValue); }
void FRuntimeMetaData::SetMetaData(UField* Field, const FName& Key, const TCHAR* InValue) { FField->SetMetaData(Key, InValue); }
FText FRuntimeMetaData::GetDisplayNameText(const FField* Field) { return Field->GetDisplayNameText(); }
FText FRuntimeMetaData::GetDisplayNameText(const UField* Field) { return Field->GetDisplayNameText(); }
FText FRuntimeMetaData::GetToolTipText(const UField* Field) { return Field->GetToolTipText(); }
FText FRuntimeMetaData::GetToolTipText(const FField* Field) { return Field->GetToolTipText(); }
bool FRuntimeMetaData::GetBoolMetaData(const UField* Field, const TCHAR* Key) { return Field->GetBoolMetaData(Key); }
bool FRuntimeMetaData::GetBoolMetaData(const UField* Field, const FName& Key) { return Field->GetBoolMetaData(Key); }
bool FRuntimeMetaData::GetBoolMetaData(const FField* Field, const TCHAR* Key) { return Field->GetBoolMetaData(Key); }
bool FRuntimeMetaData::GetBoolMetaData(const FField* Field, const FName& Key) { return Field->GetBoolMetaData(Key); }
UClass* FRuntimeMetaData::GetClassMetaData(const FField* Field, const TCHAR* Key) { return Field->GetClassMetaData(Key); }
UClass* FRuntimeMetaData::GetClassMetaData(const FField* Field, const FName& Key) { return Field->GetClassMetaData(Key); }
float FRuntimeMetaData::GetFloatMetaData(const FField* Field, const TCHAR* Key) { return Field->GetFloatMetaData(Key); }
float FRuntimeMetaData::GetFloatMetaData(const FField* Field, const FName& Key) { return Field->GetFloatMetaData(Key); }
int32 FRuntimeMetaData::GetIntMetaData(const FField* Field, const TCHAR* Key) { return Field->GetIntMetaData(Key); }
int32 FRuntimeMetaData::GetIntMetaData(const FField* Field, const FName& Key) { return Field->GetIntMetaData(Key); }
bool FRuntimeMetaData::IsAutoExpandCategory(const UClass* Class, const TCHAR* InCategory) { return Class->IsAutoExpandCategory(InCategory); }
bool FRuntimeMetaData::IsAutoCollapseCategory(const UClass* Class, const TCHAR* InCategory) { return Class->IsAutoCollapseCategory(InCategory); }
bool FRuntimeMetaData::HasMetaData(const UEnum* Enum, const TCHAR* Key, int32 NameIndex) { return Enum->HasMetaData(Key, NameIndex); };
FString FRuntimeMetaData::GetMetaData(const UEnum* Enum, const TCHAR* Key, int32 NameIndex, bool bAllowRemap) { return Enum->GetMetaData(Key, NameIndex, bAllowRemap); }
FText FRuntimeMetaData::GetToolTipTextByIndex(const UEnum* Enum, int32 NameIndex) { return Enum->GetToolTipTextByIndex(NameIndex); };
FText FRuntimeMetaData::GetDisplayNameTextByIndex(const UEnum* Enum, int32 NameIndex) { return Enum->GetDisplayNameTextByIndex(NameIndex); };
void FRuntimeMetaData::SetMetaData(UEnum* Enum, const TCHAR* Key, const TCHAR* InValue, int32 NameIndex) { Enum->SetMetaData(Key, InValue, NameIndex); }

#else // RUNTIME_METADATA_DEBUG

static FString GetFieldDisplayName(const FField& Object)
{
	if (const FProperty* Property = CastField<FProperty>(&Object))
	{
		if (auto OwnerStruct = Property->GetOwnerStruct())
		{
			return OwnerStruct->GetAuthoredNameForField(Property);
		}
	}

	return Object.GetName();
}

static FString GetFieldDisplayName(const UObject& Object)
{
	const UClass* Class = Cast<const UClass>(&Object);
	if (Class && !Class->HasAnyClassFlags(CLASS_Native))
	{
		FString Name = Object.GetName();
		Name.RemoveFromEnd(TEXT("_C"));
		Name.RemoveFromStart(TEXT("SKEL_"));
		return Name;
	}

	//if (auto Property = dynamic_cast<const FProperty*>(&Object))
	//{
	//	return Property->GetAuthoredName();
	//}

	return Object.GetName();
}

static FString GetFieldFullGroupName(const FField& Object, bool bStartWithOuter)
{
	if (bStartWithOuter)
	{
		if (Object.Owner.IsValid())
		{
			if (Object.Owner.IsUObject())
			{
				return Object.Owner.ToUObject()->GetPathName(Object.Owner.ToUObject()->GetOutermost());
			}
			else
			{
				return Object.Owner.ToField()->GetPathName(Object.GetOutermost());
			}
		}
		else
		{
			return FString();
		}
	}
	else
	{
		return Object.GetPathName(Object.GetOutermost());
	}
}

static void FormatNativeToolTip(const UField * Field, FString& ToolTipString, bool bRemoveExtraSections)
{
	// First do doxygen replace
	static const FString DoxygenSee(TEXT("@see"));
	static const FString TooltipSee(TEXT("See:"));
	ToolTipString.ReplaceInline(*DoxygenSee, *TooltipSee);

	bool bCurrentLineIsEmpty = true;
	int32 EmptyLineCount = 0;
	int32 LastContentIndex = INDEX_NONE;
	const int32 ToolTipLength = ToolTipString.Len();

	// Start looking for empty lines and whitespace to strip
	for (int32 StrIndex = 0; StrIndex < ToolTipLength; StrIndex++)
	{
		TCHAR CurrentChar = ToolTipString[StrIndex];

		if (!FChar::IsWhitespace(CurrentChar))
		{
			if (FChar::IsPunct(CurrentChar))
			{
				// Punctuation is considered content if it's on a line with alphanumeric text
				if (!bCurrentLineIsEmpty)
				{
					LastContentIndex = StrIndex;
				}
			}
			else
			{
				// This is something alphanumeric, this is always content and mark line as not empty
				bCurrentLineIsEmpty = false;
				LastContentIndex = StrIndex;
			}
		}
		else if (CurrentChar == TEXT('\n'))
		{
			if (bCurrentLineIsEmpty)
			{
				EmptyLineCount++;
				if (bRemoveExtraSections && EmptyLineCount >= 2)
				{
					// If we get two empty or punctuation/separator lines in a row, cut off the string if requested
					break;
				}
			}
			else
			{
				EmptyLineCount = 0;
			}

			bCurrentLineIsEmpty = true;
		}
	}

	// Trim string to last content character, this strips trailing whitespace as well as extra sections if needed
	if (LastContentIndex >= 0 && LastContentIndex != ToolTipLength - 1)
	{
		ToolTipString.RemoveAt(LastContentIndex + 1, ToolTipLength - (LastContentIndex + 1));
	}
}

const FString* FRuntimeMetaData::FindMetaData(const FField* Field, const TCHAR* Key) 
{
	if (const FRuntimeMetadataField * MetadataField = FindField(Field))
	{
		return MetadataField->MetadataMap.Find(Key);
	}
	return nullptr;
}
const FString* FRuntimeMetaData::FindMetaData(const FField* Field, const FName& Key) 
{
	if (const FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		return MetadataField->MetadataMap.Find(Key.ToString());
	}
	return nullptr;
}
const FString* FRuntimeMetaData::FindMetaData(const UField* Field, const TCHAR* Key) 
{
	if (const FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		return MetadataField->MetadataMap.Find(Key);
	}
	return nullptr;
}
const FString* FRuntimeMetaData::FindMetaData(const UField* Field, const FName& Key) 
{
	if (const FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		return MetadataField->MetadataMap.Find(Key.ToString());
	}
	return nullptr;
}
FText FRuntimeMetaData::GetDisplayNameText(const UField* Field)
{
	FText LocalizedDisplayName;

	static const FString Namespace = TEXT("UObjectDisplayNames");
	static const FName NAME_DisplayName(TEXT("DisplayName"));

	const FString Key = Field->GetFullGroupName(false);

	FString NativeDisplayName = GetMetaData(Field, NAME_DisplayName);
	if (NativeDisplayName.IsEmpty())
	{
		NativeDisplayName = FName::NameToDisplayString(FRuntimeEditorUtils::GetDisplayNameHelper(*Field), false);
	}

	if (!(FText::FindText(Namespace, Key, /*OUT*/LocalizedDisplayName, &NativeDisplayName)))
	{
		LocalizedDisplayName = FText::FromString(NativeDisplayName);
	}

	return LocalizedDisplayName;
}
FText FRuntimeMetaData::GetToolTipText(const UField* Field, bool bShortTooltip)
{
	bool bFoundShortTooltip = false;
	static const FName NAME_Tooltip(TEXT("Tooltip"));
	static const FName NAME_ShortTooltip(TEXT("ShortTooltip"));
	FText LocalizedToolTip;
	FString NativeToolTip;

	if (bShortTooltip)
	{
		NativeToolTip = GetMetaData(Field, NAME_ShortTooltip);
		if (NativeToolTip.IsEmpty())
		{
			NativeToolTip = GetMetaData(Field, NAME_Tooltip);
		}
		else
		{
			bFoundShortTooltip = true;
		}
	}
	else
	{
		NativeToolTip = GetMetaData(Field, NAME_Tooltip);
	}

	const FString Namespace = bFoundShortTooltip ? TEXT("UObjectShortTooltips") : TEXT("UObjectToolTips");
	const FString Key = Field->GetFullGroupName(false);
	if (!FText::FindText(Namespace, Key, /*OUT*/LocalizedToolTip, &NativeToolTip))
	{
		if (NativeToolTip.IsEmpty())
		{
			NativeToolTip = FName::NameToDisplayString(GetFieldDisplayName(*Field), false);
		}
		else if (!bShortTooltip && Field->IsNative())
		{
			FormatNativeToolTip(Field, NativeToolTip, true);
		}
		LocalizedToolTip = FText::FromString(NativeToolTip);
	}

	return LocalizedToolTip;
}
bool FRuntimeMetaData::HasMetaData(const UField* Field, const TCHAR* Key)
{
	return TryGetMetaData(Field, Key) != nullptr;
}
const FString& FRuntimeMetaData::GetMetaData(const UField* Field, const TCHAR* Key)
{
	static FString EmptyString;
	const FString * Ret = TryGetMetaData(Field, Key);
	return Ret ? *Ret : EmptyString;
}
bool FRuntimeMetaData::HasMetaData(const UField* Field, const FName& Key)
{
	return TryGetMetaData(Field, *Key.ToString()) != nullptr;
}
const FString& FRuntimeMetaData::GetMetaData(const UField* Field, const FName& Key)
{
	static FString EmptyString;
	const FString* Ret = TryGetMetaData(Field, *Key.ToString());
	return Ret ? *Ret : EmptyString;
}
FText FRuntimeMetaData::GetDisplayNameText(const FField* Field)
{
	FText LocalizedDisplayName;

	static const FString Namespace = TEXT("UObjectDisplayNames");
	static const FName NAME_DisplayName(TEXT("DisplayName"));

	const FString Key = Field->GetPathName(Field->GetOutermost()); //Field->GetFullGroupName(false);

	FString NativeDisplayName;
	if (const FString* FoundMetaData = FindMetaData(Field, NAME_DisplayName))
	{
		NativeDisplayName = *FoundMetaData;
	}
	else
	{
		NativeDisplayName = FName::NameToDisplayString(FRuntimeEditorUtils::GetDisplayNameHelper(*Field), Field->IsA<FBoolProperty>());
	}

	if (!(FText::FindText(Namespace, Key, /*OUT*/LocalizedDisplayName, &NativeDisplayName)))
	{
		LocalizedDisplayName = FText::FromString(NativeDisplayName);
	}

	return LocalizedDisplayName;
}
FText FRuntimeMetaData::GetToolTipText(const FField* Field, bool bShortTooltip)
{
	bool bFoundShortTooltip = false;
	static const FName NAME_Tooltip(TEXT("Tooltip"));
	static const FName NAME_ShortTooltip(TEXT("ShortTooltip"));
	FText LocalizedToolTip;
	FString NativeToolTip;

	if (bShortTooltip)
	{
		NativeToolTip = GetMetaData(Field, NAME_ShortTooltip);
		if (NativeToolTip.IsEmpty())
		{
			NativeToolTip = GetMetaData(Field, NAME_Tooltip);
		}
		else
		{
			bFoundShortTooltip = true;
		}
	}
	else
	{
		NativeToolTip = GetMetaData(Field, NAME_Tooltip);
	}

	const FString Namespace = bFoundShortTooltip ? TEXT("UObjectShortTooltips") : TEXT("UObjectToolTips");
	const FString Key = GetFieldFullGroupName(*Field, false);
	if (!FText::FindText(Namespace, Key, /*OUT*/LocalizedToolTip, &NativeToolTip))
	{
		if (!NativeToolTip.IsEmpty())
		{
			static const FString DoxygenSee(TEXT("@see"));
			static const FString TooltipSee(TEXT("See:"));
			if (NativeToolTip.ReplaceInline(*DoxygenSee, *TooltipSee) > 0)
			{
				NativeToolTip.TrimEndInline();
			}
		}
		LocalizedToolTip = FText::FromString(NativeToolTip);
	}

	const FText DisplayName = FText::FromString(FName::NameToDisplayString(GetFieldDisplayName(*Field), Field->IsA<FBoolProperty>()));
	if (LocalizedToolTip.IsEmpty())
	{
		LocalizedToolTip = DisplayName;
	}
	else
	{
		LocalizedToolTip = FText::Join(FText::FromString(TEXT(":" LINE_TERMINATOR_ANSI)), DisplayName, LocalizedToolTip);
	}

	return LocalizedToolTip;
	

}
bool FRuntimeMetaData::HasMetaData(const FField* Field, const TCHAR* Key)
{
	return TryGetMetaData(Field, Key) != nullptr;
}
const FString& FRuntimeMetaData::GetMetaData(const FField* Field, const TCHAR* Key)
{
	static FString EmptyString;
	const FString* Ret = TryGetMetaData(Field, Key);
	return Ret ? *Ret : EmptyString;
}
bool FRuntimeMetaData::HasMetaData(const FField* Field, const FName& Key)
{
	return TryGetMetaData(Field, *Key.ToString()) != nullptr;
}
const FString& FRuntimeMetaData::GetMetaData(const FField* Field, const FName& Key)
{
	static FString EmptyString;
	const FString* Ret = TryGetMetaData(Field, *Key.ToString());
	return Ret ? *Ret : EmptyString;
}
bool FRuntimeMetaData::GetBoolMetaData(const UField* Field, const TCHAR* Key)
{
	if (const FString* BoolString = TryGetMetaData(Field, Key))
	{
		return (BoolString->IsEmpty() || *BoolString == "true");
	}
	return false;

}
bool FRuntimeMetaData::GetBoolMetaData(const UField* Field, const FName& Key)
{
	if (const FString* BoolString = TryGetMetaData(Field, *Key.ToString()))
	{
		return (BoolString->IsEmpty() || *BoolString == "true");
	}
	return false;
}
bool FRuntimeMetaData::GetBoolMetaData(const FField* Field, const TCHAR* Key)
{
	if (const FString* BoolString = TryGetMetaData(Field, Key))
	{
		return (BoolString->IsEmpty() || *BoolString == "true");
	}
	return false;
}
bool FRuntimeMetaData::GetBoolMetaData(const FField* Field, const FName& Key)
{
	if (const FString* BoolString = TryGetMetaData(Field, *Key.ToString()))
	{
		return (BoolString->IsEmpty() || *BoolString == "true");
	}
	return false;
}
UClass* FRuntimeMetaData::GetClassMetaData(const FField* Field, const TCHAR* Key)
{
	const FString& ClassName = GetMetaData(Field, Key);
	UClass* const FoundObject = UClass::TryFindTypeSlow<UClass>(*ClassName, EFindFirstObjectOptions::ExactClass);
	return FoundObject;
}
UClass* FRuntimeMetaData::GetClassMetaData(const FField* Field, const FName& Key)
{
	const FString& ClassName = GetMetaData(Field, Key);
	UClass* const FoundObject = UClass::TryFindTypeSlow<UClass>(*ClassName, EFindFirstObjectOptions::ExactClass);
	return FoundObject;
}
float FRuntimeMetaData::GetFloatMetaData(const FField* Field, const TCHAR* Key)
{
	const FString& FLOATString = GetMetaData(Field, Key);
	float Value = FCString::Atof(*FLOATString);
	return Value;
}
float FRuntimeMetaData::GetFloatMetaData(const FField* Field, const FName& Key)
{
	const FString& FLOATString = GetMetaData(Field, Key);
	float Value = FCString::Atof(*FLOATString);
	return Value;
}
int32 FRuntimeMetaData::GetIntMetaData(const FField* Field, const TCHAR* Key)
{
	const FString& INTString = (const FString&)GetMetaData(Field, Key);
	int32 Value = FCString::Atoi(*INTString);
	return Value;
}
int32 FRuntimeMetaData::GetIntMetaData(const FField* Field, const FName& Key)
{
	const FString& INTString = GetMetaData(Field, Key);
	int32 Value = FCString::Atoi(*INTString);
	return Value;
}
bool FRuntimeMetaData::IsAutoExpandCategory(const UClass * Class, const TCHAR* InCategory)
{
	static const FName NAME_AutoExpandCategories(TEXT("AutoExpandCategories"));
	if (const FString* AutoExpandCategories = FindMetaData(Class, NAME_AutoExpandCategories))
	{
		return !!FCString::StrfindDelim(**AutoExpandCategories, InCategory, TEXT(" "));
	}
	return false;
}
bool FRuntimeMetaData::IsAutoCollapseCategory(const UClass* Class, const TCHAR* InCategory)
{
	static const FName NAME_AutoCollapseCategories(TEXT("AutoCollapseCategories"));
	if (const FString* AutoCollapseCategories = FindMetaData(Class, NAME_AutoCollapseCategories))
	{
		return !!FCString::StrfindDelim(**AutoCollapseCategories, InCategory, TEXT(" "));
	}
	return false;
}
bool FRuntimeMetaData::HasMetaData(const UEnum* Enum, const TCHAR* Key, int32 NameIndex)
{
	bool bResult = false;

	//UPackage* Package = GetOutermost();
	//check(Package);

	//UMetaData* MetaData = Package->GetMetaData();
	//check(MetaData);

	FString KeyString;

	// If an index was specified, search for metadata linked to a specified value
	if (NameIndex != INDEX_NONE)
	{
		KeyString = Enum->GetNameStringByIndex(NameIndex);
		KeyString.AppendChar(TEXT('.'));
		KeyString.Append(Key);
	}
	// If no index was specified, search for metadata for the enum itself
	else
	{
		KeyString = Key;
	}
	//bResult = MetaData->HasValue(this, *KeyString);
	bResult = HasMetaData(static_cast<const UField*>(Enum), *KeyString);

	return bResult;
}
FString FRuntimeMetaData::GetMetaData(const UEnum* Enum, const TCHAR* Key, int32 NameIndex, bool bAllowRemap)
{
	//UPackage* Package = GetOutermost();
	//check(Package);

	//UMetaData* MetaData = Package->GetMetaData();
	//check(MetaData);

	FString KeyString;

	// If an index was specified, search for metadata linked to a specified value
	if (NameIndex != INDEX_NONE)
	{
		check(Enum->NumEnums() > NameIndex);
		KeyString = Enum->GetNameStringByIndex(NameIndex) + TEXT(".") + Key;
	}
	// If no index was specified, search for metadata for the enum itself
	else
	{
		KeyString = Key;
	}

	//FString ResultString = MetaData->GetValue(this, *KeyString);
	FString ResultString = GetMetaData(static_cast<const UField*>(Enum), *KeyString);

	// look in the engine ini, in a section named after the metadata key we are looking for, and the enum's name (KeyString)
	/*
	if (bAllowRemap && ResultString.StartsWith(TEXT("ini:")))
	{
		if (!GConfig->GetString(TEXT("EnumRemap"), *KeyString, ResultString, GEngineIni))
		{
			// if this fails, then use what's after the ini:
			ResultString.MidInline(4, MAX_int32, false);
		}
	}
	*/

	return ResultString;
}
FText FRuntimeMetaData::GetToolTipTextByIndex(const UEnum* Enum, int32 NameIndex)
{
	FText LocalizedToolTip;
	FString NativeToolTip = GetMetaData(Enum, TEXT("ToolTip"), NameIndex);

	static const FString Namespace = TEXT("UObjectToolTips");

	FString Key = Enum->GetPathName(Enum->GetOutermost()) + TEXT(".") + Enum->GetNameStringByIndex(NameIndex);

	if (!FText::FindText(Namespace, Key, /*OUT*/LocalizedToolTip, &NativeToolTip))
	{
		static const FString DoxygenSee(TEXT("@see"));
		static const FString TooltipSee(TEXT("See:"));
		if (NativeToolTip.ReplaceInline(*DoxygenSee, *TooltipSee) > 0)
		{
			NativeToolTip.TrimEndInline();
		}

		LocalizedToolTip = FText::FromString(NativeToolTip);
	}

	return LocalizedToolTip;
}
FText FRuntimeMetaData::GetDisplayNameTextByIndex(const UEnum* Enum, int32 NameIndex)
{
	FString RawName = Enum->GetNameStringByIndex(NameIndex);

	if (RawName.IsEmpty())
	{
		return FText::GetEmpty();
	}

	//FText LocalizedDisplayName;
	// In the editor, use metadata and localization to look up names
	//static const FString Namespace = TEXT("UObjectDisplayNames");
	//const FString Key = GetFullGroupName(false) + TEXT(".") + RawName;

	//FString NativeDisplayName;
	//if (HasMetaData(Enum, TEXT("DisplayName"), NameIndex))
	//{
	//	NativeDisplayName = GetMetaData(Enum, TEXT("DisplayName"), NameIndex);
	//}
	//else
	//{
	//	NativeDisplayName = FName::NameToDisplayString(RawName, false);
	//}

	//if (!(FText::FindText(Namespace, Key, /*OUT*/LocalizedDisplayName, &NativeDisplayName)))
	//{
	//	LocalizedDisplayName = FText::FromString(NativeDisplayName);
	//}

	//if (!LocalizedDisplayName.IsEmpty())
	//{
	//	return LocalizedDisplayName;
	//}

	//if (EnumDisplayNameFn)
	//{
	//	return (*EnumDisplayNameFn)(NameIndex);
	//}

	return FText::FromString(Enum->GetNameStringByIndex(NameIndex));
}
void FRuntimeMetaData::SetMetaData(FField* Field, const TCHAR* Key, const TCHAR* InValue)
{
	if (FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		MetadataField->MetadataMap.Add(Key, InValue);
	}
}
void FRuntimeMetaData::SetMetaData(FField* Field, const FName& Key, const TCHAR* InValue)
{
	if (FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		MetadataField->MetadataMap.Add(Key.ToString(), InValue);
	}
}
void FRuntimeMetaData::SetMetaData(FField* Field, const TCHAR* Key, FString&& InValue)
{
	if (FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		MetadataField->MetadataMap.Add(Key, MoveTemp(InValue));
	}
}
void FRuntimeMetaData::SetMetaData(FField* Field, const FName& Key, FString&& InValue)
{
	if (FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		MetadataField->MetadataMap.Add(Key.ToString(), MoveTemp(InValue));
	}
}
void FRuntimeMetaData::SetMetaData(UField* Field, const TCHAR* Key, const TCHAR* InValue)
{
	if (FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		MetadataField->MetadataMap.Add(Key, MoveTemp(InValue));
	}
}
void FRuntimeMetaData::SetMetaData(UField* Field, const FName& Key, const TCHAR* InValue)
{
	if (FRuntimeMetadataField* MetadataField = FindField(Field))
	{
		MetadataField->MetadataMap.Add(Key.ToString(), MoveTemp(InValue));
	}
}
void FRuntimeMetaData::SetMetaData(UEnum* Enum, const TCHAR* Key, const TCHAR* InValue, int32 NameIndex)
{
	if (FRuntimeMetadataField* MetadataField = FindField(Enum))
	{
		FString KeyString;

		// If an index was specified, search for metadata linked to a specified value
		if (NameIndex != INDEX_NONE)
		{
			//check(Names.IsValidIndex(NameIndex));
			KeyString = Enum->GetNameStringByIndex(NameIndex) + TEXT(".") + Key;
		}
		// If no index was specified, search for metadata for the enum itself
		else
		{
			KeyString = Key;
		}
		MetadataField->MetadataMap.Add(KeyString, InValue);
	}
}
#endif // RUNTIME_METADATA_DEBUG