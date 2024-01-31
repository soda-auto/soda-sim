// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaSpectator.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"

ASodalSpectator::ASodalSpectator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = CameraComponent;

	for (int i = 0; i < 8; ++i)
	{
		SpeedProfile.Add(50 * std::pow(3, i));
	}

	CurrentSpeedProfile = 2;
	Speed = SpeedProfile[CurrentSpeedProfile];
	Mode = ESodaSpectatorMode::Perspective;
}

void ASodalSpectator::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		if (Controller)
		{
			FRotator const ControlSpaceRot = Controller->GetControlRotation();

			// transform to world space and add it
			AddMovementInput(FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::Y), Val);
		}
	}
}

void ASodalSpectator::MoveForward(float Val)
{
	if (Val != 0.f)
	{
		if (Controller)
		{
			FRotator const ControlSpaceRot = Controller->GetControlRotation();

			// transform to world space and add it
			AddMovementInput(FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::X), Val);
		}
	}
}

void ASodalSpectator::MoveUp_World(float Val)
{
	if (Val != 0.f)
	{
		AddMovementInput(FVector::UpVector, Val);
	}
}

void ASodalSpectator::BeginPlay()
{
	Super::BeginPlay();
}

void ASodalSpectator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ASodalSpectator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport();

	if (!PlayerController || !GameViewportClient)
	{
		return;
	}

	float ForwardInput =
		(PlayerController->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) +
		(PlayerController->IsInputKeyDown(EKeys::S) ? -1.f : 0.f);

	float RightInput =
		(PlayerController->IsInputKeyDown(EKeys::A) ? -1.f : 0.f) +
		(PlayerController->IsInputKeyDown(EKeys::D) ? 1.f : 0.f);

	if (Mode == ESodaSpectatorMode::Perspective || Mode == ESodaSpectatorMode::OrthographicFree)
	{
		if (bAllowMove)
		{
			MoveForward(ForwardInput * Speed * DeltaTime);
			MoveRight(RightInput * Speed * DeltaTime);
		}
		float X = 0, Y = 0;
		PlayerController->GetInputMouseDelta(X, Y);
		float Yaw = X * DeltaTime * MouseSpeed;
		float Pitch = -Y * DeltaTime * MouseSpeed;

		if (PlayerController->IsInputKeyDown(EKeys::Up))
		{
			Pitch -= DeltaTime * KeySpeed;
		}

		if (PlayerController->IsInputKeyDown(EKeys::Down))
		{
			Pitch += DeltaTime * KeySpeed;
		}

		if (PlayerController->IsInputKeyDown(EKeys::Left))
		{
			Yaw -= DeltaTime * KeySpeed;
		}

		if (PlayerController->IsInputKeyDown(EKeys::Right))
		{
			Yaw += DeltaTime * KeySpeed;
		}

		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
		SetActorRotation(GetControlRotation());
	}
	else
	{
		if (bAllowMove)
		{
			FVector Dir = FVector::ZeroVector;
			switch (Mode)
			{
			case ESodaSpectatorMode::Top:    Dir = FVector(RightInput,  -ForwardInput , 0); break;
			case ESodaSpectatorMode::Bottom: Dir = FVector(-RightInput,  -ForwardInput, 0); break;
			case ESodaSpectatorMode::Left:   Dir = FVector(RightInput, 0, ForwardInput); break;
			case ESodaSpectatorMode::Right:  Dir = FVector(-RightInput, 0, ForwardInput); break;
			case ESodaSpectatorMode::Front:  Dir = FVector(0, -RightInput, ForwardInput); break;
			case ESodaSpectatorMode::Back:   Dir = FVector(0, RightInput, ForwardInput); break;
			}

			static const float OrthoWidthSpeed = 0.01;
			AddActorWorldOffset(Dir * CameraComponent->OrthoWidth * OrthoWidthSpeed);
		}
		
		float X = 0, Y = 0;
		PlayerController->GetInputMouseDelta(X, Y);
		static const float OrthoWidthSpeed = 1.0;
		CameraComponent->SetOrthoWidth(Y * OrthoWidthSpeed * DeltaTime * CameraComponent->OrthoWidth + CameraComponent->OrthoWidth);
	}

	FVector Delta = ConsumeMovementInputVector();
	if (!Delta.IsNearlyZero())
	{
		AddActorWorldOffset(Delta);
	}

}

void ASodalSpectator::SetSpeedProfile(int Ind)
{
	if (SpeedProfile.Num() == 0) return;
	if (Ind < 0) Ind = 0;
	else if (Ind >= SpeedProfile.Num()) Ind = SpeedProfile.Num() - 1;
	Speed = SpeedProfile[Ind];
	CurrentSpeedProfile = Ind;
}

void ASodalSpectator::SetMode(ESodaSpectatorMode InMode)
{
	Mode = InMode;

	if (Mode == ESodaSpectatorMode::Perspective)
	{
		CameraComponent->SetProjectionMode(ECameraProjectionMode::Perspective);
	}
	else
	{
		CameraComponent->SetProjectionMode(ECameraProjectionMode::Orthographic);
		CameraComponent->SetOrthoWidth(512);
	}

	switch (Mode)
	{
	//case ESodaSpectatorMode::Perspective:
	//	break;

	//case ESodaSpectatorMode::OrthographicFree:
	//	break;

	case ESodaSpectatorMode::Top:
		SetActorRotation(FRotator(-90, -90,0)); 
		break;

	case ESodaSpectatorMode::Bottom:
		SetActorRotation(FRotator(+90, +90, 0));
		break;

	case ESodaSpectatorMode::Left:
		SetActorRotation(FRotator(0, -90, 0));
		break;

	case ESodaSpectatorMode::Right:
		SetActorRotation(FRotator(0, +90, 0));
		break;

	case ESodaSpectatorMode::Front:
		SetActorRotation(FRotator(0, 180, 0));
		break;

	case ESodaSpectatorMode::Back:
		SetActorRotation(FRotator(0, 0, 0));
		break;
	}
}