// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/V2XViewer.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Misc/CString.h"
#include "Soda/Misc/SodaPhysicsInterface.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Soda/LevelState.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

#if PLATFORM_LINUX
#include "BSDSockets/SocketsBSD.h"
#	ifndef SO_REUSEPORT 
#		define SO_REUSEPORT	15
#	endif
#endif

AV2XViewer::AV2XViewer()
{
	BoxStaticMeshesObstacle = CreateDefaultSubobject< UInstancedStaticMeshComponent >(TEXT("BoxStaticMeshesObstacle"));
	BoxStaticMeshesCollectable = CreateDefaultSubobject< UInstancedStaticMeshComponent >(TEXT("BoxStaticMeshesCollectable"));
	BoxStaticMeshesUndef = CreateDefaultSubobject< UInstancedStaticMeshComponent >(TEXT("BoxStaticMeshesUndef"));

	static ConstructorHelpers::FObjectFinder< UStaticMesh > CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder< UMaterial > BoxObstacleMatAsset(TEXT("/SodaSim/Assets/CPP/V2X/BoxObstacleMat"));
	static ConstructorHelpers::FObjectFinder< UMaterial > BoxCollectableMatAsset(TEXT("/SodaSim/Assets/CPP/V2X/BoxCollectableMat"));
	static ConstructorHelpers::FObjectFinder< UMaterial > BoxUndefMatAsset(TEXT("/SodaSim/Assets/CPP/V2X/BoxUndefMat"));

	if (CubeMeshAsset.Succeeded())
	{
		BoxStaticMeshesObstacle->SetStaticMesh(CubeMeshAsset.Object);
		BoxStaticMeshesCollectable->SetStaticMesh(CubeMeshAsset.Object);
		BoxStaticMeshesUndef->SetStaticMesh(CubeMeshAsset.Object);
	}
	if (BoxObstacleMatAsset.Succeeded())
	{
		BoxStaticMeshesObstacle->SetMaterial(0, BoxObstacleMatAsset.Object);
	}
	if (BoxCollectableMatAsset.Succeeded())
	{
		BoxStaticMeshesCollectable->SetMaterial(0, BoxCollectableMatAsset.Object);
	}
	if (BoxUndefMatAsset.Succeeded())
	{
		BoxStaticMeshesUndef->SetMaterial(0, BoxUndefMatAsset.Object);
	}

	BoxStaticMeshesObstacle->SetSimulatePhysics(false);
	BoxStaticMeshesObstacle->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	BoxStaticMeshesObstacle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BoxStaticMeshesCollectable->SetSimulatePhysics(false);
	BoxStaticMeshesCollectable->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	BoxStaticMeshesCollectable->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BoxStaticMeshesUndef->SetSimulatePhysics(false);
	BoxStaticMeshesUndef->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	BoxStaticMeshesUndef->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	Shift = FVector(0, 0, 20.f);
}


void AV2XViewer::BeginPlay()
{
	Super::BeginPlay();

	if(bUseCustomRefPoint)
	{
		ALevelState::GetChecked()->GetLLConverter().LLA2UE(OrignShift, Longitude, Latitude, Altitude);
	}

	FIPv4Endpoint Endpoint(FIPv4Address(0, 0, 0, 0), Port);

	ListenSocket = FUdpSocketBuilder(TEXT("V2XViewer"))
					   .AsNonBlocking()
					   .AsReusable()
					   .BoundToEndpoint(Endpoint)
					   .WithReceiveBufferSize(0xFFFF);

	if(ListenSocket)
	{
#if PLATFORM_LINUX
		SOCKET RawSocket = ((FSocketBSD*)ListenSocket)->GetNativeSocket();
		int Param = 1;
		if(setsockopt(RawSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&Param, sizeof(Param)) != 0)
		{
			UE_LOG(LogSoda, Warning, TEXT("AV2XViewer::BeginPlay(). Can't setsockopt(SO_REUSEPORT)"));	
		}
#endif
		FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
		UDPReceiver = new FUdpSocketReceiver(ListenSocket, ThreadWaitTime, TEXT("V2XViewer"));
		UDPReceiver->OnDataReceived().BindUObject(this, &AV2XViewer::Recv);
		UDPReceiver->Start();
		UE_LOG(LogSoda, Log, TEXT("Start V2X Viwer on port: %i, shift: %s"), Port, *OrignShift.ToString());
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("AV2XViewer::BeginPlay(). Can't create socket"));
	}
}

void AV2XViewer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UDPReceiver)
	{
		UDPReceiver->Stop();
		delete UDPReceiver;
	}

	if (ListenSocket)
	{
		delete ListenSocket;
	}
}

void AV2XViewer::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	TTimestamp Timestamp = SodaApp.GetRealtimeTimestamp();

	Mutex.lock();

	static const std::chrono::milliseconds Timeout(1000);

	for(auto It = V2XObjects.CreateIterator(); It; ++It)
	{
		if(Timestamp - It->Value.Timestamp > Timeout)
		{
			It.RemoveCurrent();
		}
	}

	BatchStart.SetNum(0, false);
	BatchEnd.SetNum(0, false);

	for(auto & Obj : V2XObjects)
	{
		if(!Obj.Value.Traced)
		{
			BatchStart.Add(FVector(Obj.Value.Pos.X, Obj.Value.Pos.Y, +100000.f));
			BatchEnd.Add(FVector(Obj.Value.Pos.X, Obj.Value.Pos.Y, -100000.f));
		}
	}
	
	TArray<FHitResult> OutHits;
	FSodaPhysicsInterface::RaycastSingleScope(
		GetWorld(), OutHits, BatchStart, BatchEnd,
		ECollisionChannel::ECC_WorldStatic,
		FCollisionQueryParams(NAME_None, false),
		FCollisionResponseParams::DefaultResponseParam,
		FCollisionObjectQueryParams::DefaultObjectQueryParam);
	check(OutHits.Num() == BatchStart.Num());

	int RaysNum = 0;
	int ObstacleNum = 0, CollectableNum = 0, UndefNum = 0;
	for(auto & Obj : V2XObjects)
	{
		if(!Obj.Value.Traced)
		{
			Obj.Value.TracedValid = false;
			auto& Hit = OutHits[RaysNum];
			if (Hit.bBlockingHit)
			{
				Obj.Value.Pos.Z  = -FLT_MAX;
				if (Hit.GetActor())
				{
					Obj.Value.Pos.Z = std::max(Obj.Value.Pos.Z, Hit.Location.Z);
					Obj.Value.TracedValid = true;
				}
			}

			Obj.Value.Traced = true;
			++RaysNum;
		}

		if (Obj.Value.TracedValid)
		{
			FTransform Transform(
				Obj.Value.Rot.Quaternion(),
				Obj.Value.Pos + FVector(0.f, 0.f, Obj.Value.Extent.Z) + Shift,
				Obj.Value.Extent / 100);
			
			if (Obj.Value.Data.object_type & (uint32_t)soda::V2XRenderObjectType::OBSTACLE)
			{
				++ObstacleNum;
				if (BoxStaticMeshesObstacle->GetInstanceCount() < ObstacleNum)
				{
					BoxStaticMeshesObstacle->AddInstanceWorldSpace(FTransform());
				}
				BoxStaticMeshesObstacle->UpdateInstanceTransform(ObstacleNum - 1, Transform, true, false, true);
			}
			else if (Obj.Value.Data.object_type & (uint32_t)soda::V2XRenderObjectType::COLLECTABLE)
			{
				++CollectableNum;
				if (BoxStaticMeshesCollectable->GetInstanceCount() < CollectableNum)
				{
					BoxStaticMeshesCollectable->AddInstanceWorldSpace(FTransform());
				}
				BoxStaticMeshesCollectable->UpdateInstanceTransform(CollectableNum - 1, Transform, true, false, true);
			}
			else
			{
				++UndefNum;
				if (BoxStaticMeshesUndef->GetInstanceCount() < UndefNum)
				{
					BoxStaticMeshesUndef->AddInstanceWorldSpace(FTransform());
				}
				BoxStaticMeshesUndef->UpdateInstanceTransform(UndefNum - 1, Transform, true, false, true);
			}
		}
	
		if(bShowLog)
			UE_LOG(LogSoda, Warning, TEXT("V2X ID: %i; TracedValid: %i; Pos: %s; Extent: %s; Yaw %f"), 
				Obj.Key, Obj.Value.TracedValid, *Obj.Value.Pos.ToString(), *Obj.Value.Extent.ToString(), Obj.Value.Rot.Yaw);
	}

	Mutex.unlock();


	int Diff = BoxStaticMeshesObstacle->GetInstanceCount() - ObstacleNum;
	for (int i = 0; i < Diff; ++i)
	{
		BoxStaticMeshesObstacle->RemoveInstance(BoxStaticMeshesObstacle->GetInstanceCount() - 1);
	}
	Diff = BoxStaticMeshesCollectable->GetInstanceCount() - CollectableNum;
	for (int i = 0; i < Diff; ++i)
	{
		BoxStaticMeshesCollectable->RemoveInstance(BoxStaticMeshesCollectable->GetInstanceCount() - 1);
	}
	Diff = BoxStaticMeshesUndef->GetInstanceCount() - UndefNum;
	for (int i = 0; i < Diff; ++i)
	{
		BoxStaticMeshesUndef->RemoveInstance(BoxStaticMeshesUndef->GetInstanceCount() - 1);
	}

	BoxStaticMeshesObstacle->MarkRenderStateDirty();
	BoxStaticMeshesCollectable->MarkRenderStateDirty();
	BoxStaticMeshesUndef->MarkRenderStateDirty();

}

void AV2XViewer::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
	if(ArrayReaderPtr->Num() != sizeof(soda::V2XRenderObject))
	{
		UE_LOG(LogSoda, Error, TEXT("AV2XViewer::Recv(); Protocol error, recve %d bytes, was expected %d bytes"), ArrayReaderPtr->Num(), sizeof(soda::V2XRenderObject));
		return;
	}

	soda::V2XRenderObject GenericObject;
	::memcpy(&GenericObject, ArrayReaderPtr->GetData(), sizeof(soda::V2XRenderObject));

	//if(!(GenericObject.tags & (uint32_t)soda::V2XRenderObjectTag::VIRTUAL_OBJECT)) return;
	
	Mutex.lock();
	FV2XObject & V2XObject = V2XObjects.FindOrAdd(GenericObject.object_id);
	V2XObject.Data = GenericObject;
	V2XObject.Timestamp = SodaApp.GetRealtimeTimestamp();
	V2XObject.Extent = FVector(GenericObject.length * 100.f, GenericObject.width * 100.f, GenericObject.height * 100.f);
	V2XObject.Rot = FRotator(0.f, -GenericObject.yaw_angle / M_PI * 180.f, 0.f);
	V2XObject.Pos.X = -GenericObject.east * 100.f;
	V2XObject.Pos.Y = GenericObject.north * 100.f;
	if(!V2XObject.Traced) V2XObject.Pos.Z = GenericObject.up * 100.f;
	if(bUseCustomRefPoint) 
	{
		V2XObject.Pos.X += OrignShift.X;
		V2XObject.Pos.Y += OrignShift.Y;
		if(!V2XObject.Traced) V2XObject.Pos.Z += OrignShift.Z;
	}
	if(bUseDefaultHeight)
	{
		V2XObject.Extent.Z = DefaultHeight;
	}
	Mutex.unlock();
}

const FSodaActorDescriptor* AV2XViewer::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("V2X Viewer"), /*DisplayName*/
		TEXT("Tools"), /*Category*/
		TEXT("Experimental"), /*SubCategory*/
		TEXT("Icons.Box"), /*Icon*/
		false, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}