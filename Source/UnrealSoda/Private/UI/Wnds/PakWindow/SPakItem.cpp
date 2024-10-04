// Copyright Epic Games, Inc. All Rights Reserved.

#include "SPakItem.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/MessageDialog.h"
#include "IDesktopPlatform.h"
#include "Misc/App.h"
#include "Framework/Application/SlateApplication.h"
#include "PluginReferenceDescriptor.h"
#include "Widgets/Images/SImage.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Input/SCheckBox.h"
#include "SPakItemList.h"
#include "Widgets/Input/SHyperlink.h"
#include "DesktopPlatformModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "SodaStyleSet.h"
#include "Interfaces/IPluginManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "PakListItem"

void SPakItem::Construct( const FArguments& Args, const TSharedRef<SPakItemList> Owner, TSharedRef<FSodaPak> InSodaPak)
{
	OwnerWeak = Owner;
	SodaPak = InSodaPak;

	RecreateWidgets();
}

FText SPakItem::GetPakNameText() const
{
	return FText::FromString(SodaPak->GetDescriptor().PakName);
}

void SPakItem::RecreateWidgets()
{
	const float PaddingAmount = FSodaStyle::Get().GetFloat( "PakItem.Padding" );
	const float ThumbnailImageSize = FSodaStyle::Get().GetFloat("PakItem.ThumbnailImageSize");
	const float HorizontalTilePadding = FSodaStyle::Get().GetFloat("PakItem.HorizontalTilePadding");
	const float VerticalTilePadding = FSodaStyle::Get().GetFloat("PakItem.VerticalTilePadding");

	// @todo plugedit: Also display whether plugin is editor-only, runtime-only, developer or a combination?
	//		-> Maybe a filter for this too?  (show only editor plugins, etc.)
	// @todo plugedit: Indicate whether plugin has content?  Filter to show only content plugins, and vice-versa?

	// @todo plugedit: Maybe we should do the FileExists check ONCE at plugin load time and not at query time

	const auto & PakDescriptor = SodaPak->GetDescriptor();

	FString Icon128FilePath = SodaPak->GetIconFileName();
	if(!FPlatformFileManager::Get().GetPlatformFile().FileExists(*Icon128FilePath))
	{
		Icon128FilePath = IPluginManager::Get().FindPlugin(TEXT("SodaSim"))->GetContentDir() / TEXT("Slate") / TEXT("pak.png");
	}

	const FName BrushName( *Icon128FilePath );
	const FIntPoint Size = FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(BrushName);
	if ((Size.X > 0) && (Size.Y > 0))
	{
		PluginIconDynamicImageBrush = MakeShareable(new FSlateDynamicImageBrush(BrushName, FVector2D(Size.X, Size.Y)));
	}




	// create vendor link
	TSharedPtr<SWidget> CreatedByWidget;
	{
		if (PakDescriptor.CreatedBy.IsEmpty())
		{
			CreatedByWidget = SNullWidget::NullWidget;
		}
		else if (PakDescriptor.CreatedByURL.IsEmpty())
		{
			CreatedByWidget = SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(PakDescriptor.CreatedBy))
				];
		}
		else
		{
			FString CreatedByURL = PakDescriptor.CreatedByURL;
			CreatedByWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f, 0.0f, 0.0f)
				[				
					SNew(SHyperlink)
						.Text(FText::FromString(PakDescriptor.CreatedBy))
						.ToolTipText(FText::Format(LOCTEXT("NavigateToCreatedByURL", "Visit the vendor's web site ({0})"), FText::FromString(CreatedByURL)))
						.OnNavigate_Lambda([=]() { FPlatformProcess::LaunchURL(*CreatedByURL, nullptr, nullptr); })
						.Style(FSodaStyle::Get(), "HoverOnlyHyperlink")
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FSodaStyle::Get().GetBrush("Icons.OpenInBrowser"))
				];
		}
	}


	ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			.Padding(FMargin(HorizontalTilePadding, VerticalTilePadding))
			[
				SNew(SBorder)
					.BorderImage(FSodaStyle::Get().GetBrush("PakItem.BorderImage"))
					.Padding(PaddingAmount)
					[
						SNew(SHorizontalBox)

						// Enable checkbox
						+ SHorizontalBox::Slot()
							.Padding(FMargin(18, 0, 17, 0))
							.HAlign(HAlign_Left)
							.AutoWidth()
							[
								SNew(SCheckBox)
									.OnCheckStateChanged(this, &SPakItem::OnEnablePluginCheckboxChanged)
									.IsChecked(this, &SPakItem::IsPakInstalled)
									.ToolTipText(LOCTEXT("EnableDisableButtonToolTip", "Toggles whether this pak is enabled for your current project."))
							]
						// Thumbnail image
						+ SHorizontalBox::Slot()
							.Padding(PaddingAmount, PaddingAmount + 4.f, PaddingAmount + 10.f, PaddingAmount)
							.VAlign(VAlign_Top)
							.AutoWidth()
							[
								SNew(SBox)
								.WidthOverride(ThumbnailImageSize)
								.HeightOverride(ThumbnailImageSize)
								[
									SNew(SBorder)
									.BorderImage(FSodaStyle::Get().GetBrush("PakItem.ThumbnailBorderImage"))
									[
										SNew(SImage)
										.Image(PluginIconDynamicImageBrush.Get())
									]
								]
								
							]

						+SHorizontalBox::Slot()
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										// Friendly name
										+SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											.Padding(PaddingAmount, PaddingAmount + 3.f, PaddingAmount + 8.f, 0.f)
											[
												SNew(STextBlock)
													.Text(GetPakNameText())
													//.HighlightText_Raw(&OwnerWeak.Pin()->GetOwner().GetPluginTextFilter(), &FPluginTextFilter::GetRawFilterText)
													.TextStyle(FSodaStyle::Get(), "PakItem.NameText")
											]

										// Gap
										+ SHorizontalBox::Slot()
											[
												SNew(SSpacer)
											]

										// Version
										+ SHorizontalBox::Slot()
											.HAlign(HAlign_Right)
											.Padding(PaddingAmount, PaddingAmount, PaddingAmount, 0)
											.AutoWidth()
											[
												SNew(SHorizontalBox)
																								
												// version number
												+ SHorizontalBox::Slot()
													.AutoWidth()
													.VAlign( VAlign_Bottom )
													.Padding(0.0f, 6.0f, 0.0f, 1.0f) // Lower padding to align font with version number base
													[
														SNew(STextBlock)
															.Text(LOCTEXT("PluginVersionLabel", "Version "))
															.TextStyle(FSodaStyle::Get(), "PakItem.VersionNumberText")
													]

												+ SHorizontalBox::Slot()
													.AutoWidth()
													.VAlign( VAlign_Bottom )
													.Padding( 0.0f, 3.0f, 16.0f, 1.0f )	// Extra padding from the right edge
													[
														SNew(STextBlock)
															.Text(FText::FromString(FString::FromInt(PakDescriptor.PakVersion)))
															.TextStyle(FSodaStyle::Get(), "PakItem.VersionNumberText")
													]
											]
									]
			
								+ SVerticalBox::Slot()
									[
										SNew(SVerticalBox)
				
										// Description
										+ SVerticalBox::Slot()
											.Padding( PaddingAmount, 0, PaddingAmount, PaddingAmount)
											[
												SNew(SHorizontalBox)

												+SHorizontalBox::Slot()
												[
													SNew(STextBlock)
													.Text(FText::FromString(PakDescriptor.Description))
													//.HighlightText_Raw(&OwnerWeak.Pin()->GetOwner().GetPluginTextFilter(), &FPluginTextFilter::GetRawFilterText)
													.AutoWrapText(true)
												]
												+SHorizontalBox::Slot()
												.AutoWidth()
												.HAlign(HAlign_Right)
												.VAlign(VAlign_Top)
												.Padding(PaddingAmount + 14.f, 0.f, PaddingAmount + 14.f, 0.f)
												[
													CreatedByWidget.ToSharedRef()
												]
											]

										+ SVerticalBox::Slot()
											.Padding(PaddingAmount, PaddingAmount + 5.f, PaddingAmount, PaddingAmount + 4.f)
											.AutoHeight()
											[
												SNew(SHorizontalBox)
												+ SHorizontalBox::Slot()
												.AutoWidth()
												.VAlign(VAlign_Center)
												[
													SNew(SImage)
													.ColorAndOpacity(FSlateColor::UseForeground())
													.Image_Lambda([this]() {

														if (SodaPak->GetInstallStatus() == ESodaPakInstallStatus::Broken) return FSodaStyle::Get().GetBrush("Icons.ErrorWithColor");
														else if (SodaPak->IsMounted()) return FSodaStyle::Get().GetBrush("Icons.SuccessWithColor");
														else return FSodaStyle::Get().GetBrush("Icons.InfoWithColor");
													})
												]

												+ SHorizontalBox::Slot()
												.AutoWidth()
												.VAlign(VAlign_Center)
												.Padding(4.0f, 0.0f, 0.0f, 0.0f)
												[
													SNew(STextBlock)
													.Text_Lambda([this]() {
														if (SodaPak->GetInstallStatus() == ESodaPakInstallStatus::Broken) return LOCTEXT("SodaPakMountStatus_Faild", "Faild");
														else if (SodaPak->IsMounted()) return LOCTEXT("SodaPakMountStatus_Mounted", "Mounted");
														else return LOCTEXT("SodaPakMountStatus_Unmounted", "Unmounted");
													})
													.ToolTipText(LOCTEXT("SodaPakMountStatus_ToolTip", "Pak file mount status"))
												]
											]
									]
							]
					]
			]
	];
}

ECheckBoxState SPakItem::IsPakInstalled() const
{
	return SodaPak->GetInstallStatus() == ESodaPakInstallStatus::Installed ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

/*
void FindPakDependencies(const FString& Name, TSet<FString>& Dependencies, TMap<FString, FPakItem*>& NameToPlugin)
{
	FPakItem* Plugin = NameToPlugin.FindRef(Name);
	if (Plugin != nullptr)
	{
		for (const FPluginReferenceDescriptor& Reference : Plugin->Desc.Dependencies)
		{
			if (Reference.bEnabled && !Dependencies.Contains(Reference.Name))
			{
				Dependencies.Add(Reference.Name);
				FindPakDependencies(Reference.Name, Dependencies, NameToPlugin);
			}
		}
	}
}
*/

void SPakItem::OnEnablePluginCheckboxChanged(ECheckBoxState NewCheckedState)
{
	const bool bNewEnabledState = NewCheckedState == ECheckBoxState::Checked;

	auto PrevStatus = SodaPak->GetInstallStatus();
	
	if (bNewEnabledState)
	{
		SodaPak->Install();
		//FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Faild to mount \"%s\""), *SodaPak->GetPakFileName())));
		
	}
	else
	{
		SodaPak->Uninstall();
	}

	if ((SodaPak->GetInstallStatus() != PrevStatus) && (
		(SodaPak->GetInstallStatus() == ESodaPakInstallStatus::Installed && !SodaPak->IsMounted()) ||
		(SodaPak->GetInstallStatus() == ESodaPakInstallStatus::Uninstalled && SodaPak->IsMounted())))
	{
		FNotificationInfo Info(FText::FromString(TEXT("You must restart SODA.Sim for your changes to take effect")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.WarningWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

EVisibility SPakItem::GetAuthoringButtonsVisibility() const
{
	/*
	if (FApp::IsEngineInstalled() && Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine)
	{
		return EVisibility::Collapsed;
	}
	if (FApp::IsInstalled() && Plugin->GetType() != EPluginType::Mod)
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::Visible;
	*/

	return EVisibility::Collapsed;
}



#undef LOCTEXT_NAMESPACE
