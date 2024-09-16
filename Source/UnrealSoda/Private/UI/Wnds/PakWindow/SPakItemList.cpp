// Copyright Epic Games, Inc. All Rights Reserved.

#include "SPakItemList.h"
#include "Framework/Views/TableViewMetadata.h"
#include "Widgets/Views/SListView.h"
//#include "SPluginBrowser.h"
//#include "SPluginCategory.h"
#include "SPakItem.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "Soda/SodaApp.h"

#define LOCTEXT_NAMESPACE "PluginList"


void SPakItemList::Construct( const FArguments& Args /*, const TSharedRef< SPluginBrowser > Owner*/)
{
	//OwnerWeak = Owner;

	// Find out when the plugin text filter changes
	//Owner->GetPluginTextFilter().OnChanged().AddSP( this, &SPakItemList::OnPluginTextFilterChanged );

	bIsActiveTimerRegistered = false;
	RebuildAndFilterPluginList();

	// @todo plugedit: Have optional compact version with only plugin icon + name + version?  Only expand selected?

	PakListView =
		SNew( SListView<TSharedRef<FSodaPak>> )
		.SelectionMode( ESelectionMode::None )		// No need to select plugins!
		.ListItemsSource( &PakListItems )
		.OnGenerateRow( this, &SPakItemList::PluginListView_OnGenerateRow )
		.ListViewStyle( FAppStyle::Get(), "SimpleListView" );

	TSharedPtr<SScrollBorder> ChildWidget = SNew(SScrollBorder, PakListView.ToSharedRef())
		.BorderFadeDistance(this, &SPakItemList::GetListBorderFadeDistance)
		[
			PakListView.ToSharedRef()
		];

	ChildSlot.AttachWidget(ChildWidget.ToSharedRef());
}


SPakItemList::~SPakItemList()
{
	/*
	const TSharedPtr< SPluginBrowser > Owner( OwnerWeak.Pin() );
	if( Owner.IsValid() )
	{
		Owner->GetPluginTextFilter().OnChanged().RemoveAll( this );
	}
	*/
}


/*
SPluginBrowser& SPakItemList::GetOwner()
{
	return *OwnerWeak.Pin();
}
*/


TSharedRef<ITableRow> SPakItemList::PluginListView_OnGenerateRow( TSharedRef<FSodaPak> Item, const TSharedRef<STableViewBase>& OwnerTable )
{
	return
		SNew( STableRow< TSharedRef<FSodaPak> >, OwnerTable )
		[
			SNew( SPakItem, SharedThis( this ), Item )
		];
}



void SPakItemList::RebuildAndFilterPluginList()
{
	// Build up the initial list of plugins
	{
		PakListItems.Reset();

		// Get the currently selected category
		/*
		const TSharedPtr<FPluginCategory>& SelectedCategory = OwnerWeak.Pin()->GetSelectedCategory();
		if( SelectedCategory.IsValid() )
		{
			for(TSharedRef<FSodaPak> Plugin: SelectedCategory->Plugins)
			{
				if(OwnerWeak.Pin()->GetPluginTextFilter().PassesFilter(&Plugin.Get()))
				{
					PakListItems.Add(Plugin);
				}
			}
		}
		*/

		for (auto & Pak : FSodaPakModule::Get().GetSodaPaks())
		{
			PakListItems.Add(Pak.ToSharedRef());
		}

		// Sort the plugins alphabetically
		{
			struct FPluginListItemSorter
			{
				bool operator()(const TSharedRef<FSodaPak>& A, const TSharedRef<FSodaPak>& B) const
				{
					return A->GetDescriptor().FriendlyName < B->GetDescriptor().FriendlyName;
				}
			};
			PakListItems.Sort( FPluginListItemSorter() );
		}
	}


	// Update the list widget
	if( PakListView.IsValid() )
	{
		PakListView->RequestListRefresh();
	}
}

EActiveTimerReturnType SPakItemList::TriggerListRebuild(double InCurrentTime, float InDeltaTime)
{
	RebuildAndFilterPluginList();

	bIsActiveTimerRegistered = false;
	return EActiveTimerReturnType::Stop;
}

void SPakItemList::OnPluginTextFilterChanged()
{
	SetNeedsRefresh();
}


void SPakItemList::SetNeedsRefresh()
{
	if (!bIsActiveTimerRegistered)
	{
		bIsActiveTimerRegistered = true;
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SPakItemList::TriggerListRebuild));
	}
}

FVector2D SPakItemList::GetListBorderFadeDistance() const
{
	// Negative fade distance when there is no restart editor warning to make the shadow disappear
	//FVector2D ReturnVal = FPluginBrowserModule::Get().HasPluginsPendingEnable() ? FVector2D(0.01f, 0.01f) : FVector2D(-1.0f, -1.0f);

	//return ReturnVal;

	return FVector2D(-1.0f, -1.0f);
}

#undef LOCTEXT_NAMESPACE
