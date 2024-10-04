// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionEvent.h"
#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "RuntimeEditorModule.h"
#include "RuntimeMetaData.h"
#include "RuntimePropertyEditor/IDetailsView.h"
#include "Modules/ModuleManager.h"

UScenarioActionEvent::UScenarioActionEvent(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
	DisplayName = FText::FromString(TEXT("No Name"));
}

TSharedRef< SWidget > UScenarioActionEvent::MakeWidget() 
{

	FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
	soda::FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = false;
	Args.bShowOptions = false;
	Args.bAllowSearch = false;
	Args.NameAreaSettings = soda::FDetailsViewArgs::HideNameArea;
	TSharedPtr<soda::IDetailsView> DetailView = RuntimeEditorModule.CreateDetailView(Args);
	DetailView->SetObject(this);
	/*
	DetailView->SetIsPropertyVisibleDelegate(soda::FIsPropertyVisible::CreateLambda([](const soda::FPropertyAndParent& PropertyAndParent)
	{
		if(PropertyAndParent.ParentProperties.Num() == 0)
		{
			if (FRuntimeMetaData::HasMetaData(&PropertyAndParent.Property, TEXT("ScenarioAction")))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return true;
	}));
	*/
	return DetailView.ToSharedRef();
}