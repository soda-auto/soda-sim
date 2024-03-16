// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "AnimGraphNode_SodaWheelController.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Soda/Vehicles/SodaVehicle.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_SodaWheelController::UAnimGraphNode_SodaWheelController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_SodaWheelController::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_SodaWheelController", "Wheel Controller for ASodaVehicle");
}

FText UAnimGraphNode_SodaWheelController::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_SodaWheelController_Tooltip", "This alters the wheel transform based on set up in Wheeled Vehicle. This only works when the owner is WheeledVehicle.");
}

FText UAnimGraphNode_SodaWheelController::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText NodeTitle;
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		NodeTitle = GetControllerDescription();
	}
	else
	{
		// we don't have any run-time information, so it's limited to print  
		// anymore than what it is it would be nice to print more data such as 
		// name of bones for wheels, but it's not available in Persona
		NodeTitle = FText(LOCTEXT("UAnimGraphNode_SodaWheelController_Title", "Wheel Controller"));
	}	
	return NodeTitle;
}

void UAnimGraphNode_SodaWheelController::ValidateAnimNodePostCompile(class FCompilerResultsLog& MessageLog, class UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{
	// we only support vehicle anim instance
	if ( CompiledClass->IsChildOf(USodaVehicleAnimationInstance::StaticClass())  == false )
	{
		MessageLog.Error(TEXT("@@ is only allowwed in ASodaVehicleAnimInstance. If this is for vehicle, please change parent to be VehicleAnimInstance (Reparent Class)."), this);
	}
}

bool UAnimGraphNode_SodaWheelController::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return (Blueprint != nullptr) && Blueprint->ParentClass->IsChildOf<USodaVehicleAnimationInstance>() && Super::IsCompatibleWithGraph(TargetGraph);
}

#undef LOCTEXT_NAMESPACE
