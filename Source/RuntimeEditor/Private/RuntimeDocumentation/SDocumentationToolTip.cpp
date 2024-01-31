// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimeDocumentation/SDocumentationToolTip.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SToolTip.h"
#include "Styling/AppStyle.h"
#include "SodaStyleSet.h"
//#include "Editor/EditorPerProjectUserSettings.h"
#include "RuntimeDocumentation/IDocumentationPage.h"
#include "RuntimeDocumentation/IDocumentation.h"
#include "RuntimeDocumentation/DocumentationLink.h"
//#include "ISourceCodeAccessor.h"
//#include "ISourceCodeAccessModule.h"
//#include "SourceControlHelpers.h"
#include "EngineAnalytics.h"
#include "AnalyticsEventAttribute.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "Widgets/Input/SHyperlink.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace soda
{

void SDocumentationToolTip::Construct( const FArguments& InArgs )
{
	TextContent = InArgs._Text;
	StyleInfo = FSodaStyle::GetWidgetStyle<FTextBlockStyle>(InArgs._Style);
	SubduedStyleInfo = FSodaStyle::GetWidgetStyle<FTextBlockStyle>(InArgs._SubduedStyle);
	HyperlinkTextStyleInfo = FSodaStyle::GetWidgetStyle<FTextBlockStyle>(InArgs._HyperlinkTextStyle);
	HyperlinkButtonStyleInfo = FSodaStyle::GetWidgetStyle<FButtonStyle>(InArgs._HyperlinkButtonStyle);
	ColorAndOpacity = InArgs._ColorAndOpacity;
	DocumentationLink = InArgs._DocumentationLink;
	IsDisplayingDocumentationLink = false;
	bAddDocumentation = InArgs._AddDocumentation;
	DocumentationMargin = InArgs._DocumentationMargin;

	ExcerptName = InArgs._ExcerptName;
	IsShowingFullTip = false;

	if( InArgs._Content.Widget != SNullWidget::NullWidget )
	{
		// Widget content argument takes precedence
		// overrides the text content.
		OverrideContent = InArgs._Content.Widget;
	}

	ConstructSimpleTipContent();

	ChildSlot
	[
		SAssignNew(WidgetContent, SBox)
		[
			SimpleTipContent.ToSharedRef()
		]
	];
}

void SDocumentationToolTip::ConstructSimpleTipContent()
{
	// If there a UDN file that matches the DocumentationLink path, and that page has an excerpt whose name
	// matches ExcerptName, and that excerpt has a variable named ToolTipOverride, use the content of that
	// variable instead of the default TextContent.
	if (!DocumentationLink.IsEmpty() && !ExcerptName.IsEmpty())
	{
		TSharedRef<IDocumentation> Documentation = IDocumentation::Get();
		if (Documentation->PageExists(DocumentationLink))
		{
			DocumentationPage = Documentation->GetPage(DocumentationLink, NULL);

			FExcerpt Excerpt;
			if (DocumentationPage->HasExcerpt(ExcerptName))
			{
				if (DocumentationPage->GetExcerpt(ExcerptName, Excerpt))
				{
					if (FString* TooltipValue = Excerpt.Variables.Find(TEXT("ToolTipOverride")))
					{
						TextContent = FText::FromString(*TooltipValue);
					}
				}
			}
		}
	}

	TSharedPtr< SVerticalBox > VerticalBox;
	if ( !OverrideContent.IsValid() )
	{
		SAssignNew( SimpleTipContent, SBox )
		[
			SAssignNew( VerticalBox, SVerticalBox )
			+SVerticalBox::Slot()
			.FillHeight( 1.0f )
			[
				SNew( STextBlock )
				.Text( TextContent )
				.TextStyle( &StyleInfo )
				.ColorAndOpacity( ColorAndOpacity )
				.WrapTextAt_Static( &SToolTip::GetToolTipWrapWidth )
			]
		];
	}
	else
	{
		SAssignNew( SimpleTipContent, SBox )
		[
			SAssignNew( VerticalBox, SVerticalBox )
			+SVerticalBox::Slot()
			.FillHeight( 1.0f )
			[
				OverrideContent.ToSharedRef()
			]
		];
	}

	if (bAddDocumentation)
	{
		AddDocumentation(VerticalBox);
	}
}

void SDocumentationToolTip::AddDocumentation(TSharedPtr< SVerticalBox > VerticalBox)
{
	if ( !DocumentationLink.IsEmpty() )
	{
		//IsDisplayingDocumentationLink = GetDefault<UEditorPerProjectUserSettings>()->bDisplayDocumentationLink;

		if ( /*IsDisplayingDocumentationLink*/ false)
		{
			FString OptionalExcerptName;
			if ( !ExcerptName.IsEmpty() )
			{ 
				OptionalExcerptName = FString( TEXT(" [") ) + ExcerptName + TEXT("]");
			}

			VerticalBox->AddSlot()
			.AutoHeight()
			.Padding(0, 5, 0, 0)
			.HAlign( HAlign_Center )
			[
				SNew( STextBlock )
				.Text( FText::FromString(DocumentationLink + OptionalExcerptName) )
				.TextStyle( &SubduedStyleInfo )
			];
		}

		if ( !DocumentationPage.IsValid() )
		{
			DocumentationPage = IDocumentation::Get()->GetPage( DocumentationLink, NULL );
		}

		if ( DocumentationPage->HasExcerpt( ExcerptName ) )
		{
			FText MacShortcut = NSLOCTEXT("SToolTip", "MacRichTooltipShortcut", "(Command + Option)");
			FText WinShortcut = NSLOCTEXT("SToolTip", "WinRichTooltipShortcut", "(Ctrl + Alt)");

			FText KeyboardShortcut;
#if PLATFORM_MAC
			KeyboardShortcut = MacShortcut;
#else
			KeyboardShortcut = WinShortcut;
#endif

			VerticalBox->AddSlot()
			.AutoHeight()
			.HAlign( HAlign_Center )
			.Padding(0, 5, 0, 0)
			[
				SNew( STextBlock )
				.TextStyle( &SubduedStyleInfo )
				.Text( FText::Format( NSLOCTEXT( "SToolTip", "AdvancedToolTipMessage", "hold {0} for more" ), KeyboardShortcut) )
			];
		}
		else
		{
			if ( IsDisplayingDocumentationLink && FSlateApplication::Get().SupportsSourceAccess() )
			{
				FString DocPath = FDocumentationLink::ToSourcePath( DocumentationLink, FInternationalization::Get().GetCurrentCulture() );
				if ( !FPaths::FileExists(DocPath) )
				{
					DocPath = FPaths::ConvertRelativePathToFull(DocPath);
				}

				VerticalBox->AddSlot()
				.AutoHeight()
				.Padding(0, 5, 0, 0)
				.HAlign( HAlign_Center )
				[
					SNew( SHyperlink )
					.Text( NSLOCTEXT( "SToolTip", "EditDocumentationMessage_Create", "create" ) )
					.TextStyle( &HyperlinkTextStyleInfo )
					.UnderlineStyle( &HyperlinkButtonStyleInfo )
					.OnNavigate( this, &SDocumentationToolTip::CreateExcerpt, DocPath, ExcerptName )
				];
			}
		}
	}
}

void SDocumentationToolTip::CreateExcerpt( FString FileSource, FString InExcerptName )
{
	FText CheckoutFailReason;
	bool bNewFile = true;
	bool bCheckoutOrAddSucceeded = true;

	/*
	if (FPaths::FileExists(FileSource))
	{
		// Check out the existing file
		bNewFile = false;
		bCheckoutOrAddSucceeded = SourceControlHelpers::CheckoutOrMarkForAdd(FileSource, NSLOCTEXT("SToolTip", "DocumentationSCCActionDesc", "tool tip excerpt"), FOnPostCheckOut(), CheckoutFailReason);
	}
*/

	FArchive* FileWriter = IFileManager::Get().CreateFileWriter( *FileSource, EFileWrite::FILEWRITE_Append | EFileWrite::FILEWRITE_AllowRead | EFileWrite::FILEWRITE_EvenIfReadOnly );

	if (bNewFile)
	{
		FString UdnHeader;
		UdnHeader += "Availability:NoPublish";
		UdnHeader += LINE_TERMINATOR;
		UdnHeader += "Title:";
		UdnHeader += LINE_TERMINATOR;
		UdnHeader += "Crumbs:";
		UdnHeader += LINE_TERMINATOR;
		UdnHeader += "Description:";
		UdnHeader += LINE_TERMINATOR;

		FileWriter->Serialize( TCHAR_TO_ANSI( *UdnHeader ), UdnHeader.Len() );
	}

	FString NewExcerpt;
	NewExcerpt += LINE_TERMINATOR;
	NewExcerpt += "[EXCERPT:";
	NewExcerpt += InExcerptName;
	NewExcerpt += "]";
	NewExcerpt += LINE_TERMINATOR;

	NewExcerpt += TextContent.Get().ToString();
	NewExcerpt += LINE_TERMINATOR;

	NewExcerpt += "[/EXCERPT:";
	NewExcerpt += InExcerptName;
	NewExcerpt += "]";
	NewExcerpt += LINE_TERMINATOR;

	if (!bNewFile)
	{
		FileWriter->Seek( FMath::Max( FileWriter->TotalSize(), (int64)0 ) );
	}

	FileWriter->Serialize( TCHAR_TO_ANSI( *NewExcerpt ), NewExcerpt.Len() );

	FileWriter->Close();
	delete FileWriter;

	/*
	if (bNewFile)
	{
		// Add the new file
		bCheckoutOrAddSucceeded = SourceControlHelpers::CheckoutOrMarkForAdd(FileSource, NSLOCTEXT("SToolTip", "DocumentationSCCActionDesc", "tool tip excerpt"), FOnPostCheckOut(), CheckoutFailReason);
	}
	*/

	//ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
	//SourceCodeAccessModule.GetAccessor().OpenFileAtLine(FileSource, 0);

	if (!bCheckoutOrAddSucceeded)
	{
		FNotificationInfo Info(CheckoutFailReason);
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	ReloadDocumentation();
}

void SDocumentationToolTip::ConstructFullTipContent()
{
	TArray< FExcerpt > Excerpts;
	DocumentationPage->GetExcerpts( Excerpts );

	if ( Excerpts.Num() > 0 )
	{
		int32 ExcerptIndex = 0;
		if ( !ExcerptName.IsEmpty() )
		{
			for (int Index = 0; Index < Excerpts.Num(); Index++)
			{
				if ( Excerpts[ Index ].Name == ExcerptName )
				{
					ExcerptIndex = Index;
					break;
				}
			}
		}

		if ( !Excerpts[ ExcerptIndex ].Content.IsValid() )
		{
			DocumentationPage->GetExcerptContent( Excerpts[ ExcerptIndex ] );
		}

		if ( Excerpts[ ExcerptIndex ].Content.IsValid() )
		{
			TSharedPtr< SVerticalBox > Box;
			FullTipContent = 
				SNew(SBox)
				.Padding(DocumentationMargin)
				[
					SAssignNew(Box, SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.AutoHeight()
					[
						Excerpts[ExcerptIndex].Content.ToSharedRef()
					]
				];

			FString* FullDocumentationLink = Excerpts[ ExcerptIndex ].Variables.Find( TEXT("ToolTipFullLink") );
			FString* ExcerptBaseUrl = Excerpts[ExcerptIndex].Variables.Find(TEXT("BaseUrl"));
			if ( FullDocumentationLink != NULL && !FullDocumentationLink->IsEmpty() )
			{
				FString BaseUrl = FString();
				if (ExcerptBaseUrl != NULL)
				{
					BaseUrl = *ExcerptBaseUrl;
				}

				Box->AddSlot()
				.HAlign( HAlign_Center )
				.AutoHeight()
				[
					SNew( SHyperlink )
						.Text( NSLOCTEXT( "SToolTip", "GoToFullDocsLinkMessage", "see full documentation" ) )
						.TextStyle( &HyperlinkTextStyleInfo )
						.UnderlineStyle( &HyperlinkButtonStyleInfo )
						.OnNavigate_Static([](FString Link, FString BaseUrl) {
								if (!IDocumentation::Get()->Open(Link, FDocumentationSourceInfo(TEXT("rich_tooltips")), BaseUrl))
								{
									FNotificationInfo Info(NSLOCTEXT("SToolTip", "FailedToOpenLink", "Failed to Open Link"));
									FSlateNotificationManager::Get().AddNotification(Info);
								}
							}, *FullDocumentationLink, BaseUrl)
				];
			}

			/*
			if ( GetDefault<UEditorPerProjectUserSettings>()->bDisplayDocumentationLink && FSlateApplication::Get().SupportsSourceAccess() )
			{
				Box->AddSlot()
				.AutoHeight()
				.HAlign( HAlign_Center )
				[
					SNew( SHyperlink )
						.Text( NSLOCTEXT( "SToolTip", "EditDocumentationMessage_Edit", "edit" ) )
						.TextStyle( &HyperlinkTextStyleInfo )
						.UnderlineStyle( &HyperlinkButtonStyleInfo )
						// todo: needs to update to point to the "real" source file used for the excerpt
						.OnNavigate_Static([](FString Link, int32 LineNumber) {
								ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
								SourceCodeAccessModule.GetAccessor().OpenFileAtLine(Link, LineNumber);
							}, FPaths::ConvertRelativePathToFull(FDocumentationLink::ToSourcePath(DocumentationLink, FInternationalization::Get().GetCurrentCulture())), Excerpts[ExcerptIndex].LineNumber)
				];
			}
			*/
		}
	}
}

FReply SDocumentationToolTip::ReloadDocumentation()
{
	SimpleTipContent.Reset();
	FullTipContent.Reset();

	ConstructSimpleTipContent();

	if ( DocumentationPage.IsValid() )
	{
		DocumentationPage->Reload();

		if ( DocumentationPage->HasExcerpt( ExcerptName ) )
		{
			ConstructFullTipContent();
		}
	}

	return FReply::Handled();
}

void SDocumentationToolTip::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	const bool NeedsUpdate = false; //IsDisplayingDocumentationLink != GetDefault<UEditorPerProjectUserSettings>()->bDisplayDocumentationLink;

	if ( !IsShowingFullTip && ModifierKeys.IsAltDown() && ModifierKeys.IsControlDown() )
	{
		if ( !FullTipContent.IsValid() && DocumentationPage.IsValid() && DocumentationPage->HasExcerpt( ExcerptName ) )
		{
			ConstructFullTipContent();
		}
		else if ( /*GetDefault<UEditorPerProjectUserSettings>()->bDisplayDocumentationLink*/ false)
		{
			ReloadDocumentation();
		}

		if ( FullTipContent.IsValid() )
		{
			WidgetContent->SetContent( FullTipContent.ToSharedRef() );
			IsShowingFullTip = true;

			// Analytics event
			if (FEngineAnalytics::IsAvailable())
			{
				TArray<FAnalyticsEventAttribute> Params;
				Params.Add(FAnalyticsEventAttribute(TEXT("Page"), DocumentationLink));
				Params.Add(FAnalyticsEventAttribute(TEXT("Excerpt"), ExcerptName));

				FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.Documentation.FullTooltipShown"), Params);
			}
		}
	}
	else if ( ( IsShowingFullTip || NeedsUpdate )  && ( !ModifierKeys.IsAltDown() || !ModifierKeys.IsControlDown() ) )
	{
		if ( NeedsUpdate )
		{
			ReloadDocumentation();
			//IsDisplayingDocumentationLink = GetDefault<UEditorPerProjectUserSettings>()->bDisplayDocumentationLink;
		}

		WidgetContent->SetContent( SimpleTipContent.ToSharedRef() );
		IsShowingFullTip = false;
	}
}

bool SDocumentationToolTip::IsInteractive() const
{
	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	return ( DocumentationPage.IsValid() && ModifierKeys.IsAltDown() && ModifierKeys.IsControlDown() );
}

} // namespace soda