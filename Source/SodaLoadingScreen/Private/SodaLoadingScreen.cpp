
#include "SodaLoadingScreen.h"
#include "MoviePlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/Material.h"
#include "Modules/ModuleManager.h"
#include "PixelShaderUtils.h"
#include "RHIStaticStates.h"
#include "ScreenRendering.h"
#include "SlateMaterialBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "FSodaLoadingScreenModule"

class SSodaLoadingScreen : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SSodaLoadingScreen)
	{}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs)
	{
		if (UTexture2D* Tex = Cast<UTexture2D>(SodaLoadingMaterialPath.TryLoad()))
		{
			SodaLoadingBrush = MakeShareable(new FSlateImageBrush(Tex, FVector2D(1024 * 0.3, 512 * 0.3)));
			check(SodaLoadingBrush.IsValid());
		}

		if (UTexture2D* BackgroundTex = Cast<UTexture2D>(SodaBackgroundMaterialPath.TryLoad()))
		{
			SodaBackgroundBrush = MakeShareable(new FSlateImageBrush(BackgroundTex, FVector2D(3840 * 0.4, 2160 * 0.4)));
			check(SodaBackgroundBrush.IsValid());
		}

		ChildSlot
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBox)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(SodaBackgroundBrush.Get())
									.RenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(RotationAngle))))
							]
					]
					+ SOverlay::Slot()
					[
						SNew(SBox)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(SodaLoadingBrush.Get())
							]
					]
					+ SOverlay::Slot()
					[
						SAssignNew(BackgroundBlur, SBackgroundBlur)
							.BlurStrength(4)
							.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
					]
					+ SOverlay::Slot()
					[
						SNew(SBox)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(SodaLoadingBrush.Get())
							]
					]
			];
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		if (BackgroundBlur)
		{
			const float Period = 3.0f;
			const float Amplitude = 15.0f;

			BackgroundBlur->SetBlurStrength(FMath::Cos(InCurrentTime / (2 * PI) * Period) * Amplitude);
		}

		const float RotationSpeed = 15.0f;
		RotationAngle += RotationSpeed * InDeltaTime;

		if (RotationAngle > 360.0f)
		{
			RotationAngle -= 360.0f;
		}
	}

	SSodaLoadingScreen()
		: SodaLoadingMaterialPath(TEXT("/SodaSim/Assets/CPP/ScreenLoading/Logo")),
		SodaBackgroundMaterialPath(TEXT("/SodaSim/Assets/CPP/ScreenLoading/T_LaunchScreenLoad")),
		RotationAngle(0.0f)
	{}

	virtual ~SSodaLoadingScreen()
	{
		if (SodaLoadingBrush.IsValid())
		{
			SodaLoadingBrush->SetResourceObject(nullptr);
		}

		if (SodaBackgroundBrush.IsValid())
		{
			SodaBackgroundBrush->SetResourceObject(nullptr);
		}
	}

private:
	FSoftObjectPath SodaLoadingMaterialPath;
	FSoftObjectPath SodaBackgroundMaterialPath;
	TSharedPtr<FSlateBrush> SodaLoadingBrush;
	TSharedPtr<SBackgroundBlur> BackgroundBlur;
	TSharedPtr<FSlateBrush> SodaBackgroundBrush;
	float RotationAngle;
};

void FSodaLoadingScreenModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	if (!IsRunningDedicatedServer() && FSlateApplication::IsInitialized())
	{

		if (IsMoviePlayerEnabled())
		{
			GetMoviePlayer()->OnPrepareLoadingScreen().AddRaw(this, &FSodaLoadingScreenModule::PreSetupLoadingScreen);
		}

		SetupLoadingScreen();
	}	

	FCommandLine::Append(TEXT(" -RCWebControlEnable"));
}

void FSodaLoadingScreenModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (!IsRunningDedicatedServer())
	{
		// TODO: Unregister later
		GetMoviePlayer()->OnPrepareLoadingScreen().RemoveAll(this);
	}
}

bool FSodaLoadingScreenModule::IsGameModule() const
{
	return true;
}

void FSodaLoadingScreenModule::PreSetupLoadingScreen()
{
	SetupLoadingScreen();
}

void FSodaLoadingScreenModule::SetupLoadingScreen()
{
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.MinimumLoadingScreenDisplayTime = -1;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.bMoviesAreSkippable = true;
	LoadingScreen.bWaitForManualStop = false;
	LoadingScreen.bAllowInEarlyStartup = true;
	LoadingScreen.bAllowEngineTick = true;
	LoadingScreen.PlaybackType = EMoviePlaybackType::MT_Normal;
	LoadingScreen.WidgetLoadingScreen = SNew(SSodaLoadingScreen);

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSodaLoadingScreenModule, SodaLoadingScreen)