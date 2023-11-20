// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/Vehicles/SodaVehicle.h"
#include "GameFramework/Pawn.h"
#include "Soda/SodaTypes.h"
#include "SodaSpectator.generated.h"

class UCameraComponent;

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ASodalSpectator : public APawn
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Category=SodaSpectator, VisibleDefaultsOnly, BlueprintReadOnly)
	UCameraComponent* CameraComponent;

	/** cm/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SodaSpectator)
	TArray<float> SpeedProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SodaSpectator)
	float MouseSpeed = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SodaSpectator)
	float KeySpeed = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SodaSpectator)
	bool bAllowMove = true;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	virtual void MoveRight(float Val);
	virtual void MoveForward(float Val);
	virtual void MoveUp_World(float Val);

public:
	UFUNCTION(BlueprintCallable, Category=SodaSpectator)
	virtual void SetSpeedProfile(int Ind);

	UFUNCTION(BlueprintCallable, Category=SodaSpectator)
	virtual int GetSpeedProfile() const { return CurrentSpeedProfile; }

	UFUNCTION(BlueprintCallable, Category=SodaSpectator)
	virtual void SetMode(ESodaSpectatorMode Mode);
	
	UFUNCTION(BlueprintCallable, Category=SodaSpectator)
	virtual ESodaSpectatorMode GetMode() const { return Mode; }

protected:
	float Speed;
	int CurrentSpeedProfile;
	ESodaSpectatorMode Mode;
};
