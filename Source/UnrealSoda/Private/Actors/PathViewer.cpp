// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/PathViewer.h"
#include "Soda/UnrealSoda.h"
#include "Soda/LevelState.h"

APathViewer::APathViewer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;

	RootComponent = CreateDefaultSubobject< USceneComponent >(TEXT("RootComponent"));

	Color = FColor(0, 255, 0);
}

void APathViewer::BeginPlay()
{
	Super::BeginPlay();


	FIPv4Endpoint Endpoint(FIPv4Address(0, 0, 0, 0), Port);

	ListenSocket = FUdpSocketBuilder(TEXT("PathViwer"))
					   .AsNonBlocking()
					   .AsReusable()
					   .BoundToEndpoint(Endpoint)
					   .WithReceiveBufferSize(0xFFFF);

	if (ListenSocket == nullptr)
	{
		UE_LOG(LogSoda, Error, TEXT("APathViewer::BeginPlay() Can't create socket"));
		Destroy();
		return;
	}

	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
	UDPReceiver = new FUdpSocketReceiver(ListenSocket, ThreadWaitTime, TEXT("PathViwer"));
	UDPReceiver->OnDataReceived().BindUObject(this, &APathViewer::Recv);
	UDPReceiver->Start();
}

void APathViewer::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void APathViewer::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	mutex.lock();
	DrawBuf = Buf;
	mutex.unlock();

	auto World = GetWorld();
	FVector WorldLoc = RootComponent->GetComponentLocation();
	const auto & Conv = ALevelState::GetChecked()->GetLLConverter();

	for (int i = 0; i < (int)DrawBuf.size() / 2 - 1; i++)
	{
		FVector Pt1;
		FVector Pt2;
		if(bIsLanLong)
		{
			Conv.LLA2UE(Pt1, DrawBuf[i * 2 + 0], DrawBuf[i * 2 + 1], 0);
			Conv.LLA2UE(Pt2, DrawBuf[i * 2 + 2], DrawBuf[i * 2 + 3], 0);
			Pt1.Z = Pt2.Z = WorldLoc.Z;
		}
		else
		{
			Pt1 = FVector(DrawBuf[i * 2 + 0] * ScaleX + DX, DrawBuf[i * 2 + 1] * ScaleY + DY, WorldLoc.Z);
			Pt2 = FVector(DrawBuf[i * 2 + 2] * ScaleX + DX, DrawBuf[i * 2 + 3] * ScaleY + DY, WorldLoc.Z);
		}
		DrawDebugLine(World, Pt1, Pt2, Color, false, -1.f, 0, Thickness);
	}
}

void APathViewer::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{

	mutex.lock();
	Buf.resize(ArrayReaderPtr->Num() / sizeof(double));
	::memcpy(Buf.data(), ArrayReaderPtr->GetData(), ArrayReaderPtr->Num() / sizeof(double) * sizeof(double));
	mutex.unlock();

	//UE_LOG(LogSoda, Error, TEXT("PathViwer::Recv %d "), ArrayReaderPtr->Num());

	//for (int i = 0; i < Buf.size(); i++)
	//	UE_LOG(LogSoda, Error, TEXT("i %d: %f"), i, Buf[i]);
}

const FSodaActorDescriptor* APathViewer::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Path Viewer"), /*DisplayName*/
		TEXT("Tools"), /*Category*/
		TEXT("Experimental"), /*SubCategory*/
		TEXT("SodaIcons.Path"), /*Icon*/
		false, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 0), /*SpawnOffset*/
	};
	return &Desc;
}