// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SActorToolBox.h"
#include "Soda/UnrealSoda.h"
#include "EngineUtils.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Soda/ISodaActor.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IDetailsView.h"
#include "Soda/SodaGameMode.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/Editor/SodaSelection.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/UI/SSodaViewport.h"

#define LOCTEXT_NAMESPACE "ActorBar"

namespace soda
{

void SActorToolBox::Construct( const FArguments& InArgs )
{
	Viewport = InArgs._Viewport;

	check(InArgs._Viewport);
	ViewportClient = InArgs._Viewport->GetViewportClient();
	check(ViewportClient.IsValid());

	Caption = FText::FromString(TEXT("Editing Mode"));

	FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
	soda::FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = true;
	Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	Args.NameAreaSettings = soda::FDetailsViewArgs::HideNameArea;
	Args.bGameModeOnlyVisible = true;
	DetailView = RuntimeEditorModule.CreateDetailView(Args);
	DetailView->SetIsPropertyVisibleDelegate(soda::FIsPropertyVisible::CreateLambda([](const soda::FPropertyAndParent& PropertyAndParent)
	{
		// For details views in the level editor all properties are the instanced versions
		if (PropertyAndParent.Property.HasAllPropertyFlags(CPF_DisableEditOnInstance))
		{
			return false;
		}

		const FProperty& Property = PropertyAndParent.Property;
		const FArrayProperty* ArrayProperty = CastField<const FArrayProperty>(&Property);
		const FSetProperty* SetProperty = CastField<const FSetProperty>(&Property);
		const FMapProperty* MapProperty = CastField<const FMapProperty>(&Property);

		const FProperty* TestProperty = ArrayProperty ? ArrayProperty->Inner : &Property;
		const FObjectPropertyBase* ObjectProperty = CastField<const FObjectPropertyBase>(TestProperty);
		bool bIsActorProperty = (ObjectProperty != nullptr && ObjectProperty->PropertyClass->IsChildOf(AActor::StaticClass()));

		bool bIsComponent = (ObjectProperty != nullptr && ObjectProperty->PropertyClass->IsChildOf(UActorComponent::StaticClass()));

		if (bIsComponent)
		{
			// Don't show sub components properties, thats what selecting components in the component tree is for.
			return false;
		}
		return true;
	}));

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(EOrientation::Orient_Vertical)
		+ SSplitter::Slot()
		.Value(0.4)
		[
			SAssignNew(ListView, SActorList, ViewportClient.Get())
			.OnSelectionChanged(this, &SActorToolBox::OnSelectionChanged)
		]
		+ SSplitter::Slot()
		.Value(0.6)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(EmptyBox, SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			[
				DetailView.ToSharedRef()
			]
		]
	];

	ClearSelection();
	
	//ListView->RebuilActorList();
}

FReply SActorToolBox::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SActorToolBox::OnPush()
{
	if (ViewportClient->Selection)
	{
		ViewportClient->Selection->RetargetWidgetForSelectedActor();
	}
}

void SActorToolBox::OnSelectionChanged(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo)
{
	if (Actor.IsValid())
	{
		DetailView->SetObject(Actor.Get());
		EmptyBox->SetContent(SNullWidget::NullWidget);
	}
	else
	{
		ClearSelection();
	}
}

void SActorToolBox::ClearSelection()
{
	DetailView->SetObjects(TArray<UObject*>());
	EmptyBox->SetContent(
		SNew(STextBlock)
		.TextStyle(FAppStyle::Get(), "HintText")
		.Margin(FMargin(0, 20, 0, 0))
		.Text(FText::FromString("Select an actor to view details."))
	);
}


} // namespace soda

#undef LOCTEXT_NAMESPACE
