// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "Widgets/SPanel.h"
#include "ExtraWindow.generated.h"

/** Struct for opening extra window. */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable)
class UNREALSODA_API UExtraWindow : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	~UExtraWindow();

	TSharedPtr<SWindow> ExtraWindow;

	// Close window
	void Close();

	// Calls on window closed event
	void OnWindowClosed(const TSharedRef<SWindow>& Window);

	// Open camera window
	void OpenCameraWindow(class UTextureRenderTarget2D* NewTextureTarget);

	// Open widget window
	void OpenWidgetWindow(class TSharedRef <SWidget> NewWidget);
};
