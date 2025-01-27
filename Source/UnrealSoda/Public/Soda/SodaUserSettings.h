// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SodaUserSettings.generated.h"

class SWidget;

UCLASS(abstract, ClassGroup = Soda, config = SodaUserSettings)
class UNREALSODA_API USodaUserSettings : public UObject
{
	GENERATED_BODY()

public:
	virtual FText GetMenuItemText() const;
	virtual FText GetMenuItemDescription() const;
	virtual FName GetMenuItemIconName() const;

	virtual TSharedPtr<SWidget> GetCustomSettingsWidget() const;

	virtual void OnPreOpen() {}

	virtual void ResetToDefault();
};