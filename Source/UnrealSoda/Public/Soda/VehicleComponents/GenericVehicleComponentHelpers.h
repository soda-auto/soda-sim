// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Templates/SubclassOf.h"
#include "GenericVehicleComponentHelpers.generated.h"

//class UCanvas;
class UVehicleBaseComponent;
struct FSensorDataHeader;

/**
 * UGenericPublisher
 * Base class for all generic publishers
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericPublisher : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Advertise(UVehicleBaseComponent* Parent) { return false; }
	virtual void Shutdown() {}
	virtual bool IsInitializing() const { return false; }
	virtual bool IsOk() const { return false; }
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) {}
};


/**
 * UGenericPublisher
 * Base class for all generic listener
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericListener : public UObject
{
	GENERATED_BODY()

public:
	virtual bool StartListen(UVehicleBaseComponent * Parent) { return false; }
	virtual void StopListen() {}
	virtual bool IsOk() const { return false; }
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) {}
};


/**
 * FGenericPublisherListenerHelper
 * Helper class to fast develop of the Generic Vehicle Components with UGenericPublisher or UGenericListener
 */
template <class TVehicleComponent, class TGenericObject>
class UNREALSODA_API FGenericPublisherListenerHelper
{
public:
	FGenericPublisherListenerHelper(TVehicleComponent* VehicleComponent, TSubclassOf<TGenericObject> TVehicleComponent::* GenericClass, TObjectPtr<TGenericObject> TVehicleComponent::* GenericObject, const FString & ClassPropertyName, const FString& ObjectPropertyName, const FString & RecordName)
		: VehicleComponent(VehicleComponent)
		, GenericClass(VehicleComponent->*GenericClass)
		, GenericObject(VehicleComponent->*GenericObject)
		, ClassPropertyName(ClassPropertyName)
		, PublisherPropertyName(ObjectPropertyName)
		, RecordName(RecordName)
	{
	}

	void OnPropertyChanged(FPropertyChangedChainEvent& PropertyChangedEvent)
	{

		FProperty* Property = PropertyChangedEvent.Property;
		FProperty* MemberProperty = nullptr;
		if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode())
		{
			MemberProperty = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue();
		}

		if (MemberProperty != nullptr && Property != nullptr)
		{
			const FName MemberPropertyName = MemberProperty->GetFName();
			const FName PropertyName = Property->GetFName();
			if (MemberPropertyName == ClassPropertyName)
			{
				RefreshClass();
			}
			else if (MemberPropertyName == PublisherPropertyName)
			{
				if (!GenericObject && GenericClass)
				{
					RefreshClass();
				}
			}
		}
	}

	void OnSerialize(FArchive& Ar)
	{
		if (Ar.IsSaveGame())
		{
			if (Ar.IsLoading())
			{
				RefreshClass();
			}

			if (IsValid(GenericObject))
			{
				GenericObject->Serialize(FStructuredArchiveFromArchive(Ar).GetSlot().EnterRecord().EnterRecord(*RecordName));
			}
		}
	}

	void RefreshClass()
	{
		if (GenericClass)
		{
			if (GenericObject)
			{
				GenericObject->MarkAsGarbage();
			}
			if (TGenericObject* NewPublisher = NewObject< TGenericObject>(VehicleComponent.Get(), GenericClass.Get()))
			{
				NewPublisher->SetFlags(RF_Transactional | RF_ArchetypeObject | RF_Public);
				GenericObject = NewPublisher;
			}
			else
			{
				GenericObject = nullptr;
			}
		}
		else
		{
			if (GenericObject)
			{
				GenericObject->MarkAsGarbage();
			}
			GenericObject = nullptr;
		}
	}

	bool Advertise()
	{
		if (GenericObject)
		{
			if (GenericObject->Advertise(VehicleComponent.Get()))
			{
				return true;

			}
			else
			{
				VehicleComponent->SetHealth(EVehicleComponentHealth::Warning, "Can't Initialize publisher");
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	void Shutdown()
	{
		if (GenericObject)
		{
			GenericObject->Shutdown();
		}
	}


	bool IsPublisherInitializing() const
	{
		if (GenericObject)
		{
			return GenericObject->IsInitializing();
		}
		return false;
	}

protected:
	TWeakObjectPtr<TVehicleComponent> VehicleComponent;
	TSubclassOf<TGenericObject> & GenericClass;
	TObjectPtr<TGenericObject> & GenericObject;
	FString ClassPropertyName;
	FString PublisherPropertyName;
	FString RecordName;
};



/**
 * FGenericPublisherHelper
 */
template <class TVehicleComponent, class TGenericObject>
class FGenericPublisherHelper: public FGenericPublisherListenerHelper<TVehicleComponent, TGenericObject>
{
public:
	FGenericPublisherHelper(TVehicleComponent* VehicleComponent, TSubclassOf<TGenericObject> TVehicleComponent::* GenericClass, TObjectPtr<TGenericObject> TVehicleComponent::* GenericObject, const FString& ClassPropertyName = "PublisherClass", const FString& ObjectPropertyName = "Publisher", const FString& RecordName = "PublisherRecord")
		:FGenericPublisherListenerHelper<TVehicleComponent, TGenericObject>(VehicleComponent, GenericClass, GenericObject, ClassPropertyName, ObjectPropertyName, RecordName)
	{
	}

	void Tick()
	{
		if (this->GenericObject)
		{
			if (this->VehicleComponent->HealthIsWorkable() && this->VehicleComponent->GetHealth() != EVehicleComponentHealth::Warning)
			{
				if (!this->GenericObject->IsOk() && !this->GenericObject->IsInitializing())
				{
					this->VehicleComponent->SetHealth(EVehicleComponentHealth::Warning, "Can't Initialize publisher");
				}
			}
		}
	}

	bool Advertise()
	{
		if (this->GenericObject)
		{
			if (this->GenericObject->Advertise(this->VehicleComponent.Get()))
			{
				return true;

			}
			else
			{
				this->VehicleComponent->SetHealth(EVehicleComponentHealth::Warning, "Can't Initialize publisher");
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	void Shutdown()
	{
		if (this->GenericObject)
		{
			this->GenericObject->Shutdown();
		}
	}

	bool IsPublisherInitializing() const
	{
		if (this->GenericObject)
		{
			return this->GenericObject->IsInitializing();
		}
		return false;
	}
};



/**
 * FGenericListenerHelper
 */
template <class TVehicleComponent, class TGenericObject>
class FGenericListenerHelper : public FGenericPublisherListenerHelper<TVehicleComponent, TGenericObject>
{
public:
	FGenericListenerHelper(TVehicleComponent* VehicleComponent, TSubclassOf<TGenericObject> TVehicleComponent::* GenericClass, TObjectPtr<TGenericObject> TVehicleComponent::* GenericObject, const FString& ClassPropertyName = "ListenerClass", const FString& ObjectPropertyName = "Listener", const FString& RecordName = "ListenerRecord")
		:FGenericPublisherListenerHelper<TVehicleComponent, TGenericObject>(VehicleComponent, GenericClass, GenericObject, ClassPropertyName, ObjectPropertyName, RecordName)
	{
	}

	bool StartListen()
	{
		if (this->GenericObject)
		{
			if (this->GenericObject->StartListen(this->VehicleComponent.Get()))
			{
				return true;

			}
			else
			{
				this->VehicleComponent->SetHealth(EVehicleComponentHealth::Warning, "Can't Initialize listener");
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	void StopListen()
	{
		if (this->GenericObject)
		{
			this->GenericObject->StopListen();
		}
	}
};

