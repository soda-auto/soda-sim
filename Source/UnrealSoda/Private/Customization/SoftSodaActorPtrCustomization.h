// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "RuntimePropertyEditor/IPropertyTypeCustomization.h"
#include "Templates/SharedPointer.h"
#include "RuntimeEditorModule.h"
#include "Soda/ISodaActor.h"
#include "Widgets/Input/SComboButton.h"

namespace soda
{

class IPropertyHandle;


/**
 * Customizes a soft object path to look like a UObject property
 */
class FSoftSodaActorPtrCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() 
	{
		return MakeShareable( new FSoftSodaActorPtrCustomization);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> InPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> InPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;

protected:
	FPropertyAccess::Result GetValue(UObject *& OutValue) const;
	void SetValue(const UObject * Value);

	void BuildComboBox();

	const FSlateBrush* GetActorIcon() const;
	FText OnGetActorName() const;
	TSharedRef<SWidget> OnGetMenuContent();
	void OnMenuOpenChanged(bool bOpen);
	int32 OnGetComboContentWidgetIndex() const;
	void OnPropertyValueChanged();
	void CloseComboButton();
	void OnActorSelected(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo);
	bool IsFilteredActor(const AActor* const Actor) const;

private:
	/** Handle to the struct property being customized */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	UClass* MetaClass = nullptr;
	TWeakObjectPtr<AActor> CachedActor;
	FSodaActorDescriptor CachedActorDesc;
	FPropertyAccess::Result CachedPropertyAccess;
	TSharedPtr<SComboButton> SubobjectComboButton;
};

} // namespace soda