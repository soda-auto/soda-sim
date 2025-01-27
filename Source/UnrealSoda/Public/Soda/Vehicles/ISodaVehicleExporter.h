// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once
#include "Containers/UnrealString.h"

class ASodaVehicle;

class ISodaVehicleExporter
{
public:
	virtual bool ExportToString(const ASodaVehicle* Vehicle, FString& String) = 0;
	virtual const FName& GetExporterName() const = 0;
	virtual const FString& GetFileTypes() const = 0;
};
