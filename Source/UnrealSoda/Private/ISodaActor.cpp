// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ISodaActor.h"
#include "Soda/UnrealSoda.h"
#include "GameFramework/Actor.h"
#include "Soda/LevelState.h"

void ISodaActor::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	IEditableObject::RuntimePostEditChangeProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;
	//const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	static FName RelativeLocationName("RelativeLocation");
	static FName RelativeRotationName("RelativeRotation");
	static FName RelativeScale3DName("RelativeScale3D");

	if (PropertyName == RelativeLocationName || PropertyName == RelativeRotationName || PropertyName == RelativeScale3DName)
	{
		if (ALevelState* LevelState = ALevelState::Get())
		{
			LevelState->MarkAsDirty();
		}
	}

	if (PropertyChangedEvent.Property->HasAllPropertyFlags(CPF_SaveGame))
	{
		MarkAsDirty();
	}
}

void ISodaActor::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	IEditableObject::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;
	//const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	static FName RelativeLocationName("RelativeLocation");
	static FName RelativeRotationName("RelativeRotation");
	static FName RelativeScale3DName("RelativeScale3D");

	if (PropertyName == RelativeLocationName || PropertyName == RelativeRotationName || PropertyName == RelativeScale3DName)
	{
		if (ALevelState* LevelState = ALevelState::Get())
		{
			LevelState->MarkAsDirty();
		}
	}

	if (PropertyChangedEvent.Property->HasAllPropertyFlags(CPF_SaveGame))
	{
		MarkAsDirty();
	}
}

void ISodaActor::ScenarioBegin()
{
	//ReceiveScenarioBegin();
	ISodaActor::Execute_ReceiveScenarioBegin(AsActor());
}

void ISodaActor::ScenarioEnd()
{
	//ReceiveScenarioEnd();
	ISodaActor::Execute_ReceiveScenarioEnd(AsActor());
}

AActor* ISodaActor::AsActor() 
{ 
	return CastChecked<AActor>(this); 
}

const AActor* ISodaActor::AsActor() const
{ 
	return CastChecked<AActor>(this); 
}

void ISodaActor::MarkAsDirty() 
{
	if (!IsPinnedActor())
	{
		if (ALevelState* LevelState = ALevelState::Get())
		{
			LevelState->MarkAsDirty();
		}
	}
	else
	{
		bIsDirty = true;
	}
}