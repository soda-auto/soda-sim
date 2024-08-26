// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/SerializationHelpers.h"
#include "Soda/UnrealSoda.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/UObjectGlobals.h"

//TODO: See to FConcertSyncObjectWriter and FConcertSyncObjectReader

struct FSaveExtensionArchive : public FObjectAndNameAsStringProxyArchive 
{
	FSaveExtensionArchive(FArchive &InInnerArchive, bool bInLoadIfFindFails) : FObjectAndNameAsStringProxyArchive(InInnerArchive, bInLoadIfFindFails) 
	{
		ArIsSaveGame = true;
    	ArNoDelta = true;
	}

	virtual FArchive &operator<<(struct FSoftObjectPtr &Value) override
	{
		*this << Value.GetUniqueID();
		return *this;
	}

	virtual FArchive &operator<<(struct FSoftObjectPath &Value) override
	{
		FString Path = Value.ToString();
		*this << Path;
		if (IsLoading())
		{
			Value.SetPath(MoveTemp(Path));
		}
		return *this;
	}
	
	virtual FArchive& operator<<(UObject*& Obj) override
	{
		FProperty* SerializingProperty = GetSerializedProperty();
		const bool bHasInstancedValue = SerializingProperty && SerializingProperty->HasAnyPropertyFlags(CPF_PersistentInstance);

		if (IsSaveGame() && bHasInstancedValue)
		{
			if (IsLoading())
			{
				UObject* Outer = SerializingProperty->GetOwnerUObject();
				if (!Outer)
				{
					Outer = GetTransientPackage();
				}

				//FObjectProperty* ObjectProperty = CastField<FObjectProperty>(SerializingProperty);
				//UClass* PropertyClass = ObjectProperty->PropertyClass;

				// load the path name to the object
				FString ClassString;
				InnerArchive << ClassString;

				if (!ClassString.IsEmpty())
				{
					UClass* FoundClass = FPackageName::IsShortPackageName(ClassString) ? FindFirstObject<UClass>(*ClassString) : UClass::TryFindTypeSlow<UClass>(ClassString);
					if (FoundClass)
					{
						Obj = StaticAllocateObject(FoundClass, Outer, NAME_None, EObjectFlags::RF_NoFlags, EInternalObjectFlags::None, false);
						(*FoundClass->ClassConstructor)(FObjectInitializer(Obj, FoundClass, EObjectInitializerOptions::None));
						Obj->Serialize(*this);
					}
				}
			}
			else if (Obj)
			{
				// save out the fully qualified object name
				FString ClassString(Obj->GetClass()->GetPathName());
				InnerArchive << ClassString;
				Obj->Serialize(*this);
			}
			else
			{
				// for null pointer, output empty string
				FString ClassString;
				InnerArchive << ClassString;
			}
		}
		return *this;
	}
	
};

/******************************************************************
* FObjectRecord
*******************************************************************/
void FObjectRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.GetError())
	{
		UE_LOG(LogSoda, Error, TEXT("FObjectRecord::Serialize(); FArchive is in error state; Name: %s; IsCriticalError: %i"), *Name.ToString(), Ar.IsCriticalError());
	}

	if (Ar.IsSaving() && !Name.IsNone() && !Class)
	{
		UE_LOG(LogSoda, Fatal, TEXT("FObjectRecord::Serialize(); Don't need to save named object without class"));
	}
	if (!Name.IsNone()) Ar << Class;
	Ar << Data;
}

bool FObjectRecord::SerializeObject(UObject* Object)
{
	Data.Reset();
	Name = NAME_None;
	Class.Reset();

	if (!IsValid(Object))
	{
		return false;
	}

	Name = Object->GetFName();
	Class = Object->GetClass();

	FMemoryWriter MemoryWriter(Data, true);
	FSaveExtensionArchive Archive(MemoryWriter, false);
	Object->Serialize(Archive);

	return true;
}

bool FObjectRecord::DeserializeObject(UObject* Object)
{
	if (!IsRecordValid() || !IsValid(Object))
	{
		return false;
	}

	if (Object->GetClass() != Class)
	{
		UE_LOG(LogSoda, Warning, TEXT("DeserializeObject(); Component %s: Class %s != Record.Class(%s)"),
			*Object->GetFName().ToString(),
			*Object->GetClass()->GetDefaultObjectName().ToString(),
			*Class->GetDefaultObjectName().ToString());
		return false;
	}

	FMemoryReader MemoryReader(Data, true);
	FSaveExtensionArchive Archive(MemoryReader, false);
	Object->Serialize(Archive);

	return true;
}

/******************************************************************
* FComponentRecord
*******************************************************************/
void FComponentRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Transform;
}

bool FComponentRecord::SerializeComponent(UActorComponent* Component)
{
	if (!SerializeObject(Component))
	{
		return false;
	}

	const USceneComponent* Scene = Cast<USceneComponent>(Component);
	if (Scene && Scene->Mobility == EComponentMobility::Movable)
	{
		Transform = Scene->GetRelativeTransform();
	}

	return true;
}

bool FComponentRecord::DeserializeComponent(UActorComponent* Component)
{
	if (!DeserializeObject(Component))
	{
		return false;
	}

	USceneComponent* Root = nullptr;
	AActor* Actor = Component->GetOwner();
	if (Actor) Root = Actor->GetRootComponent();

	USceneComponent* Scene = Cast<USceneComponent>(Component);
	if (Scene && Scene->Mobility == EComponentMobility::Movable && Scene != Root)
	{
		Scene->SetRelativeTransform(Transform);
	}

	return true;
}

/******************************************************************
* FActorRecord
*******************************************************************/
void FActorRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Transform;
	Ar << ComponentRecords;
}

bool FActorRecord::SerializeComponent(UActorComponent* Component)
{
	if (!IsValid(Component))
	{
		return false;
	}

	FComponentRecord* ComponentRecord = ComponentRecords.FindByKey(Component);

	if (!ComponentRecord)
	{
		ComponentRecord = &ComponentRecords.Add_GetRef(FComponentRecord());
	}

	return ComponentRecord->SerializeComponent(Component);;
}

bool FActorRecord::DeserializeComponent(UActorComponent* Component)
{
	if (!IsValid(Component))
	{
		return false;
	}

	if (FComponentRecord* Record = ComponentRecords.FindByKey(Component))
	{
		return Record->DeserializeComponent(Component);
	}

	return false;
}

bool FActorRecord::SerializeActor(AActor* Actor, bool bWithComponents)
{
	if (!SerializeObject(Actor))
	{
		return false;
	}

	Transform = Actor->GetTransform();

	if (bWithComponents)
	{
		const TSet<UActorComponent*>& Components = Actor->GetComponents();
		for (auto* Component : Components)
		{
			SerializeComponent(Component);
		}
	}

	return true;
}

bool FActorRecord::DeserializeActor(AActor* Actor, bool bWithComponents, bool bSetTransform)
{
	if (!DeserializeObject(Actor))
	{
		return false;
	}

	if (bWithComponents)
	{
		const TSet<UActorComponent*>& Components = Actor->GetComponents();
		for (auto* Component : Components)
		{
			DeserializeComponent(Component);
		}
	}

	if (bSetTransform)
	{
		Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::ResetPhysics);
	}

	return true;
}

/******************************************************************
* USaveGameActor
*******************************************************************/
bool USaveGameActor::SerializeComponent(UActorComponent* Component)
{
	return ActorRecord.SerializeComponent(Component);
}

bool USaveGameActor::DeserializeComponent(UActorComponent* Component)
{
	return ActorRecord.DeserializeComponent(Component);
}

bool USaveGameActor::SerializeActor(AActor* Actor, bool bWithComponents)
{
	return ActorRecord.SerializeActor(Actor, bWithComponents);
}

bool USaveGameActor::DeserializeActor(AActor* Actor, bool bWithComponents)
{
	return ActorRecord.DeserializeActor(Actor, bWithComponents);
}

void USaveGameActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << ActorRecord;
}