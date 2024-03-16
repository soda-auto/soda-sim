// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SoftSodaActorPtrCustomization.h"
#include "Containers/UnrealString.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
#include "RuntimePropertyEditor/PropertyCustomizationHelpers.h"
#include "RuntimePropertyEditor/PropertyHandle.h"
#include "RuntimePropertyEditor/DetailLayoutBuilder.h"
#include "RuntimePropertyEditor/IDetailChildrenBuilder.h"
#include "RuntimePropertyEditor/PropertyCustomizationHelpers.h"
#include "HAL/PlatformCrt.h"
#include "UObject/Object.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "RuntimeEditorUtils.h"
#include "Soda/SodaGameMode.h"
#include "Soda/UI/SActorList.h"
#include "Soda/SodaGameViewportClient.h"

class IDetailChildrenBuilder;
class UClass;


namespace soda
{

void FSoftSodaActorPtrCustomization::CustomizeHeader( TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	PropertyHandle = InPropertyHandle;

	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	check(GameMode);
	
	CachedActor.Reset();
	CachedPropertyAccess = FPropertyAccess::Fail;

	FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(InPropertyHandle->GetProperty());
	check(ObjectProperty);
	MetaClass = ObjectProperty->PropertyClass;
	check(MetaClass);

	BuildComboBox();

	InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSoftSodaActorPtrCustomization::OnPropertyValueChanged));

	UObject * TmpObject;
	CachedPropertyAccess = GetValue(TmpObject);
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		CachedActor = Cast<AActor>(TmpObject);
		if (CachedActor.IsValid())
		{
			CachedActorDesc = GameMode->GetSodaActorDescriptor(CachedActor->GetClass());
		}
	}

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SubobjectComboButton.ToSharedRef()
	];
}

void FSoftSodaActorPtrCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

void FSoftSodaActorPtrCustomization::SetValue(const UObject* Value)
{
	SubobjectComboButton->SetIsOpen(false);
	ensure(PropertyHandle->SetValue(Value) == FPropertyAccess::Result::Success);
}

FPropertyAccess::Result FSoftSodaActorPtrCustomization::GetValue(UObject*& OutValue) const
{
	// Potentially accessing the value while garbage collecting or saving the package could trigger a crash.
	// so we fail to get the value when that is occurring.
	if (GIsSavingPackage || IsGarbageCollecting())
	{
		return FPropertyAccess::Fail;
	}

	FPropertyAccess::Result Result = FPropertyAccess::Fail;
	
	UObject* Object = nullptr;
	Result = PropertyHandle->GetValue(Object);

	if (Object == nullptr)
	{
		// Check to see if it's pointing to an unloaded object
		FString CurrentObjectPath;
		FSoftObjectPath SoftObjectPath;
		PropertyHandle->GetValueAsFormattedString(CurrentObjectPath);
		if (CurrentObjectPath.Len() > 0 && CurrentObjectPath != TEXT("None"))
		{
			if (SoftObjectPath.IsAsset())
			{
				Result = FPropertyAccess::Fail;
			}
			else
			{
				Object = SoftObjectPath.ResolveObject(); // TODO: OutValue can be UObject or FSoftObjectPath
				if (Object != nullptr)
				{
					Result = FPropertyAccess::Success;
				}
			}
		}
	}

	if (Object == nullptr)
	{
		// No property editor was specified so check if multiple property values are associated with the property handle
		TArray<FString> ObjectValues;
		PropertyHandle->GetPerObjectValues(ObjectValues);

		if (ObjectValues.Num() > 1)
		{
			for (int32 ObjectIndex = 1; ObjectIndex < ObjectValues.Num() && Result == FPropertyAccess::Success; ++ObjectIndex)
			{
				if (ObjectValues[ObjectIndex] != ObjectValues[0])
				{
					Result = FPropertyAccess::MultipleValues;
				}
			}
		}
	}
	
	OutValue = Object;
	return Result;
}

void FSoftSodaActorPtrCustomization::BuildComboBox()
{
	/*
	TAttribute<bool> IsEnabledAttribute(this, &FComponentReferenceCustomization::CanEdit);
	TAttribute<FText> TooltipAttribute;
	if (PropertyHandle->GetMetaDataProperty()->HasAnyPropertyFlags(CPF_EditConst | CPF_DisableEditOnTemplate))
	{
		TArray<UObject*> ObjectList;
		PropertyHandle->GetOuterObjects(ObjectList);

		// If there is no objects, that means we must have a struct asset managing this property
		if (ObjectList.Num() == 0)
		{
			IsEnabledAttribute.Set(false);
			TooltipAttribute.Set(LOCTEXT("VariableHasDisableEditOnTemplate", "Editing this value in structure's defaults is not allowed"));
		}
		else
		{
			// Go through all the found objects and see if any are a CDO, we can't set an actor in a CDO default.
			for (UObject* Obj : ObjectList)
			{
				if (Obj->IsTemplate() && !Obj->IsA<ALevelScriptActor>())
				{
					IsEnabledAttribute.Set(false);
					TooltipAttribute.Set(LOCTEXT("VariableHasDisableEditOnTemplateTooltip", "Editing this value in a Class Default Object is not allowed"));
					break;
				}

			}
		}
	}
	*/


	SubobjectComboButton = SNew(SComboButton)
		//.ToolTipText(TooltipAttribute)
		//.ButtonStyle(FSodaStyle::Get(), "PropertyEditor.AssetComboStyle")
		//.ForegroundColor(FSodaStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
		.OnGetMenuContent(this, &FSoftSodaActorPtrCustomization::OnGetMenuContent)
		.OnMenuOpenChanged(this, &FSoftSodaActorPtrCustomization::OnMenuOpenChanged)
		//.IsEnabled(IsEnabledAttribute)
		//.ContentPadding(2.0f)
		.ButtonContent()
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &FSoftSodaActorPtrCustomization::OnGetComboContentWidgetIndex)
			+ SWidgetSwitcher::Slot()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("<multiple values>")))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 2, 0)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(this, &FSoftSodaActorPtrCustomization::GetActorIcon)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.VAlign(VAlign_Center)
				[
					// Show the name of the asset or actor
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text(this, &FSoftSodaActorPtrCustomization::OnGetActorName)
				]
			]
		];
}

const FSlateBrush* FSoftSodaActorPtrCustomization::GetActorIcon() const
{
	if (CachedActor.IsValid())
	{
		return FSodaStyle::GetBrush(CachedActorDesc.Icon);
	}
	else
	{
		return FSodaStyle::GetBrush(TEXT("ClassIcon.Actor"));
	}
}

FText FSoftSodaActorPtrCustomization::OnGetActorName() const
{
	if (CachedActor.IsValid())
	{
		return FText::FromString(CachedActor->GetName());
	}
	else
	{
		return FText::FromString(TEXT("None"));
	}
}

TSharedRef<SWidget> FSoftSodaActorPtrCustomization::OnGetMenuContent()
{
	return 
		SNew(SBorder)
		.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Header")))
		.Padding(1)
		[
			SNew(SBox)
			.MinDesiredWidth(200)
			.MinDesiredHeight(300)
			[
				SNew(soda::SActorList, Cast<USodaGameViewportClient>(USodaGameModeComponent::GetChecked()->GetWorld()->GetGameViewport()))
				.bInteractiveMode(false)
				.OnSelectionChanged(this, &FSoftSodaActorPtrCustomization::OnActorSelected)
				.ActorFilter(FOnShouldFilterActor::CreateSP(this, &FSoftSodaActorPtrCustomization::IsFilteredActor))
			]
		];
}

bool FSoftSodaActorPtrCustomization::IsFilteredActor(const AActor* const Actor) const
{
	return Actor->IsA(MetaClass);
}

void FSoftSodaActorPtrCustomization::OnActorSelected(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo)
{
	SetValue(Actor.Get());
	//FSlateApplication::Get().DismissAllMenus();
}

void FSoftSodaActorPtrCustomization::OnMenuOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		SubobjectComboButton->SetMenuContent(SNullWidget::NullWidget);
	}
}

int32 FSoftSodaActorPtrCustomization::OnGetComboContentWidgetIndex() const
{
	switch (CachedPropertyAccess)
	{
	case FPropertyAccess::MultipleValues: return 0;
	case FPropertyAccess::Success:
	default:
		return 1;
	}
}

void FSoftSodaActorPtrCustomization::OnPropertyValueChanged()
{
	CachedActor.Reset();

	UObject * TmpObject;
	CachedPropertyAccess = GetValue(TmpObject);
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		CachedActor = Cast<AActor>(TmpObject);
		if (CachedActor.IsValid())
		{
			USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
			check(GameMode);
			CachedActorDesc = GameMode->GetSodaActorDescriptor(CachedActor->GetClass());
		}
	}
}

void FSoftSodaActorPtrCustomization::CloseComboButton()
{
	SubobjectComboButton->SetIsOpen(false);
}


} // namespace soda