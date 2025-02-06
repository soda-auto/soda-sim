// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SActorList.h"
#include "Soda/UnrealSoda.h"
#include "EngineUtils.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Soda/ISodaActor.h"
//#include "RuntimeEditorModule.h"
//#include "RuntimePropertyEditor/IDetailsView.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/LevelState.h"
#include "Soda/Editor/SodaSelection.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/SodaApp.h"
//#include "Framework/SlateDelegates.h"
#include "UI/Common/SPinWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "RuntimeEditorUtils.h"

#define LOCTEXT_NAMESPACE "ActorBar"

namespace soda
{

static const FName Column_Pin("Pin");
static const FName Column_ItemName("ItemName");
static const FName Column_Type("Type");

//********************************************************************************************************

class SActorListRow : public SMultiColumnTableRow<TWeakObjectPtr<AActor>>
{
public:
	SLATE_BEGIN_ARGS(SActorListRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<AActor> InActor, TWeakPtr<SActorList> InActorListView)
	{
		USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
		check(SodaSubsystem);
		ASodaActorFactory * ActorFactory = SodaSubsystem->GetActorFactory();
		check(ActorFactory);

		Actor = InActor;
		Desc = SodaSubsystem->GetSodaActorDescriptor(Actor->GetClass());
		IsSpawnedActor = ActorFactory->CheckActorIsExist(Actor.Get());
		ActorListView = InActorListView;

		FSuperRowType::FArguments OutArgs;
		//OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
		SMultiColumnTableRow<TWeakObjectPtr<AActor>>::Construct(OutArgs, InOwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		
		if(InColumnName == Column_Pin)
		{
			TSharedPtr< SPinWidget > PinWidget;
			return 
				SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SAssignNew(PinWidget, SPinWidget)
					.CheckedHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Icons.Unpinned")))
					.CheckedNotHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Icons.Unpinned")))
					.NotCheckedHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Icons.Unpinned")))
					.NotCheckedNotHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Icons.Unpinned")))
					.bHideUnchecked(true)
					.IsChecked(TAttribute<bool>::CreateLambda([this]() 
					{
						ISodaActor* SodaActor = Cast<ISodaActor>(Actor.Get());
						return SodaActor ? SodaActor->IsPinnedActor() : false;
					}))
					.OnClicked(ActorListView.Pin().Get(), &SActorList::OnPinClicked, Actor)
					.Row(SharedThis(this))
				];
		}

		if(InColumnName == Column_ItemName)
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
					.Image(FSodaStyle::GetBrush(Desc.Icon))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 1, 1, 1)
				[
					SAssignNew(InlineTextBlock, SInlineEditableTextBlock)
					.Text_Lambda([this]()
					{
						if(Actor.IsValid())
						{
							return FText::FromString(*Actor->GetName());
						}
						return FText::FromString("None");
					})
					.ColorAndOpacity_Lambda([this]() { return IsSpawnedActor ? FLinearColor(0.95, 0.9, 0.4) : FLinearColor::White; })
					.OnTextCommitted(this, &SActorListRow::OnRenameActorCommited)
					.IsSelected(this, &SMultiColumnTableRow<TWeakObjectPtr<AActor>>::IsSelectedExclusively)
					.IsReadOnly(!IsSpawnedActor)
				];
		}

		if (InColumnName == Column_Type)
		{
			return SNew(SBox)
				.Padding(FMargin(5, 1, 1, 1))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(*Desc.DisplayName))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							FString Ret = "";
							ISodaActor* SodaActor = Cast<ISodaActor>(Actor);
							if (SodaActor && SodaActor->IsPinnedActor())
							{
								Ret = " [" + SodaActor->GetPinnedActorName() + "]";
							}
							return FText::FromString(*Ret);
						})
					]
				];
		}

		return SNullWidget::NullWidget;
	}

	void OnRenameActorCommited(const FText& NewName, ETextCommit::Type Type)
	{
		USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
		check(SodaSubsystem);
		ASodaActorFactory* ActorFactory = SodaSubsystem->GetActorFactory();
		check(ActorFactory);

		if (!Actor.IsValid())
		{
			UE_LOG(LogSoda, Error, TEXT("SActorListRow::OnRenameActorCommited(); Selected actor faild"));
		}
		else
		{
			if (!ActorFactory->RenameActor(Actor.Get(), NewName.ToString()))
			{
				ActorListView.Pin()->RebuilActorList();
				SodaSubsystem->LevelState->MarkAsDirty();
			}
		}
	}

	TWeakObjectPtr<AActor> Actor;
	FSodaActorDescriptor Desc;
	bool IsSpawnedActor;
	TWeakPtr<SActorList> ActorListView;
	TSharedPtr<SInlineEditableTextBlock> InlineTextBlock;
};

//********************************************************************************************************************

void SActorList::Construct(const FArguments& InArgs, USodaGameViewportClient * InViewportClient)
{
	check(IsValid(InViewportClient));
	ViewportClient = InViewportClient;
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	ActorFactory = SodaSubsystem->GetActorFactory();
	OnSelectionChangedDelegate = InArgs._OnSelectionChanged;
	bIsInteractiveMode = InArgs._bInteractiveMode.Get();
	ActorFilter = InArgs._ActorFilter;

	InvalidateDelegateHandle = ActorFactory->OnInvalidateDelegate.AddLambda([this]() 
	{ 
		RebuilActorList(); 
	});

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Recessed")))
			.Padding(5)
			[
				SAssignNew(SearchBox, SSearchBox)
				//.OnTextChanged(this, &SVehicleComponentClassCombo::OnSearchBoxTextChanged)
				//.OnTextCommitted(this, &SVehicleComponentClassCombo::OnSearchBoxTextCommitted);
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SAssignNew(TreeView, SListView<TWeakObjectPtr<AActor>>)
			.SelectionMode(ESelectionMode::Single)
			.ListItemsSource(&ActorList)
			.OnGenerateRow(this, &SActorList::MakeRowWidget)
			.OnContextMenuOpening(this, &SActorList::OnMenuOpening)
			.OnSelectionChanged(this, &SActorList::OnSelectionChanged)
			.HeaderRow(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(Column_Pin)
				.FixedWidth(24)
				.HAlignCell(HAlign_Center)
				.HeaderContent()
				[
					SNew(SBox)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(0)
					[
						SNew(SImage)
						.Image(FSodaStyle::Get().GetBrush(TEXT("Icons.Unpinned")))
					]
				]
				+ SHeaderRow::Column(Column_ItemName)
				.DefaultLabel(FText::FromString("Item Name"))
				+ SHeaderRow::Column(Column_Type)
				.DefaultLabel(FText::FromString("Type"))
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Header")))
			.Padding(15, 5, 5, 5)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() {return FText::FromString(FString::FromInt(ActorList.Num()) + " actors"); })
			]
		]
	];

	RebuilActorList();
}

SActorList::~SActorList()
{
	if (ActorFactory.IsValid() && InvalidateDelegateHandle.IsValid())
	{
		ActorFactory->OnInvalidateDelegate.Remove(InvalidateDelegateHandle);
	}
}

TSharedPtr<SWidget> SActorList::OnMenuOpening()
{
	if (TreeView->GetNumItemsSelected() != 1)
	{
		return TSharedPtr<SWidget>();
	}

	AActor* SelectedActor = GetSelectedActor();
	if (!IsValid(SelectedActor))
	{
		return TSharedPtr<SWidget>();
	}

	bool IsSpawnedActor = ActorFactory->CheckActorIsExist(SelectedActor);

	FMenuBuilder MenuBuilder(true /*bCloseAfterSelection*/, TSharedPtr<FUICommandList>());
	MenuBuilder.BeginSection(NAME_None);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Delete")),
		FText::GetEmpty(),
		FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "Icons.Delete"),
		FUIAction(
			FExecuteAction::CreateSP(this, &SActorList::OnDeleteActor, SelectedActor),
			FCanExecuteAction::CreateLambda([IsSpawnedActor]() { return IsSpawnedActor; })
		));
		
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Rename")),
		FText::GetEmpty(),
		FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "Icons.Edit"),
		FUIAction(
			FExecuteAction::CreateSP(this, &SActorList::OnRenameActor, SelectedActor),
			FCanExecuteAction::CreateLambda([IsSpawnedActor]() { return IsSpawnedActor; })
		));

	if (APawn* Pawn = Cast<APawn>(SelectedActor))
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Posses")),
			FText::GetEmpty(),
			FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "ClassIcon.Pawn"),
			FUIAction(
				FExecuteAction::CreateSP(this, &SActorList::OnPossesActor, Pawn)
			));
	}
		
	MenuBuilder.EndSection();
	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
}

FReply SActorList::OnPinClicked(TWeakObjectPtr<AActor> Actor)
{
	if (ISodaActor* SodaActor = Cast<ISodaActor>(Actor))
	{
		bool IsPinnedActor = !SodaActor->IsPinnedActor();
		if (!SodaActor->OnSetPinnedActor(IsPinnedActor))
		{
			soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("The \"%s\" actor can't be pinned"), *Actor->GetClass()->GetName());
		}
		SodaActor->MarkAsDirty();
		USodaSubsystem::GetChecked()->LevelState->MarkAsDirty();
	}
	return FReply::Handled();
}

void SActorList::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bIsInteractiveMode)
	{
		AActor* SelectedActor = ViewportClient->Selection->GetSelectedActor();
		if (SelectedActor != SelectedItem.Get())
		{
			if (SelectedActor)
			{
				TreeView->SetSelection(SelectedActor);
			}
			else
			{
				TreeView->ClearSelection();
			}
		}
	}

	if (RequestedEditeTextBox.IsValid())
	{
		RequestedEditeTextBox.Pin()->EnterEditingMode();
		RequestedEditeTextBox.Reset();
	}
}

void SActorList::OnSelectionChanged(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo)
{
	SelectedItem = Actor;

	if (bIsInteractiveMode)
	{
		if (Actor.IsValid())
		{
			if (ViewportClient->Selection)
			{
				ViewportClient->Selection->SelectActor(Actor.Get(), nullptr);
			}
		}
		else
		{
			if (ViewportClient->Selection)
			{
				ViewportClient->Selection->UnselectActor();
			}
		}
	}

	OnSelectionChangedDelegate.ExecuteIfBound(Actor, SelectInfo);
}

void SActorList::RebuilActorList()
{
	ActorList.Empty();
	if (!ViewportClient.IsValid())
	{
		return;
	}
	for (FActorIterator It(ViewportClient->GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
		{
			if (!ActorFilter.IsBound() || ActorFilter.Execute(Actor))
			{
				ActorList.Add(Actor);
			}
		}
	}
	TreeView->RebuildList();
}

void SActorList::OnDeleteActor(AActor* SelectedActor)
{
	if (IsValid(SelectedActor))
	{
		ActorFactory->RemoveActor(SelectedActor);
		USodaSubsystem::GetChecked()->LevelState->MarkAsDirty();
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("SActorList::OnDeleteActor(); Selected actor faild"));
	}
}

AActor* SActorList::GetSelectedActor() const
{
	return SelectedItem.Get();
}


TSharedRef< ITableRow > SActorList::MakeRowWidget(TWeakObjectPtr<AActor> Actor, const TSharedRef< STableViewBase >& OwnerTable)
{
	return SNew(SActorListRow, OwnerTable, Actor, SharedThis(this));
}

void SActorList::OnRenameActor(AActor* SelectedActor)
{
	if (!IsValid(SelectedActor))
	{
		UE_LOG(LogSoda, Error, TEXT("SActorList::OnRenameActor(); Selected item faild"));
		RebuilActorList();
		return;
	}

	TSharedPtr<ITableRow> TableRow = TreeView->WidgetFromItem(SelectedActor);
	if (!TableRow.IsValid())
	{
		UE_LOG(LogSoda, Error, TEXT("SActorList::OnRenameActor(); Selected item faild"));
		RebuilActorList();
		return;
	}

	if (TableRow->AsWidget()->GetType() != "SActorListRow")
	{
		UE_LOG(LogSoda, Error, TEXT("SActorList::OnRenameActor(); Selected widget faild (%s)"), *TableRow->AsWidget()->GetTypeAsString());
		RebuilActorList();
		return;
	}

	SActorListRow* TableRowWidget = static_cast<SActorListRow*>(&TableRow->AsWidget().Get());

	RequestedEditeTextBox = TableRowWidget->InlineTextBlock;
}

void SActorList::OnPossesActor(APawn* SelectedActor)
{
	if (IsValid(SelectedActor))
	{
		if (APlayerController* PlayerController = SelectedActor->GetWorld()->GetFirstPlayerController())
		{
			PlayerController->Possess(SelectedActor);
		}
	}
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
