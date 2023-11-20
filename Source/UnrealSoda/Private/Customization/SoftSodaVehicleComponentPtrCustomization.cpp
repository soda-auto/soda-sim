// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SoftSodaVehicleComponentPtrCustomization.h"
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
#include "Soda/UI/SVehicleComponentsList.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Components/ActorComponent.h"

class IDetailChildrenBuilder;
class UClass;


namespace soda
{

void FSoftSodaVehicleComponentPtrCustomization::CustomizeHeader( TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	PropertyHandle = InPropertyHandle;

	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	check(GameMode);
	
	CachedSubobject.Reset();
	CachedPropertyAccess = FPropertyAccess::Fail;

	FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(InPropertyHandle->GetProperty());
	check(ObjectProperty);
	MetaClass = ObjectProperty->PropertyClass;
	check(MetaClass);

	BuildComboBox();

	InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSoftSodaVehicleComponentPtrCustomization::OnPropertyValueChanged));

	UObject * TmpObject;
	CachedPropertyAccess = GetValue(TmpObject);
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		CachedSubobject = Cast<UActorComponent>(TmpObject);
		if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(CachedSubobject.Get()))
		{
			CachedComponentDesc = VehicleComponent->GetVehicleComponentGUI();
		}
		else
		{
			CachedComponentDesc = FVehicleComponentGUI();
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

void FSoftSodaVehicleComponentPtrCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

void FSoftSodaVehicleComponentPtrCustomization::SetValue(const UObject* Value)
{
	SubobjectComboButton->SetIsOpen(false);
	ensure(PropertyHandle->SetValue(Value) == FPropertyAccess::Result::Success);
}

FPropertyAccess::Result FSoftSodaVehicleComponentPtrCustomization::GetValue(UObject*& OutValue) const
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

void FSoftSodaVehicleComponentPtrCustomization::BuildComboBox()
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
		.OnGetMenuContent(this, &FSoftSodaVehicleComponentPtrCustomization::OnGetMenuContent)
		.OnMenuOpenChanged(this, &FSoftSodaVehicleComponentPtrCustomization::OnMenuOpenChanged)
		//.IsEnabled(IsEnabledAttribute)
		//.ContentPadding(2.0f)
		.ButtonContent()
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &FSoftSodaVehicleComponentPtrCustomization::OnGetComboContentWidgetIndex)
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
					.Image(this, &FSoftSodaVehicleComponentPtrCustomization::GetComponentIcon)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text(this, &FSoftSodaVehicleComponentPtrCustomization::OnGetSubobjectName)
				]
			]
		];
}

const FSlateBrush* FSoftSodaVehicleComponentPtrCustomization::GetComponentIcon() const
{
	if (CachedSubobject.IsValid())
	{
		return FSodaStyle::GetBrush(CachedComponentDesc.IcanName);
	}
	else
	{
		return FSodaStyle::GetBrush(TEXT("ClassIcon.Actor"));
	}
}

FText FSoftSodaVehicleComponentPtrCustomization::OnGetSubobjectName() const
{
	if (CachedSubobject.IsValid())
	{
		return FText::FromString(CachedSubobject->GetName());
	}
	else
	{
		return FText::FromString(TEXT("None"));
	}
}

TSharedRef<SWidget> FSoftSodaVehicleComponentPtrCustomization::OnGetMenuContent()
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);
	ASodaVehicle* SodaVehicle = nullptr;
	if (OuterObjects.Num() == 1)
	{
		SodaVehicle = Cast<ASodaVehicle>(OuterObjects[0]);
	}

	if (!SodaVehicle)
	{
		return SNullWidget::NullWidget;
	}

	return 
		SNew(SBorder)
		.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Header")))
		.Padding(1)
		[
			SNew(SBox)
			.MinDesiredWidth(200)
			.MinDesiredHeight(300)
			[
				SNew(soda::SVehicleComponentsList, SodaVehicle, false)
				//.bInteractiveMode(false)
				//.OnSelectionChanged(this, &FSoftSodaVehicleComponentPtrCustomization::OnComponentSelected)
				//.ComponentFilter(FOnShouldFilterActor::CreateSP(this, &FSoftSodaVehicleComponentPtrCustomization::IsFilteredComponent))
			]
		];
}

bool FSoftSodaVehicleComponentPtrCustomization::IsFilteredComponent(const UActorComponent* const Component) const
{
	return Component->IsA(MetaClass);
}

void FSoftSodaVehicleComponentPtrCustomization::OnComponentSelected(TWeakObjectPtr<UActorComponent> Component, ESelectInfo::Type SelectInfo)
{
	SetValue(Component.Get());
	//FSlateApplication::Get().DismissAllMenus();
}

void FSoftSodaVehicleComponentPtrCustomization::OnMenuOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		SubobjectComboButton->SetMenuContent(SNullWidget::NullWidget);
	}
}

int32 FSoftSodaVehicleComponentPtrCustomization::OnGetComboContentWidgetIndex() const
{
	switch (CachedPropertyAccess)
	{
	case FPropertyAccess::MultipleValues: return 0;
	case FPropertyAccess::Success:
	default:
		return 1;
	}
}

void FSoftSodaVehicleComponentPtrCustomization::OnPropertyValueChanged()
{
	CachedSubobject.Reset();

	UObject * TmpObject;
	CachedPropertyAccess = GetValue(TmpObject);
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		CachedSubobject = Cast<UActorComponent>(TmpObject);
		if (CachedSubobject.IsValid())
		{
			if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(CachedSubobject.Get()))
			{
				CachedComponentDesc = VehicleComponent->GetVehicleComponentGUI();
			}
			else
			{
				CachedComponentDesc = FVehicleComponentGUI();
			}
		}
	}
}

void FSoftSodaVehicleComponentPtrCustomization::CloseComboButton()
{
	SubobjectComboButton->SetIsOpen(false);
}


} // namespace soda