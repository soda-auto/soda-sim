// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyEditorClass.h"
#include "Engine/Blueprint.h"
#include "Misc/FeedbackContext.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBox.h"
//#include "DragAndDrop/ClassDragDropOp.h"
//#include "DragAndDrop/AssetDragDropOp.h"
#include "RuntimeClassViewer/ClassViewerFilter.h"
#include "UObject/UObjectIterator.h"
#include "RuntimePropertyEditor/PropertyRestriction.h"
#include "RuntimeClassViewer/SClassViewer.h"

#include "RuntimeMetaData.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

namespace soda
{

class FPropertyEditorClassFilter  : public IClassViewerFilter
{
public:
	virtual ~FPropertyEditorClassFilter() {}
	/** The meta class for the property that classes must be a child-of. */
	const UClass* ClassPropertyMetaClass;

	/** The interface that must be implemented. */
	const UClass* InterfaceThatMustBeImplemented;

	/** Whether or not abstract classes are allowed. */
	bool bAllowAbstract;

	/** Classes that can be picked */
	TArray<const UClass*> AllowedClassFilters;

	/** Classes that can't be picked */
	TArray<const UClass*> DisallowedClassFilters;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return IsClassAllowedHelper(InClass);
	}
	
	
	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InBlueprint, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return IsClassAllowedHelper(InBlueprint);
	}
	

private:

	template <typename TClass>
	bool IsClassAllowedHelper(TClass InClass)
	{
		bool bMatchesFlags = !InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated) &&
			(bAllowAbstract || !InClass->HasAnyClassFlags(CLASS_Abstract));

		if (bMatchesFlags && InClass->IsChildOf(ClassPropertyMetaClass)
			&& (!InterfaceThatMustBeImplemented || InClass->ImplementsInterface(InterfaceThatMustBeImplemented)))
		{
			auto PredicateFn = [InClass](const UClass* Class)
			{
				return InClass->IsChildOf(Class);
			};

			if (DisallowedClassFilters.FindByPredicate(PredicateFn) == nullptr &&
				(AllowedClassFilters.Num() == 0 || AllowedClassFilters.FindByPredicate(PredicateFn) != nullptr))
			{
				return true;
			}
		}

		return false;
	}
};

static UClass* FindOrLoadClass(const FString& ClassName)
{
	UClass* Class = UClass::TryFindTypeSlow<UClass>(ClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);

	if (!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassName);
	}

	return Class;
}

void SPropertyEditorClass::GetDesiredWidth(float& OutMinDesiredWidth, float& OutMaxDesiredWidth)
{
	OutMinDesiredWidth = 200.0f;
	OutMaxDesiredWidth = 400.0f;
}

bool SPropertyEditorClass::Supports(const TSharedRef< class FPropertyEditor >& InPropertyEditor)
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	const FProperty* Property = InPropertyEditor->GetProperty();
	int32 ArrayIndex = PropertyNode->GetArrayIndex();

	if ((Property->IsA(FClassProperty::StaticClass()) || Property->IsA(FSoftClassProperty::StaticClass())) 
		&& ((ArrayIndex == -1 && Property->ArrayDim == 1) || (ArrayIndex > -1 && Property->ArrayDim > 0)))
	{
		return true;
	}

	return false;
}

/** @return True if the property can be edited */
bool SPropertyEditorClass::CanEdit() const
{
	return PropertyEditor.IsValid() ? !PropertyEditor->IsEditConst() : true;
}

void SPropertyEditorClass::Construct(const FArguments& InArgs, const TSharedPtr< class FPropertyEditor >& InPropertyEditor)
{
	PropertyEditor = InPropertyEditor;
	
	if (PropertyEditor.IsValid())
	{
		const TSharedRef<FPropertyNode> PropertyNode = PropertyEditor->GetPropertyNode();
		FProperty* const Property = PropertyNode->GetProperty();
		if (FClassProperty* const ClassProp = CastField<FClassProperty>(Property))
		{
			MetaClass = ClassProp->MetaClass;
		}
		else if (FSoftClassProperty* const SoftClassProperty = CastField<FSoftClassProperty>(Property))
		{
			MetaClass = SoftClassProperty->MetaClass;
		}
		else
		{
			check(false);
		}
		
		const FString* AllowAbstractString = FRuntimeMetaData::FindMetaData(Property->GetOwnerProperty(), TEXT("AllowAbstract"));
		bAllowAbstract = AllowAbstractString && (AllowAbstractString->IsEmpty() || AllowAbstractString->ToBool());
		
		bAllowOnlyPlaceable = FRuntimeMetaData::HasMetaData(Property->GetOwnerProperty(), TEXT("OnlyPlaceable"));
		bIsBlueprintBaseOnly = FRuntimeMetaData::HasMetaData(Property->GetOwnerProperty(), TEXT("BlueprintBaseOnly"));
		RequiredInterface = FRuntimeMetaData::GetClassMetaData(Property->GetOwnerProperty(), TEXT("MustImplement"));
		bAllowNone = !(Property->PropertyFlags & CPF_NoClear);
		bShowViewOptions = FRuntimeMetaData::HasMetaData(Property->GetOwnerProperty(), TEXT("HideViewOptions")) ? false : true;
		bShowTree = FRuntimeMetaData::HasMetaData(Property->GetOwnerProperty(), TEXT("ShowTreeView"));
		bShowDisplayNames = FRuntimeMetaData::HasMetaData(Property->GetOwnerProperty(), TEXT("ShowDisplayNames"));

		auto FillClassFilters = [&](TArray<const UClass*> &ClassFilters, FName TagName)
		{
			const FString* ClassesFilterString = &(const FString &)FRuntimeMetaData::GetMetaData(Property, TagName);

			ClassFilters.Empty();

			if (!ClassesFilterString->IsEmpty())
			{
				TArray<FString> ClassFilterNames;
				ClassesFilterString->ParseIntoArrayWS(ClassFilterNames, TEXT(","), true);

				for (const FString& ClassName : ClassFilterNames)
				{
					UClass* Class = FindOrLoadClass(ClassName);

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
									ClassFilters.Add(ClassWithInterface);
								}
							}
						}
						else
						{
							ClassFilters.Add(Class);
						}
					}
				}
			}
		};

		// Filter based on UPROPERTY meta data
		FillClassFilters(AllowedClassFilters, "AllowedClasses");
		FillClassFilters(DisallowedClassFilters, "DisallowedClasses");

		// Filter based on restrictions
		for (const TSharedRef<const FPropertyRestriction>& ClassRestriction : PropertyNode->GetRestrictions())
		{
			for (TArray<FString>::TConstIterator Iter= ClassRestriction.Get().GetHiddenValuesIterator(); Iter; ++Iter)
			{
				if (UClass* HiddenClass = FindOrLoadClass(*Iter))
				{
					DisallowedClassFilters.Add(HiddenClass);
				}
			}

			for (TArray<FString>::TConstIterator Iter = ClassRestriction.Get().GetDisabledValuesIterator(); Iter; ++Iter)
			{
				if (UClass* DisabledClass = FindOrLoadClass(*Iter))
				{
					DisallowedClassFilters.Add(DisabledClass);
				}
			}
		}
	}
	else
	{
		check(InArgs._MetaClass);
		check(InArgs._SelectedClass.IsSet());
		check(InArgs._OnSetClass.IsBound());

		MetaClass = InArgs._MetaClass;
		RequiredInterface = InArgs._RequiredInterface;
		bAllowAbstract = InArgs._AllowAbstract;
		bIsBlueprintBaseOnly = InArgs._IsBlueprintBaseOnly;
		bAllowNone = InArgs._AllowNone;
		bAllowOnlyPlaceable = false;
		bShowViewOptions = InArgs._ShowViewOptions;
		bShowTree = InArgs._ShowTree;
		bShowDisplayNames = InArgs._ShowDisplayNames;
		AllowedClassFilters.Empty();
		DisallowedClassFilters.Empty();
		SelectedClass = InArgs._SelectedClass;
		OnSetClass = InArgs._OnSetClass;
	}

	CreateClassFilter(InArgs._ClassViewerFilters);

	SAssignNew(ComboButton, SComboButton)
		.OnGetMenuContent(this, &SPropertyEditorClass::GenerateClassPicker)
		.ToolTipText(this, &SPropertyEditorClass::GetDisplayValueAsString)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(this, &SPropertyEditorClass::GetDisplayValueAsString)
			.Font(InArgs._Font)
		];

	ChildSlot
	[
		ComboButton.ToSharedRef()
	];

	SetEnabled(TAttribute<bool>(this, &SPropertyEditorClass::CanEdit));
}

/** Util to give better names for BP generated classes */
static FString GetClassDisplayName(const UObject* Object, bool bShowDisplayNames)
{
	const UClass* Class = Cast<UClass>(Object);
	if (Class != NULL)
	{
#if WITH_EDITORONLY_DATA
		UBlueprint* BP = UBlueprint::GetBlueprintFromClass(Class);
		if(BP != NULL)
		{
			return BP->GetName();
		}
#endif
		if (bShowDisplayNames && FRuntimeMetaData::HasMetaData(Class, TEXT("DisplayName")))
		{
			return FRuntimeMetaData::GetMetaData(Class, TEXT("DisplayName"));
		}
	}
	return (Object) ? Object->GetName() : "None";
}

FText SPropertyEditorClass::GetDisplayValueAsString() const
{
	static bool bIsReentrant = false;

	// Guard against re-entrancy which can happen if the delegate executed below (SelectedClass.Get()) forces a slow task dialog to open, thus causing this to lose context and regain focus later starting the loop over again
	if( !bIsReentrant )
	{
		TGuardValue<bool> Guard( bIsReentrant, true );
		if(PropertyEditor.IsValid())
		{
			UObject* ObjectValue = NULL;
			FPropertyAccess::Result Result = PropertyEditor->GetPropertyHandle()->GetValue(ObjectValue);

			if(Result == FPropertyAccess::Success && ObjectValue != NULL)
			{
				return FText::FromString(GetClassDisplayName(ObjectValue, bShowDisplayNames));
			}

			return FText::FromString(FPaths::GetBaseFilename(PropertyEditor->GetValueAsString()));
		}

		return FText::FromString(GetClassDisplayName(SelectedClass.Get(), bShowDisplayNames));
	}
	else
	{
		return FText::GetEmpty();
	}
	
}

void SPropertyEditorClass::CreateClassFilter(const TArray<TSharedRef<IClassViewerFilter>>& InClassFilters)
{
	
	ClassViewerOptions.bShowBackgroundBorder = false;
	ClassViewerOptions.bShowUnloadedBlueprints = true;
	ClassViewerOptions.bShowNoneOption = bAllowNone;

	if (PropertyEditor.IsValid())
	{
		ClassViewerOptions.PropertyHandle = PropertyEditor->GetPropertyHandle();
	}

	ClassViewerOptions.bIsBlueprintBaseOnly = bIsBlueprintBaseOnly;
	ClassViewerOptions.bIsPlaceableOnly = bAllowOnlyPlaceable;
	ClassViewerOptions.NameTypeToDisplay = (bShowDisplayNames ? EClassViewerNameTypeToDisplay::DisplayName : EClassViewerNameTypeToDisplay::ClassName);
	ClassViewerOptions.DisplayMode = bShowTree ? EClassViewerDisplayMode::TreeView : EClassViewerDisplayMode::ListView;
	ClassViewerOptions.bAllowViewOptions = bShowViewOptions;
	ClassViewerOptions.ClassFilters.Append(InClassFilters);

	TSharedRef<FPropertyEditorClassFilter> PropEdClassFilter = MakeShared<FPropertyEditorClassFilter>();
	ClassViewerOptions.ClassFilters.Add(PropEdClassFilter);
	

	PropEdClassFilter->ClassPropertyMetaClass = MetaClass;
	PropEdClassFilter->InterfaceThatMustBeImplemented = RequiredInterface;
	PropEdClassFilter->bAllowAbstract = bAllowAbstract;
	PropEdClassFilter->AllowedClassFilters = AllowedClassFilters;
	PropEdClassFilter->DisallowedClassFilters = DisallowedClassFilters;
	

	ClassFilter = TSharedRef<IClassViewerFilter>(new FClassViewerFilter(ClassViewerOptions)); //FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassFilter(ClassViewerOptions);
	ClassFilterFuncs = TSharedRef<FClassViewerFilterFuncs>(new FClassViewerFilterFuncs());// FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateFilterFuncs();
}


TSharedRef<SWidget> SPropertyEditorClass::GenerateClassPicker()
{
	FOnClassPicked OnPicked(FOnClassPicked::CreateRaw(this, &SPropertyEditorClass::OnClassPicked));

	return SNew(SBox)
		.WidthOverride(280)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.MaxHeight(500)
			[
				//FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(ClassViewerOptions, OnPicked)
				SNew(SClassViewer, ClassViewerOptions)
				.OnClassPickedDelegate(OnPicked)
			]			
		];
}

void SPropertyEditorClass::OnClassPicked(UClass* InClass)
{
	if(!InClass)
	{
		SendToObjects(TEXT("None"));
	}
	else
	{
		SendToObjects(InClass->GetPathName());
	}

	ComboButton->SetIsOpen(false);
}

void SPropertyEditorClass::SendToObjects(const FString& NewValue)
{
	if(PropertyEditor.IsValid())
	{
		const TSharedRef<IPropertyHandle> PropertyHandle = PropertyEditor->GetPropertyHandle();
		PropertyHandle->SetValueFromFormattedString(NewValue);
	}
	else if (!NewValue.IsEmpty() && NewValue != TEXT("None"))
	{
		UClass* NewClass = FindOrLoadClass(NewValue);
		OnSetClass.Execute(NewClass);
	}
	else
	{
		OnSetClass.Execute(nullptr);
	}
}
/*
static UObject* LoadDragDropObject(TSharedPtr<FSodaDragDropOp> UnloadedClassOp)
{
	FString AssetPath;

	// Find the class/blueprint path
	if (UnloadedClassOp->HasAssets())
	{
		AssetPath = UnloadedClassOp->GetAssets()[0].ObjectPath.ToString();
	}
	else if (UnloadedClassOp->HasAssetPaths())
	{
		AssetPath = UnloadedClassOp->GetAssetPaths()[0];
	}

	// Check to see if the asset can be found, otherwise load it.
	UObject* Object = FindObject<UObject>(nullptr, *AssetPath);
	if (Object == nullptr)
	{
		// Load the package.
		GWarn->BeginSlowTask(LOCTEXT("OnDrop_LoadPackage", "Fully Loading Package For Drop"), true, false);

		Object = LoadObject<UObject>(nullptr, *AssetPath);

		GWarn->EndSlowTask();
	}

	return Object;
}

void SPropertyEditorClass::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FSodaDragDropOp> UnloadedClassOp = DragDropEvent.GetOperationAs<FSodaDragDropOp>();
	if (UnloadedClassOp.IsValid())
	{
		UObject* Object = LoadDragDropObject(UnloadedClassOp);

		bool bOK = false;

		if (UClass* Class = Cast<UClass>(Object))
		{
			bOK = ClassFilter->IsClassAllowed(ClassViewerOptions, Class, ClassFilterFuncs.ToSharedRef());
		}
		else if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			if (Blueprint->GeneratedClass)
			{
				bOK = ClassFilter->IsClassAllowed(ClassViewerOptions, Blueprint->GeneratedClass, ClassFilterFuncs.ToSharedRef());
			}
		}
		
		if (bOK)
		{
			UnloadedClassOp->SetToolTip(FText::GetEmpty(), FSodaStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")));
		}
		else
		{
			UnloadedClassOp->SetToolTip(FText::GetEmpty(), FSodaStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error")));
		}
	}
}

void SPropertyEditorClass::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FSodaDragDropOp> UnloadedClassOp = DragDropEvent.GetOperationAs<FSodaDragDropOp>();
	if (UnloadedClassOp.IsValid())
	{
		UnloadedClassOp->ResetToDefaultToolTip();
	}
}

FReply SPropertyEditorClass::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FClassDragDropOp> ClassOperation = DragDropEvent.GetOperationAs<FClassDragDropOp>();
	if (ClassOperation.IsValid())
	{
		// We can only drop one item into the combo box, so drop the first one.
		FString ClassPath = ClassOperation->ClassesToDrop[0]->GetPathName();

		// Set the property, it will be verified as valid.
		SendToObjects(ClassPath);

		return FReply::Handled();
	}
	
	TSharedPtr<FSodaDragDropOp> UnloadedClassOp = DragDropEvent.GetOperationAs<FSodaDragDropOp>();
	if (UnloadedClassOp.IsValid())
	{
		bool bAllAssetWereLoaded = true;

		FString AssetPath;

		// Find the class/blueprint path
		if (UnloadedClassOp->HasAssets())
		{
			AssetPath = UnloadedClassOp->GetAssets()[0].ObjectPath.ToString();
		}
		else if (UnloadedClassOp->HasAssetPaths())
		{
			AssetPath = UnloadedClassOp->GetAssetPaths()[0];
		}

		// Check to see if the asset can be found, otherwise load it.
		UObject* Object = FindObject<UObject>(nullptr, *AssetPath);
		if(Object == nullptr)
		{
			// Load the package.
			GWarn->BeginSlowTask(LOCTEXT("OnDrop_LoadPackage", "Fully Loading Package For Drop"), true, false);

			Object = LoadObject<UObject>(nullptr, *AssetPath);

			GWarn->EndSlowTask();
		}

		if (UClass* Class = Cast<UClass>(Object))
		{
			if (ClassFilter->IsClassAllowed(ClassViewerOptions, Class, ClassFilterFuncs.ToSharedRef()))
			{
				// This was pointing to a class directly
				SendToObjects(Class->GetPathName());
			}
		}
		else if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			if (Blueprint->GeneratedClass)
			{
				if (ClassFilter->IsClassAllowed(ClassViewerOptions, Blueprint->GeneratedClass, ClassFilterFuncs.ToSharedRef()))
				{
					// This was pointing to a blueprint, get generated class
					SendToObjects(Blueprint->GeneratedClass->GetPathName());
				}
			}
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}
*/
} // namespace soda

#undef LOCTEXT_NAMESPACE
