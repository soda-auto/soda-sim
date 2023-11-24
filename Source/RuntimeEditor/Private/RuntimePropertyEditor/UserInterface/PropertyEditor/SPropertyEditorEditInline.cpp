// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyEditorEditInline.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Images/SImage.h"
#include "RuntimePropertyEditor/PropertyEditorHelpers.h"
#include "RuntimePropertyEditor/ObjectPropertyNode.h"
#include "RuntimePropertyEditor/PropertyHandleImpl.h"
//#include "ClassViewerModule.h"
//#include "ClassViewerFilter.h"
#include "Styling/SlateIconFinder.h"
#include "UObject/ConstructorHelpers.h"
//#include "Editor.h"

#include "RuntimeMetaData.h"

namespace soda
{
/*
class FPropertyEditorInlineClassFilter : public IClassViewerFilter
{
public:
	// The Object Property, classes are examined for a child-of relationship of the property's class. 
	FObjectPropertyBase* ObjProperty;

	// The Interface Property, classes are examined for implementing the property's class.
	FInterfaceProperty* IntProperty;

	// Whether or not abstract classes are allowed. 
	bool bAllowAbstract;

	// Hierarchy of objects that own this property. Used to check against ClassWithin.
	TSet< const UObject* > OwningObjects;

	bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		const bool bChildOfObjectClass = ObjProperty && InClass->IsChildOf(ObjProperty->PropertyClass);
		const bool bDerivedInterfaceClass = IntProperty && InClass->ImplementsInterface(IntProperty->InterfaceClass);

		const bool bMatchesFlags = InClass->HasAnyClassFlags(CLASS_EditInlineNew) && 
			!InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated) &&
			(bAllowAbstract || !InClass->HasAnyClassFlags(CLASS_Abstract));

		if( (bChildOfObjectClass || bDerivedInterfaceClass) && bMatchesFlags )
		{
			// Verify that the Owners of the property satisfy the ClassWithin constraint of the given class.
			// When ClassWithin is null, assume it can be owned by anything.
			return InClass->ClassWithin == nullptr || InFilterFuncs->IfMatchesAll_ObjectsSetIsAClass(OwningObjects, InClass->ClassWithin) != EFilterReturn::Failed;
		}

		return false;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		const bool bChildOfObjectClass = InUnloadedClassData->IsChildOf(ObjProperty->PropertyClass);

		const bool bMatchesFlags = InUnloadedClassData->HasAnyClassFlags(CLASS_EditInlineNew) && 
			!InUnloadedClassData->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated) &&
			(bAllowAbstract || !InUnloadedClassData->HasAnyClassFlags((CLASS_Abstract)));

		if (bChildOfObjectClass && bMatchesFlags)
		{
			const UClass* ClassWithin = InUnloadedClassData->GetClassWithin();

			// Verify that the Owners of the property satisfy the ClassWithin constraint of the given class.
			// When ClassWithin is null, assume it can be owned by anything.
			return ClassWithin == nullptr || InFilterFuncs->IfMatchesAll_ObjectsSetIsAClass(OwningObjects, ClassWithin) != EFilterReturn::Failed;
		}
		return false;
	}
};
*/

void SPropertyEditorEditInline::Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;

	TWeakPtr<IPropertyHandle> WeakHandlePtr = InPropertyEditor->GetPropertyHandle();

	ChildSlot
	[
		SAssignNew(ComboButton, SComboButton)
		.IsEnabled(this, &SPropertyEditorEditInline::IsValueEnabled, WeakHandlePtr)
		//.OnGetMenuContent(this, &SPropertyEditorEditInline::GenerateClassPicker)
		.ContentPadding(0)
		.ToolTipText(InPropertyEditor, &FPropertyEditor::GetValueAsText )
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew( SImage )
				.Image( this, &SPropertyEditorEditInline::GetDisplayValueIcon )
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew( STextBlock )
				.Text( this, &SPropertyEditorEditInline::GetDisplayValueAsString )
				.Font( InArgs._Font )
			]
		]
	];
}

bool SPropertyEditorEditInline::IsValueEnabled(TWeakPtr<IPropertyHandle> WeakHandlePtr) const
{
	if (WeakHandlePtr.IsValid())
	{
		return !WeakHandlePtr.Pin()->IsEditConst();
	}

	return false;
}

FText SPropertyEditorEditInline::GetDisplayValueAsString() const
{
	UObject* CurrentValue = NULL;
	FPropertyAccess::Result Result = PropertyEditor->GetPropertyHandle()->GetValue( CurrentValue );
	if( Result == FPropertyAccess::Success && CurrentValue != NULL )
	{
		return FRuntimeMetaData::GetDisplayNameText(CurrentValue->GetClass());
	}
	else
	{
		return PropertyEditor->GetValueAsText();
	}
}

const FSlateBrush* SPropertyEditorEditInline::GetDisplayValueIcon() const
{
	UObject* CurrentValue = nullptr;
	FPropertyAccess::Result Result = PropertyEditor->GetPropertyHandle()->GetValue( CurrentValue );
	if( Result == FPropertyAccess::Success && CurrentValue != nullptr )
	{
		return FSlateIconFinder::FindIconBrushForClass(CurrentValue->GetClass());
	}

	return nullptr;
}

void SPropertyEditorEditInline::GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth )
{
	OutMinDesiredWidth = 250.0f;
	OutMaxDesiredWidth = 600.0f;
}

bool SPropertyEditorEditInline::Supports( const FPropertyNode* InTreeNode, int32 InArrayIdx )
{
	return InTreeNode
		&& InTreeNode->HasNodeFlags(EPropertyNodeFlags::EditInlineNew)
		&& InTreeNode->FindObjectItemParent()
		&& !InTreeNode->IsPropertyConst();
}

bool SPropertyEditorEditInline::Supports( const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	return SPropertyEditorEditInline::Supports( &PropertyNode.Get(), PropertyNode->GetArrayIndex() );
}

bool SPropertyEditorEditInline::IsClassAllowed( UClass* CheckClass, bool bAllowAbstract ) const
{
	check(CheckClass);
	return PropertyEditorHelpers::IsEditInlineClassAllowed( CheckClass, bAllowAbstract ) &&  CheckClass->HasAnyClassFlags(CLASS_EditInlineNew);
}
/*
TSharedRef<SWidget> SPropertyEditorEditInline::GenerateClassPicker()
{
	FClassViewerInitializationOptions Options;
	Options.bShowBackgroundBorder = false;
	Options.bShowUnloadedBlueprints = true;
	Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;

	TSharedPtr<FPropertyEditorInlineClassFilter> ClassFilter = MakeShareable( new FPropertyEditorInlineClassFilter );
	Options.ClassFilters.Add(ClassFilter.ToSharedRef());
	ClassFilter->bAllowAbstract = false;

	const TSharedRef< FPropertyNode > PropertyNode = PropertyEditor->GetPropertyNode();
	FProperty* Property = PropertyNode->GetProperty();
	ClassFilter->ObjProperty = CastField<FObjectPropertyBase>( Property );
	ClassFilter->IntProperty = CastField<FInterfaceProperty>( Property );
	Options.bShowNoneOption = !(Property->PropertyFlags & CPF_NoClear);

	FObjectPropertyNode* ObjectPropertyNode = PropertyNode->FindObjectItemParent();
	if( ObjectPropertyNode )
	{
		for ( TPropObjectIterator Itor( ObjectPropertyNode->ObjectIterator() ); Itor; ++Itor )
		{
			UObject* OwnerObject = Itor->Get();
			ClassFilter->OwningObjects.Add( OwnerObject );
		}
	}

	Options.PropertyHandle = PropertyEditor->GetPropertyHandle();

	FOnClassPicked OnPicked( FOnClassPicked::CreateRaw( this, &SPropertyEditorEditInline::OnClassPicked ) );

	return FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, OnPicked);
}
*/
void SPropertyEditorEditInline::OnClassPicked(UClass* InClass)
{
	TArray<FObjectBaseAddress> ObjectsToModify;
	TArray<FString> NewValues;

	const TSharedRef< FPropertyNode > PropertyNode = PropertyEditor->GetPropertyNode();
	FObjectPropertyNode* ObjectNode = PropertyNode->FindObjectItemParent();

	if( ObjectNode )
	{
		//GEditor->BeginTransaction(TEXT("PropertyEditor"), NSLOCTEXT("PropertyEditor", "OnClassPicked", "Set Class"), nullptr /* PropertyNode->GetProperty()) */ );

		auto ExtractClassAndObjectNames = [](FStringView PathName, FStringView& ClassName, FStringView& ObjectName)
		{
			int32 ClassEnd;
			PathName.FindChar(TCHAR('\''), ClassEnd);
			if (ensure(ClassEnd != INDEX_NONE))
			{
				ClassName = PathName.Left(ClassEnd);
			}

			int32 LastPeriod, LastColon;
			PathName.FindLastChar(TCHAR('.'), LastPeriod);
			PathName.FindLastChar(TCHAR(':'), LastColon);
			const int32 ObjectNameStart = FMath::Max(LastPeriod, LastColon);

			if (ensure(ObjectNameStart != INDEX_NONE))
			{
				ObjectName = PathName.RightChop(ObjectNameStart + 1).LeftChop(1);
			}
		};

		FString NewObjectName;
		UObject* NewObjectTemplate = nullptr;

		// If we've picked the same class as our archetype, then we want to create an object with the same name and properties
		if (InClass)
		{
			FString DefaultValue = PropertyNode->GetDefaultValueAsString(/*bUseDisplayName=*/false);
			if (!DefaultValue.IsEmpty() && DefaultValue != FName(NAME_None).ToString())
			{
				FStringView ClassName, ObjectName;
				ExtractClassAndObjectNames(DefaultValue, ClassName, ObjectName);
				if (InClass->GetName() == ClassName)
				{
					NewObjectName = ObjectName;

					ConstructorHelpers::StripObjectClass(DefaultValue);
					NewObjectTemplate = StaticFindObject(InClass, nullptr, *DefaultValue);
				}
			}
		}

		const TSharedRef<IPropertyHandle> PropertyHandle = PropertyEditor->GetPropertyHandle();

		// If this is an instanced component property collect current names so we can clean them properly if necessary
		TArray<FString> PrevPerObjectValues;
		FObjectProperty* ObjectProperty = CastFieldChecked<FObjectProperty>(PropertyHandle->GetProperty());
		if (ObjectProperty && ObjectProperty->HasAnyPropertyFlags(CPF_InstancedReference))
		{
			PropertyHandle->GetPerObjectValues(PrevPerObjectValues);
		}

		for ( TPropObjectIterator Itor( ObjectNode->ObjectIterator() ) ; Itor ; ++Itor )
		{
			FString NewValue;
			if (InClass)
			{
				FStringView CurClassName, CurObjectName;
				if (PrevPerObjectValues.IsValidIndex(NewValues.Num()) && PrevPerObjectValues[NewValues.Num()] != FName(NAME_None).ToString())
				{
					ExtractClassAndObjectNames(PrevPerObjectValues[NewValues.Num()], CurClassName, CurObjectName);
				}

				if (CurObjectName == NewObjectName && InClass->GetName() == CurClassName)
				{
					NewValue = MoveTemp(PrevPerObjectValues[NewValues.Num()]);
					PrevPerObjectValues[NewValues.Num()].Reset();
				}
				else
				{
					UObject*		Object = Itor->Get();
					UObject*		UseOuter = (InClass->IsChildOf(UClass::StaticClass()) ? Cast<UClass>(Object)->GetDefaultObject() : Object);
					EObjectFlags	MaskedOuterFlags = UseOuter ? UseOuter->GetMaskedFlags(RF_PropagateToSubObjects) : RF_NoFlags;
					if (UseOuter && UseOuter->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
					{
						MaskedOuterFlags |= RF_ArchetypeObject;
					}

					if (NewObjectName.Len() > 0)
					{
						if (UObject* SubObject = StaticFindObject(UObject::StaticClass(), UseOuter, *NewObjectName))
						{
							SubObject->Rename(*MakeUniqueObjectName(GetTransientPackage(), SubObject->GetClass()).ToString(), GetTransientPackage(), REN_DontCreateRedirectors);

							// If we've renamed the object out of the way here, we don't need to do it again below
							if (PrevPerObjectValues.IsValidIndex(NewValues.Num()))
							{
								PrevPerObjectValues[NewValues.Num()].Reset();
							}
						}
					}

					UObject* NewUObject = NewObject<UObject>(UseOuter, InClass, *NewObjectName, MaskedOuterFlags, NewObjectTemplate);

					NewValue = NewUObject->GetPathName();
				}
			}
			else
			{
				NewValue = FName(NAME_None).ToString();
			}
			NewValues.Add(MoveTemp(NewValue));
		}

		PropertyHandle->SetPerObjectValues(NewValues);
		check(PrevPerObjectValues.Num() == 0 || PrevPerObjectValues.Num() == NewValues.Num());

		for (int32 Index = 0; Index < PrevPerObjectValues.Num(); ++Index)
		{
			if (PrevPerObjectValues[Index].Len() > 0 && PrevPerObjectValues[Index] != NewValues[Index])
			{
				// Move the old subobject to the transient package so GetObjectsWithOuter will not return it
				// This is particularly important for UActorComponent objects so resetting owned components on the parent doesn't find it
				ConstructorHelpers::StripObjectClass(PrevPerObjectValues[Index]);
				if (UObject* SubObject = StaticFindObject(UObject::StaticClass(), nullptr, *PrevPerObjectValues[Index]))
				{
					SubObject->Rename(nullptr, GetTransientOuterForRename(SubObject->GetClass()), REN_DontCreateRedirectors);
				}
			}
		}

		// End the transaction if we called PreChange
		//GEditor->EndTransaction();

		// Force a rebuild of the children when this node changes
		PropertyNode->RequestRebuildChildren();

		ComboButton->SetIsOpen(false);
	}
}

} // namespace soda