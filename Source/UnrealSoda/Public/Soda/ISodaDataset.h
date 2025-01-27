// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/SodaTypes.h"
#include "Templates/SharedPointer.h"
#include "ISodaDataset.generated.h"

namespace soda
{

enum class EDatasetManagerStatus: uint8
{
	Standby,
	Fluishing,
	Recording,
	Faild
};

class IDatasetManager
{
public:
	virtual bool BeginRecording() = 0;
	/** @param bImmediately  don't wait until all datasets are written to the DB */
	virtual void EndRecording(EScenarioStopReason Reasone, bool bImmediately) = 0;
	//virtual TSharedRef<IObjectDatasetHandler> CreateHandlerForObject(IObjectDataset& Object) = 0;
	//virtual TSharedRef<SWidget> GetWidget() = 0;
	virtual EDatasetManagerStatus GetStatus() const = 0;
	virtual FText GetToolTip() const = 0;
	virtual FText GetDisplayName() const = 0;
	virtual FName GetIconName() const = 0;
	//virtual int GetQueueSize() const = 0;
	virtual void ClearDatasetsQueue() = 0;
	virtual void Tick(float DeltaSeconds) {}
};

class IObjectDatasetHandler
{
public:
	virtual void BeginRecording() = 0;
	virtual void EndRecording() = 0;
	virtual bool Sync() = 0;
	//virtual UClass* GetClass();
};

} // namespace soda

UINTERFACE()
class UObjectDataset:  public UInterface
{
	GENERATED_BODY()
};

class IObjectDataset
{
	GENERATED_BODY()

public:

	virtual bool ShouldRecordDataset() const { return true; }
	virtual int GetDatasetSyncTopPriority() const { return 0; }

	virtual void SyncDataset(int Priority = 0) const
	{
		if (GetDatasetSyncTopPriority() >= Priority)
		{
			for (auto& It : DatasetHandlers)
			{
				It->Sync();
			}
		}
	}

	virtual void AddDatasetHandler(TSharedRef<soda::IObjectDatasetHandler> InDatasetHandler)
	{
		DatasetHandlers.Add(InDatasetHandler);
	}

	virtual void ReleaseDatasetHandlers()
	{
		DatasetHandlers.Reset();
	}

private:
	TArray<TSharedPtr<soda::IObjectDatasetHandler>> DatasetHandlers;
};

