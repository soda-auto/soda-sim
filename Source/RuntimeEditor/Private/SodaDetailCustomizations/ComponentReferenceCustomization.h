// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "RuntimePropertyEditor/IPropertyTypeCustomization.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "RuntimeEditorModule.h"

class SComboButton;
class SWidget;
struct FSlateBrush;

namespace soda
{

class FComponentReferenceCustomization : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this customization for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;

private:
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
	void SetValue(const FComponentReference& Value);

	/** Get the value referenced by this widget. */
	FPropertyAccess::Result GetValue(FComponentReference& OutValue) const;

	/** Is the Value valid */
	bool IsComponentReferenceValid(const FComponentReference& Value) const;

	/** Callback when the property value changed. */
	void OnPropertyValueChanged();

private:
	/**
	 * Return 0, if we have multiple values to edit.
	 * Return 1, if we display the widget normally.
	 */
	int32 OnGetComboContentWidgetIndex() const;

	bool CanEdit() const;
	bool CanEditChildren() const;

	const FSlateBrush* GetActorIcon() const;
	FText OnGetActorName() const;
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
	bool IsFilteredActor(const AActor* const Actor) const;
	bool IsFilteredComponent(const UActorComponent* const Component) const;
	static bool IsFilteredObject(const UObject* const Object, const TArray<const UClass*>& AllowedFilters, const TArray<const UClass*>& DisallowedFilters);

	/**
	 * Delegate for handling selection in the scene outliner.
	 * @param	InActor	The chosen component
	 */
	void OnComponentSelected(const UActorComponent* InComponent);

	/**
	 * Closes the combo button.
	 */
	void CloseComboButton();

private:
	/** The property handle we are customizing */
	TSharedPtr<IPropertyHandle> PropertyHandle;

	/** Main combo button */
	TSharedPtr<SComboButton> SubobjectComboButton;

	/** Classes that can be used with this property */
	TArray<const UClass*> AllowedActorClassFilters;
	TArray<const UClass*> AllowedComponentClassFilters;

	/** Classes that can NOT be used with this property */
	TArray<const UClass*> DisallowedActorClassFilters;
	TArray<const UClass*> DisallowedComponentClassFilters;

	/** Whether the asset can be 'None' in this case */
	bool bAllowClear;

	/** Can the actor be different/selected */
	bool bAllowAnyActor;

	/** Do it has the UseComponentPicker metadata */
	bool bUseComponentPicker;

	/** Cached ComponentReference */
	TWeakObjectPtr<AActor> CachedFirstOuterActor;
	TWeakObjectPtr<UActorComponent> CachedSubobject;
	FPropertyAccess::Result CachedPropertyAccess;
};

} // namespace soda