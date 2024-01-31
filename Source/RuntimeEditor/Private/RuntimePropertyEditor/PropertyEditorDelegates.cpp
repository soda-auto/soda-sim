// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/PropertyEditorDelegates.h"
#include "RuntimePropertyEditor/ObjectPropertyNode.h"
#include "RuntimePropertyEditor/PropertyHandle.h"
#include "RuntimePropertyEditor/PropertyNode.h"

namespace soda
{

FPropertyAndParent::FPropertyAndParent(const TSharedRef<IPropertyHandle>& InPropertyHandle) :
	Property(*InPropertyHandle->GetProperty())
{
	Initialize(InPropertyHandle->GetPropertyNode().ToSharedRef());
}

FPropertyAndParent::FPropertyAndParent(const TSharedRef<FPropertyNode>& InPropertyNode) :
	Property(*InPropertyNode->GetProperty())
{
	Initialize(InPropertyNode);
}

void FPropertyAndParent::Initialize(const TSharedRef<FPropertyNode>& InPropertyNode)
{
	checkf(InPropertyNode->GetProperty() != nullptr, TEXT("Creating an FPropertyAndParent with a null property!"));

	FObjectPropertyNode* ObjectNode = InPropertyNode->FindObjectItemParent();
	if (ObjectNode)
	{
		for (int32 ObjectIndex = 0; ObjectIndex < ObjectNode->GetNumObjects(); ++ObjectIndex)
		{
			Objects.Add(ObjectNode->GetUObject(ObjectIndex));
		}
	}

	ArrayIndex = InPropertyNode->GetArrayIndex();

	TSharedPtr<FPropertyNode> ParentNode = InPropertyNode->GetParentNodeSharedPtr();
	while (ParentNode.IsValid())
	{
		const FProperty* ParentProperty = ParentNode->GetProperty();
		if (ParentProperty != nullptr)
		{
			ParentProperties.Add(ParentProperty);
			ParentArrayIndices.Add(ParentNode->GetArrayIndex());
		}

		ParentNode = ParentNode->GetParentNodeSharedPtr();
	}
}

} // namespace soda