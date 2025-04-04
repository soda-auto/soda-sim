// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UObject/Class.h"
#include "Soda/SodaApp.h"
#include "JsonObjectWrapper.h"
#include "MongoDBBluprintTools.generated.h"

/**
 * Helper class to allow create dataset record for any actor from blueprint. It is slow implementation, not recomended use it.
 * TODO: fast implementation
 */
UCLASS(BlueprintType)
class UNREALSODA_API UMongoDBBluprintTools : public UObject
{
	GENERATED_BODY()

protected:

	UFUNCTION(BlueprintCallable, Category = "Dataset Recorder")
	bool BeginRecord(const FString& ActorDatasetName, const FString& DatasetType, const FJsonObjectWrapper & Descriptor);

	UFUNCTION(BlueprintCallable, Category = "Dataset Recorder")
	void EndRecord();

	UFUNCTION(BlueprintCallable, Category = "Dataset Recorder")
	bool TickDataset(const FJsonObjectWrapper& DatasetTickData);

	UFUNCTION(BlueprintCallable, Category = "Dataset Recorder")
	bool IsRecording() const { return false; /*Dataset.IsValid();*/ }
};