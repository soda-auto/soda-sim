// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionCondition.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IStructureDetailsView.h"
#include "RuntimeMetaData.h"
#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "Modules/ModuleManager.h"

UScenarioActionCondition::UScenarioActionCondition(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("No Name"));
}

UScenarioActionConditionFunction::UScenarioActionConditionFunction(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
}

void UScenarioActionConditionFunction::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame())
	{
		if (Ar.IsLoading() && Function.IsValid())
		{
			StructOnScope = MakeShared<FStructOnScope>(Function.Get());
			DisplayName = FRuntimeMetaData::GetDisplayNameText(Function.Get());
		}
		FScenarioActionUtils::SerializeUStruct(Ar, Function.Get(), Function.IsValid() ? StructOnScope->GetStructMemory() : nullptr, true);
	}
}

TSharedRef< SWidget > UScenarioActionConditionFunction::MakeWidget()
{
	if (StructOnScope)
	{
		int NumProperties = 0;
		for (TFieldIterator<FProperty> It(StructOnScope->GetStruct()); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_ReturnParm)) ++NumProperties;
		}

		//TSharedPtr< SWidget > Body;
		if (NumProperties)
		{
			FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
			soda::FDetailsViewArgs Args;
			Args.bHideSelectionTip = true;
			Args.bLockable = false;
			Args.bShowOptions = false;
			Args.bAllowSearch = false;
			Args.NameAreaSettings = soda::FDetailsViewArgs::HideNameArea;
			Args.bHideRightColumn = true;
			soda::FStructureDetailsViewArgs StructureArgs;
			return RuntimeEditorModule.CreateStructureDetailView(Args, StructureArgs, StructOnScope)->GetWidget().ToSharedRef();
		}
	}
	return SNullWidget::NullWidget;
}

bool UScenarioActionConditionFunction::Execute()
{
	//FScopedScriptExceptionHandler ExceptionHandler([](ELogVerbosity::Type Verbosity, const TCHAR* ExceptionMessage, const TCHAR* StackMessage) {});
	FEditorScriptExecutionGuard ScriptGuard;

	if (Function && StructOnScope.IsValid() && FunctionOwner.IsValid())
	{
		if (FProperty* RetProperty = Function->GetReturnProperty())
		{
			FunctionOwner.Get()->GetDefaultObject()->ProcessEvent(Function.Get(), StructOnScope->GetStructMemory());
			bool* ReValue = RetProperty->ContainerPtrToValuePtr<bool>(StructOnScope->GetStructMemory());
			if (ReValue)
			{
				return *ReValue;
			}
		}
	}
	return false;
}

void UScenarioActionConditionFunction::SetFunction(UFunction* InFunction, UClass* InOwnerClass)
{
	check(IsValid(InFunction) && IsValid(InOwnerClass));

	Function = InFunction;
	FunctionOwner = InOwnerClass;

	StructOnScope = MakeShared< FStructOnScope>(Function.Get());
	if (StructOnScope.IsValid())
	{
		DisplayName = FRuntimeMetaData::GetDisplayNameText(Function.Get());
	}
	else
	{
		DisplayName = FText::FromString(TEXT("No Name"));
	}
}