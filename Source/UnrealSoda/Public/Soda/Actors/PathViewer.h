// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "DrawDebugHelpers.h"
#include "Soda/ISodaActor.h"
#include <vector>
#include <mutex>
#include "PathViewer.generated.h"

/**
 * APathViewer (experemental)
 * Used to visualize routes obtained from AD/ADS algorithms over the UDP protocol.
 * UDP protocol format: |double(Lon0),double(Lat0)|double(Lon1),double(Lat1)|double(Lon2),double(Lat2)|...|
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API APathViewer 
	: public AActor
	, public ISodaActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = PathViewer, SaveGame, meta = (EditInRuntime, ReactivateActor))
	int Port = 20000;

	UPROPERTY(EditAnywhere, Category = PathViewer, SaveGame)
	FColor Color;

	UPROPERTY(EditAnywhere, Category = PathViewer, SaveGame, meta = (EditInRuntime))
	float Thickness = 10.0f;

	UPROPERTY(EditAnywhere, Category = PathViewer, SaveGame, meta = (EditInRuntime))
	bool bIsLanLong = false;

    /** X = inX * ScaleX + DX*/
	UPROPERTY(EditAnywhere, Category = PathViewer, meta=(EditCondition="!bIsLanLong"), SaveGame, meta = (EditInRuntime))
	float ScaleX = 100.0;

    /** Y = inY * ScaleY + DY*/
	UPROPERTY(EditAnywhere, Category = PathViewer, meta=(EditCondition="!bIsLanLong"), SaveGame, meta = (EditInRuntime))
	float ScaleY = -100.0;

    /** X = inX * ScaleX + DX*/
	UPROPERTY(EditAnywhere, Category = PathViewer, meta=(EditCondition="!bIsLanLong"), SaveGame, meta = (EditInRuntime))
	float DX = 0.0;

    /** Y = inY * ScaleY + DY*/
	UPROPERTY(EditAnywhere, Category = PathViewer, meta=(EditCondition="!bIsLanLong"), SaveGame, meta = (EditInRuntime))
	float DY = 0.0;

public:
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor * GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;

public:
	APathViewer();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

private:
	FSocket* ListenSocket = nullptr;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	std::vector< double > Buf;
	std::vector< double > DrawBuf;
	std::mutex mutex;

	bool bIsPinnedActor = true;

	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);
};
