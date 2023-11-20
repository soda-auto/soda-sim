// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "RuntimeDocumentation/IDocumentation.h"
#include "Internationalization/CulturePointer.h"
#include "Templates/SharedPointer.h"

class FText;
class SVerticalBox;
class SWidget;
template< typename ObjectType > class TAttribute;

DECLARE_LOG_CATEGORY_EXTERN(LogDocumentation, Log, All);

namespace soda
{

class IDocumentationPage;

class FDocumentation : public IDocumentation
{
public:

	static TSharedRef< IDocumentation > Create();

public:

	virtual ~FDocumentation();

	virtual bool OpenHome(FDocumentationSourceInfo Source = FDocumentationSourceInfo(), const FString& BaseUrlId = FString()) const override;

	virtual bool OpenHome(const FCultureRef& Culture, FDocumentationSourceInfo Source = FDocumentationSourceInfo(), const FString& BaseUrlId = FString()) const override;

	virtual bool OpenAPIHome(FDocumentationSourceInfo Source = FDocumentationSourceInfo()) const override;

	virtual bool Open( const FString& Link, FDocumentationSourceInfo Source = FDocumentationSourceInfo(), const FString& BaseUrlId = FString()) const override;

	virtual bool Open( const FString& Link, const FCultureRef& Culture, FDocumentationSourceInfo Source = FDocumentationSourceInfo(), const FString& BaseUrlId = FString()) const override;

	virtual TSharedRef< SWidget > CreateAnchor( const TAttribute<FString>& Link, const FString& PreviewLink = FString(), const FString& PreviewExcerptName = FString(), const TAttribute<FString>& BaseUrlId = FString()) const override;

	virtual TSharedRef< IDocumentationPage > GetPage( const FString& Link, const TSharedPtr< FParserConfiguration >& Config, const FDocumentationStyle& Style = FDocumentationStyle() ) override;

	virtual bool PageExists(const FString& Link) const override;

	virtual bool PageExists(const FString& Link, const FCultureRef& Culture) const override;

	virtual const TArray < FString >& GetSourcePaths() const override;

	virtual TSharedRef< class SToolTip > CreateToolTip( const TAttribute<FText>& Text, const TSharedPtr<SWidget>& OverrideContent, const FString& Link, const FString& ExcerptName ) const override;
	
	virtual TSharedRef< class SToolTip > CreateToolTip(const TAttribute<FText>& Text, const TSharedRef<SWidget>& OverrideContent, const TSharedPtr<SVerticalBox>& DocVerticalBox, const FString& Link, const FString& ExcerptName) const override;

	virtual bool RegisterBaseUrl(const FString& Id, const FString& Url) override;

	virtual FString GetBaseUrl(const FString& Id) const override;

private:

	FDocumentation();

private:

	TMap< FString, TWeakPtr< IDocumentationPage > > LoadedPages;

	TMap< const FString, const FString > RegisteredBaseUrls;

	TArray < FString > SourcePaths;

	//void HandleSDKNotInstalled(const FString& PlatformName, const FString& InDocumentationPage);

	bool AddSourcePath(const FString& Path);
};

} // namespace soda
