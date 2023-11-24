// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "RuntimeDocumentation/IDocumentation.h"

#include "RuntimeDocumentationSettings.generated.h"


USTRUCT()
struct FRuntimeDocumentationBaseUrl
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Url;

	FRuntimeDocumentationBaseUrl() {};

	FRuntimeDocumentationBaseUrl(FString const& InId, FString const& InUrl)
		: Id(InId), Url(InUrl)
	{};

	/** Returns true if either the ID or URL is unset. */
	bool IsEmpty() const
	{
		return Id.IsEmpty() || Url.IsEmpty();
	}
};


UCLASS(config=Editor)
class URuntimeDocumentationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Array of base URLs for documentation links that are loaded on startup
	UPROPERTY(Config)
	TArray<FRuntimeDocumentationBaseUrl> DocumentationBaseUrls;
};