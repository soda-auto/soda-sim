// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Styling/SlateColor.h"
#include "IDetailCustomNodeBuilder.h"

class IDetailCategoryBuilder;
class IPropertyHandle;
enum class ECheckBoxState : uint8;

// Helper class to create a Mobility customization for the specified Property in the specified CategoryBuilder.
class DETAILCUSTOMIZATIONS_API FMobilityCustomization : public IDetailCustomNodeBuilder, public TSharedFromThis<FMobilityCustomization>
{
public:
	enum
	{
		StaticMobilityBitMask = (1u << EComponentMobility::Static),
		StationaryMobilityBitMask = (1u << EComponentMobility::Stationary),
		MovableMobilityBitMask = (1u << EComponentMobility::Movable),
	};

	FMobilityCustomization(TSharedPtr<IPropertyHandle> InMobilityHandle, uint8 InRestrictedMobilityBits, bool InForLight);

	virtual void GenerateHeaderRowContent(FDetailWidgetRow& WidgetRow) override;
	virtual FName GetName() const override;
	virtual TSharedPtr<IPropertyHandle> GetPropertyHandle() const override { return MobilityHandle; }

private:

	EComponentMobility::Type GetActiveMobility() const;
	FSlateColor GetMobilityTextColor(EComponentMobility::Type InMobility) const;
	void OnMobilityChanged(EComponentMobility::Type InMobility);
	FText GetMobilityToolTip() const;

	TSharedPtr<IPropertyHandle> MobilityHandle;
	bool bForLight;
	uint8 RestrictedMobilityBits;
};
