// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaHUD.h"
#include "Soda/UnrealSodaVersion.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/Editor/SodaSelection.h"

ASodaHUD::ASodaHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASodaHUD::DrawHUD()
{
	Super::DrawHUD();
	
	if (bDrawVehicleDebugPanel)
	{
		ASodaVehicle* Vehicle = Cast< ASodaVehicle >(GetOwningPawn());
		if (!Vehicle)
		{
			if (USodaGameViewportClient* ViewportClient = Cast<USodaGameViewportClient>(GetWorld()->GetGameViewport()))
			{
				if (ViewportClient->GetGameMode() == soda::EUIMode::Editing)
				{
					Vehicle = Cast< ASodaVehicle >(ViewportClient->Selection->GetSelectedActor());
				}
			}
		}

		if (Vehicle)
		{
			float YL = 10;
			float YPos = 10;
			Vehicle->DrawDebug(Canvas, YL, YPos);
		}
	}

	UFont* RenderFont = GEngine->GetSmallFont();
	Canvas->SetDrawColor(FColor::White);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ver: %s"), UNREALSODA_VERSION_STRING), Canvas->SizeX - 100, Canvas->SizeY - 20);
}

void ASodaHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ASodaHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}