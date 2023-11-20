// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimeMetaDataStatic.h"

void URuntimeMetaDataStatic::MakeRuntimeMetadataProperties(const FString& CategoryName, const TMap<FString, FRuntimeMetadataPropertyBuilder>& In, FRuntimeMetadataObject& Out)
{
	for (auto& It : In)
	{
		FRuntimeMetadataField & MetadataField = Out.Fields.FindOrAdd(It.Key);
		MetadataField.DisplayName = It.Value.DisplayName;
		//MetadataField.ToolTip = It.Value.ToolTip;
		MetadataField.MetadataMap.Add("Category", CategoryName);
		MetadataField.MetadataMap.Add("EditInRuntime", "");
		if (It.Value.bIsInitialize)
		{
			//MetadataField.MetadataMap.Add("ReactivateComponent");
		}
	}
}

void URuntimeMetaDataStatic::MakeRuntimeMetadataButtons(const FString& CategoryName, const TMap<FString, FRuntimeMetadataButtonBuilder>& In, FRuntimeMetadataObject& Out)
{
	for (auto& It : In)
	{
		FRuntimeMetadataField& MetadataField = Out.Fields.FindOrAdd(It.Key);
		MetadataField.DisplayName = It.Value.DisplayName;
		//MetadataField.ToolTip = It.Value.ToolTip;
		MetadataField.MetadataMap.Add("Category", CategoryName);
		MetadataField.MetadataMap.Add("CallInRuntime", "");
	}
}

void URuntimeMetaDataStatic::MergeRuntimeMetadata(const TArray<FRuntimeMetadataObject>& In, FRuntimeMetadataObject& Out)
{
	for (auto& It : In)
	{
		Out.Fields.Append(It.Fields);
	}
}