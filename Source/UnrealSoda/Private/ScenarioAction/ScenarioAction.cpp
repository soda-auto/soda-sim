// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioAction.h"
#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"
#include "Soda/SodaTypes.h"
#include "Soda/SodaGameMode.h"
#include "UI/ScenarioAction/SScenarioActionEditor.h"
#include "SodaStyleSet.h"
#include "Soda/ScenarioAction/ScenarioActionConditionFunctionLibrary.h"
#include "Soda/ScenarioAction/ScenarioActionBlock.h"
#include "Soda/ScenarioAction/ScenarioActionEvent.h"
#include "Styling/StyleColors.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Soda/Misc/SerializationHelpers.h"

AScenarioAction::AScenarioAction()
{
	PrimaryActorTick.bCanEverTick = true;
}

UScenarioActionBlock* AScenarioAction::CreateNewBlock()
{
	UScenarioActionBlock *& ScenarioBlock = ScenarioBlocks.Add_GetRef(NewObject< UScenarioActionBlock>(this));
	check(ScenarioBlock);
	ScenarioBlock->OwnedScenario = this;
	return ScenarioBlock;
}

bool AScenarioAction::RemoveBlock(UScenarioActionBlock* Block)
{
	return ScenarioBlocks.Remove(Block) > 0;
}

void AScenarioAction::BeginPlay()
{
	Super::BeginPlay();
}

void AScenarioAction::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	CloseEditorWindow();
}

void AScenarioAction::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (USodaGameModeComponent::Get()->IsScenarioRunning())
	{
		for (auto& It : ScenarioBlocks)
		{
			for (auto& EventIt : It->Events)
			{
				EventIt->Tick(DeltaTime);
			}
		}
	}
}

void AScenarioAction::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame())
	{
		if (Ar.IsSaving())
		{
			int Num = ScenarioBlocks.Num();
			Ar << Num;
			for (auto& It : ScenarioBlocks)
			{
				FObjectRecord Record;
				Record.SerializeObject(It);
				Ar << Record;
			}
		}
		else if (Ar.IsLoading())
		{
			int Num = 0;
			Ar << Num;
			for (int i = 0; i < Num; ++i)
			{
				FObjectRecord Record;
				Ar << Record;
				if (Record.IsRecordValid())
				{
					UScenarioActionBlock* Block = CreateNewBlock();
					Record.DeserializeObject(Block);
				}
			}
		}
	}
}

void AScenarioAction::ScenarioBegin()
{
	CloseEditorWindow();

	for (auto& It : ScenarioBlocks)
	{
		It->ResetExexuteCounter();
		for (auto& EventIt : It->Events)
		{
			EventIt->EventDelegate.BindUObject(It, &UScenarioActionBlock::OnEvent);
			EventIt->ScenarioBegin();
		}
	}
}

void AScenarioAction::ScenarioEnd()
{
	for (auto& It : ScenarioBlocks)
	{
		for (auto& EventIt : It->Events)
		{
			EventIt->ScenarioEnd();
			EventIt->EventDelegate.Unbind();
		}
	}
}

TSharedPtr<SWidget> AScenarioAction::GenerateToolBar()
{
	FUniformToolBarBuilder ToolbarBuilder(TSharedPtr<const FUICommandList>(), FMultiBoxCustomization::None);
	ToolbarBuilder.SetStyle(&FSodaStyle::Get(), "PaletteToolBar");
	ToolbarBuilder.BeginSection(NAME_None);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([this] 
			{
				CloseEditorWindow();
				TSharedRef<SWindow> NewSlateWindow = SNew(SWindow)
					.Title(FText::FromString(TEXT("Scenario Action Editor")))
					.ClientSize(FVector2D(800, 600));
				FSlateApplication::Get().AddWindow(NewSlateWindow);
				NewSlateWindow->SetContent(
					SAssignNew(ScenarioActionEditor, SScenarioActionEditor, this)
				);
			})),
		NAME_None, 
		FText::FromString("Editor"),
		FText::FromString("Open Scenario Action Editor"), 
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "ScenarioAction.OpenEditor"),
		EUserInterfaceActionType::Button
	);
	ToolbarBuilder.EndSection();

	return
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FStyleColors::Panel)
		.Padding(3)
		[
			ToolbarBuilder.MakeWidget()
		];
}

void AScenarioAction::CloseEditorWindow()
{
	if (ScenarioActionEditor.IsValid())
	{
		TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow(ScenarioActionEditor.ToSharedRef());
		if (ContainingWindow.IsValid())
		{
			ContainingWindow->RequestDestroyWindow();
		}
	}

	ScenarioActionEditor.Reset();
}

const FSodaActorDescriptor* AScenarioAction::GenerateActorDescriptor() const
{

	static FSodaActorDescriptor Desc{
		TEXT("Scenario Action"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Experimental"), /*SubCategory*/
		TEXT("Icons.Sequence"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}


