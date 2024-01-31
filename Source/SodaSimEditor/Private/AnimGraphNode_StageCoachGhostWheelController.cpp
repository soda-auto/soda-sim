// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "AnimGraphNode_StageCoachGhostWheelController.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Soda/Actors/GhostVehicle/GhostVehicle.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_StageCoachGhostWheelController::UAnimGraphNode_StageCoachGhostWheelController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_StageCoachGhostWheelController::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_StageCoachGhostWheelController", "Stagecoach Effect Wheel Controller for ChaosWheeledVehicle");
}

FText UAnimGraphNode_StageCoachGhostWheelController::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_StageCoachGhostWheelController_Tooltip", "This alters the wheel transform based on set up in Wheeled Vehicle. This only works when the owner is ChaosWheeledVehicle.");
}

FText UAnimGraphNode_StageCoachGhostWheelController::GetNodeTitle(ENodeTitleType::Type TitleType) const
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
		// name of bones for wheels but it's not available in Persona
		NodeTitle = FText(LOCTEXT("UAnimGraphNode_StageCoachGhostWheelController_Title", "Stagecoach Effect Wheel Controller"));
	}	
	return NodeTitle;
}

void UAnimGraphNode_StageCoachGhostWheelController::ValidateAnimNodePostCompile(class FCompilerResultsLog& MessageLog, class UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{
	// we only support vehicle anim instance
	if ( CompiledClass->IsChildOf(UGhostVehicleAnimationInstance::StaticClass())  == false )
	{
		MessageLog.Error(TEXT("@@ is only allowed in AGhostVehicleAnimInstance. If this is for vehicle, please change parent to be VehicleAnimInstance (Reparent Class)."), this);
	}
}

bool UAnimGraphNode_StageCoachGhostWheelController::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return (Blueprint != nullptr) && Blueprint->ParentClass->IsChildOf<UGhostVehicleAnimationInstance>() && Super::IsCompatibleWithGraph(TargetGraph);
}

#undef LOCTEXT_NAMESPACE
