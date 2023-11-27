// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimeEditorUtils.h"
#include "HAL/FileManager.h"
#include "Widgets/Layout/SSpacer.h"
#include "SodaStyleSet.h"
#include "Widgets/Input/SHyperlink.h"
#include "RuntimeDocumentation/IDocumentation.h"
#include "RuntimeMetaData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Components/SceneComponent.h"
#include "Blueprint/BlueprintSupport.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Blueprint.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetData.h"

namespace FRuntimeEditorUtils
{

//------------------------------------------------------------------------------
FString GetDocumentationPage(const UClass* Class)
{
	return (Class ? FString::Printf( TEXT("Shared/Types/%s%s"), Class->GetPrefixCPP(), *Class->GetName() ) : FString());
}

//------------------------------------------------------------------------------
FString GetDocumentationExcerpt(const UClass* Class)
{
	return (Class ? FString::Printf( TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName() ) : FString());
}

//------------------------------------------------------------------------------
TSharedRef<SToolTip> GetTooltip(const UClass* Class)
{
	const bool bShowShortTooltips = true;
	return (Class ? GetTooltip(Class, FRuntimeMetaData::GetToolTipText(Class, bShowShortTooltips)) : SNew(SToolTip));
	//return (Class ? GetTooltip(Class, FText::GetEmpty()) : SNew(SToolTip));
}

//------------------------------------------------------------------------------
TSharedRef<SToolTip> GetTooltip(const UClass* Class, const TAttribute<FText>& OverrideText)
{
	return (Class ? soda::IDocumentation::Get()->CreateToolTip(OverrideText, nullptr, GetDocumentationPage(Class), GetDocumentationExcerpt(Class)) : SNew(SToolTip));
	//return (Class ? CreateDocToolTip(OverrideText, nullptr) : SNew(SToolTip));
}

//------------------------------------------------------------------------------
UClass* GetClassFromString(const FString& ClassName)
{
	if (ClassName.IsEmpty() || ClassName == TEXT("None"))
	{
		return nullptr;
	}

	UClass* Class = nullptr;
	if (!FPackageName::IsShortPackageName(ClassName))
	{
		Class = FindObject<UClass>(nullptr, *ClassName);
	}
	else
	{
		Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::None, ELogVerbosity::Warning, TEXT("FEditorClassUtils::GetClassFromString"));
	}
	if (!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassName);
	}
	return Class;
}

//------------------------------------------------------------------------------
bool ValidateActorName(const FText& InName, FText& OutErrorMessage)
{
	FText TrimmedLabel = FText::TrimPrecedingAndTrailing(InName);

	if (TrimmedLabel.IsEmpty())
	{
		OutErrorMessage = FText::FromString(TEXT("Names cannot be left blank"));
		return false;
	}

	if (TrimmedLabel.ToString().Len() >= NAME_SIZE)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("CharCount"), NAME_SIZE);
		OutErrorMessage = FText::Format(FText::FromString(TEXT("Names must be less than {CharCount} characters long.")), Arguments);
		return false;
	}

	if (FName(*TrimmedLabel.ToString()) == NAME_None)
	{
		OutErrorMessage = FText::FromString(TEXT("\"None\" is a reserved term and cannot be used for actor names"));
		return false;
	}

	return true;
}


//------------------------------------------------------------------------------
FText GetCategoryText(const FField* InField)
{
	static const FTextKey CategoryLocalizationNamespace = TEXT("UObjectCategory");
	static const FName CategoryMetaDataKey = TEXT("Category");

	if (InField)
	{
		const FString& NativeCategory = FRuntimeMetaData::GetMetaData(InField, CategoryMetaDataKey);
		if (!NativeCategory.IsEmpty())
		{
			FText LocalizedCategory;
			if (!FText::FindText(CategoryLocalizationNamespace, NativeCategory, /*OUT*/LocalizedCategory, &NativeCategory))
			{
				LocalizedCategory = FText::AsCultureInvariant(NativeCategory);
			}
			return LocalizedCategory;
		}
	}

	return FText::GetEmpty();
}

//------------------------------------------------------------------------------
FText GetCategoryText(const UField* InField)
{
	static const FTextKey CategoryLocalizationNamespace = TEXT("UObjectCategory");
	static const FName CategoryMetaDataKey = TEXT("Category");

	if (InField)
	{
		const FString& NativeCategory = FRuntimeMetaData::GetMetaData(InField, CategoryMetaDataKey);
		if (!NativeCategory.IsEmpty())
		{
			FText LocalizedCategory;
			if (!FText::FindText(CategoryLocalizationNamespace, NativeCategory, /*OUT*/LocalizedCategory, &NativeCategory))
			{
				LocalizedCategory = FText::AsCultureInvariant(NativeCategory);
			}
			return LocalizedCategory;
		}
	}

	return FText::GetEmpty();
}

//------------------------------------------------------------------------------
FString GetCategory(const FField* InField)
{
	return GetCategoryText(InField).ToString();
}

//------------------------------------------------------------------------------
FString GetCategory(const UField* InField)
{
	return GetCategoryText(InField).ToString();
}

//------------------------------------------------------------------------------
FName GetCategoryFName(const FField* InField)
{
	static const FName CategoryKey(TEXT("Category"));

	const FField* CurrentField = InField;
	FString CategoryString;
	while (CurrentField != nullptr && CategoryString.IsEmpty())
	{
		CategoryString = FRuntimeMetaData::GetMetaData(CurrentField, CategoryKey);
		CurrentField = CurrentField->GetOwner<FField>();
	}

	if (!CategoryString.IsEmpty())
	{
		return FName(*CategoryString);
	}

	return NAME_None;
}

//------------------------------------------------------------------------------
FName GetCategoryFName(const UField* InField)
{
	static const FName CategoryKey(TEXT("Category"));
	if (InField && FRuntimeMetaData::HasMetaData(InField, CategoryKey))
	{
		return FName(FRuntimeMetaData::GetMetaData(InField, CategoryKey));
	}
	return NAME_None;
}

//------------------------------------------------------------------------------
bool CanCreateBlueprintOfClass(const UClass* Class)
{
	bool bCanCreateBlueprint = false;

	if (Class)
	{
		bool bAllowDerivedBlueprints = false;
		GConfig->GetBool(TEXT("Kismet"), TEXT("AllowDerivedBlueprints"), /*out*/ bAllowDerivedBlueprints, GEngineIni);

#if WITH_EDITORONLY_DATA
		bCanCreateBlueprint = !Class->HasAnyClassFlags(CLASS_Deprecated)
			&& !Class->HasAnyClassFlags(CLASS_NewerVersionExists)
			&& (!Class->ClassGeneratedBy || (bAllowDerivedBlueprints && !IsClassABlueprintSkeleton(Class)));
#else
		bCanCreateBlueprint = !Class->HasAnyClassFlags(CLASS_Deprecated)
			&& !Class->HasAnyClassFlags(CLASS_NewerVersionExists)
			&& (/*!Class->ClassGeneratedBy ||*/ (bAllowDerivedBlueprints /*&& !IsClassABlueprintSkeleton(Class)*/));
#endif


		const bool bIsBPGC = (Cast<UBlueprintGeneratedClass>(Class) != nullptr);

		const bool bIsValidClass = /*Class->GetBoolMetaDataHierarchical(FBlueprintMetadata::MD_IsBlueprintBase)*/
			/*||*/ (Class == UObject::StaticClass())
			|| (Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || Class == USceneComponent::StaticClass() || Class == UActorComponent::StaticClass())
			|| bIsBPGC;  // BPs are always considered inheritable

		bCanCreateBlueprint &= bIsValidClass;
	}

	return bCanCreateBlueprint;
}

//------------------------------------------------------------------------------
FString GetFriendlyName(const FProperty* Property, UStruct* OwnerStruct/* = NULL*/)
{
	// first, try to pull the friendly name from the loc file
	check(Property);
	UStruct* RealOwnerStruct = Property->GetOwnerStruct();
	if (OwnerStruct == NULL)
	{
		OwnerStruct = RealOwnerStruct;
	}
	checkSlow(OwnerStruct);

	FText FoundText;
	bool DidFindText = false;
	UStruct* CurrentStruct = OwnerStruct;
	do
	{
		FString PropertyPathName = Property->GetPathName(CurrentStruct);

		DidFindText = FText::FindText(*CurrentStruct->GetName(), *(PropertyPathName + TEXT(".FriendlyName")), /*OUT*/FoundText);
		CurrentStruct = CurrentStruct->GetSuperStruct();
	} while (CurrentStruct != NULL && CurrentStruct->IsChildOf(RealOwnerStruct) && !DidFindText);

	if (!DidFindText)
	{
		const FString& DefaultFriendlyName = FRuntimeMetaData::GetMetaData(Property, TEXT("DisplayName"));
		if (DefaultFriendlyName.IsEmpty())
		{
			const bool bIsBool = CastField<const FBoolProperty>(Property) != NULL;
			return FName::NameToDisplayString(Property->GetName(), bIsBool);
		}
		return DefaultFriendlyName;
	}

	return FoundText.ToString();
}

//------------------------------------------------------------------------------
bool IsCategoryHiddenFromClass(const UStruct* Class, const FText& Category)
{
	return IsCategoryHiddenFromClass(Class, Category.ToString());
}

//------------------------------------------------------------------------------
bool IsCategoryHiddenFromClass(const UStruct* Class, const FString& Category)
{
	TArray<FString> ClassHideCategories;
	GetClassHideCategories(Class, ClassHideCategories);
	return IsCategoryHiddenFromClass(ClassHideCategories, Class, Category);
}

//------------------------------------------------------------------------------
bool IsCategoryHiddenFromClass(const TArray<FString>& ClassHideCategories, const UStruct* Class, const FString& Category)
{
	bool bIsHidden = false;

	// run the category through sanitization so we can ensure compares will hit
	const FString DisplayCategory = GetCategoryDisplayString(Category);

	for (const FString& HideCategory : ClassHideCategories)
	{
		bIsHidden = (HideCategory == DisplayCategory);
		if (bIsHidden)
		{
			TArray<FString> ClassShowCategories;
			GetClassShowCategories(Class, ClassShowCategories);
			// if they hid it, and showed it... favor showing (could be a shown in a sub-class, and hid in a super)
			bIsHidden = (ClassShowCategories.Find(DisplayCategory) == INDEX_NONE);
		}
		else // see if the category's root is hidden
		{
			TArray<FString> SubCategoryList;
			DisplayCategory.ParseIntoArray(SubCategoryList, TEXT("|"), /*InCullEmpty =*/true);

			FString FullSubCategoryPath;
			for (const FString& SubCategory : SubCategoryList)
			{
				FullSubCategoryPath += SubCategory;
				if ((HideCategory == SubCategory) || (HideCategory == FullSubCategoryPath))
				{
					TArray<FString> ClassShowCategories;
					GetClassShowCategories(Class, ClassShowCategories);
					// if they hid it, and showed it... favor showing (could be a shown in a sub-class, and hid in a super)
					bIsHidden = (ClassShowCategories.Find(DisplayCategory) == INDEX_NONE);
				}
				FullSubCategoryPath += "|";
			}
		}

		if (bIsHidden)
		{
			break;
		}
	}

	return bIsHidden;
}

//------------------------------------------------------------------------------
void GetClassHideCategories(const UStruct* Class, TArray<FString>& CategoriesOut, bool bHomogenize)
{
	CategoriesOut.Empty();

	if (FRuntimeMetaData::HasMetaData(Class, TEXT("HideCategories")))
	{
		const FString& HideCategories = FRuntimeMetaData::GetMetaData(Class, TEXT("HideCategories"));

		HideCategories.ParseIntoArray(CategoriesOut, TEXT(" "), /*InCullEmpty =*/true);

		if (bHomogenize)
		{
			for (FString& Category : CategoriesOut)
			{
				Category = GetCategoryDisplayString(Category);
			}
		}
	}
}


//------------------------------------------------------------------------------
FText GetCategoryDisplayString(const FText& UnsanitizedCategory)
{
	return FText::FromString(GetCategoryDisplayString(UnsanitizedCategory.ToString()));
}

//------------------------------------------------------------------------------
FString GetCategoryDisplayString(const FString& UnsanitizedCategory)
{
	FString DisplayString = UnsanitizedCategory;

	int32 KeyIndex = INDEX_NONE;
	do
	{
		KeyIndex = DisplayString.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, KeyIndex);
		if (KeyIndex != INDEX_NONE)
		{
			int32 EndIndex = DisplayString.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, KeyIndex);
			if (EndIndex != INDEX_NONE)
			{
				FString ToReplaceStr(EndIndex + 1 - KeyIndex, *DisplayString + KeyIndex);
				FString ReplacementStr;

				int32 KeyLen = EndIndex - (KeyIndex + 1);
				if (KeyLen > 0)
				{
					FString Key(KeyLen, *DisplayString + KeyIndex + 1);
					Key.TrimStartInline();
					ReplacementStr = GetCategory(*Key).ToString();
				}
				DisplayString.ReplaceInline(*ToReplaceStr, *ReplacementStr);
			}
			KeyIndex = EndIndex;
		}

	} while (KeyIndex != INDEX_NONE);

	DisplayString = FName::NameToDisplayString(DisplayString, /*bIsBool =*/false);
	DisplayString.ReplaceInline(TEXT("| "), TEXT("|"), ESearchCase::CaseSensitive);

	return DisplayString;
}

//------------------------------------------------------------------------------
void  GetClassShowCategories(const UStruct* Class, TArray<FString>& CategoriesOut)
{
	CategoriesOut.Empty();

	if (FRuntimeMetaData::HasMetaData(Class, TEXT("ShowCategories")))
	{
		const FString& ShowCategories = FRuntimeMetaData::GetMetaData(Class, TEXT("ShowCategories"));
		ShowCategories.ParseIntoArray(CategoriesOut, TEXT(" "), /*InCullEmpty =*/true);

		for (FString& Category : CategoriesOut)
		{
			Category = GetCategoryDisplayString(FText::FromString(Category)).ToString();
		}
	}
}

//------------------------------------------------------------------------------
const FText& GetCategory(const FString& Key)
{
	/*
	if (FEditorCategoryUtilsImpl::FCategoryInfo const* FoundCategory = GetCategoryTable().Find(Key))
	{
		return FoundCategory->DisplayName;
	}
	*/
	return FText::GetEmpty();

}

//------------------------------------------------------------------------------
/*
TSharedRef< class SToolTip > CreateDocToolTip(const TAttribute<FText>& Text, const TSharedPtr<SWidget>& OverrideContent, const FString& Link, const FString& ExcerptName)
{
	return
		SNew(SToolTip)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(STextBlock)
				.Text(FText::FromString("TODO!!!"))
			]
		];
}
*/
//------------------------------------------------------------------------------

/*
TSharedRef< class SToolTip > CreateDocToolTip(const TAttribute<FText>& Text, const TSharedRef<SWidget>& OverrideContent, const TSharedPtr<SVerticalBox>& DocVerticalBox, const FString& Link, const FString& ExcerptName)
{
	return
		SNew(SToolTip)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString("TODO!!!"))
		]
		];
}
*/

//------------------------------------------------------------------------------
FTopLevelAssetPath GetClassPathNameFromAssetTag(const FAssetData& InAssetData)
{
	const FString GeneratedClassPath = InAssetData.GetTagValueRef<FString>(FBlueprintTags::GeneratedClassPath);
	return FTopLevelAssetPath(FPackageName::ExportTextPathToObjectPath(FStringView(GeneratedClassPath)));
}

//------------------------------------------------------------------------------
FTopLevelAssetPath GetClassPathNameFromAsset(const FAssetData& InAssetData, bool bGenerateClassPathIfMissing /*= false*/)
{
	bool bIsBPGC = false;
	const bool bIsBP = IsBlueprintAsset(InAssetData, &bIsBPGC);

	if (bIsBPGC)
	{
		return FTopLevelAssetPath(InAssetData.GetSoftObjectPath().GetAssetPath());
	}
	else if (bIsBP)
	{
		FTopLevelAssetPath ClassPath = GetClassPathNameFromAssetTag(InAssetData);
		if (bGenerateClassPathIfMissing && ClassPath.IsNull())
		{
			FString ClassPathString = InAssetData.GetObjectPathString();
			ClassPathString += TEXT("_C");
			ClassPath = FTopLevelAssetPath(ClassPathString);
		}
		return ClassPath;
	}
	return FTopLevelAssetPath();
}

//------------------------------------------------------------------------------
bool IsBlueprintAsset(const FAssetData& InAssetData, bool* bOutIsBPGC /*= nullptr*/)
{
	bool bIsBP = (InAssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName());
	bool bIsBPGC = (InAssetData.AssetClassPath == UBlueprintGeneratedClass::StaticClass()->GetClassPathName());

	if (!bIsBP && !bIsBPGC)
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
		TArray<FTopLevelAssetPath> AncestorClassNames;
		AssetRegistry.GetAncestorClassNames(InAssetData.AssetClassPath, AncestorClassNames);

		if (AncestorClassNames.Contains(UBlueprint::StaticClass()->GetClassPathName()))
		{
			bIsBP = true;
		}
		else if (AncestorClassNames.Contains(UBlueprintGeneratedClass::StaticClass()->GetClassPathName()))
		{
			bIsBPGC = true;
		}
	}

	if (bOutIsBPGC)
	{
		*bOutIsBPGC = bIsBPGC;
	}

	return bIsBP || bIsBPGC;
}

//------------------------------------------------------------------------------
void GetImplementedInterfaceClassPathsFromAsset(const struct FAssetData& InAssetData, TArray<FString>& OutClassPaths)
{
	if (!InAssetData.IsValid())
	{
		return;
	}

	const FString ImplementedInterfaces = InAssetData.GetTagValueRef<FString>(FBlueprintTags::ImplementedInterfaces);
	if (!ImplementedInterfaces.IsEmpty())
	{
		// Parse string like "((Interface=Class'"/Script/VPBookmark.VPBookmarkProvider"'),(Interface=Class'"/Script/VPUtilities.VPContextMenuProvider"'))"
		// We don't want to actually resolve the hard ref so do some manual parsing

		FString FullInterface;
		FString CurrentString = *ImplementedInterfaces;
		while (CurrentString.Split(TEXT("Interface="), nullptr, &FullInterface))
		{
			// Cutoff at next )
			int32 RightParen = INDEX_NONE;
			if (FullInterface.FindChar(TCHAR(')'), RightParen))
			{
				// Keep parsing
				CurrentString = FullInterface.Mid(RightParen);

				// Strip class name
				FullInterface = *FPackageName::ExportTextPathToObjectPath(FullInterface.Left(RightParen));

				// Handle quotes
				FString InterfacePath;
				const TCHAR* NewBuffer = FPropertyHelpers::ReadToken(*FullInterface, InterfacePath, true);

				if (NewBuffer)
				{
					OutClassPaths.Add(InterfacePath);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
bool IsClassABlueprintSkeleton(const UClass* Class)
{
	// Find generating blueprint for a class
#if WITH_EDITORONLY_DATA
	UBlueprint* GeneratingBP = Cast<UBlueprint>(Class->ClassGeneratedBy);
	if (GeneratingBP && GeneratingBP->SkeletonGeneratedClass)
	{
		return (Class == GeneratingBP->SkeletonGeneratedClass) && (GeneratingBP->SkeletonGeneratedClass != GeneratingBP->GeneratedClass);
	}
#endif
	return Class->HasAnyFlags(RF_Transient) && Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
}
//------------------------------------------------------------------------------
FString GetDisplayNameHelper(const UObject& Object)
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

//------------------------------------------------------------------------------
FString GetDisplayNameHelper(const FField& Object)
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

}

