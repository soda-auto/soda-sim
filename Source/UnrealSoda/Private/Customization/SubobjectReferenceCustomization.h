// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Soda/SodaTypes.h"
#include "RuntimePropertyEditor/IPropertyTypeCustomization.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/PropertyHandle.h"

class SComboButton;
class SWidget;
struct FSlateBrush;
class UClass;

namespace soda
{

class FSubobjectReferenceCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;

protected:
	/** From the property metadata, build the list of allowed and disallowed class. */
	void BuildClassFilters();

	/** Build the combobox widget. */
	void BuildComboBox();

	/**
	 * From the Detail panel outer hierarchy, find the first actor or component owner we find.
	 * This is use in case we want only component on the Self actor and to check if we did a cross-level reference.
	 */
	AActor* GetFirstOuterActor() const;

	/**
	 * Set the value of the asset referenced by this property editor.
	 * Will set the underlying property handle if there is one.
	 */
	void SetValue(const FSubobjectReference& Value);

	/** Get the value referenced by this widget. */
	bool GetValue(FSubobjectReference& OutValue) const;

	/** Is the Value valid */
	bool IsSubobjectReferenceValid(const FSubobjectReference& Value) const;

	/** Callback when the property value changed. */
	void OnPropertyValueChanged();

protected:
	/**
	 * Return 0, if we have multiple values to edit.
	 * Return 1, if we display the widget normally.
	 */
	int32 OnGetComboContentWidgetIndex() const;

	bool CanEdit() const;
	bool CanEditChildren() const;

	const FSlateBrush* GetComponentIcon() const;
	FText OnGetSubobjectName() const;
	const FSlateBrush* GetStatusIcon() const;

	/**
	 * Get the content to be displayed in the asset/actor picker menu
	 * @returns the widget for the menu content
	 */
	TSharedRef<SWidget> OnGetMenuContent();

	/**
	 * Called when the asset menu is closed, we handle this to force the destruction of the asset menu to
	 * ensure any settings the user set are saved.
	 */
	void OnMenuOpenChanged(bool bOpen);

	/**
	 * Returns whether the actor/component should be filtered out from selection.
	 */
	bool IsFilteredSubobject(const UObject* const Subobject) const;
	static bool IsFilteredObject(const UObject* const Object, const TArray<const UClass*>& AllowedFilters, const TArray<const UClass*>& DisallowedFilters);

	/**
	 * Delegate for handling selection in the scene outliner.
	 * @param	InActor	The chosen component
	 */
	void OnComponentSelected(const UObject* InComponent);

	/**
	 * Closes the combo button.
	 */
	void CloseComboButton();

protected:
	/** The property handle we are customizing */
	TSharedPtr<IPropertyHandle> PropertyHandle;

	/** Main combo button */
	TSharedPtr<SComboButton> SubobjectComboButton;


	/** Whether the asset can be 'None' in this case */
	bool bAllowClear;

	/** Cached ComponentReference */
	TWeakObjectPtr<AActor> CachedFirstOuterActor;
	TWeakObjectPtr<UObject> CachedSubobject;
	bool bCachedPropertyAccess;

	TArray<const UClass*> AllowedSubobjectClassFilters;
	TArray<const UClass*> DisallowedSubobjectClassFilters;
};

}


