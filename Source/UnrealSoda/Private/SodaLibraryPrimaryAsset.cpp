// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaLibraryPrimaryAsset.h"
#include "UObject/Package.h"

FPrimaryAssetId USodaLibraryPrimaryAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FName("SodaLibraryPrimaryAsset"), FPackageName::GetShortFName(GetOutermost()->GetName()));
}
