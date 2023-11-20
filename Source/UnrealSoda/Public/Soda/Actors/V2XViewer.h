// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Soda/Misc/Time.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "SodaSimProto/V2X.hpp"
#include "Soda/IToolActor.h"
#include <mutex>
#include "V2XViewer.generated.h"

class UInstancedStaticMeshComponent;

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API AV2XViewer : 
	public AActor,
	public IToolActor
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, Category = V2XViewer)
	UInstancedStaticMeshComponent* BoxStaticMeshesObstacle;

	UPROPERTY(EditAnywhere, Category = V2XViewer)
	UInstancedStaticMeshComponent* BoxStaticMeshesCollectable;

	UPROPERTY(EditAnywhere, Category = V2XViewer)
	UInstancedStaticMeshComponent* BoxStaticMeshesUndef;

	UPROPERTY(EditAnywhere, Category = V2XViewer, SaveGame, meta = (EditInRuntime, ReactivateActor))
	int Port = 3003;

	UPROPERTY(EditAnywhere, Category = V2XViewer, SaveGame, meta = (EditInRuntime))
	float Thickness = 10.0f;

	UPROPERTY(EditAnywhere, Category = V2XViewer, SaveGame, meta = (EditInRuntime))
	bool bUseCustomRefPoint = false;

	UPROPERTY(EditAnywhere, Category = V2XViewer, meta=(EditCondition="bUseCustomRefPoint"), SaveGame, meta = (EditInRuntime, ReactivateActor))
	double Latitude = 59.995;

	UPROPERTY(EditAnywhere, Category = V2XViewer, meta=(EditCondition="bUseCustomRefPoint"), SaveGame, meta = (EditInRuntime, ReactivateActor))
	double Longitude = 30.13;

	UPROPERTY(EditAnywhere, Category = V2XViewer, meta=(EditCondition="bUseCustomRefPoint"), SaveGame, meta = (EditInRuntime, ReactivateActor))
	double Altitude = 10.0;

	UPROPERTY(EditAnywhere, Category = V2XViewer, meta=(EditCondition="bUseCustomRefPoint"), SaveGame, meta = (EditInRuntime, ReactivateActor))
	FVector Shift;

	UPROPERTY(EditAnywhere, Category = V2XViewer, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bUseDefaultHeight = true;

	UPROPERTY(EditAnywhere, Category = V2XViewer, meta=(EditCondition="bUseDefaultHeight"), SaveGame, meta = (EditInRuntime, ReactivateActor))
	float DefaultHeight = 100.f;
	
	UPROPERTY(EditInstanceOnly, Category = V2XViewer, SaveGame)
	TArray<AActor*> HiddenActors;

	UPROPERTY(EditInstanceOnly, Category = V2XViewer, SaveGame)
	TArray<AActor*> ShowOnlyActors;

	//UPROPERTY(EditInstanceOnly, Category=V2XViewer, SaveGame)
	//TSubclassOf<AActor> ShowOnlyActorsClass;

	//UPROPERTY(EditInstanceOnly, Category=V2XViewer, SaveGame)
	//TSubclassOf<AActor> HiddenActorsClass;

	UPROPERTY(EditAnywhere, Category = V2XViewer, SaveGame, meta = (EditInRuntime))
	bool bShowLog = false;

public:
	/* Override from ISodaActor */
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;

public:
	AV2XViewer();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void Serialize(FArchive& Ar) override { Super::Serialize(Ar); ToolActorSerialize(Ar); }

private:
	FVector OrignShift;
	struct FV2XObject
	{
		TTimestamp Timestamp;
		bool Traced = false;
		bool TracedValid = false;
		FVector Pos;
		FRotator Rot;
		FVector Extent;
		soda::V2XRenderObject Data;
	};

	FSocket* ListenSocket = nullptr;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	TMap<uint32_t, FV2XObject> V2XObjects;
	std::mutex Mutex;
	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);
	void SetUpBatchedSceneQuery(int NumRays);

	TArray<FVector> BatchStart;
	TArray<FVector> BatchEnd;
};
