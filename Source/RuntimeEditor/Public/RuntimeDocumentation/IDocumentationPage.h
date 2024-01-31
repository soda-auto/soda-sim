// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"

class SWidget;

namespace soda
{

struct FExcerpt
{
	/** FExcerpt constructor */
	FExcerpt()
		: LineNumber( INDEX_NONE )
	{}

	/** FExcerpt overloaded constructor */
	FExcerpt( const FString& InName, const TSharedPtr< SWidget >& InContent, const TMap< FString, FString >& InVariables, int32 InLineNumber )
		: Name( InName )
		, Content( InContent )
		, Variables( InVariables )
		, LineNumber( InLineNumber )
	{}

	FExcerpt(const FString& InName, const TSharedPtr< SWidget >& InContent, const TMap< FString, FString >& InVariables, int32 InLineNumber, const FString& InSourcePath)
		: Name(InName)
		, Content(InContent)
		, Variables(InVariables)
		, LineNumber(InLineNumber)
		, SourcePath(InSourcePath)
	{}

	/** The name of the excerpt, as specified in the [EXCERPT:NAME] line at the top of the excerpt. */
	FString Name;
	/** The Slate content generated for the excerpt. */
	TSharedPtr<SWidget> Content;
	/** The content of the [VAR] sections read from the excerpt. The keys are the names from the [VAR:NAME] lines
		that start the variables, and the value for each key is the text inside the [VAR:NAME].....[/VAR] block. */
	TMap< FString, FString > Variables;
	/** The line number that this excerpt begins within its source file. */
	int32 LineNumber;
	/** A rich text version of the excerpt content. */
	FString RichText;
	/** The path to the file that this excerpt was read from. */
	FString SourcePath;
};

class IDocumentationPage
{
public:

	/** Returns true if this page contains a match for the ExcerptName */
	virtual bool HasExcerpt( const FString& ExcerptName ) = 0;
	/** Return the number of excerpts this page holds */
	virtual int32 GetNumExcerpts() const = 0;
	/** Populates the argument excerpt with this content found using the excer */
	virtual bool GetExcerpt( const FString& ExcerptName, FExcerpt& Excerpt) = 0;
	/** Populates the argument TArray with Excerpts this page contains */
	virtual void GetExcerpts( /*OUT*/ TArray< FExcerpt >& Excerpts ) = 0;
	/** Builds the Excerpt content using the excerpt name from the argument */
	virtual bool GetExcerptContent( FExcerpt& Excerpt ) = 0;

	/** Returns the title of the excerpt */
	virtual FText GetTitle() = 0;

	/** Rebuilds the excerpt content */
	virtual void Reload() = 0;

	/** Sets the argument as the width control for text wrap in the excerpt widgets */
	virtual void SetTextWrapAt( TAttribute<float> WrapAt ) = 0;

};

} // namespace soda
