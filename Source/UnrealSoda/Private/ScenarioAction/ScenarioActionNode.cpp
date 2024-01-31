// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Soda/ScenarioAction/ScenarioActionBlock.h"
#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionActorRow.h"
#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionComponentRow.h"
#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionFunctionBodyRow.h"
#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionFunctionRow.h"
#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionPropertyRow.h"
#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionPropertyBodyRow.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "UObject/StructOnScope.h"
#include "Widgets/Views/STableViewBase.h"

//-------------------------------------------------------------------------------------------------------------------------
UScenarioActionNode::UScenarioActionNode(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
}

void UScenarioActionNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame() && ShouldSerializeChildren())
	{
		if (Ar.IsSaving())
		{
			int Num = 0;
			for (int i = 0; i < Children.Num(); ++i)
			{
				UScenarioActionNode* Node = Children[i];
				if (IsValid(Node) && !Node->IsTransitentActionNode())
				{
					++Num;
				}
			}
			Ar << Num;
			for (int i = 0; i < Children.Num(); ++i)
			{
				UScenarioActionNode* Node = Children[i];
				if (IsValid(Node) && !Node->IsTransitentActionNode())
				{
					check(IsValid(Node));
					FObjectRecord Record;
					Record.SerializeObject(Node);
					Ar << Record;
				}
			}
		}
		else if (Ar.IsLoading())
		{
			// Loading the ActionNodes
			int Num = 0;
			Ar << Num;
			for (int i = 0; i < Num; ++i)
			{
				FObjectRecord Record;
				Ar << Record;
				if (Record.IsRecordValid())
				{
					UScenarioActionNode* Node = NewObject<UScenarioActionNode>(this, Record.Class.Get());
					check(Node);
					Record.DeserializeObject(Node);
					AddChildrenNode(Node, true);
				}
			}
		}
	}
}

void UScenarioActionNode::AddChildrenNode(UScenarioActionNode* Node, bool bSaveHardRef)
{
	check(Node);
	WeakChildren.Add(Node);
	if (bSaveHardRef) Children.Add(Node);
	Node->ParenNode = this;
}

bool UScenarioActionNode::RemoveChildrenNode(UScenarioActionNode* Node)
{
	return WeakChildren.Remove(Node) + Children.Remove(Node) ? true : false;
}

bool UScenarioActionNode::RemoveThisNode()
{
	if (UScenarioActionNode* Parent = GetParentNode())
	{
		return Parent->RemoveChildrenNode(this);
	}

	if (UScenarioActionBlock* Block = Cast<UScenarioActionBlock>(this->GetOuter()))
	{
		return Block->ActionRootNodes.Remove(this) > 0;
	}

	return false;
}

void UScenarioActionNode::Execute() 
{
	for (auto& It : Children)
	{
		if (IsValid(It))
		{
			It->Execute();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------
UScenarioObjectNode::UScenarioObjectNode(const FObjectInitializer& InInitializer)
	: UScenarioActionNode(InInitializer) 
{
	ChildPropertyNode = CreateDefaultSubobject< UScenarioActionPropertyNode>(TEXT("Boody"));
	AddChildrenNode(ChildPropertyNode.Get());
}

bool UScenarioObjectNode::IsNodeValid() const 
{ 
	return Object.IsValid() && StructOnScope.IsValid(); 
}

void UScenarioObjectNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	UClass* Class = Object.IsValid() ? Object.Get()->GetClass() : nullptr;
	
	if (Ar.IsSaveGame())
	{
		if (Ar.IsLoading())
		{
			StructOnScope = MakeShared<FStructOnScope>(Class);
		}
		FScenarioActionUtils::SerializeUStruct(Ar, Class, Class ? StructOnScope->GetStructMemory() : nullptr, false);

		if (Ar.IsSaving())
		{
			// Save Properties
			TArray<FString> SavedProperties;
			for (auto& It : CapturedProperties)
			{
				if (It.IsValid())
				{
					SavedProperties.Add(It->GetName());
				}
			}
			Ar << SavedProperties;
		}
		else if (Ar.IsLoading())
		{
			// Load Properties
			TArray<FString> SavedProperties;
			Ar << SavedProperties;
			if (Object.IsValid())
			{
				for (auto& It : SavedProperties)
				{
					for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
					{
						if (PropIt->GetName() == It)
						{
							CapturedProperties.Add(*PropIt);
						}
					}
				}
			}

			ChildPropertyNode->UpdateChildBodyNode();
		}
	}
}

void UScenarioObjectNode::InitializeNode(UObject* InObject)
{
	check(InObject);
	Object = InObject;
	StructOnScope = MakeShared<FStructOnScope>(InObject->GetClass());
}

void UScenarioObjectNode::Execute()
{
	Super::Execute();

	if (StructOnScope.IsValid() && Object.IsValid())
	{
		for (auto& Prop : CapturedProperties)
		{
			void* SrcPropertyValue = Prop->ContainerPtrToValuePtr<void>(StructOnScope->GetStructMemory());
			void* DstPropertyValue = Prop->ContainerPtrToValuePtr<void>(Object.Get());
			Prop->CopySingleValue(DstPropertyValue, SrcPropertyValue);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------
TSharedRef< ITableRow > UScenarioActionActorNode::MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)
{
	return SNew(SScenarioActionActorRow, OwnerTable, this, Tree);
}

bool UScenarioActionActorNode::IsNodeValid() const
{
	return Super::IsNodeValid() && Actor.IsValid(); 
}

void UScenarioActionActorNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame() && ShouldSerializeChildren())
	{
		if (Ar.IsSaving())
		{

		}
		else if (Ar.IsLoading())
		{

		}
	}
}

void UScenarioActionActorNode::InitializeNode(AActor* InActor)
{ 
	UScenarioObjectNode::InitializeNode(InActor); 
	Actor = InActor; 
}

AActor* UScenarioActionActorNode::GetActor() const 
{
	return Actor.Get(); 
}

//-------------------------------------------------------------------------------------------------------------------------
TSharedRef< ITableRow > UScenarioActionComponentNode::MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)
{
	return SNew(SScenarioActionComponentRow, OwnerTable, this, Tree);
}

void UScenarioActionComponentNode::InitializeNode(UActorComponent* InActorComponent)
{ 
	UScenarioObjectNode::InitializeNode(InActorComponent); 
	ActorComponent = InActorComponent; 
}

UActorComponent* UScenarioActionComponentNode::GetActorComponent() const
{ 
	return ActorComponent.Get(); 
}

bool UScenarioActionComponentNode::IsNodeValid() const 
{
	return Super::IsNodeValid() && ActorComponent.IsValid(); 
}

//-------------------------------------------------------------------------------------------------------------------------
UScenarioActionPropertyNode::UScenarioActionPropertyNode(const FObjectInitializer& InInitializer)
	: UScenarioActionNode(InInitializer)
{
}

bool UScenarioActionPropertyNode::IsNodeValid() const
{ 
	UScenarioObjectNode* ParentNode = Cast<UScenarioObjectNode>(GetParentNode()); 
	return ParentNode && ParentNode->IsNodeValid(); 
}

void UScenarioActionPropertyNode::UpdateChildBodyNode()
{
	WeakChildren.Empty();
	Children.Empty();

	UScenarioObjectNode* Parent = Cast<UScenarioObjectNode>(GetParentNode());

	if (Parent && Parent->GetCapturedProperties().Num())
	{
		ChildBodyNode = NewObject< UScenarioActionPropertyBodyNode>(this);
		check(ChildBodyNode.IsValid());
		AddChildrenNode(ChildBodyNode.Get());
	}
}

TSharedRef< ITableRow > UScenarioActionPropertyNode::MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)
{
	return SNew(SScenarioActionPropertyRow, OwnerTable, this, Tree);
}

//-------------------------------------------------------------------------------------------------------------------------
TSharedRef< ITableRow > UScenarioActionPropertyBodyNode::MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)
{
	return SNew(SScenarioActionPropertyBodyRow, OwnerTable, this, Tree);
}

bool UScenarioActionPropertyBodyNode::IsNodeValid() const 
{
	UScenarioActionPropertyNode* ParentNode = Cast<UScenarioActionPropertyNode>(GetParentNode()); 
	return ParentNode && ParentNode->IsNodeValid(); 
}

//-------------------------------------------------------------------------------------------------------------------------
UScenarioActionFunctionNode::UScenarioActionFunctionNode(const FObjectInitializer& InInitializer)
	: UScenarioActionNode(InInitializer) 
{
	UScenarioActionFunctionBodyNode * ChildNode = CreateDefaultSubobject< UScenarioActionFunctionBodyNode>(TEXT("Boody"));
	AddChildrenNode(ChildNode);
}

void UScenarioActionFunctionNode::InitializeNode(UObject* InFunctionOwner, UFunction* InFunction)
{
	check(InFunction && InFunctionOwner);
	Function = InFunction;
	StructOnScope = MakeShared< FStructOnScope>(InFunction);
	FunctionOwner = InFunctionOwner;
}

void UScenarioActionFunctionNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame())
	{
		if (Ar.IsLoading() && Function.IsValid())
		{
			StructOnScope = MakeShared<FStructOnScope>(Function.Get());
		}
		FScenarioActionUtils::SerializeUStruct(Ar, Function.Get(), Function.IsValid() ? StructOnScope->GetStructMemory() : nullptr, true);
	}
}

TSharedRef< ITableRow > UScenarioActionFunctionNode::MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)
{
	return SNew(SScenarioActionFunctionRow, OwnerTable, this, Tree);
}

void UScenarioActionFunctionNode::Execute()
{
	Super::Execute();

	if (Function && StructOnScope.IsValid())
	{
		if (FunctionOwner)
		{
			FunctionOwner->ProcessEvent(Function.Get(), StructOnScope->GetStructMemory());
		}
		else
		{
			//TODO here ....
			check(0);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------
TSharedRef< ITableRow > UScenarioActionFunctionBodyNode::MakeRowWidget(const TSharedRef< STableViewBase >& OwnerTable, const TSharedRef<SScenarioActionTree>& Tree)
{
	return SNew(SScenarioActionFunctionBodyRow, OwnerTable, this, Tree);
}

bool UScenarioActionFunctionBodyNode::IsNodeValid() const 
{ 
	UScenarioActionFunctionNode* ParentNode = Cast<UScenarioActionFunctionNode>(GetParentNode()); 
	return ParentNode->IsNodeValid(); 
}