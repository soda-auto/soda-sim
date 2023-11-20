// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "IEditableObject.h"

FArchive& operator << (FArchive& Ar, FRuntimeMetadataField& This)
{
	Ar << This.MetadataMap;
	Ar << This.DisplayName;
	Ar << This.ToolTip_Depricated;
	return Ar;
}

void operator << (FStructuredArchive::FSlot Slot, FRuntimeMetadataField& This)
{
	FStructuredArchive::FRecord Record = Slot.EnterRecord();
	Record << SA_VALUE(TEXT("MetadataMap"), This.MetadataMap);
	Record << SA_VALUE(TEXT("DisplayName"), This.DisplayName);
	Record << SA_VALUE(TEXT("ToolTip"), This.ToolTip_Depricated);
}

FArchive& operator << (FArchive& Ar, FRuntimeMetadataObject& This)
{
	Ar << This.Fields;
	Ar << This.MetadataMap;
	return Ar;
}

void operator << (FStructuredArchive::FSlot Slot, FRuntimeMetadataObject& This)
{
	FStructuredArchive::FRecord Record = Slot.EnterRecord();
	Record << SA_VALUE(TEXT("Fields"), This.Fields);
	Record << SA_VALUE(TEXT("MetadataMap"), This.MetadataMap);
}

void IEditableObject::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (UEditableObject* This = Cast<UEditableObject>(this))
	{
		IEditableObject::Execute_ReceivePostEditChangeProperty(This, FPropertyChangedEventBP(&PropertyChangedEvent));
	}
}

void IEditableObject::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (UEditableObject* This = Cast<UEditableObject>(this))
	{
		IEditableObject::Execute_ReceivePostEditChangeProperty(This, FPropertyChangedEventBP(&PropertyChangedEvent));
	}
}

void IEditableObject::RuntimePreEditChange(FProperty* PropertyAboutToChange)
{
}

void IEditableObject::RuntimePreEditChange(FEditPropertyChain& PropertyAboutToChange)
{
}

bool IEditableObject::RuntimeCanEditChange(const FProperty* InProperty) const
{
#if WITH_EDITOR
	return CastChecked<UObject>(this)->CanEditChange(InProperty);
#else
	const bool bIsMutable = !InProperty->HasAnyPropertyFlags(CPF_EditConst);
	return bIsMutable;
#endif
}

bool IEditableObject::RuntimeCanEditChange(const FEditPropertyChain& PropertyChain) const
{
#if WITH_EDITOR
	return CastChecked<UObject>(this)->CanEditChange(PropertyChain);
#else
	return RuntimeCanEditChange(PropertyChain.GetActiveNode()->GetValue());
#endif
}

TSharedPtr<SWidget> IEditableObject::GenerateToolBar()
{
	return TSharedPtr<SWidget>(); 
}