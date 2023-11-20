// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SubobjectReferenceCustomization.h"
#include "Components/SceneComponent.h"
#include "RuntimePropertyEditor/DetailLayoutBuilder.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
#include "SodaStyleSet.h"
#include "Engine/LevelScriptActor.h"
#include "RuntimePropertyEditor/IDetailChildrenBuilder.h"
#include "RuntimePropertyEditor/IDetailPropertyRow.h"
#include "RuntimePropertyEditor/IPropertyTypeCustomization.h"
#include "RuntimePropertyEditor/PropertyCustomizationHelpers.h"
#include "RuntimePropertyEditor/PropertyHandle.h"
#include "Styling/SlateIconFinder.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "SPropertyMenuSubobjectPicker.h"
#include "Soda/Misc/EditorUtils.h"
#include "Soda/UnrealSoda.h"
#include "Soda/ISodaVehicleComponent.h"

#define LOCTEXT_NAMESPACE "ComponentReferenceCustomization"

namespace soda
{

static const FName NAME_AllowAnyActor = "AllowAnyActor";
static const FName NAME_AllowedClasses = "AllowedClasses";
static const FName NAME_DisallowedClasses = "DisallowedClasses";
static const FName NAME_UseComponentPicker = "UseComponentPicker";

static FSubobjectReference MakeSubobjectReference(const AActor* InOwner, const UObject* InSubobject)
{
	FSubobjectReference Result;
	if (InSubobject)
	{
		AActor* Owner = FEditorUtils::FindOuterActor(InSubobject);
		Result.PathToSubobject = InSubobject->GetPathName(Owner);
	}
	return Result;
}

TSharedRef<IPropertyTypeCustomization> FSubobjectReferenceCustomization::MakeInstance()
{
	return MakeShareable(new FSubobjectReferenceCustomization);
}

void FSubobjectReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils)
{
	PropertyHandle = InPropertyHandle;

	CachedSubobject.Reset();
	CachedFirstOuterActor.Reset();
	bCachedPropertyAccess = false;

	bAllowClear = false;

	FProperty* Property = InPropertyHandle->GetProperty();
	check(CastField<FStructProperty>(Property) && FSubobjectReference::StaticStruct() == CastFieldChecked<const FStructProperty>(Property)->Struct);

	bAllowClear = !(InPropertyHandle->GetMetaDataProperty()->PropertyFlags & CPF_NoClear);

	BuildClassFilters();
	BuildComboBox();

	InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSubobjectReferenceCustomization::OnPropertyValueChanged));

	// set cached values
	{
		CachedSubobject.Reset();
		CachedFirstOuterActor = GetFirstOuterActor();

		FSubobjectReference TmpSubobjectReference;
		bCachedPropertyAccess = GetValue(TmpSubobjectReference);
		if (bCachedPropertyAccess)
		{
			CachedSubobject = TmpSubobjectReference.GetObject<UObject>(CachedFirstOuterActor.Get());
			if (!IsSubobjectReferenceValid(TmpSubobjectReference))
			{
				CachedSubobject.Reset();
			}
		}
	}

	HeaderRow.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SubobjectComboButton.ToSharedRef()
	]
	.IsEnabled(MakeAttributeSP(this, &FSubobjectReferenceCustomization::CanEdit));

}

void FSubobjectReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils)
{
	uint32 NumberOfChild;
	if (InPropertyHandle->GetNumChildren(NumberOfChild) == FPropertyAccess::Success)
	{
		for (uint32 Index = 0; Index < NumberOfChild; ++Index)
		{
			TSharedRef<IPropertyHandle> ChildPropertyHandle = InPropertyHandle->GetChildHandle(Index).ToSharedRef();
			ChildPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSubobjectReferenceCustomization::OnPropertyValueChanged));
			StructBuilder.AddProperty(ChildPropertyHandle)
				.ShowPropertyButtons(true)
				.IsEnabled(MakeAttributeSP(this, &FSubobjectReferenceCustomization::CanEditChildren));
		}
	}
}

void FSubobjectReferenceCustomization::BuildClassFilters()
{

	auto ParseClassFilters = [this](const FString& MetaDataString, TArray<const UClass*>& ComponentList)
	{
		if (!MetaDataString.IsEmpty())
		{
			TArray<FString> ClassFilterNames;
			MetaDataString.ParseIntoArrayWS(ClassFilterNames, TEXT(","), true);

			for (const FString& ClassName : ClassFilterNames)
			{
				UClass* Class = UClass::TryFindTypeSlow<UClass>(ClassName);
				if (!Class)
				{
					Class = LoadObject<UClass>(nullptr, *ClassName);
				}

				if (Class)
				{
					// If the class is an interface, expand it to be all classes in memory that implement the class.
					if (Class->HasAnyClassFlags(CLASS_Interface))
					{
						for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
						{
							UClass* const ClassWithInterface = (*ClassIt);
							if (ClassWithInterface->ImplementsInterface(Class))
							{
								ComponentList.Add(ClassWithInterface);
							}
						}
					}
					else
					{
						ComponentList.Add(Class);
					}
				}
			}
		}
	};

	// Account for the allowed classes specified in the property metadata
	const FString& AllowedClassesFilterString = PropertyHandle->GetMetaData(NAME_AllowedClasses);
	ParseClassFilters(AllowedClassesFilterString, AllowedSubobjectClassFilters);

	const FString& DisallowedClassesFilterString = PropertyHandle->GetMetaData(NAME_DisallowedClasses);
	ParseClassFilters(DisallowedClassesFilterString, DisallowedSubobjectClassFilters);
}

void FSubobjectReferenceCustomization::BuildComboBox()
{
	TAttribute<bool> IsEnabledAttribute(this, &FSubobjectReferenceCustomization::CanEdit);
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

	TSharedRef<SVerticalBox> ObjectContent = SNew(SVerticalBox);
	ObjectContent->AddSlot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(SImage)
			.Image(this, &FSubobjectReferenceCustomization::GetComponentIcon)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.VAlign(VAlign_Center)
		[
			// Show the name of the asset or actor
			SNew(STextBlock)
			.Font(soda::IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FSubobjectReferenceCustomization::OnGetSubobjectName)
		]
	];

	TSharedRef<SHorizontalBox> ComboButtonContent = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(this, &FSubobjectReferenceCustomization::GetStatusIcon)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.VAlign(VAlign_Center)
		[
			ObjectContent
		];

	SubobjectComboButton = SNew(SComboButton)
		.ToolTipText(TooltipAttribute)
		//.ButtonStyle(FSodaStyle::Get(), "PropertyEditor.AssetComboStyle")
		//.ForegroundColor(FSodaStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
		.OnGetMenuContent(this, &FSubobjectReferenceCustomization::OnGetMenuContent)
		.OnMenuOpenChanged(this, &FSubobjectReferenceCustomization::OnMenuOpenChanged)
		.IsEnabled(IsEnabledAttribute)
		.ContentPadding(2.0f)
		.ButtonContent()
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &FSubobjectReferenceCustomization::OnGetComboContentWidgetIndex)
			+ SWidgetSwitcher::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MultipleValuesText", "<multiple values>"))
				.Font(soda::IDetailLayoutBuilder::GetDetailFont())
			]
			+ SWidgetSwitcher::Slot()
			[
				ComboButtonContent
			]
		];
}

AActor* FSubobjectReferenceCustomization::GetFirstOuterActor() const
{
	TArray<UObject*> ObjectList;
	PropertyHandle->GetOuterObjects(ObjectList);
	for (UObject* Obj : ObjectList)
	{
		while (Obj)
		{
			if (AActor* Actor = Cast<AActor>(Obj))
			{
				return Actor;
			}
			else if (UActorComponent* Component = Cast<UActorComponent>(Obj))
			{
				if (Component->GetOwner())
				{
					return Component->GetOwner();
				}
			}
			else if (UBlueprintGeneratedClass* BP = Cast<UBlueprintGeneratedClass>(Obj))
			{
				
				bool bIsActor = BP->IsChildOf(AActor::StaticClass());
				if (UObject* DefObj = BP->GetDefaultObject())
				{
					UE_LOG(LogSoda, Error, TEXT("*** %i %s"), bIsActor, *DefObj->GetName());
				}

			}
			Obj = Obj->GetOuter();
		}
	}

	if (const UClass* Class = PropertyHandle->GetOuterBaseClass())
	{
		UE_LOG(LogSoda, Error, TEXT("*** %s"), *Class->GetName());
	}

	TArray<UPackage*> OuterPackages;
	PropertyHandle->GetOuterPackages(OuterPackages);
	UE_LOG(LogSoda, Error, TEXT("*** %i"), OuterPackages.Num());

	if (FProperty* Prop = PropertyHandle->GetProperty())
	{
		UE_LOG(LogSoda, Error, TEXT("*** %s"), *Prop->GetName());
	}


	if (const FFieldClass* temp = PropertyHandle->GetPropertyClass())
	{
		UE_LOG(LogSoda, Error, TEXT("*** %s"), *temp->GetName());
	}

	if (TSharedPtr<IPropertyHandle>  Parent = PropertyHandle->GetParentHandle())
	{
		UE_LOG(LogSoda, Error, TEXT("*** %s"), *Parent->GetPropertyDisplayName().ToString());
	}

	/*
	if (ObjectList.Num())
	{
		for (TObjectIterator<UBlueprintGeneratedClass> It; It; ++It)
		{
			for (TFieldIterator<FObjectProperty> FieldIt(It); It; ++It)
			{
				FieldIt->GetObjectPropertyValue_InContainer();
				if (FieldIt == ObjectList[0])
				{
					UE_LOG(LogSoda, Error, TEXT("***"), );
				}
			}
		}
	}
	*/
	



	return nullptr;
}

void FSubobjectReferenceCustomization::SetValue(const FSubobjectReference& Value)
{
	SubobjectComboButton->SetIsOpen(false);

	const bool bIsEmpty = Value == FSubobjectReference();
	const bool bAllowedToSetBasedOnFilter = IsSubobjectReferenceValid(Value);
	if (bIsEmpty || bAllowedToSetBasedOnFilter)
	{
		FString TextValue;
		CastFieldChecked<const FStructProperty>(PropertyHandle->GetProperty())->Struct->ExportText(TextValue, &Value, &Value, nullptr, EPropertyPortFlags::PPF_None, nullptr);
		ensure(PropertyHandle->SetValueFromFormattedString(TextValue) == FPropertyAccess::Result::Success);
	}
}

bool FSubobjectReferenceCustomization::GetValue(FSubobjectReference& OutValue) const
{
	// Potentially accessing the value while garbage collecting or saving the package could trigger a crash.
	// so we fail to get the value when that is occurring.
	if (GIsSavingPackage || IsGarbageCollecting())
	{
		return false;
	}

	FPropertyAccess::Result Result = FPropertyAccess::Fail;
	if (PropertyHandle.IsValid() && PropertyHandle->IsValidHandle())
	{
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);
		UObject* CurrentSubobject = nullptr;
		AActor* CurrentActor = CachedFirstOuterActor.Get();
		for (const void* RawPtr : RawData)
		{
			if (RawPtr)
			{
				const FSubobjectReference& ThisReference = *reinterpret_cast<const FSubobjectReference*>(RawPtr);
				if (Result == FPropertyAccess::Success)
				{
					if (ThisReference.GetObject<UObject>(CurrentActor) != CurrentSubobject)
					{
						Result = FPropertyAccess::MultipleValues;
						break;
					}
				}
				else
				{
					OutValue = ThisReference;
					CurrentSubobject = OutValue.GetObject<UObject>(CurrentActor);
					Result = FPropertyAccess::Success;
				}
			}
			else if (Result == FPropertyAccess::Success)
			{
				Result = FPropertyAccess::MultipleValues;
				break;
			}
		}
	}
	return Result == FPropertyAccess::Success;
}

bool FSubobjectReferenceCustomization::IsSubobjectReferenceValid(const FSubobjectReference& Value) const
{
	AActor* CachedActor = CachedFirstOuterActor.Get();
	UObject* NewSubobject = Value.GetObject<UObject>(CachedActor);
	if (!NewSubobject)
	{
		return false;
	}

	if (!IsFilteredSubobject(NewSubobject))
	{
		return false;
	}

	return true;
}

void FSubobjectReferenceCustomization::OnPropertyValueChanged()
{
	CachedSubobject.Reset();
	CachedFirstOuterActor = GetFirstOuterActor();

	FSubobjectReference TmpComponentReference;
	bCachedPropertyAccess = GetValue(TmpComponentReference);
	if (bCachedPropertyAccess)
	{
		CachedSubobject = TmpComponentReference.GetObject<UObject>(CachedFirstOuterActor.Get());
		if (!IsSubobjectReferenceValid(TmpComponentReference))
		{
			CachedSubobject.Reset();
			if (!(TmpComponentReference == FSubobjectReference()))
			{
				SetValue(FSubobjectReference());
			}
		}
	}
}

int32 FSubobjectReferenceCustomization::OnGetComboContentWidgetIndex() const
{
	if (bCachedPropertyAccess)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool FSubobjectReferenceCustomization::CanEdit() const
{
	return PropertyHandle.IsValid() ? !PropertyHandle->IsEditConst() : true;
}

bool FSubobjectReferenceCustomization::CanEditChildren() const
{
	return CanEdit() && !CachedFirstOuterActor.IsValid();
}

const FSlateBrush* FSubobjectReferenceCustomization::GetComponentIcon() const
{
	if (const UObject* ActorComponent = CachedSubobject.Get())
	{
		if (const ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(ActorComponent))
		{
			return FSodaStyle::GetBrush(VehicleComponent->GetVehicleComponentGUI().IcanName);
		}
		return FSlateIconFinder::FindIconBrushForClass(ActorComponent->GetClass());
	}
	return FSlateIconFinder::FindIconBrushForClass(UActorComponent::StaticClass());
}

FText FSubobjectReferenceCustomization::OnGetSubobjectName() const
{
	if (bCachedPropertyAccess)
	{
		if (UObject* Subobject = CachedSubobject.Get())
		{
			/*
			const FName ComponentName = FComponentEditorUtils::FindVariableNameGivenComponentInstance(Subobject);
			const bool bIsArrayVariable = !ComponentName.IsNone() && GetOuterActor(Subobject) != nullptr && FindFProperty<FArrayProperty>(GetOuterActor(Subobject)->GetClass(), ComponentName);

			if (!ComponentName.IsNone() && !bIsArrayVariable)
			{
				return FText::FromName(ComponentName);
			}
			*/
			return FText::AsCultureInvariant(Subobject->GetName());
		}
	}

	return LOCTEXT("NoObject", "None");
}

const FSlateBrush* FSubobjectReferenceCustomization::GetStatusIcon() const
{
	static FSlateNoResource EmptyBrush = FSlateNoResource();

	if (!bCachedPropertyAccess)
	{
		return FSodaStyle::GetBrush("Icons.Error");
	}
	return &EmptyBrush;
}

TSharedRef<SWidget> FSubobjectReferenceCustomization::OnGetMenuContent()
{
	UObject * InitialComponent = CachedSubobject.Get();

	return SNew(soda::SPropertyMenuSubobjectPicker)
		.OwnedActor(CachedFirstOuterActor.Get())
		.AllowClear(bAllowClear)
		.SubobjectFilter(soda::FOnShouldFilterSubobject::CreateSP(this, &FSubobjectReferenceCustomization::IsFilteredSubobject))
		.OnSet(soda::FOnSubobjectSelected::CreateSP(this, &FSubobjectReferenceCustomization::OnComponentSelected))
		.OnClose(FSimpleDelegate::CreateSP(this, &FSubobjectReferenceCustomization::CloseComboButton));
}

void FSubobjectReferenceCustomization::OnMenuOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		SubobjectComboButton->SetMenuContent(SNullWidget::NullWidget);
	}
}

bool FSubobjectReferenceCustomization::IsFilteredSubobject(const UObject* const Subobject) const
{
	const AActor* OuterActor = CachedFirstOuterActor.Get();
	const AActor* FindedOwner = FEditorUtils::FindOuterActor(Subobject);
	return FindedOwner
		&& (FindedOwner == CachedFirstOuterActor.Get())
		&& IsFilteredObject(Subobject, AllowedSubobjectClassFilters, DisallowedSubobjectClassFilters);
}

bool FSubobjectReferenceCustomization::IsFilteredObject(const UObject* const Object, const TArray<const UClass*>& AllowedFilters, const TArray<const UClass*>& DisallowedFilters)
{
	bool bAllowedToSetBasedOnFilter = true;

	const UClass* ObjectClass = Object->GetClass();
	if (AllowedFilters.Num() > 0)
	{
		bAllowedToSetBasedOnFilter = false;
		for (const UClass* AllowedClass : AllowedFilters)
		{
			const bool bAllowedClassIsInterface = AllowedClass->HasAnyClassFlags(CLASS_Interface);
			if (ObjectClass->IsChildOf(AllowedClass) || (bAllowedClassIsInterface && ObjectClass->ImplementsInterface(AllowedClass)))
			{
				bAllowedToSetBasedOnFilter = true;
				break;
			}
		}
	}

	if (DisallowedFilters.Num() > 0 && bAllowedToSetBasedOnFilter)
	{
		for (const UClass* DisallowedClass : DisallowedFilters)
		{
			const bool bDisallowedClassIsInterface = DisallowedClass->HasAnyClassFlags(CLASS_Interface);
			if (ObjectClass->IsChildOf(DisallowedClass) || (bDisallowedClassIsInterface && ObjectClass->ImplementsInterface(DisallowedClass)))
			{
				bAllowedToSetBasedOnFilter = false;
				break;
			}
		}
	}

	return bAllowedToSetBasedOnFilter;
}

void FSubobjectReferenceCustomization::OnComponentSelected(const UObject* InSubobject)
{
	SubobjectComboButton->SetIsOpen(false);

	FSubobjectReference SubobjectReference = MakeSubobjectReference(CachedFirstOuterActor.Get(), InSubobject);
	SetValue(SubobjectReference);
}

void FSubobjectReferenceCustomization::CloseComboButton()
{
	SubobjectComboButton->SetIsOpen(false);
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
