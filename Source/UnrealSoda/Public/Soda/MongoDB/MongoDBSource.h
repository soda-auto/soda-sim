// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/FileDatabaseManager.h"

namespace soda
{

	class FMongoDBSource: public IFileDatabaseSource
	{
	public:
		virtual FName GetSourceName() const override;
		virtual TSet<EFileSlotType> SupportedTypes() const override;
		virtual bool PullSlotsInfo(TMap<FGuid, FFileDatabaseSlotInfo>& OutSlots, EFileSlotType Type) const override;
		virtual bool DeleteSlot(const FGuid& Guid) override;
		virtual bool PullSlot(const FGuid& Guid, FFileDatabaseSlotInfo& OutInfo, TArray<uint8>& OutData) override;
		virtual bool PushSlot(const FFileDatabaseSlotInfo& Info, const TArray<uint8>& Data) override;
	};


} // namespace soda