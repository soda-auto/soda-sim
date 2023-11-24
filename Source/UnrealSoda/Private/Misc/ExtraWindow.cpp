// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/ExtraWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateImageBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SWindow.h"

UExtraWindow::UExtraWindow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UExtraWindow::~UExtraWindow()
{
	if (ExtraWindow.IsValid())
		ExtraWindow->DestroyWindowImmediately();
}

void UExtraWindow::Close()
{
	if (ExtraWindow.IsValid())
		ExtraWindow->RequestDestroyWindow();
}

void UExtraWindow::OnWindowClosed(const TSharedRef<SWindow>& Window)
{
	ExtraWindow.Reset();
}

void UExtraWindow::OpenCameraWindow(UTextureRenderTarget2D* NewTextureTarget)
{
	if (ExtraWindow.IsValid()) // is window opened
		return;

	if (NewTextureTarget == nullptr)
		return;

	float SizeX = NewTextureTarget->SizeX;
	float SizeY = NewTextureTarget->SizeY;
	const FSlateBrush* Brush = new FSlateImageBrush(NewTextureTarget, FVector2D(SizeX, SizeY));

	float Coef = FMath::Max(SizeX, SizeY) / 1024.f;
	if (Coef > 1.f) // if max image side exceeds 1024 pixels
	{
		SizeX /= Coef;
		SizeY /= Coef;
	}
	TSharedRef<SWindow> WindowRef = SNew(SWindow).Visibility(EVisibility::Visible)
		.ClientSize(FVector2D(SizeX, SizeY))
		.SizingRule(ESizingRule::UserSized)
		[
			SNew(SImage).Image(Brush)
		];
	ExtraWindow = WindowRef;
	FSlateApplication::Get().AddWindow(WindowRef, true);
	ExtraWindow->GetOnWindowClosedEvent().AddUObject(this, &UExtraWindow::OnWindowClosed);
}

void UExtraWindow::OpenWidgetWindow(TSharedRef<SWidget> NewWidget)
{
	if (ExtraWindow.IsValid()) // is window opened
		return;
	TSharedRef<SWindow> WindowRef = SNew(SWindow).Visibility(EVisibility::Visible)
		.SizingRule(ESizingRule::Autosized)
		[
			NewWidget
		];
	ExtraWindow = WindowRef;
	FSlateApplication::Get().AddWindow(WindowRef, true);
	ExtraWindow->GetOnWindowClosedEvent().AddUObject(this, &UExtraWindow::OnWindowClosed);
}