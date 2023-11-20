// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "UObject/WeakFieldPtr.h"
#include "HAL/Platform.h"
#include "Internationalization/Text.h"
#include "Math/Color.h"
#include "Misc/AssertionMacros.h"
#include "Misc/EnumClassFlags.h"
#include "Misc/Guid.h"
#include "Misc/InlineValue.h"
#include "Templates/SubclassOf.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealNames.h"
#include "UObject/UnrealType.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Widgets/Views/ITableRow.h"
#include "Widgets/SNullWidget.h"
#include "UObject/SoftObjectPtr.h"
#include "ScenarioActionNode.generated.h"

class STableViewBase;
class SScenarioActionEditor;
class SScenarioActionTree;
class FStructOnScope;
class UActorComponent;
class AActor;

/**
 * UScenarioActionNode
 */
UCLASS(abstract)
class UNREALSODA_API UScenarioActionNode : public UObject
{
	GENERATED_BODY()

public:
	UScenarioActionNode(const FObjectInitializer& InInitializer);
	virtual ~UScenarioActionNode() {};

public:
	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree) PURE_VIRTUAL(UScenarioActionNode::MakeRowWidget, return TSharedPtr< ITableRow >().ToSharedRef(); );
	virtual TSharedPtr<SWidget> OnContextMenuOpening() { return SNullWidget::NullWidget; }
	virtual TArray< TWeakObjectPtr<UScenarioActionNode> >& GetChildren() { return WeakChildren; }
	virtual void AddChildrenNode(UScenarioActionNode* Node, bool bSaveHardRef = true);
	virtual UScenarioActionNode* GetParentNode() const { return ParenNode.Get(); }
	virtual bool RemoveChildrenNode(UScenarioActionNode* Node);
	virtual bool RemoveThisNode();
	virtual bool IsNodeValid() const { return false; }
	virtual bool ShouldSerializeChildren() const { return false; };
	virtual bool IsTransitentActionNode() const { return false; };
	virtual void Execute();

	template <typename T> 
	T* GetParentNode() { return Cast<T>(GetParentNode()); }

	template <typename T>
	T* GetParentNodeChecked() { return CastChecked<T>(GetParentNode()); }

public:
	virtual void Serialize(FArchive& Ar) override;
	
public:

	/**
	 * Get the track row's display name.
	 *
	 * @return Display name text.
	 */
	//virtual FText GetRowDisplayName(int32 RowIndex) const { return FText::FromString(TEXT("Unnamed Track")); }

	/**
	 * Get the track's display tooltip text to be shown on the track's name.
	 *
	 * @return Display tooltip text.
	 */
	//virtual FText GetDisplayNameToolTipText() const { return FText::GetEmpty(); }


	/**
	 * Get this folder's desired sorting order
	 */
	int32 GetSortingOrder() const
	{
		return SortingOrder;
	}

	/**
	 * Set this folder's desired sorting order.
	 *
	 * @param InSortingOrder The higher the value the further down the list the folder will be.
	 */
	void SetSortingOrder(const int32 InSortingOrder)
	{
		SortingOrder = InSortingOrder;
	}

protected:
	/** This folder's desired sorting order */
	UPROPERTY()
	int32 SortingOrder = -1;

	UPROPERTY()
	TArray <UScenarioActionNode*> Children;

	TArray <TWeakObjectPtr<UScenarioActionNode>> WeakChildren;

	TWeakObjectPtr<UScenarioActionNode> ParenNode;
};

/**
 * UScenarioObjectNode
 */
UCLASS()
class UScenarioObjectNode : public UScenarioActionNode
{
	GENERATED_BODY()

public:
	UScenarioObjectNode(const FObjectInitializer& InInitializer);
	virtual ~UScenarioObjectNode() {}

public:
	virtual bool IsNodeValid() const override;
	virtual bool ShouldSerializeChildren() const { return true; }
	virtual void Execute() override;

public:
	virtual void Serialize(FArchive& Ar) override;

public:
	void InitializeNode(UObject* InObject);
	UObject * GetObject() const { return Object.Get(); }
	TSharedPtr<FStructOnScope> GetStructOnScope() { return StructOnScope; }
	void CaptureProperty(FProperty* Property) { check(Property); CapturedProperties.Add(Property); }
	const TSet<TWeakFieldPtr<FProperty>> & GetCapturedProperties() const { return CapturedProperties; };
	TSet<TWeakFieldPtr<FProperty>>& GetCapturedProperties() { return CapturedProperties; };

protected:
	UPROPERTY(SaveGame)
	TSoftObjectPtr<UObject> Object;

	TSharedPtr<FStructOnScope> StructOnScope;

	TSet<TWeakFieldPtr<FProperty>> CapturedProperties;

	TWeakObjectPtr<UScenarioActionPropertyNode> ChildPropertyNode;
};

/**
 * UScenarioActionActorNode
 */
UCLASS()
class UScenarioActionActorNode : public UScenarioObjectNode
{
	GENERATED_BODY()

public:
	UScenarioActionActorNode(const FObjectInitializer& InInitializer)
		: UScenarioObjectNode(InInitializer) {}
	virtual ~UScenarioActionActorNode() {}

public:
	virtual void Serialize(FArchive& Ar) override;

public:
	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)  override;
	virtual TSharedPtr<SWidget> OnContextMenuOpening() override { return SNullWidget::NullWidget; }
	virtual bool IsNodeValid() const override;

public:
	void InitializeNode(AActor* InActor);
	AActor* GetActor() const;

protected:
	UPROPERTY(SaveGame)
	TSoftObjectPtr<AActor> Actor;
};

/**
 * UScenarioActionComponentNode
 */
UCLASS()
class UScenarioActionComponentNode : public UScenarioObjectNode
{
	GENERATED_BODY()

public:
	UScenarioActionComponentNode(const FObjectInitializer& InInitializer)
		: UScenarioObjectNode(InInitializer) {}
	virtual ~UScenarioActionComponentNode() {}

public:
	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)  override;
	virtual TSharedPtr<SWidget> OnContextMenuOpening() override { return SNullWidget::NullWidget; }
	virtual bool IsNodeValid() const override;

public:
	void InitializeNode(UActorComponent* InActorComponent);
	UActorComponent* GetActorComponent() const;

protected:
	UPROPERTY(SaveGame)
	TSoftObjectPtr<UActorComponent> ActorComponent;
};

/**
 * UScenarioActionPropertyNode
 */
UCLASS()
class UScenarioActionPropertyNode : public UScenarioActionNode
{
	GENERATED_BODY()

public:
	UScenarioActionPropertyNode(const FObjectInitializer& InInitializer);
	virtual ~UScenarioActionPropertyNode() {}

public:
	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree) override;
	virtual TSharedPtr<SWidget> OnContextMenuOpening() override { return SNullWidget::NullWidget; }
	virtual bool IsNodeValid() const override;
	virtual bool IsTransitentActionNode() const { return true; }

public:
	void UpdateChildBodyNode();
	
	UScenarioActionPropertyBodyNode* GetChildBodyNode() const { return ChildBodyNode.Get(); }

protected:
	TWeakObjectPtr<UScenarioActionPropertyBodyNode> ChildBodyNode;
};

/**
 * UScenarioActionPropertyBodyNode
 */
UCLASS()
class UScenarioActionPropertyBodyNode : public UScenarioActionNode
{
	GENERATED_BODY()

public:
	UScenarioActionPropertyBodyNode(const FObjectInitializer& InInitializer)
		: UScenarioActionNode(InInitializer) {}
	virtual ~UScenarioActionPropertyBodyNode() {}

	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree) override;
	virtual TSharedPtr<SWidget> OnContextMenuOpening() override { return SNullWidget::NullWidget; }
	virtual bool IsNodeValid() const override;
	virtual bool IsTransitentActionNode() const { return true; }
};

/**
 * UScenarioActionFunctionNode
 */
UCLASS()
class UScenarioActionFunctionNode : public UScenarioActionNode
{
	GENERATED_BODY()

public:
	UScenarioActionFunctionNode(const FObjectInitializer& InInitializer);
	virtual ~UScenarioActionFunctionNode() {}

public:
	virtual void Serialize(FArchive& Ar) override;

public:
	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)  override;
	virtual TSharedPtr<SWidget> OnContextMenuOpening() override { return SNullWidget::NullWidget; }
	virtual bool IsNodeValid() const override { return StructOnScope.IsValid(); }
	virtual void Execute();

public:
	void InitializeNode(UObject* FunctionOwner, UFunction* Function);
	TSharedPtr<FStructOnScope> GetStructOnScope() const { return StructOnScope; }

protected:
	TSharedPtr<FStructOnScope> StructOnScope;

	UPROPERTY(SaveGame)
	TSoftObjectPtr<UFunction> Function;

	UPROPERTY(SaveGame)
	TSoftObjectPtr<UObject> FunctionOwner;
};

/**
 * UScenarioActionFunctionBodyNode
 */
UCLASS()
class UScenarioActionFunctionBodyNode : public UScenarioActionNode
{
	GENERATED_BODY()

public:
	UScenarioActionFunctionBodyNode(const FObjectInitializer& InInitializer)
		: UScenarioActionNode(InInitializer) {}
	virtual ~UScenarioActionFunctionBodyNode() {}

public:
	virtual TSharedRef< ITableRow > MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)  override;
	virtual TSharedPtr<SWidget> OnContextMenuOpening() override { return SNullWidget::NullWidget; }
	virtual bool IsNodeValid() const override;
};


