// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/Vehicles/SodaVehicle.h"
#include "SodaSimProto/Gazebo.hpp"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include <zmq.hpp>
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#include "GazeboVehicle.generated.h"

class AGazeboPawn;

/*
namespace zmq
{
	class socket_t;
	class context_t;
}
*/

USTRUCT()
struct FSecondaryTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	class AGazeboPawn* Target;

	UNREALSODA_API virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	UNREALSODA_API virtual FString DiagnosticMessage() override;
};

template <>
struct TStructOpsTypeTraits< FSecondaryTickFunction > : public TStructOpsTypeTraitsBase2< FSecondaryTickFunction >
{
	enum
	{
		WithCopy = false
	};
};

/**
 * AGazeboPawn
 * DEPRICATED
 * TODO: Remove it
 */
UCLASS(ClassGroup = (Gazebo), meta = (BlueprintSpawnableComponent))
class UNREALSODA_API AGazeboPawn : public ASodaVehicle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Gazebo)
	FString CollisionPublisherAddr = "tcp://*:5559";

	UPROPERTY(EditAnywhere, Category = Gazebo)
	FString ModelStateSubAddr = "tcp://127.0.0.1:5558";

	UPROPERTY(EditAnywhere, Category = Gazebo)
	bool DebugOut = true;

	UPROPERTY(EditDefaultsOnly, Category = "Tick")
	FSecondaryTickFunction SecondaryActorTick;

public:
	UPROPERTY(BlueprintReadOnly, Category = Gazebo)
	TArray< float > GazeboJoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gazebo")
	UPrimitiveComponent* Model = 0;

public:
	AGazeboPawn();
	~AGazeboPawn();

public:
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation,
						   FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction);
	virtual void PostInitProperties() override;
	virtual void PostInitializeComponents() override;

	void TickActor2(float DeltaTime, enum ELevelTick TickType, FSecondaryTickFunction& ThisTickFunction);

private:
	/*
	zmq::context_t* Ctx;
	zmq::socket_t* SockSub;
	zmq::socket_t* SockPub;
	*/

	soda::ModelState StateMsg;
	soda::ModelCollision CollisionMsg;
	std::vector< unsigned char > SendBuf;
};
