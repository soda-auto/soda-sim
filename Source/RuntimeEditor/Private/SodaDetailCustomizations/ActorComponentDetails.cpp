// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaDetailCustomizations/ActorComponentDetails.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "SodaStyleSet.h"
#include "Engine/EngineBaseTypes.h"
#include "Components/ActorComponent.h"
#include "RuntimePropertyEditor/DetailLayoutBuilder.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
#include "RuntimePropertyEditor/DetailCategoryBuilder.h"
#include "RuntimePropertyEditor/IDetailsView.h"
//#include "ObjectEditorUtils.h"
#include "Widgets/SToolTip.h"
#include "RuntimeDocumentation/IDocumentation.h"

#define LOCTEXT_NAMESPACE "ActorComponentDetails"

namespace soda
{

TSharedRef<IDetailCustomization> FActorComponentDetails::MakeInstance()
{
	return MakeShareable( new FActorComponentDetails );
}

void FActorComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TSharedPtr<IPropertyHandle> PrimaryTickProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UActorComponent, PrimaryComponentTick));

	// Defaults only show tick properties
	if (PrimaryTickProperty->IsValidHandle() && DetailBuilder.HasClassDefaultObject())
	{
		IDetailCategoryBuilder& TickCategory = DetailBuilder.EditCategory("ComponentTick");

		TickCategory.AddProperty(PrimaryTickProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTickFunction, bStartWithTickEnabled)));
		TickCategory.AddProperty(PrimaryTickProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTickFunction, TickInterval)));
		TickCategory.AddProperty(PrimaryTickProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTickFunction, bTickEvenWhenPaused)), EPropertyLocation::Advanced);
		TickCategory.AddProperty(PrimaryTickProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTickFunction, bAllowTickOnDedicatedServer)), EPropertyLocation::Advanced);
		TickCategory.AddProperty(PrimaryTickProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTickFunction, TickGroup)), EPropertyLocation::Advanced);
	}

	PrimaryTickProperty->MarkHiddenByCustomization();

	TArray<TWeakObjectPtr<UObject>> WeakObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(WeakObjectsBeingCustomized);

	bool bHideReplicates = false;
	for (TWeakObjectPtr<UObject>& WeakObjectBeingCustomized : WeakObjectsBeingCustomized)
	{
		if (UObject* ObjectBeingCustomized = WeakObjectBeingCustomized.Get())
		{
			if (UActorComponent* Component = Cast<UActorComponent>(ObjectBeingCustomized))
			{
				if (!Component->GetComponentClassCanReplicate())
				{
					bHideReplicates = true;
					break;
				}
			}
			else
			{
				bHideReplicates = true;
				break;
			}
		}
	}

	/*
	if (bHideReplicates)
	{
		TSharedPtr<IPropertyHandle> ReplicatesProperty = DetailBuilder.GetProperty(UActorComponent::GetReplicatesPropertyName());
		ReplicatesProperty->MarkHiddenByCustomization();
	}
	*/
}

} // namespace soda


#undef LOCTEXT_NAMESPACE
