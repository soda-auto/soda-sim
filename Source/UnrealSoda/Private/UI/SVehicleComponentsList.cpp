// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SVehicleComponentsList.h"
#include "Soda/UnrealSoda.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/SSearchBox.h"
#include "Kismet/GameplayStatics.h"
#include "Soda/ISodaActor.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IDetailsView.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaGameMode.h"
#include "Soda/Editor/SodaSelection.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "UI/Common/SVehicleComponentClassCombo.h"
#include "UI/Common/SPinWidget.h"
#include "UI/Toolbar/SodaDragDropOp.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/UI/SToolBox.h"
#include "Soda/SodaGameViewportClient.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"

#define LOCTEXT_NAMESPACE "VehicleComponentsBar"

namespace soda
{

static const FName Column_CB_Enable("Enable");
static const FName Column_CB_StartUp("StartUp");
static const FName Column_CB_ItemName("ItemName");
static const FName Column_CB_Remark("Remark");
static const FName Column_CB_Health("Health");

enum class  EComponentsBarTreeType
{
	Filter,
	Component
};

/***********************************************************************************/
class FVehcielComponentDragDropOp : public FSodaDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FVehcielComponentDragDropOp, FDragDropOperation)

	static TSharedRef<FVehcielComponentDragDropOp> New(TWeakInterfacePtr<ISodaVehicleComponent> InComponent)
	{
		check(InComponent.IsValid());
		auto DragDropOp = MakeShared<FVehcielComponentDragDropOp>();
		DragDropOp->CurrentHoverText = FText::FromString(InComponent->AsActorComponent()->GetName());
		DragDropOp->CurrentIconBrush = FSodaStyle::GetBrush(InComponent->GetVehicleComponentGUI().IcanName);
		DragDropOp->Component = InComponent;
		DragDropOp->Construct();
		DragDropOp->SetupDefaults();
		return DragDropOp;
	}

	TWeakInterfacePtr<ISodaVehicleComponent> Component;
};

/***********************************************************************************/
class IComponentsBarTreeNode
{
public:
	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable) = 0;
	virtual TSharedPtr<SWidget> OnContextMenuOpening() = 0;
	virtual EComponentsBarTreeType GetType() = 0;
	virtual TArray< TSharedRef<IComponentsBarTreeNode> > & GetChildren() = 0;
};

/***********************************************************************************/
class SComponentTreeRow : public SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>
{
public:
	SLATE_BEGIN_ARGS(SComponentTreeRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakInterfacePtr<ISodaVehicleComponent> InComponent, TWeakPtr<SVehicleComponentsList> InVehicleComponentsBar)
	{
		check(InComponent.IsValid());
		Component = InComponent;
		VehicleComponentsBar = InVehicleComponentsBar;
		FSuperRowType::FArguments OutArgs;
		//OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
		SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>::Construct(OutArgs, InOwnerTable);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		LayerId = SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		if (bIsDragEventRecognized)
		{
			const float Width = 2.0f;
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId++,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(0, 0), 
					FVector2D(AllottedGeometry.GetLocalSize().X, Width)),
				FSodaStyle::Get().GetBrush(TEXT("Brushes.White")),
				ESlateDrawEffect::None,
				FLinearColor(1, 1, 1, 0.8f));
		}

		return LayerId;
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled().BeginDragDrop(FVehcielComponentDragDropOp::New(Component));
	}

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>::OnDragEnter(MyGeometry, DragDropEvent);

		TSharedPtr< FDragDropOperation > Operation = DragDropEvent.GetOperation();
		if (!Operation.IsValid())
		{
			return;
		}

		if (!Operation->IsOfType<FVehcielComponentDragDropOp>())
		{
			return;
		}

		bIsDragEventRecognized = true;
		TSharedPtr<FVehcielComponentDragDropOp> DragDropOp = StaticCastSharedPtr<FVehcielComponentDragDropOp>(Operation);
		if (Component->GetVehicleComponentGUI().Category == DragDropOp->Component->GetVehicleComponentGUI().Category)
		{
			DragDropOp->CurrentIconBrush = FSodaStyle::GetBrush("Icons.Success");
			DragDropOp->CurrentIconColorAndOpacity = FSlateColor(FLinearColor(0.2f, 8.0f, 0.2f, 0.5f));
		}
		else
		{
			DragDropOp->CurrentIconBrush = FSodaStyle::GetBrush("Icons.Warning");
			DragDropOp->CurrentIconColorAndOpacity = FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f, 0.5f));
		}
	}
	
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>::OnDragLeave(DragDropEvent);

		TSharedPtr< FDragDropOperation > Operation = DragDropEvent.GetOperation();
		if (!Operation.IsValid())
		{
			return;
		}

		if (!Operation->IsOfType<FVehcielComponentDragDropOp>())
		{
			return;
		}

		bIsDragEventRecognized = false;
		TSharedPtr<FVehcielComponentDragDropOp> DragDropOp = StaticCastSharedPtr<FVehcielComponentDragDropOp>(Operation);
		DragDropOp->ResetToDefaultToolTip();
	}
	
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		TSharedPtr< FDragDropOperation > Operation = DragDropEvent.GetOperation();
		if (!Operation.IsValid())
		{
			return FReply::Unhandled();
		}

		if (!Operation->IsOfType<FVehcielComponentDragDropOp>())
		{
			return FReply::Unhandled();
		}

		bIsDragEventRecognized = false;
		TSharedPtr<FVehcielComponentDragDropOp> DragDropOp = StaticCastSharedPtr<FVehcielComponentDragDropOp>(Operation);
		TWeakInterfacePtr<ISodaVehicleComponent> TargetComponent = DragDropOp->Component;

		if (TargetComponent.IsValid() && Component.IsValid() && IsValid(Component->GetVehicle()) && IsValid(TargetComponent->GetVehicle()) && Component->GetVehicle() == TargetComponent->GetVehicle())
		{
			if (Component->GetVehicleComponentGUI().Category == DragDropOp->Component->GetVehicleComponentGUI().Category)
			{
				TargetComponent->GetVehicleComponentGUI().Order = Component->GetVehicleComponentGUI().Order;
				bool bIncriment = false;
				for (auto& It : Component->GetVehicle()->GetVehicleComponentsSorted(Component->GetVehicleComponentGUI().Category, true))
				{
					if (It == Component.Get()) bIncriment = true;
					if(It != TargetComponent.Get() && bIncriment)
					{
						++It->GetVehicleComponentGUI().Order;
					}
				}
				Component->GetVehicle()->GetVehicleComponentsSorted("", true); //Just normalize
				Component->MarkAsDirty();
				VehicleComponentsBar.Pin()->RequestSelectRow(TargetComponent.Get());
				VehicleComponentsBar.Pin()->RebuildNodes();
			}
		}
		return FReply::Handled();
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		check(Component.IsValid());
		if (InColumnName == Column_CB_Enable)
		{
			return
				SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SPinWidget)
					.CheckedHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Sensor")))
					.CheckedNotHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Sensor")))
					.NotCheckedHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Sensor")))
					.NotCheckedNotHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Sensor")))
					.bHideUnchecked(true)
					.IsChecked(TAttribute<bool>::CreateLambda([this]() { return Component->IsVehicleComponentActiveted(); }))
					.OnClicked(FOnClicked::CreateLambda([this]() { Component->Toggle(); return FReply::Handled(); }))
					.Row(SharedThis(this))
				];
		}

		if (InColumnName == Column_CB_StartUp)
		{
			return
				SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SPinWidget)
					.ToolTipText_Lambda([this]() 
					{
						switch(Component->GetVehicleComponentCommon().Activation)
						{
						//case EVehicleComponentActivation::None:
						case EVehicleComponentActivation::OnBeginPlay:
							return FText::FromString("Activate on creation");
						case EVehicleComponentActivation::OnStartScenario:
							return FText::FromString("Activate on scaenario run");
						}
						return FText::FromString("Don't activate");
					})
					.CheckedHoveredBrush_Lambda([this]() 
					{
						switch(Component->GetVehicleComponentCommon().Activation)
						{
						//case EVehicleComponentActivation::None:
						case EVehicleComponentActivation::OnBeginPlay:
							return FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Forever"));
						case EVehicleComponentActivation::OnStartScenario:
							return FSodaStyle::Get().GetBrush(TEXT("SodaViewport.ToggleRealTime"));
						}
						return FSodaStyle::Get().GetBrush(TEXT("None"));
					})
					.CheckedNotHoveredBrush_Lambda([this]() 
					{
						switch(Component->GetVehicleComponentCommon().Activation)
						{
						//case EVehicleComponentActivation::None:
						case EVehicleComponentActivation::OnBeginPlay:
							return FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Forever"));
						case EVehicleComponentActivation::OnStartScenario:
							return FSodaStyle::Get().GetBrush(TEXT("SodaViewport.ToggleRealTime"));
						}
						return FSodaStyle::Get().GetBrush(TEXT("None"));
					})
					.NotCheckedHoveredBrush_Lambda([this]() {
						if (Component->GetVehicleComponentCommon().Activation == EVehicleComponentActivation::None)
						{
							return FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Forever"));
						}
						return FSodaStyle::Get().GetBrush(TEXT("None"));
					})
					//.NotCheckedNotHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Sensor")))
					.bHideUnchecked(true)
					.IsChecked(TAttribute<bool>::CreateLambda([this]() 
					{
						return Component->GetVehicleComponentCommon().Activation != EVehicleComponentActivation::None;
					}))
					.OnClicked(FOnClicked::CreateLambda([this]() 
					{ 
						switch(Component->GetVehicleComponentCommon().Activation)
						{
						case EVehicleComponentActivation::None:
							Component->GetVehicleComponentCommon().Activation = EVehicleComponentActivation::OnBeginPlay;
							break;
						case EVehicleComponentActivation::OnBeginPlay:
							Component->GetVehicleComponentCommon().Activation = EVehicleComponentActivation::OnStartScenario;
							break;
						case EVehicleComponentActivation::OnStartScenario:
							Component->GetVehicleComponentCommon().Activation = EVehicleComponentActivation::None;
							break;
						}
						Component->MarkAsDirty();
						return FReply::Handled();
					}))
					.Row(SharedThis(this))
				];
		}

		if (InColumnName == Column_CB_ItemName)
		{
			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(18, 1, 6, 1)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FSodaStyle::GetBrush(Component->GetVehicleComponentGUI().IcanName))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.7))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 1, 1, 1)
				[
					SAssignNew(InlineTextBlock, SInlineEditableTextBlock)
					.Text(FText::FromString(Component->AsActorComponent()->GetName()))
					.OnTextCommitted(this, &SComponentTreeRow::OnRenameComponent)
					.IsSelected(this, &SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>::IsSelectedExclusively)
				];
		}

		if (InColumnName == Column_CB_Remark)
		{
			return
				SNew(STextBlock)
				.Text_Lambda([this]() 
				{
					FString Remark;
					Component->GetRemark(Remark);
					return FText::FromString(Remark); 
				})
				.ColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f));
		}

		if (InColumnName == Column_CB_Health)
		{
			return SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image( this, &SComponentTreeRow::GetComponentHealthBrush)
					.ToolTipText(this, &SComponentTreeRow::GetComponentHealthToolTip)
					.ColorAndOpacity(this, &SComponentTreeRow::GetComponentHealthColor)
				];
		}

		return SNullWidget::NullWidget;
	}

	void OnRenameComponent(const FText& NewName, ETextCommit::Type Type)
	{
		if (Component.IsValid() && Component->RenameVehicleComponent(NewName.ToString()) && Component->GetVehicleComponentCommon().bIsTopologyComponent)
		{
			//ASodaVehicle* Vehicle = Component->GetVehicle();
			//check(Vehicle);
			//ASodaVehicle* NewVehicle = Vehicle->RespawnVehcile(FVector(0, 0, 50), FRotator::ZeroRotator, true, nullptr);
			//VehicleComponentsBar->SetVehicle(NewVehicle);
		}
		VehicleComponentsBar.Pin()->RebuildNodes();
	}

	const FSlateBrush* GetComponentHealthBrush() const
	{
		if (Component->IsVehicleComponentInitializing())
		{
			return FSodaStyle::GetBrush("SodaIcons.Hourglass");
		}
		switch (Component->GetHealth())
		{
		case EVehicleComponentHealth::Disabled:
			return FSodaStyle::GetBrush("Icons.Hidden");
		case EVehicleComponentHealth::Ok:
			return FSodaStyle::GetBrush("Icons.Success");
		case EVehicleComponentHealth::Warning:
			return FSodaStyle::GetBrush("Icons.Warning");
		case EVehicleComponentHealth::Error:
			return FSodaStyle::GetBrush("SodaIcons.Critical");
		case EVehicleComponentHealth::Undefined:
		default:
			return FSodaStyle::GetBrush("SodaIcons.WTF");
		}
	}

	FSlateColor GetComponentHealthColor() const
	{
		if (Component->IsVehicleComponentInitializing())
		{
			return FSlateColor(FLinearColor(0.0f, 0.5f, 0.8f, 0.5f));
		}
		switch (Component->GetHealth())
		{
		case EVehicleComponentHealth::Disabled:
			return FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.2f));
		case EVehicleComponentHealth::Ok:
			return FSlateColor(FLinearColor(0.2f, 8.0f, 0.2f, 0.5f));
		case EVehicleComponentHealth::Warning:
			return FSlateColor(FLinearColor(0.8f, 0.8f, 0.2f, 0.5f));
		case EVehicleComponentHealth::Error:
			return FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f, 0.5f));
		case EVehicleComponentHealth::Undefined:
		default:
			return FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f, 0.5f));
		}
	}

	FText GetComponentHealthToolTip() const
	{
		if (Component->IsVehicleComponentInitializing())
		{
			return FText::FromString("Component is initializing...");
		}
		switch (Component->GetHealth())
		{
		case EVehicleComponentHealth::Disabled:
			return FText::FromString("Component is disabled");
			break;
		case EVehicleComponentHealth::Ok:
			return FText::FromString("Component is OK");
		case EVehicleComponentHealth::Warning:
			return FText::FromString("There is/are some warning(s)");
		case EVehicleComponentHealth::Error:
			return FText::FromString("Component health is critical");
		case EVehicleComponentHealth::Undefined:
		default:
			return FText::FromString("Undefined health");
		}
	}

	TWeakInterfacePtr<ISodaVehicleComponent> Component;
	bool bIsDragEventRecognized = false;
	TWeakPtr <SVehicleComponentsList> VehicleComponentsBar;
	TSharedPtr<SInlineEditableTextBlock> InlineTextBlock;
};

/***********************************************************************************/
class SFilterTreeRow : public SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>
{
public:
	SLATE_BEGIN_ARGS(SFilterTreeRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, const FString & InCategory, TWeakPtr<SVehicleComponentsList> InVehicleComponentsBar)
	{
		Category = InCategory;
		VehicleComponentsBar = InVehicleComponentsBar;
		FSuperRowType::FArguments OutArgs;
		//OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
		SMultiColumnTableRow<TSharedRef<IComponentsBarTreeNode>>::Construct(OutArgs, InOwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName == Column_CB_ItemName)
		{
			return
				SNew(SBox)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0, 2, 0)
					[
						SNew(SExpanderArrow, SharedThis(this)).IndentAmount(12)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Category))
						.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
					]
				];
		}


		return SNullWidget::NullWidget;
	}

	FString Category;
	TWeakPtr<SVehicleComponentsList> VehicleComponentsBar;
};

/***********************************************************************************/
class FComponentTreeNode : public IComponentsBarTreeNode, public TSharedFromThis<FComponentTreeNode>
{
public:
	FComponentTreeNode(TWeakInterfacePtr<ISodaVehicleComponent> InComponent, TWeakPtr<SVehicleComponentsList> InVehicleComponentsBar) :
		Component(InComponent),
		VehicleComponentsBar(InVehicleComponentsBar)
	{}
	virtual ~FComponentTreeNode() {}

	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable) override
	{
		return SAssignNew(TreeRow, SComponentTreeRow, OwnerTable, Component, VehicleComponentsBar);
	}

	virtual TSharedPtr<SWidget> OnContextMenuOpening() override
	{
		const bool bCloseAfterSelection = true;
		FMenuBuilder MenuBuilder(bCloseAfterSelection, TSharedPtr<FUICommandList>());
		MenuBuilder.BeginSection(NAME_None);

		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Delete")),
			FText::GetEmpty(),
			FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "Icons.Delete"),
			FUIAction(FExecuteAction::CreateSP(this, &FComponentTreeNode::OnDeleteComponent)));

		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Rename")),
			FText::GetEmpty(),
			FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "Icons.Edit"),
			FUIAction(FExecuteAction::CreateSP(this, &FComponentTreeNode::OnRenameComponent)));

		if (Component->GetVehicleComponentGUI().bIsDeleted)
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString(TEXT("Restore")),
				FText::GetEmpty(),
				FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Reset"),
				FUIAction(FExecuteAction::CreateSP(this, &FComponentTreeNode::OnRestoreComponent)));
		}


		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	virtual EComponentsBarTreeType GetType() override { return EComponentsBarTreeType::Component; }

	virtual TArray<TSharedRef<IComponentsBarTreeNode>> & GetChildren() override 
	{ 
		static TArray<TSharedRef<IComponentsBarTreeNode>> Dummy;
		return Dummy;
	}

	void OnRenameComponent()
	{
		VehicleComponentsBar.Pin()->RequestEditeTextBox(TreeRow->InlineTextBlock);
	}

	void OnRestoreComponent()
	{
		Component->GetVehicleComponentGUI().bIsDeleted = false;
		Component->MarkAsDirty();

		if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
		{
			TSharedPtr<soda::SMessageBox> MsgBox = GameMode->ShowMessageBox(
				soda::EMessageBoxType::OK, "Restore Vehicle Component", "Save and restart vehicle to apply the changes");
		}

	}

	void OnDeleteComponent()
	{
		if (Component.IsValid())
		{
			//bool bIsTopologyComponent = Component->GetVehicleComponentCommon().bIsTopologyComponent;
			ASodaVehicle* Vehicle = Component->GetVehicle();
			check(Vehicle);
			Component->GetVehicle()->MarkAsDirty();
			Component->RemoveVehicleComponent();
			//if (bIsTopologyComponent)
			//{
			//	ASodaVehicle * NewVehicle = Vehicle->RespawnVehcile(FVector(0, 0, 50), FRotator::ZeroRotator, true, nullptr);
			//	VehicleComponentsBar->SetVehicle(NewVehicle);
			//}
			VehicleComponentsBar.Pin()->RebuildNodes();
		}
	}

	TWeakInterfacePtr< ISodaVehicleComponent> & GetComponent() { return Component; }

private:
	TWeakInterfacePtr<ISodaVehicleComponent> Component;
	TWeakPtr<SVehicleComponentsList> VehicleComponentsBar;
	TSharedPtr< SComponentTreeRow > TreeRow;
};

/***********************************************************************************/
class FFilterTreeNode: public IComponentsBarTreeNode, public TSharedFromThis<FFilterTreeNode>
{
public:
	FFilterTreeNode(TWeakObjectPtr<ASodaVehicle> InVehicle, const FString& InCategory, const TSharedPtr<SVehicleComponentsList>& InVehicleComponentsBar) :
		Vehicle(InVehicle),
		Category(InCategory),
		VehicleComponentsBar(InVehicleComponentsBar)
	{
		TArray<ISodaVehicleComponent*> VehicleComponents = Vehicle->GetVehicleComponentsSorted(InCategory);
		for (auto& It : VehicleComponents)
		{
			ChildNodes.Add(MakeShared<FComponentTreeNode>(It, VehicleComponentsBar));
		}
	}

	virtual ~FFilterTreeNode() {}

	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable) override
	{
		return SNew(SFilterTreeRow, OwnerTable, Category, VehicleComponentsBar);
	}

	virtual TSharedPtr<SWidget> OnContextMenuOpening() override
	{
		return TSharedPtr<SWidget>();
	}

	virtual EComponentsBarTreeType GetType() override { return EComponentsBarTreeType::Filter; }

	virtual TArray< TSharedRef<IComponentsBarTreeNode> > & GetChildren() override { return ChildNodes; }

private:
	TWeakObjectPtr<ASodaVehicle> Vehicle;
	FString Category;
	TArray<TSharedRef<IComponentsBarTreeNode>> ChildNodes;
	TWeakPtr<SVehicleComponentsList> VehicleComponentsBar;
};

/***********************************************************************************/

SVehicleComponentsList::~SVehicleComponentsList()
{
	if (bInteractiveMode && Vehicle.IsValid())
	{
		if (USodaGameViewportClient* ViewportClient = Cast<USodaGameViewportClient>(Vehicle->GetWorld()->GetGameViewport()))
		{
			ViewportClient->Selection->UnfreeseSelectedActor();
		}
	}

	if (LastSelectdComponen.IsValid())
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(LastSelectdComponen.Get()))
		{
			PrimitiveComponent->MarkRenderStateDirty();
		}
	}
}

void SVehicleComponentsList::Construct( const FArguments& InArgs, ASodaVehicle* InVehicle, bool bInInteractiveMode)
{
	Vehicle = InVehicle;
	bInteractiveMode = bInInteractiveMode;

	if (bInteractiveMode)
	{
		if (USodaGameViewportClient* ViewportClient = Cast<USodaGameViewportClient>(Vehicle->GetWorld()->GetGameViewport()))
		{
			ViewportClient->Selection->FreeseSelectedActor();
		}
	}

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
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				//.Padding(0, 0, 0, 0)
				[
					SNew(SVehicleComponentClassCombo)
					.OnComponentClassSelected(this, &SVehicleComponentsList::OnComponentClassSelected)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.VAlign(VAlign_Center)
				.Padding(5, 0, 0, 0)
				[
					SAssignNew(SearchBox, SSearchBox)
					//.OnTextChanged(this, &SVehicleComponentClassCombo::OnSearchBoxTextChanged)
					//.OnTextCommitted(this, &SVehicleComponentClassCombo::OnSearchBoxTextCommitted);
				] 
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SAssignNew(TreeView, STreeView<TSharedRef<IComponentsBarTreeNode>>)
			.HeaderRow(
				SNew(SHeaderRow)
				+SHeaderRow::Column(Column_CB_Enable)
				.ToolTipText(FText::FromString("Activate/deactivate vehicle Component"))
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
						.Image(FSodaStyle::Get().GetBrush(TEXT("SodaIcons.Sensor")))
					]
				]
				+SHeaderRow::Column(Column_CB_StartUp)
				.ToolTipText(FText::FromString("Sensor auto-activation behavior (never / after creation / when scenario is run)"))
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
						.Image(FSodaStyle::Get().GetBrush(TEXT("SodaIcons.LapCount")))
					]
				]
				+ SHeaderRow::Column(Column_CB_ItemName)
				.DefaultLabel(FText::FromString("Component Name"))
				+ SHeaderRow::Column(Column_CB_Remark)
				.DefaultLabel(FText::FromString("Remark"))
				+ SHeaderRow::Column(Column_CB_Health)	
				.FixedWidth(24)
				.DefaultLabel(FText::FromString(""))
			)
			.SelectionMode(ESelectionMode::Single)
			.TreeItemsSource(&RootNodes)
			.OnGenerateRow(this, &SVehicleComponentsList::MakeRowWidget)
			.OnSelectionChanged(this, &SVehicleComponentsList::OnSelectionChanged)
			.OnContextMenuOpening(this, &SVehicleComponentsList::OnContextMenuOpening)
			.OnGetChildren(this, &SVehicleComponentsList::OnGetChildren)
			.OnMouseButtonDoubleClick(this, &SVehicleComponentsList::OnDoubleClick)
		]
	];
	
	RebuildNodes();
}

UActorComponent* SVehicleComponentsList::OnComponentClassSelected(TSubclassOf<UActorComponent> Component, UObject* Object)
{
	if (Vehicle.IsValid())
	{
		TWeakInterfacePtr<ISodaVehicleComponent> NewComponent = Vehicle->AddVehicleComponent(Component, NAME_None);
		if (NewComponent.IsValid())
		{
			RequestSelectRow(NewComponent.Get());
			Vehicle->MarkAsDirty();
		}
		RebuildNodes();
	}
	return nullptr;
}

void SVehicleComponentsList::RebuildNodes()
{
	RootNodes.Empty();

	if (!Vehicle.IsValid())
	{
		TreeView->RebuildList();
		return;
	}

	TArray<ISodaVehicleComponent*> Components = Vehicle->GetVehicleComponents();
	TSet<FString> Categories;
	for (auto& It : Components)
	{
		Categories.Add(It->GetVehicleComponentGUI().Category);
	}
	TArray<FString> CategoriesSorted = Categories.Array();
	CategoriesSorted.Sort([](const auto& A, const auto& B) { return A.Compare(B, ESearchCase::IgnoreCase) < 0; });
	bool bLastNodeIsDeleted = false;
	if (CategoriesSorted.Remove("DELETED") > 0)
	{
		CategoriesSorted.Add("DELETED"); // Push "DELETED" to the and of the array
		bLastNodeIsDeleted = true;
	}
	for (auto& It : CategoriesSorted)
	{
		RootNodes.Add(MakeShared<FFilterTreeNode>(Vehicle, It, SharedThis(this)));
	}

	TreeView->RebuildList();

	for (auto& It : RootNodes)
	{
		TreeView->SetItemExpansion(It, true);
	}

	if (bLastNodeIsDeleted)
	{
		TreeView->SetItemExpansion(RootNodes.Last(), false);
	}
}

TSharedRef< ITableRow > SVehicleComponentsList::MakeRowWidget(TSharedRef<IComponentsBarTreeNode> Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	return Item->MakeRowWidget(OwnerTable);
}

void SVehicleComponentsList::OnGetChildren(TSharedRef<IComponentsBarTreeNode> InTreeNode, TArray< TSharedRef<IComponentsBarTreeNode> >& OutChildren)
{
	OutChildren = InTreeNode->GetChildren();
}

void SVehicleComponentsList::OnSelectionChanged(TSharedPtr<IComponentsBarTreeNode> Node, ESelectInfo::Type SelectInfo)
{
	if (USodaGameViewportClient* ViewportClient = Cast<USodaGameViewportClient>(Vehicle->GetWorld()->GetGameViewport()))
	{
		if (LastSelectdComponen.IsValid())
		{
			if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(LastSelectdComponen.Get()))
			{
				PrimitiveComponent->MarkRenderStateDirty();
			}
			LastSelectdComponen.Reset();
		}

		if (Node)
		{
			if (Node->GetType() == EComponentsBarTreeType::Component)
			{
				FComponentTreeNode* ComponentNode = static_cast<FComponentTreeNode*> (Node.Get());
				TWeakInterfacePtr<ISodaVehicleComponent> Component = ComponentNode->GetComponent();
				LastSelectdComponen = Component;
				if (Component.IsValid())
				{
					
					if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component.Get()))
					{
						PrimitiveComponent->MarkRenderStateDirty();
					}

					if (USceneComponent* SceneComponent = Cast<USceneComponent>(Component.Get()))
					{
						ViewportClient->SetWidgetTarget(SceneComponent);
						return;
					}
				}
			}
		}
		ViewportClient->SetWidgetTarget(nullptr);
	}
}

void SVehicleComponentsList::OnDoubleClick(TSharedRef<IComponentsBarTreeNode> Node)
{
	if (Node->GetType() == EComponentsBarTreeType::Component)
	{
		FComponentTreeNode* ComponentNode = static_cast<FComponentTreeNode*> (&Node.Get());
		TWeakInterfacePtr<ISodaVehicleComponent> Component = ComponentNode->GetComponent();
		if (Component.IsValid())
		{
			if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
			{
				GameMode->PushToolBox(
					SNew(SToolBox)
					.Caption(FText::FromString(Component->AsActorComponent()->GetOwner()->GetName() + " -> " + Component->AsActorComponent()->GetName()))
					[
						MakeComponentDetailsBar(Component.Get())
					]
				);
			}
		}
	}
}

TSharedPtr<SWidget> SVehicleComponentsList::OnContextMenuOpening()
{
	const int32 NumItems = TreeView->GetNumItemsSelected();
	if (NumItems == 1)
	{
		TArray<TSharedRef<IComponentsBarTreeNode>> SelectedNodes;
		TreeView->GetSelectedItems(SelectedNodes);
		return SelectedNodes[0]->OnContextMenuOpening();
	}
	return TSharedPtr<SWidget>();
}

FReply SVehicleComponentsList::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SVehicleComponentsList::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!Vehicle.IsValid() && RootNodes.Num())
	{
		RebuildNodes();
		RequestedSelecteComponent.Reset();
		RequestedEditeTextBox.Reset();
		return;
	}

	if (!TreeView->IsPendingRefresh())
	{
		if (RequestedSelecteComponent.IsValid())
		{
			class STreeViewPrivate: public STreeView<TSharedRef<IComponentsBarTreeNode>>
			{
			public:
				TArray<TSharedRef<IComponentsBarTreeNode>>& GetLinearizedItems() { return LinearizedItems; }
			};
			STreeViewPrivate* TreeViewPrivate = static_cast<STreeViewPrivate*>(TreeView.Get());
			TArray<TSharedRef<IComponentsBarTreeNode>>& LinearizedItems = TreeViewPrivate->GetLinearizedItems();
			for (auto& It : LinearizedItems)
			{
				if (It->GetType() == EComponentsBarTreeType::Component)
				{
					FComponentTreeNode* Node = static_cast<FComponentTreeNode*>(&It.Get());
					if (Node->GetComponent().IsValid() && Node->GetComponent() == RequestedSelecteComponent)
					{
						TreeView->SetSelection(It);
					}
				}
			}
			RequestedSelecteComponent.Reset();
		}
	}

	if (RequestedEditeTextBox.IsValid())
	{
		RequestedEditeTextBox.Pin()->EnterEditingMode();
		RequestedEditeTextBox.Reset();
	}
}

TSharedRef<SWidget> SVehicleComponentsList::MakeComponentDetailsBar(ISodaVehicleComponent* VehicleComponent)
{
	check(VehicleComponent);

	FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
	soda::FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = false;
	Args.NameAreaSettings = soda::FDetailsViewArgs::HideNameArea;
	Args.bGameModeOnlyVisible = true;
	TSharedPtr<soda::IDetailsView> DetailView = RuntimeEditorModule.CreateDetailView(Args);
	DetailView->SetObject(VehicleComponent->AsActorComponent());
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

	return DetailView.ToSharedRef();
}


} // namespace soda

#undef LOCTEXT_NAMESPACE
