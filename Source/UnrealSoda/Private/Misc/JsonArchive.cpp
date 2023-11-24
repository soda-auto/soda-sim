// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/JsonArchive.h"
#include "Soda/UnrealSodaVersion.h"
#include "Soda/UnrealSoda.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

FJsonArchive::FJsonArchive()
{
	RootJsonObject = MakeShareable(new FJsonObject());
}

bool FJsonArchive::LoadFromString(const FString& JsonString)
{
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(*JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootJsonObject))
	{
		UE_LOG(LogSoda, Warning, TEXT("FJsonArchive::LoadFromString(); Can't deserialize"));
		return false;
	}

	return PostLoad();
}

bool FJsonArchive::SaveToString(FString& JsonString)
{
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogSoda, Error, TEXT("FJsonArchive::SaveToString(); Can't serialize"));
		return false;
	}
	return true;
}

bool FJsonArchive::LoadFromFile(const FString& FileName)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FileName))
	{
		UE_LOG(LogSoda, Warning, TEXT("FJsonArchive::LoadFromFile(); Can't read '%s' file"), *FileName);
		return false;
	}

	return LoadFromString(JsonString);
}

bool FJsonArchive::SaveToFile(const FString& FileName)
{
	FString JsonString;
	if (!SaveToString(JsonString))
	{
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *FileName))
	{
		UE_LOG(LogSoda, Error, TEXT("FJsonArchive::SaveToFile(); Can't write to '%s' file"), *FileName);
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FJsonArchive::UObjectToJsonObject(const UObject* InObject)
{
	if (!IsValid(InObject))
	{
		return TSharedPtr<FJsonObject>();
	}

	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	if (FJsonObjectConverter::UStructToJsonObject(InObject->GetClass(), InObject, JsonObject, CPF_SaveGame, 0))
	{
		JsonObject->SetField(TEXT("__class__"), ClassToJsonValue(InObject->GetClass()));
		JsonObject->SetStringField(TEXT("__name__"), InObject->GetName());

		const USceneComponent* SceneComponent = nullptr;
		if (const AActor* Actor = Cast<AActor>(InObject))
		{
			SceneComponent = Actor->GetRootComponent();
		}
		else
		{
			SceneComponent = Cast<USceneComponent>(InObject);
		}
		if (SceneComponent)
		{
			JsonObject->SetField(TEXT("__location__"), VectorToJsonValue(SceneComponent->GetRelativeLocation()));
			JsonObject->SetField(TEXT("__rotation__"), RotatorToJsonValue(SceneComponent->GetRelativeRotation()));
			JsonObject->SetField(TEXT("__scale3D__"), VectorToJsonValue(SceneComponent->GetRelativeScale3D()));
		}

		return JsonObject;
	}
	return TSharedPtr<FJsonObject>(); // something went wrong
}


bool FJsonArchive::JsonObjectToUObject(const TSharedRef<FJsonObject>& JsonObject, UObject* OutObject, bool bTransform)
{
	if (!IsValid(OutObject))
	{
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject, OutObject->GetClass(), OutObject, CPF_SaveGame, 0))
	{
		return false;
	}

	if (bTransform)
	{
		USceneComponent* SceneComponent = nullptr;
		if (AActor* Actor = Cast<AActor>(OutObject))
		{
			SceneComponent = Actor->GetRootComponent();
		}
		else
		{
			SceneComponent = Cast<USceneComponent>(OutObject);
			if (SceneComponent && SceneComponent->GetOwner() && SceneComponent->GetOwner()->GetRootComponent() == SceneComponent)
			{
				SceneComponent = nullptr;
			}
		}
		if (SceneComponent)
		{
			TSharedPtr<FJsonValue> LocationValue = JsonObject->TryGetField(TEXT("__location__"));
			TSharedPtr<FJsonValue> RotationValue = JsonObject->TryGetField(TEXT("__rotation__"));
			TSharedPtr<FJsonValue> ScaleValue = JsonObject->TryGetField(TEXT("__scale3D__"));

			if (LocationValue && RotationValue && ScaleValue)
			{
				FVector Location = JsonValueToVector(LocationValue);
				FRotator Rotator = JsonValueToRotator(RotationValue);
				FVector Scale = JsonValueToVector(ScaleValue);

				SceneComponent->SetRelativeLocation(Location, false, nullptr, ETeleportType::ResetPhysics);
				SceneComponent->SetRelativeRotation(Rotator, false, nullptr, ETeleportType::ResetPhysics);
				SceneComponent->SetRelativeScale3D(Scale);
			}
		}
	}

	return true;
}

UClass* FJsonArchive::GetObjectClass(const TSharedPtr<FJsonObject>& Object)
{
	ClassHelper = nullptr;
	TSharedPtr<FJsonValue> ClassValue = Object->TryGetField(TEXT("__class__"));
	if (!ClassValue)
	{
		return nullptr;
	}

	return JsonValueToClass(ClassValue);
}

FString FJsonArchive::GetObjectName(const TSharedPtr<FJsonObject>& Object)
{
	FString Name;
	if (Object->TryGetStringField(TEXT("__name__"), Name))
	{
		return Name;
	}

	return FString();
}

TSharedRef<FJsonValue> FJsonArchive::ClassToJsonValue(const UClass* Class)
{
	ClassHelper = const_cast<UClass*>(Class);
	FProperty* ClassProperty = StaticStruct()->FindPropertyByName(TEXT("ClassHelper"));
	check(ClassProperty);
	const void* Value = ClassProperty->ContainerPtrToValuePtr<uint8>(this);
	return FJsonObjectConverter::UPropertyToJsonValue(ClassProperty, Value, 0, 0).ToSharedRef();
}

TSharedRef<FJsonValue> FJsonArchive::VectorToJsonValue(const FVector& Vector)
{
	VectorHelper = Vector;
	FProperty* ClassProperty = StaticStruct()->FindPropertyByName(TEXT("VectorHelper"));
	check(ClassProperty);
	const void* Value = ClassProperty->ContainerPtrToValuePtr<uint8>(this);
	return FJsonObjectConverter::UPropertyToJsonValue(ClassProperty, Value, 0, 0).ToSharedRef();
}

TSharedRef<FJsonValue> FJsonArchive::RotatorToJsonValue(const FRotator& Rotator)
{
	RotatorHelper = Rotator;
	FProperty* ClassProperty = StaticStruct()->FindPropertyByName(TEXT("RotatorHelper"));
	check(ClassProperty);
	const void* Value = ClassProperty->ContainerPtrToValuePtr<uint8>(this);
	return FJsonObjectConverter::UPropertyToJsonValue(ClassProperty, Value, 0, 0).ToSharedRef();
}

UClass* FJsonArchive::JsonValueToClass(const TSharedPtr<FJsonValue>& ClassJsonValue)
{
	FProperty* Property = StaticStruct()->FindPropertyByName(TEXT("ClassHelper"));
	check(Property);
	void* Value = Property->ContainerPtrToValuePtr<uint8>(this);
	if (!FJsonObjectConverter::JsonValueToUProperty(ClassJsonValue, Property, Value, 0, 0))
	{
		return nullptr;
	}
	return ClassHelper;
}

FVector FJsonArchive::JsonValueToVector(const TSharedPtr<FJsonValue>& VectorJsonValue)
{
	FProperty* Property = StaticStruct()->FindPropertyByName(TEXT("VectorHelper"));
	check(Property);
	void* Value = Property->ContainerPtrToValuePtr<uint8>(this);
	if (!FJsonObjectConverter::JsonValueToUProperty(VectorJsonValue, Property, Value, 0, 0))
	{
		return FVector::ZeroVector;
	}
	return VectorHelper;
}

FRotator FJsonArchive::JsonValueToRotator(const TSharedPtr<FJsonValue>& RotatorJsonValue)
{
	FProperty* Property = StaticStruct()->FindPropertyByName(TEXT("RotatorHelper"));
	check(Property);
	void* Value = Property->ContainerPtrToValuePtr<uint8>(this);
	if (!FJsonObjectConverter::JsonValueToUProperty(RotatorJsonValue, Property, Value, 0, 0))
	{
		return FRotator::ZeroRotator;
	}
	return RotatorHelper;
}

bool FJsonArchive::DeserializeObject(UObject* Object)
{
	return JsonObjectToUObject(RootJsonObject.ToSharedRef(), Object, false);
}

bool FJsonArchive::SerializeObject(const UObject* Object)
{
	RootJsonObject = UObjectToJsonObject(Object);
	return !!RootJsonObject;
}

FJsonActorArchive::FJsonActorArchive()
{
	Components = MakeShareable(new FJsonObject());
	RootJsonObject->SetObjectField(TEXT("__components__"), Components);
}

bool FJsonActorArchive::PostLoad()
{
	const TSharedPtr<FJsonObject>* ComponentsJsonObject;
	if (!RootJsonObject->TryGetObjectField(TEXT("__components__"), ComponentsJsonObject))
	{
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::ImportVehicleFromJson(); Vehicle dosen't contain __components__"));
		return false;
	}
	Components = *ComponentsJsonObject;
	return true;
}

bool FJsonActorArchive::DeserializeActor(AActor* OutActor, bool bWithComponents)
{
	if (!JsonObjectToUObject(RootJsonObject.ToSharedRef(), OutActor, false))
	{
		return false;
	}

	if (bWithComponents)
	{
		const TSet<UActorComponent*>& ActorComponents = OutActor->GetComponents();
		for (auto* Component : ActorComponents)
		{
			DeserializeComponent(Component);
		}
	}

	return true;
}

bool FJsonActorArchive::SerializeActor(const AActor* OutActor, bool bWithComponents)
{
	RootJsonObject = UObjectToJsonObject(OutActor);
	if (!RootJsonObject)
	{
		return false;
	}

	RootJsonObject->SetStringField(TEXT("__unreal_soda_ver__"), UNREALSODA_VERSION_STRING);
	RootJsonObject->SetObjectField(TEXT("__components__"), Components);

	if (bWithComponents)
	{
		const TSet<UActorComponent*>& ActorComponents = OutActor->GetComponents();
		for (auto* Component : ActorComponents)
		{
			SerializeActorComponent(Component);
		}
	}

	return (RootJsonObject.Get() != nullptr);
}

bool FJsonActorArchive::DeserializeComponent(UActorComponent* ActorComponent)
{
	if (TSharedPtr<FJsonValue>* JsonValue = Components->Values.Find(ActorComponent->GetName()))
	{
		TSharedPtr<FJsonObject> JsonObject = (*JsonValue)->AsObject();
		if (JsonObject)
		{
			return JsonObjectToUObject(JsonObject.ToSharedRef(), ActorComponent, true);
		}
	}
	return false;
}

bool FJsonActorArchive::SerializeActorComponent(const UActorComponent* ActorComponent)
{
	TSharedPtr<FJsonObject> JsonObject = UObjectToJsonObject(ActorComponent);
	if (JsonObject)
	{
		Components->SetObjectField(ActorComponent->GetName(), JsonObject);
		return true;
	}
	return false;
}