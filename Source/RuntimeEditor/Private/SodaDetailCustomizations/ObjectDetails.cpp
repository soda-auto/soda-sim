// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaDetailCustomizations/ObjectDetails.h"
//#include "ScopedTransaction.h"
#include "Engine/EngineBaseTypes.h"
#include "UObject/UnrealType.h"
#include "SodaStyleSet.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
//#include "SWarningOrErrorBox.h"
#include "RuntimePropertyEditor/DetailLayoutBuilder.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
#include "RuntimePropertyEditor/DetailCategoryBuilder.h"
#include "RuntimePropertyEditor/IDetailsView.h"
//#include "EdGraphSchema_K2.h"
//#include "Kismet2/BlueprintEditorUtils.h"
#include "Widgets/SToolTip.h"
#include "RuntimeDocumentation/IDocumentation.h"
//#include "ObjectEditorUtils.h"

#include "RuntimeMetaData.h"

#define LOCTEXT_NAMESPACE "ObjectDetails"

namespace soda
{
static const FName MD_FunctionCategory(TEXT("Category"));
static const FName MD_CallInEditor(TEXT("CallInRuntime"));

TSharedRef<IDetailCustomization> FObjectDetails::MakeInstance()
{
	return MakeShareable(new FObjectDetails);
}

void FObjectDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	AddExperimentalWarningCategory(DetailBuilder);
	AddCallInEditorMethods(DetailBuilder);
}

void FObjectDetails::AddExperimentalWarningCategory(IDetailLayoutBuilder& DetailBuilder)
{
	/*
	bool bBaseClassIsExperimental = false;
	bool bBaseClassIsEarlyAccess = false;
	FString MostDerivedDevelopmentClassName;
	FObjectEditorUtils::GetClassDevelopmentStatus(DetailBuilder.GetBaseClass(), bBaseClassIsExperimental, bBaseClassIsEarlyAccess, MostDerivedDevelopmentClassName);

	
	if (bBaseClassIsExperimental || bBaseClassIsEarlyAccess)
	{
		const FName CategoryName(TEXT("Warning"));
		const FText CategoryDisplayName = LOCTEXT("WarningCategoryDisplayName", "Warning");
		const FText WarningText = bBaseClassIsExperimental ? FText::Format( LOCTEXT("ExperimentalClassWarning", "Uses experimental class: {0}") , FText::FromString(MostDerivedDevelopmentClassName) )
			: FText::Format( LOCTEXT("EarlyAccessClassWarning", "Uses beta class {0}"), FText::FromString(MostDerivedDevelopmentClassName) );
		const FText SearchString = WarningText;

		IDetailCategoryBuilder& WarningCategory = DetailBuilder.EditCategory(CategoryName, CategoryDisplayName, ECategoryPriority::Transform);

		FDetailWidgetRow& WarningRow = WarningCategory.AddCustomRow(SearchString)
			.WholeRowContent()
			[
				SNew(SBox)
				.Padding(FMargin(0.f, 4.f))
				[
					SNew(SWarningOrErrorBox)
					.MessageStyle(EMessageStyle::Warning)
					.Message(WarningText)
				]
			];
	}
	*/
}

void FObjectDetails::AddCallInEditorMethods(IDetailLayoutBuilder& DetailBuilder)
{
	// metadata tag for defining sort order of function buttons within a Category
	static const FName NAME_DisplayPriority("DisplayPriority");

	// Get all of the functions we need to display (done ahead of time so we can sort them)
	TArray<UFunction*, TInlineAllocator<8>> CallInEditorFunctions;
	for (TFieldIterator<UFunction> FunctionIter(DetailBuilder.GetBaseClass(), EFieldIteratorFlags::IncludeSuper); FunctionIter; ++FunctionIter)
	{
		UFunction* TestFunction = *FunctionIter;

		if (FRuntimeMetaData::GetBoolMetaData(TestFunction, MD_CallInEditor) && (TestFunction->ParmsSize == 0))
		{
			/*
			if (UClass* TestFunctionOwnerClass = TestFunction->GetOwnerClass())
			{
				if (UBlueprint* Blueprint = Cast<UBlueprint>(TestFunctionOwnerClass->ClassGeneratedBy))
				{
					if (FBlueprintEditorUtils::IsEditorUtilityBlueprint(Blueprint))
					{
						// Skip Blutilities as these are handled by FEditorUtilityInstanceDetails
						continue;
					}
				}
			}
			*/

			const FName FunctionName = TestFunction->GetFName();
			if (!CallInEditorFunctions.FindByPredicate([&FunctionName](const UFunction* Func) { return Func->GetFName() == FunctionName; }))
			{
				CallInEditorFunctions.Add(*FunctionIter);
			}
		}
	}

	if (CallInEditorFunctions.Num() > 0)
	{
		// Copy off the objects being customized so we can invoke a function on them later, removing any that are a CDO
		DetailBuilder.GetObjectsBeingCustomized(/*out*/ SelectedObjectsList);
		SelectedObjectsList.RemoveAllSwap([](TWeakObjectPtr<UObject> ObjPtr) { UObject* Obj = ObjPtr.Get(); return (Obj == nullptr) || Obj->HasAnyFlags(RF_ArchetypeObject); });
		if (SelectedObjectsList.Num() == 0)
		{
			return;
		}

		// Sort the functions by category and then by DisplayPriority meta tag, and then by name
		CallInEditorFunctions.Sort([](UFunction& A, UFunction& B)
		{
			const int32 CategorySort = FRuntimeMetaData::GetMetaData(&A, MD_FunctionCategory).Compare(FRuntimeMetaData::GetMetaData(&B, MD_FunctionCategory));
			if (CategorySort != 0)
			{
				return (CategorySort <= 0);
			}
			else 
			{
				FString DisplayPriorityAStr = FRuntimeMetaData::GetMetaData(&A, NAME_DisplayPriority);
				int32 DisplayPriorityA = (DisplayPriorityAStr.IsEmpty() ? MAX_int32 : FCString::Atoi(*DisplayPriorityAStr));
				if (DisplayPriorityA == 0 && !FCString::IsNumeric(*DisplayPriorityAStr))
				{
					DisplayPriorityA = MAX_int32;
				}

				FString DisplayPriorityBStr = FRuntimeMetaData::GetMetaData(&B, NAME_DisplayPriority);
				int32 DisplayPriorityB = (DisplayPriorityBStr.IsEmpty() ? MAX_int32 : FCString::Atoi(*DisplayPriorityBStr));
				if (DisplayPriorityB == 0 && !FCString::IsNumeric(*DisplayPriorityBStr))
				{
					DisplayPriorityB = MAX_int32;
				}

				return (DisplayPriorityA == DisplayPriorityB) ? (A.GetName() <= B.GetName()) : (DisplayPriorityA <= DisplayPriorityB);
			}
		});

		struct FCategoryEntry
		{
			FName CategoryName;
			FName RowTag;
			TSharedPtr<SWrapBox> WrapBox;
			FTextBuilder FunctionSearchText;

			FCategoryEntry(FName InCategoryName)
				: CategoryName(InCategoryName)
			{
				WrapBox = SNew(SWrapBox).UseAllottedSize(true);
			}
		};

		// Build up a set of functions for each category, accumulating search text and buttons in a wrap box
		FName ActiveCategory;
		TArray<FCategoryEntry, TInlineAllocator<8>> CategoryList;
		for (UFunction* Function : CallInEditorFunctions)
		{
			FName FunctionCategoryName(NAME_Default);
			if (FRuntimeMetaData::HasMetaData(Function, MD_FunctionCategory))
			{
				FunctionCategoryName = FName(*FRuntimeMetaData::GetMetaData(Function, MD_FunctionCategory));
			}

			if (FunctionCategoryName != ActiveCategory)
			{
				ActiveCategory = FunctionCategoryName;
				CategoryList.Emplace(FunctionCategoryName);
			}
			FCategoryEntry& CategoryEntry = CategoryList.Last();

			//@TODO: Expose the code in UK2Node_CallFunction::GetUserFacingFunctionName / etc...
			const FText ButtonCaption = FText::FromString(FName::NameToDisplayString(*Function->GetName(), false));
			FText FunctionTooltip;// = Function->GetToolTipText();
			if (FunctionTooltip.IsEmpty())
			{
				FunctionTooltip = FText::FromString(Function->GetName());
			}
			

			TWeakObjectPtr<UFunction> WeakFunctionPtr(Function);
			CategoryEntry.WrapBox->AddSlot()
			.Padding(0.0f, 0.0f, 5.0f, 3.0f)
			[
				SNew(SButton)
				.Text(ButtonCaption)
				.OnClicked(FOnClicked::CreateSP(this, &FObjectDetails::OnExecuteCallInEditorFunction, WeakFunctionPtr))
				.ToolTipText(FText::Format(LOCTEXT("CallInEditorTooltip", "Call an event on the selected object(s)\n\n\n{0}"), FunctionTooltip))
			];

			CategoryEntry.RowTag = Function->GetFName();
			CategoryEntry.FunctionSearchText.AppendLine(ButtonCaption);
			CategoryEntry.FunctionSearchText.AppendLine(FunctionTooltip);
		}
		
		// Now edit the categories, adding the button strips to the details panel
		for (FCategoryEntry& CategoryEntry : CategoryList)
		{
			IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(CategoryEntry.CategoryName);
			CategoryBuilder.AddCustomRow(CategoryEntry.FunctionSearchText.ToText())
			.RowTag(CategoryEntry.RowTag)
			[
				CategoryEntry.WrapBox.ToSharedRef()
			];
		}
	}
}

FReply FObjectDetails::OnExecuteCallInEditorFunction(TWeakObjectPtr<UFunction> WeakFunctionPtr)
{
	if (UFunction* Function = WeakFunctionPtr.Get())
	{
		//@TODO: Consider naming the transaction scope after the fully qualified function name for better UX
		//FScopedTransaction Transaction(LOCTEXT("ExecuteCallInEditorMethod", "Call In Editor Action"));

		FEditorScriptExecutionGuard ScriptGuard;
		for (TWeakObjectPtr<UObject> SelectedObjectPtr : SelectedObjectsList)
		{
			if (UObject* Object = SelectedObjectPtr.Get())
			{
				Object->ProcessEvent(Function, nullptr);
			}
		}
	}

	return FReply::Handled();
}

}

#undef LOCTEXT_NAMESPACE
