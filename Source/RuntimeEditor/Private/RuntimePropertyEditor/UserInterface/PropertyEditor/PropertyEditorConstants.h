// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Styling/SlateColor.h"
#include "UObject/NameTypes.h"
#include "Templates/SharedPointer.h"

struct FSlateBrush;

namespace soda
{

class PropertyEditorConstants
{
public:

	static const FName PropertyFontStyle;
	static const FName CategoryFontStyle;

	static const FName MD_Bitmask;
	static const FName MD_BitmaskEnum;
	static const FName MD_UseEnumValuesAsMaskValuesInEditor;

	static constexpr float PropertyRowHeight = 26.0f;

	static const FSlateBrush* GetOverlayBrush(const TSharedRef<class FPropertyEditor> PropertyEditor);

	static FSlateColor GetRowBackgroundColor(int32 IndentLevel, bool IsHovered);
};

} // namespace soda
