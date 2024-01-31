// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "RuntimePropertyEditor/IPropertyTypeCustomization.h"
#include "Templates/SharedPointer.h"
#include "RuntimeEditorModule.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Widgets/Input/SComboButton.h"

namespace soda
{

class IPropertyHandle;


/**
 * Customizes a soft object path to look like a UObject property
 */
class FSoftSodaVehicleComponentPtrCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() 
	{
		return MakeShareable( new FSoftSodaVehicleComponentPtrCustomization);
	}

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader( TSharedRef<class IPropertyHandle> InPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;
	virtual void CustomizeChildren( TSharedRef<class IPropertyHandle> InPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils ) override;

protected:
	FPropertyAccess::Result GetValue(UObject *& OutValue) const;
	void SetValue(const UObject * Value);

	void BuildComboBox();

	const FSlateBrush* GetComponentIcon() const;
	FText OnGetSubobjectName() const;
	TSharedRef<SWidget> OnGetMenuContent();
	void OnMenuOpenChanged(bool bOpen);
	int32 OnGetComboContentWidgetIndex() const;
	void OnPropertyValueChanged();
	void CloseComboButton();
	void OnComponentSelected(TWeakObjectPtr<UActorComponent> Component, ESelectInfo::Type SelectInfo);
	bool IsFilteredComponent(const UActorComponent* const Component) const;

private:
	/** Handle to the struct property being customized */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	UClass* MetaClass = nullptr;
	TWeakObjectPtr<UActorComponent> CachedSubobject;
	FVehicleComponentGUI CachedComponentDesc;
	FPropertyAccess::Result CachedPropertyAccess;
	TSharedPtr<SComboButton> SubobjectComboButton;
};

} // namespace soda