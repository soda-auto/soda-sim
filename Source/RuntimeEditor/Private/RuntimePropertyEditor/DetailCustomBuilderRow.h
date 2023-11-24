// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
#include "RuntimePropertyEditor/SDetailsViewBase.h"
#include "RuntimePropertyEditor/DetailCategoryBuilder.h"

namespace soda
{

class FCustomChildrenBuilder;
class FDetailCategoryImpl;
class FDetailItemNode;
class IDetailCustomNodeBuilder;

class FDetailCustomBuilderRow : public IDetailLayoutRow, public TSharedFromThis<FDetailCustomBuilderRow>
{
public:
	FDetailCustomBuilderRow( TSharedRef<IDetailCustomNodeBuilder> CustomBuilder );
	virtual ~FDetailCustomBuilderRow() {}

	/** IDetailLayoutRow interface */
	virtual FName GetRowName() const override { return GetCustomBuilderName(); }

	void Tick( float DeltaTime );
	bool RequiresTick() const;
	bool HasColumns() const;
	bool ShowOnlyChildren() const;
	void OnItemNodeInitialized( TSharedRef<FDetailItemNode> InTreeNode, TSharedRef<FDetailCategoryImpl> InParentCategory, const TAttribute<bool>& InIsParentEnabled );
	TSharedRef<IDetailCustomNodeBuilder> GetCustomBuilder() const { return CustomNodeBuilder; }
	FName GetCustomBuilderName() const;
	TSharedPtr<IPropertyHandle> GetPropertyHandle() const;
	void OnGenerateChildren( FDetailNodeList& OutChildren );
	bool IsInitiallyCollapsed() const;
	FDetailWidgetRow GetWidgetRow();
	bool AreChildCustomizationsHidden() const;
	void SetOriginalPath(FStringView Path) { OriginalPath = Path; }
	const FString& GetOriginalPath() const { return OriginalPath; }

private:
	/** Whether or not our parent is enabled */
	TAttribute<bool> IsParentEnabled;
	TSharedPtr<FDetailWidgetRow> HeaderRow;
	TSharedRef<IDetailCustomNodeBuilder> CustomNodeBuilder;
	TSharedPtr<FCustomChildrenBuilder> ChildrenBuilder;
	TWeakPtr<FDetailCategoryImpl> ParentCategory;
	FString OriginalPath;
};

} // namespace soda
