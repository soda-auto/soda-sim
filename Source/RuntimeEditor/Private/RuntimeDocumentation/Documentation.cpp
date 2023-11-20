// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimeDocumentation/Documentation.h"

#include "CoreTypes.h"
#include "Delegates/Delegate.h"
//#include "Dialogs/Dialogs.h"
#include "RuntimeDocumentation/DocumentationLink.h"
#include "RuntimeDocumentation/DocumentationPage.h"
#include "RuntimeDocumentation/RuntimeDocumentationSettings.h"
#include "EngineAnalytics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "IAnalyticsProviderET.h"
//#include "Interfaces/IMainFrameModule.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RuntimeDocumentation/SDocumentationAnchor.h"
#include "RuntimeDocumentation/SDocumentationToolTip.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "SodaStyleSet.h"
//#include "RuntimeDocumentation/UDNParser.h"
//#include "UnrealEdMisc.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SToolTip.h"

#include "Kismet/GameplayStatics.h"

class IDocumentationPage;
class SVerticalBox;
class SWidget;

#define LOCTEXT_NAMESPACE "DocumentationActor"

DEFINE_LOG_CATEGORY(LogDocumentation);



namespace soda
{

TSharedRef< IDocumentation > FDocumentation::Create() 
{
	return MakeShareable( new FDocumentation() );
}

FDocumentation::FDocumentation() 
{
	//IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	//MainFrameModule.OnMainFrameSDKNotInstalled().AddRaw(this, &FDocumentation::HandleSDKNotInstalled);

	if (const URuntimeDocumentationSettings* DocSettings = GetDefault<URuntimeDocumentationSettings>())
	{
		for (const FRuntimeDocumentationBaseUrl& BaseUrl : DocSettings->DocumentationBaseUrls)
		{
			if (!RegisterBaseUrl(BaseUrl.Id, BaseUrl.Url))
			{
				UE_LOG(LogDocumentation, Warning, TEXT("Could not register documentation base URL: %s"), *BaseUrl.Id);
			}
		}
	}

	AddSourcePath(FPaths::Combine(FPaths::ProjectDir(), TEXT("Documentation"), TEXT("Source")));
	for (TSharedRef<IPlugin> Plugin : IPluginManager::Get().GetEnabledPlugins())
	{
		AddSourcePath(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Documentation"), TEXT("Source")));
	}
	AddSourcePath(FPaths::Combine(FPaths::EngineDir(), TEXT("Documentation"), TEXT("Source")));
}

FDocumentation::~FDocumentation() 
{

}

bool FDocumentation::OpenHome(FDocumentationSourceInfo Source, const FString& BaseUrlId) const
{
	return Open(TEXT("%ROOT%"), Source, BaseUrlId);
}

bool FDocumentation::OpenHome(const FCultureRef& Culture, FDocumentationSourceInfo Source, const FString& BaseUrlId) const
{
	return Open(TEXT("%ROOT%"), Culture, Source, BaseUrlId);
}

bool FDocumentation::OpenAPIHome(FDocumentationSourceInfo Source) const
{
	/*
	FString Url;
	FUnrealEdMisc::Get().GetURL(TEXT("APIDocsURL"), Url, true);

	if (!Url.IsEmpty())
	{
		FUnrealEdMisc::Get().ReplaceDocumentationURLWildcards(Url, FInternationalization::Get().GetCurrentCulture());
		FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);

		return true;
	}
	*/
	return false;
}

bool FDocumentation::Open(const FString& Link, FDocumentationSourceInfo Source, const FString& BaseUrlId) const
{
	FString DocumentationUrl;

	// Warn the user if they are opening a URL
	if (Link.StartsWith(TEXT("http")) || Link.StartsWith(TEXT("https")))
	{
		FText Message = LOCTEXT("OpeningURLMessage", "You are about to open an external URL. This will open your web browser. Do you want to proceed?");
		FText URLDialog = LOCTEXT("OpeningURLTitle", "Open external link");

		/*
		FSuppressableWarningDialog::FSetupInfo Info(Message, URLDialog, "SuppressOpenURLWarning");
		Info.ConfirmText = LOCTEXT("OpenURL_yes", "Yes");
		Info.CancelText = LOCTEXT("OpenURL_no", "No");
		FSuppressableWarningDialog OpenURLWarning(Info);
		if (OpenURLWarning.ShowModal() == FSuppressableWarningDialog::Cancel)
		{
			return false;
		}
		else
		{
			FPlatformProcess::LaunchURL(*Link, nullptr, nullptr);
			return true;
		}
		*/
		return false;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("testdocs")))
	{
		FString OnDiskPath = FDocumentationLink::ToFilePath(Link);
		if (IFileManager::Get().FileSize(*OnDiskPath) != INDEX_NONE)
		{
			DocumentationUrl = FDocumentationLink::ToFileUrl(Link, Source);
		}
	}

	
	if (DocumentationUrl.IsEmpty())
	{
		// When opening a doc website we always request the most ideal culture for our documentation.
		// The DNS will redirect us if necessary.
		DocumentationUrl = FDocumentationLink::ToUrl(Link, Source, BaseUrlId);
	}

	if (!DocumentationUrl.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*DocumentationUrl, NULL, NULL);
	}

	if (!DocumentationUrl.IsEmpty() && FEngineAnalytics::IsAvailable())
	{
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.Documentation"), TEXT("OpenedPage"), Link);
	}

	return !DocumentationUrl.IsEmpty();
}

bool FDocumentation::Open(const FString& Link, const FCultureRef& Culture, FDocumentationSourceInfo Source, const FString& BaseUrlId) const
{
	FString DocumentationUrl;

	if (!FParse::Param(FCommandLine::Get(), TEXT("testdocs")))
	{
		FString OnDiskPath = FDocumentationLink::ToFilePath(Link, Culture);
		if (IFileManager::Get().FileSize(*OnDiskPath) != INDEX_NONE)
		{
			DocumentationUrl = FDocumentationLink::ToFileUrl(Link, Culture, Source);
		}
	}

	if (DocumentationUrl.IsEmpty())
	{
		DocumentationUrl = FDocumentationLink::ToUrl(Link, Culture, Source, BaseUrlId);
	}

	if (!DocumentationUrl.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*DocumentationUrl, NULL, NULL);
	}

	if (!DocumentationUrl.IsEmpty() && FEngineAnalytics::IsAvailable())
	{
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.Documentation"), TEXT("OpenedPage"), Link);
	}

	return !DocumentationUrl.IsEmpty();
}

TSharedRef< SWidget > FDocumentation::CreateAnchor( const TAttribute<FString>& Link, const FString& PreviewLink, const FString& PreviewExcerptName, const TAttribute<FString>& BaseUrlId) const
{
	return SNew( SDocumentationAnchor )
		.Link(Link)
		.PreviewLink(PreviewLink)
		.PreviewExcerptName(PreviewExcerptName)
		.BaseUrlId(BaseUrlId);
}

TSharedRef< IDocumentationPage > FDocumentation::GetPage( const FString& Link, const TSharedPtr< FParserConfiguration >& Config, const FDocumentationStyle& Style )
{
	TSharedPtr< IDocumentationPage > Page;
	const TWeakPtr< IDocumentationPage >* ExistingPagePtr = LoadedPages.Find( Link );

	if ( ExistingPagePtr != NULL )
	{
		const TSharedPtr< IDocumentationPage > ExistingPage = ExistingPagePtr->Pin();
		if ( ExistingPage.IsValid() )
		{
			Page = ExistingPage;
		}
	}

	if ( !Page.IsValid() )
	{
		Page = FDocumentationPage::Create( Link/*, FUDNParser::Create(Config, Style)*/);
		LoadedPages.Add( Link, TWeakPtr< IDocumentationPage >( Page ) );
	}

	return Page.ToSharedRef();
}

bool FDocumentation::PageExists(const FString& Link) const
{
	const TWeakPtr< IDocumentationPage >* ExistingPagePtr = LoadedPages.Find(Link);
	if (ExistingPagePtr != NULL)
	{
		return true;
	}

	for (const FString& SourcePath : SourcePaths)
	{
		FString LinkPath = FDocumentationLink::ToSourcePath(Link, SourcePath);
		if (FPaths::FileExists(LinkPath))
		{
			return true;
		}
	}
	return false;
}

bool FDocumentation::PageExists(const FString& Link, const FCultureRef& Culture) const
{
	const TWeakPtr< IDocumentationPage >* ExistingPagePtr = LoadedPages.Find(Link);
	if (ExistingPagePtr != NULL)
	{
		return true;
	}

	for (const FString& SourcePath : SourcePaths)
	{
		FString LinkPath = FDocumentationLink::ToSourcePath(Link, Culture, SourcePath);
		if (FPaths::FileExists(LinkPath))
		{
			return true;
		}
	}
	return false;
}

const TArray <FString>& FDocumentation::GetSourcePaths() const
{
	return SourcePaths;
}

TSharedRef< SToolTip > FDocumentation::CreateToolTip(const TAttribute<FText>& Text, const TSharedPtr<SWidget>& OverrideContent, const FString& Link, const FString& ExcerptName) const
{
	TSharedPtr< SDocumentationToolTip > DocToolTip;

	if ( !Text.IsBound() && Text.Get().IsEmpty() )
	{
		return SNew( SToolTip );
	}

	if ( OverrideContent.IsValid() )
	{
		SAssignNew( DocToolTip, SDocumentationToolTip )
		.DocumentationLink( Link )
		.ExcerptName( ExcerptName )
		[
			OverrideContent.ToSharedRef()
		];
	}
	else
	{
		SAssignNew( DocToolTip, SDocumentationToolTip )
		.Text( Text )
		.DocumentationLink( Link )
		.ExcerptName( ExcerptName );
	}
	
	return SNew(SToolTip)
		.IsInteractive( DocToolTip.ToSharedRef(), &SDocumentationToolTip::IsInteractive )

		// Emulate text-only tool-tip styling that SToolTip uses when no custom content is supplied.  We want documentation tool-tips to 
		// be styled just like text-only tool-tips
		.BorderImage( FSodaStyle::Get().GetBrush("ToolTip.BrightBackground") )
		.TextMargin(FMargin(11.0f))
		[
			DocToolTip.ToSharedRef()
		];
}

TSharedRef< class SToolTip > FDocumentation::CreateToolTip(const TAttribute<FText>& Text, const TSharedRef<SWidget>& OverrideContent, const TSharedPtr<SVerticalBox>& DocVerticalBox, const FString& Link, const FString& ExcerptName) const
{
	TSharedRef<SDocumentationToolTip> DocToolTip =
		SNew(SDocumentationToolTip)
		.Text(Text)
		.DocumentationLink(Link)
		.ExcerptName(ExcerptName)
		.AddDocumentation(false)
		.DocumentationMargin(7)
		[
			OverrideContent
		];

	if (DocVerticalBox.IsValid())
	{
		DocToolTip->AddDocumentation(DocVerticalBox);
	}

	return SNew(SToolTip)
		.IsInteractive(DocToolTip, &SDocumentationToolTip::IsInteractive)

		// Emulate text-only tool-tip styling that SToolTip uses when no custom content is supplied.  We want documentation tool-tips to 
		// be styled just like text-only tool-tips
		.BorderImage( FSodaStyle::Get().GetBrush("ToolTip.BrightBackground") )
		.TextMargin(FMargin(11.0f))
		[
			DocToolTip
		];
}

/*
void FDocumentation::HandleSDKNotInstalled(const FString& PlatformName, const FString& InDocumentationPage)
{
	if (FPackageName::IsValidLongPackageName(InDocumentationPage, true))
	{
		return;
	}
	IDocumentation::Get()->Open(InDocumentationPage);
}
*/

bool FDocumentation::RegisterBaseUrl(const FString& Id, const FString& Url)
{
	if (!Id.IsEmpty() && !Url.IsEmpty())
	{
		if (!RegisteredBaseUrls.Contains(Id))
		{
			RegisteredBaseUrls.Add(Id, Url);
			return true;
		}
		UE_LOG(LogDocumentation, Warning, TEXT("Could not register documentation base URL with ID: %s. This ID is already in use."), *Id);
		return false;
	}
	return false;
}

FString FDocumentation::GetBaseUrl(const FString& Id) const
{
	if (!Id.IsEmpty())
	{
		const FString* BaseUrl = RegisteredBaseUrls.Find(Id);
		if (BaseUrl != NULL && !BaseUrl->IsEmpty())
		{
			return *BaseUrl;
		}
		UE_LOG(LogDocumentation, Warning, TEXT("Could not resolve base URL with ID: %s. It may not have been registered."), *Id);
	}

	FString DefaultUrl = "soda.auto";
	//FUnrealEdMisc::Get().GetURL(TEXT("DocumentationURL"), DefaultUrl, true);
	return DefaultUrl;
}

bool FDocumentation::AddSourcePath(const FString& Path)
{
	if (!Path.IsEmpty() && FPaths::DirectoryExists(Path))
	{
		SourcePaths.Add(Path);
		return true;
	}
	return false;
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
