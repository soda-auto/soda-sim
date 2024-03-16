// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/UnrealSoda.h"
#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Soda/IToolActor.h"
#include "TileScanner.generated.h"


UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ATileScanner : 
	public AActor,
	public IToolActor
{
GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, Category = TileScanner)
	class USceneCaptureComponent2D* CaptureComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TileScanner)
	float OrthoWidth = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TileScanner)
	float ZeroLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TileScanner)
	int TileSize = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TileScanner)
	int LeftCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TileScanner)
	int RightCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TileScanner)
	int UpCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TileScanner)
	int DownCount = 5;

public:
	ATileScanner();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Serialize(FArchive& Ar) override { Super::Serialize(Ar); ToolActorSerialize(Ar); }

	UFUNCTION(BlueprintCallable, Category = Screenshot)
	void Scan();

	UFUNCTION(BlueprintCallable, Category = Screenshot)
	void MakeScreenshot(const FString& InFileName);
};