// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "Soda/ScenarioAction/ScenarioActionConditionFunctionLibrary.h"
#include "Soda/ScenarioAction/ScenarioActionFunctionLibrary.h"
#include "Soda/ScenarioAction/ScenarioActionEvent.h"
#include "RuntimeMetaData.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "UObject/UObjectIterator.h"

namespace FScenarioActionUtils
{

void GetComponentsByCategory(AActor* Actor, TMap<FString, TArray<UActorComponent*>> & Out)
{
	check(Actor);
	for (auto& Component : Actor->GetComponents())
	{
		if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(Component))
		{
			Out.FindOrAdd(VehicleComponent->GetVehicleComponentGUI().Category).Add(Component);
		}
	}
}


void GetScenarioFunctionsByCategory(UClass* Class, TMap<FString, TArray<UFunction*>> & Out)
{
	for (TFieldIterator<UFunction> FuncIt(Class); FuncIt; ++FuncIt)
	{
		if (FRuntimeMetaData::HasMetaData(*FuncIt, TEXT("ScenarioAction")))
		{
			Out.FindOrAdd(FRuntimeMetaData::GetMetaData(*FuncIt, TEXT("Category"))).Add(*FuncIt);
		}
	}

	do
	{
		for (auto& Interface : Class->Interfaces)
		{
			GetScenarioFunctionsByCategory(Interface.Class, Out);
		}
		Class = Class->GetSuperClass();
	} 
	while (Class);
}

void GetScenarioPropertiesByCategory(UClass* Class, TMap<FString, TArray<FProperty*>> & Out)
{
	// TODO: Try to invoke AddGeneratedMetadataToGlobalScope() once when object loaded
	if (Class->ImplementsInterface(UEditableObject::StaticClass()))
	{
		FRuntimeMetaData::AddGeneratedMetadataToGlobalScope(Class);
	}

	for (TFieldIterator<FProperty> It(Class); It; ++It)
	{
		if (FRuntimeMetaData::HasMetaData(*It, TEXT("EditInRuntime")))
		{
			Out.FindOrAdd(FRuntimeMetaData::GetMetaData(*It, TEXT("Category"))).Add(*It);
		}
	}
}

void GetScenarioActionFunctionsByCategory(TMap<FString, TArray<UFunction*>> & Out)
{
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UScenarioActionFunctionLibraryBase::StaticClass()) &&
			!It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden) &&
			It->GetPackage() != GetTransientPackage())
		{
			for (TFieldIterator<UFunction> FuncIt(*It); FuncIt; ++FuncIt)
			{
				if (FRuntimeMetaData::HasMetaData(*FuncIt, TEXT("Category")))
				{
					Out.FindOrAdd(FRuntimeMetaData::GetMetaData(*FuncIt, TEXT("Category"))).Add(*FuncIt);
				}
			}
		}
	}
}

void GetScenarioConditionFunctionsByCategory(TMap<FString, TArray<UFunction*>> & Out)
{
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UScenarioActionConditionFunctionLibraryBase::StaticClass()) &&
			!It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden) &&
			It->GetPackage() != GetTransientPackage())
		{
			for (TFieldIterator<UFunction> FuncIt(*It); FuncIt; ++FuncIt)
			{
				if (FuncIt->GetReturnProperty())
				{
					if (FuncIt->GetReturnProperty()->IsA<FBoolProperty>())
					{
						Out.FindOrAdd(FRuntimeMetaData::GetMetaData(*FuncIt, TEXT("Category"))).Add(*FuncIt);
					}
				}
			}
		}
	}
}

void GetScenarioActionEvents(TMap<FString, TArray<UClass*>> & Out)
{
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UScenarioActionEvent::StaticClass()) &&
			!It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden) &&
			It->GetPackage() != GetTransientPackage())
		{
			Out.FindOrAdd(TEXT("Default")).Add(*It);
		}
	}
}

bool SerializeUStruct(FArchive& Ar, UStruct * Struct, uint8* StructMemory, bool bForceSaveGame)
{
	//FScenarioActionUtils::Serialize(StructOnScope, Ar);

	bool Res = false;

	if (Ar.IsSaveGame())
	{
		if (bForceSaveGame)
		{
			Ar.ArIsSaveGame = 0; // Hack to serialize properties in UFunction
		}

		if (Ar.IsSaving())
		{
			// The following code serializes the size of the arguments to serialize so that when loading,
			// we can skip over it if the function could not be loaded.
			const int64 ArgumentsSizePos = Ar.Tell();

			// Serialize a temporary value for the delta in order to end up with an archive of the right size.
			int64 ArgumentsSize = 0;
			Ar << ArgumentsSize;

			// Serialize the number of argument bytes if there is actually an underlying buffer to seek to.
			if (ArgumentsSizePos != INDEX_NONE && IsValid(Struct))
			{
				// Then serialize the arguments in order to get its size.
				const int64 ArgsBegin = Ar.Tell();
				Struct->SerializeTaggedProperties(Ar, StructMemory, Struct, nullptr);
				const int64 ArgsEnd = Ar.Tell();
				ArgumentsSize = (ArgsEnd - ArgsBegin);

				// Come back to the temporary value we wrote and overwrite it with the arguments size we just calculated.
				Ar.Seek(ArgumentsSizePos);
				Ar << ArgumentsSize;

				// And finally seek back to the end.
				Ar.Seek(ArgsEnd);
			}
			Res = true;
		}
		else if (Ar.IsLoading())
		{
			int64 NumSerializedBytesFromArchive = 0;
			Ar << NumSerializedBytesFromArchive;

			if (NumSerializedBytesFromArchive > 0)
			{
				if (IsValid(Struct))
				{
					Struct->InitializeStruct(StructMemory);

					const int64 ArgsBegin = Ar.Tell();
					Struct->SerializeTaggedProperties(Ar, StructMemory, Struct, nullptr);
					const int64 ArgsEnd = Ar.Tell();

					if (ArgsEnd - ArgsBegin != NumSerializedBytesFromArchive)
					{
						UE_LOG(LogSoda, Warning, TEXT("Arguments size mismatch from size serialized in asset. Expected %d, Actual: %d"), NumSerializedBytesFromArchive, ArgsEnd - ArgsBegin);
						Ar.SetError();
					}
					else
					{
						Res = true;
					}
				}
				else
				{
					UE_LOG(LogSoda, Warning, TEXT("%s could not be loaded while deserialzing."), *Struct->GetStructPathName().ToString());
					Ar.Seek(Ar.Tell() + NumSerializedBytesFromArchive);
					Ar.SetError();
				}
			}
		}

		if (bForceSaveGame)
		{
			Ar.ArIsSaveGame = 1; // Return SaveGame flag 
		}
	}

	return Res;
}

} // namespace FScenarioActionUtils