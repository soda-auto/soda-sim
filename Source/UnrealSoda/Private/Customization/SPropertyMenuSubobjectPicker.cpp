// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SPropertyMenuSubobjectPicker.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SodaStyleSet.h"
#include "GameFramework/Actor.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Soda/Misc/EditorUtils.h"
#include "Soda/ISodaVehicleComponent.h"
#include "RuntimeEditorUtils.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

namespace soda
{

static const FName Column_Name("Name");
static const FName Column_Class("Class");


class SSubobjectRow : public SMultiColumnTableRow<TWeakObjectPtr<UObject>>
{
public:
	SLATE_BEGIN_ARGS(SSubobjectRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UObject> InSubobject)
	{
		Subobject = InSubobject;

		if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(InSubobject.Get()))
		{
			Icon = VehicleComponent->GetVehicleComponentGUI().IcanName;
		}

		FSuperRowType::FArguments OutArgs;
		//OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
		SMultiColumnTableRow<TWeakObjectPtr<UObject>>::Construct(OutArgs, InOwnerTable);

	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{

		if (InColumnName == Column_Name)
		{
			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 1, 1, 1)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FSodaStyle::GetBrush(Icon))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 1, 1, 1)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Subobject->GetName()))
				];
		}

		if (InColumnName == Column_Class)
		{
			return SNew(SBox)
				.Padding(FMargin(5, 1, 1, 1))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Subobject->GetClass()->GetName()))
				];
		}

		return SNullWidget::NullWidget;
	}

protected:
	TWeakObjectPtr<UObject> Subobject;
	FName Icon = TEXT("ClassIcon.Actor");
};


void SPropertyMenuSubobjectPicker::Construct(const FArguments& InArgs)
{
	OwnedActor = InArgs._OwnedActor;
	InitialSubobject = InArgs._InitialSubobject;
	bAllowClear = InArgs._AllowClear;
	SubobjectFilter = InArgs._SubobjectFilter;
	OnSet = InArgs._OnSet;
	OnClose = InArgs._OnClose;

	FMenuBuilder MenuBuilder(true, NULL);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("CurrentSubobjectOperationsHeader", "Current Subobject"));
	{
		if (InitialSubobject.IsValid())
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("EditSubobject", "Edit"),
				LOCTEXT("EditSubobject_Tooltip", "Edit this Subobject"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SPropertyMenuSubobjectPicker::OnEdit)));
		}

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CopySubobject", "Copy"),
			LOCTEXT("CopySubobject_Tooltip", "Copies the Subobject to the clipboard"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SPropertyMenuSubobjectPicker::OnCopy))
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("PasteSubobject", "Paste"),
			LOCTEXT("PasteSubobject_Tooltip", "Pastes an Subobject from the clipboard to this field"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SPropertyMenuSubobjectPicker::OnPaste),
				FCanExecuteAction::CreateSP(this, &SPropertyMenuSubobjectPicker::CanPaste))
			);

		if (bAllowClear)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ClearSubobject", "Clear"),
				LOCTEXT("ClearSubobject_ToolTip", "Clears the Subobject set on this field"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SPropertyMenuSubobjectPicker::OnClear))
			);
		}
	}

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		TSharedPtr<SWidget> MenuContent;

		ForEachObjectWithOuter(OwnedActor.Get(), [this](UObject* Object)
		{
			if(SubobjectFilter.IsBound())
			{
				if (!SubobjectFilter.Execute(Object)) return;
			}
			Source.Add(Object);
		}, true);

		MenuContent =
			SNew(SBox)
			.WidthOverride(350.0f)
			.HeightOverride(300.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("Menu.Background"))
				[
					SNew(SListView<TWeakObjectPtr<UObject>>)
					.SelectionMode(ESelectionMode::Single)
					.ListItemsSource(&Source)
					.OnGenerateRow(this, &SPropertyMenuSubobjectPicker::MakeRowWidget)
					.OnSelectionChanged(this, &SPropertyMenuSubobjectPicker::OnItemSelected)
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(Column_Name)
						.DefaultLabel(FText::FromString("Name"))
						+ SHeaderRow::Column(Column_Class)
						.DefaultLabel(FText::FromString("Class"))
					)
				]
			];

		MenuBuilder.AddWidget(MenuContent.ToSharedRef(), FText::GetEmpty(), true);

	}
	MenuBuilder.EndSection();

	ChildSlot
	[
		FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000)
	];
}

void SPropertyMenuSubobjectPicker::OnEdit()
{
	if (InitialSubobject.IsValid())
	{
		//GEditor->EditObject(InitialSubobject);
	}
	OnClose.ExecuteIfBound();
}

void SPropertyMenuSubobjectPicker::OnCopy()
{
	if (InitialSubobject.IsValid())
	{
		FPlatformApplicationMisc::ClipboardCopy(*FString::Printf(TEXT("%s %s"), *InitialSubobject->GetClass()->GetPathName(), *InitialSubobject->GetPathName()));
	}
	OnClose.ExecuteIfBound();
}

void SPropertyMenuSubobjectPicker::OnPaste()
{
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

	bool bFound = false;

	int32 SpaceIndex = INDEX_NONE;
	if (ClipboardText.FindChar(TEXT(' '), SpaceIndex))
	{
		FString ClassPath = ClipboardText.Left(SpaceIndex);
		FString PossibleObjectPath = ClipboardText.Mid(SpaceIndex);

		if (UClass* ClassPtr = LoadClass<UObject>(nullptr, *ClassPath))
		{
			UObject* Subobject = FindObject<UObject>(nullptr, *PossibleObjectPath);
			if (Subobject && Subobject->IsA(ClassPtr) && FEditorUtils::FindOuterActor(Subobject) && (!SubobjectFilter.IsBound() || SubobjectFilter.Execute(Subobject)))
			{
				SetValue(Subobject);
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		SetValue(nullptr);
	}

	OnClose.ExecuteIfBound();
}

bool SPropertyMenuSubobjectPicker::CanPaste()
{
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

	bool bCanPaste = false;

	int32 SpaceIndex = INDEX_NONE;
	if (ClipboardText.FindChar(TEXT(' '), SpaceIndex))
	{
		FString Class = ClipboardText.Left(SpaceIndex);
		FString PossibleObjectPath = ClipboardText.Mid(SpaceIndex);

		bCanPaste = !Class.IsEmpty() && !PossibleObjectPath.IsEmpty();
		if (bCanPaste)
		{
			bCanPaste = LoadClass<UObject>(nullptr, *Class) != nullptr;
		}
		if (bCanPaste)
		{
			bCanPaste = FindObject<UObject>(nullptr, *PossibleObjectPath) != nullptr;
		}
	}

	return bCanPaste;
}

void SPropertyMenuSubobjectPicker::OnClear()
{
	SetValue(nullptr);
	OnClose.ExecuteIfBound();
}

void SPropertyMenuSubobjectPicker::OnItemSelected(TWeakObjectPtr<UObject> Subobject, ESelectInfo::Type SelectInfo)
{
	SetValue(Subobject.Get());
	OnClose.ExecuteIfBound();
}

void SPropertyMenuSubobjectPicker::SetValue(UObject* Subobject)
{
	OnSet.ExecuteIfBound(Subobject);
}

TSharedRef< ITableRow > SPropertyMenuSubobjectPicker::MakeRowWidget(TWeakObjectPtr<UObject> Subobject, const TSharedRef< STableViewBase >& OwnerTable)
{
	return SNew(SSubobjectRow, OwnerTable, Subobject);
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
