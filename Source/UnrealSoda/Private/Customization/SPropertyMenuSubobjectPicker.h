// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "RuntimePropertyEditor/PropertyCustomizationHelpers.h"


/*
namespace SceneOutliner
{
	struct ISceneOutlinerTreeItem;
}
*/

namespace soda
{

DECLARE_DELEGATE_OneParam(FOnSubobjectSelected, const UObject* /*Subobject*/);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnShouldFilterSubobject, const UObject* /*Subobject*/);

class SPropertyMenuSubobjectPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPropertyMenuSubobjectPicker)
		: _OwnedActor(nullptr)
		, _InitialSubobject(nullptr)
		, _AllowClear(true)
	{}
		SLATE_ARGUMENT(AActor*, OwnedActor)
		SLATE_ARGUMENT(UObject*, InitialSubobject)
		SLATE_ARGUMENT(bool, AllowClear)
		SLATE_ARGUMENT(FOnShouldFilterSubobject, SubobjectFilter)
		SLATE_EVENT(FOnSubobjectSelected, OnSet)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	/**
	 * Construct the widget.
	 * @param	InArgs				Arguments for widget construction
	 * @param	InPropertyHandle	The property handle that this widget will operate on.
	 */
	void Construct(const FArguments& InArgs);

private:
	/**
	 * Edit the object referenced by this widget
	 */
	void OnEdit();

	/**
	 * Delegate handling ctrl+c
	 */
	void OnCopy();

	/**
	 * Delegate handling ctrl+v
	 */
	void OnPaste();

	/**
	 * @return True of the current clipboard contents can be pasted
	 */
	bool CanPaste();

	/**
	 * Clear the referenced object
	 */
	void OnClear();

	/**
	 * Delegate for handling selection by the actor picker.
	 */
	void OnItemSelected(TWeakObjectPtr<UObject> Subobject, ESelectInfo::Type SelectInfo);

	/**
	 * Set the value of the asset referenced by this property editor.
	 * Will set the underlying property handle if there is one.
	 * @param	InObject	The object to set the reference to
	 */
	void SetValue(UObject* Subobject);

	TSharedRef< ITableRow > MakeRowWidget(TWeakObjectPtr<UObject> Subobject, const TSharedRef< STableViewBase >& OwnerTable);


private:
	TWeakObjectPtr<AActor> OwnedActor;
	TWeakObjectPtr<UObject> InitialSubobject;
	TArray<TWeakObjectPtr<UObject>> Source;

	/** Whether the asset can be 'None' in this case */
	bool bAllowClear;

	/** Delegate used to test whether a item should be displayed or not */
	FOnShouldFilterSubobject SubobjectFilter;

	/** Delegate to call when our object value should be set */
	FOnSubobjectSelected OnSet;

	/** Delegate for closing the containing menu */
	FSimpleDelegate OnClose;
};

} // namespace soda
