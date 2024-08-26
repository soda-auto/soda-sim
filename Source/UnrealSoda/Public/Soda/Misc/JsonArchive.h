// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "JsonObjectConverter.h"
#include "Templates/SharedPointer.h"
#include "JsonArchive.generated.h"

class UActorComponent;

/** TODO: Thinking to use FJsonArchiveInputFormatterEx/FJsonArchiveOutputFormatterEx insted of FJsonObjectConverter */
USTRUCT()
struct UNREALSODA_API FJsonArchive
{
	GENERATED_BODY()

	FJsonArchive();
	virtual ~FJsonArchive() {}

	bool LoadFromString(const FString& JsonString);
	bool SaveToString(FString& JsonString);

	bool LoadFromFile(const FString& FileName);
	bool SaveToFile(const FString& FileName);

	virtual bool PostLoad() { return true; }


	/**
	* Helper function for serialization UObject to FJsonObject
	*/
	TSharedPtr<FJsonObject> UObjectToJsonObject(const UObject * InStruct);

	/**
	* Helper function for deserialization FJsonObject to UObject
	* @param bTransform - if OutObject is AActor or USceneComponent then will be set the object location, rotation and scale
	*/
	bool JsonObjectToUObject(const TSharedRef<FJsonObject>& JsonObject, UObject* OutObject, bool bTransform = true);
	
	/**
	* Helper function for detect UClass from JSON object
	*/
	UClass* GetObjectClass(const TSharedPtr<FJsonObject> & Object);

	/**
	* Helper function for get object name from JSON object
	*/
	FString GetObjectName(const TSharedPtr<FJsonObject>& Object);

	bool DeserializeObject(UObject* Object);
	bool SerializeObject(const UObject* Object);

	TSharedPtr<FJsonObject> RootJsonObject;

protected:
	TSharedRef<FJsonValue> ClassToJsonValue(const UClass* Class);
	TSharedRef<FJsonValue> VectorToJsonValue(const FVector & Vector);
	TSharedRef<FJsonValue> RotatorToJsonValue(const FRotator& Rotator);

	UClass* JsonValueToClass(const TSharedPtr<FJsonValue>& Value);
	FVector JsonValueToVector(const TSharedPtr<FJsonValue> & Value);
	FRotator JsonValueToRotator(const TSharedPtr<FJsonValue>& Value);

private:
	UPROPERTY()
	UClass * ClassHelper = nullptr;

	UPROPERTY()
	FVector VectorHelper;

	UPROPERTY()
	FRotator RotatorHelper;
};

USTRUCT()
struct UNREALSODA_API FJsonActorArchive : public FJsonArchive
{
	GENERATED_BODY()

	FJsonActorArchive();
	virtual ~FJsonActorArchive() {}

	UClass* GetActorClass() { return GetObjectClass(RootJsonObject); }
	FString GetActorName() { return GetObjectName(RootJsonObject); }

	bool DeserializeActor(AActor* OutActor, bool bWithComponents);
	bool SerializeActor(const AActor* OutActor, bool bWithComponents);

	bool DeserializeComponent(UActorComponent* ActorComponent);
	bool SerializeActorComponent(const UActorComponent* ActorComponent);

	virtual bool PostLoad() override;

	TSharedPtr<FJsonObject> Components;
};