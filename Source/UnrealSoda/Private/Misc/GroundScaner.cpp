// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/GroundScaner.h"
#include "Soda/UnrealSoda.h"
#include "DrawDebugHelpers.h"
#include <algorithm>
#include "Soda/Misc/SodaPhysicsInterface.h"

DECLARE_STATS_GROUP(TEXT("GroundScaner"), STATGROUP_GroundScaner, STATGROUP_Advanced);
DECLARE_CYCLE_STAT(TEXT("Scan"), STAT_Scan, STATGROUP_GroundScaner);


static inline FVector ToVector(const FVector2D & V, float Z = 0) { return FVector(V.X, V.Y, Z); }
static inline FVector2D ToVector2D(const FVector & V) { return FVector2D(V.X, V.Y); }

static inline void DrawDebugRect(
	const UWorld * InWorld,
	FVector const& Center,
	FVector2D const& Extent,
	float Yaw,
	FColor const& Color,
	bool bPersistentLines,
	float LifeTime,
	uint8 DepthPriority,
	float Thickness
) 
{
	FVector Pt1 = ToVector(FVector2D(Extent.X, Extent.Y).GetRotated(Yaw)) + Center;
	FVector Pt2 = ToVector(FVector2D(Extent.X, -Extent.Y).GetRotated(Yaw)) + Center;
	FVector Pt3 = ToVector(FVector2D(-Extent.X, -Extent.Y).GetRotated(Yaw)) + Center;
	FVector Pt4 = ToVector(FVector2D(-Extent.X, Extent.Y).GetRotated(Yaw)) + Center;

	DrawDebugLine(InWorld, Pt1, Pt2, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(InWorld, Pt2, Pt3, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(InWorld, Pt3, Pt4, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	DrawDebugLine(InWorld, Pt4, Pt1, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
}

UGroundScaner::UGroundScaner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGroundScaner::BeginDestroy()
{
	Super::BeginDestroy();
	//UE_LOG(LogSoda, Error, TEXT("UGroundScaner::BeginDestroy()"));
}

bool UGroundScaner::Scan(const FVector & Position, const FVector & Size, float Yaw)
{
	SCOPE_CYCLE_COUNTER(STAT_Scan);

	
	if(Size.X / MeshStepX > 200 || Size.Y / MeshStepY > 200)
	{
		UE_LOG(LogSoda, Error, TEXT("UGroundScaner::Scan(); Map size (%f or %f) > 200"), Size.X / MeshStepX, Size.Y / MeshStepY );
		return false;
	}

	FVector2D Bbox1 =  FVector2D(Size.X / 2 , Size.Y / 2).GetRotated(Yaw);
	FVector2D Bbox2 =  FVector2D(-Size.X / 2 , Size.Y / 2).GetRotated(Yaw);

	int HalfWidth = Floor(std::max(std::abs(Bbox1.X), std::abs(Bbox2.X)) / MeshStepX);
	int HalfLength = Floor(std::max(std::abs(Bbox1.Y), std::abs(Bbox2.Y)) / MeshStepY);

	int TmpX0 = Floor(Position.X / MeshStepX - HalfWidth);
	int TmpY0 = Floor(Position.Y / MeshStepY - HalfLength);
	
	int TmpWidth = HalfWidth * 2 + 1;
	int TmpLength = HalfLength * 2 + 1;

	TmpMap.SetNum(TmpWidth * TmpLength, false);

	TArray<FVector> Start;
	TArray<FVector> End;
	Start.Reserve(TmpLength * TmpWidth);
	End.Reserve(TmpLength * TmpWidth);

	for (int Y = 0; Y < TmpLength; ++Y) 
	{ 
		for (int X = 0; X < TmpWidth; ++X) 
		{ 
			FVector2D CheckPt = FVector2D((X - HalfWidth) * MeshStepX, (Y - HalfLength) * MeshStepY).GetRotated(-Yaw);
			FVector Pt((X + TmpX0) * MeshStepX, (Y + TmpY0) * MeshStepY, Position.Z + Size.Z / 2.0);
			if( CheckPt.X <=  Size.X/2  && CheckPt.X >= -Size.X/2  && CheckPt.Y <=  Size.Y/2  && CheckPt.Y >= -Size.Y/2 )
			{
				Start.Push({ Pt.X + 0.1f, Pt.Y + 0.1f, Pt.Z });
				End.Push(Start.Last() + FVector(0, 0, -Size.Z));
				TmpMap[Y * TmpWidth + X].Valid = true;
			}
			else
			{
				TmpMap[Y * TmpWidth + X].Valid = false;
			}
			TmpMap[Y * TmpWidth + X].Value = Pt;
		} 
	} 

	TArray<FHitResult> OutHits;
	{
		//SCOPE_CYCLE_COUNTER(STAT_BatchExecute);

		FSodaPhysicsInterface::RaycastSingleScope(
			GetWorld(), OutHits, Start, End,
			CollisionChannel,
			CollisionQueryParams,
			CollisionResponseParams,
			CollisionObjectQueryParams);

	};
	check(OutHits.Num() == Start.Num());
	
	Mutex.lock();
	X0 = TmpX0;
	Y0 = TmpY0;
	Width = TmpWidth;
	Length = TmpLength;
	Map.SetNum(Width * Length, false);
	for (int i=0, j=0; i < Map.Num(); ++i) 
	{ 
		Map[i] = TmpMap[i];
		if(Map[i].Valid)
		{
			if(OutHits[j].bBlockingHit)
			{
				Map[i].Value.Z = OutHits[j].Location.Z;
			}
			else
			{
				Map[i].Valid = false;
			}
			++j;
		}
	}
	Mutex.unlock();

	if(bDrawDebug)
	{
		UWorld* World = GetWorld();
		check(World);

		float fHalfWidth = std::max(std::abs(Bbox1.X), std::abs(Bbox2.X));
		float fHalfLength = std::max(std::abs(Bbox1.Y), std::abs(Bbox2.Y));

		DrawDebugRect(World, Position, ToVector2D(Size / 2) , Yaw, FColor(0, 255, 0), false, -1.f, 0, 2.f);
		DrawDebugBox(World, Position, Size / 2, FRotator(0.f, Yaw, 0.f).Quaternion(), FColor(0, 255, 0), false, -1.f, 0, 2.f);
		DrawDebugRect(World, Position, FVector2D(fHalfWidth, fHalfLength), 0, FColor(0, 0, 255), false, -1.f, 0, 2.f);
		
		for (auto& Pt : Map)
		{
			if (Pt.Valid)
			{
				DrawDebugPoint(World, Pt.Value, 5.f, FColor(0, 255, 0), false, -1.f, 0);
			}
		}
	}

	return true;

}

bool UGroundScaner::Scan(const FPhysBodyKinematic & VehicleKinematic, float DeltaTime)
{
	while(DtStat.size() < RenderTimeHistorySize) DtStat.push_back(0.5);
	DtStat.push_back(DeltaTime);
	DtStat.pop_front();
	float MaxDeltaTime = DtStat[0];
	for(auto & el : DtStat) MaxDeltaTime = std::max(el, MaxDeltaTime);
	MaxDeltaTime += RenderTimeExtra;

	if(MaxDeltaTime > 0.5) MaxDeltaTime = 0.5;
	
	FTransform NextWorldPose;
	VehicleKinematic.EstimatePose(MaxDeltaTime, NextWorldPose);
	FVector Pt1 = VehicleKinematic.Curr.GlobalPose.TransformPosition(FVector(WheelBase/2, 0.f, 0.f));
	FVector Pt2 = NextWorldPose.TransformPosition(FVector(WheelBase/2, 0.f, 0.f));
	FVector BodySize = FVector(WheelBase, TrackWidth, ScanHeight);
	float Yaw = VehicleKinematic.Curr.GlobalPose.Rotator().Yaw;

	float dYaw = NextWorldPose.Rotator().Yaw - Yaw;
	FVector2D Shift = ToVector2D(Pt2 - Pt1).GetRotated(-Yaw);
	FVector2D Points[8];
	Points[0] = FVector2D(BodySize.X/2, BodySize.Y/2);
	Points[1] = FVector2D(BodySize.X/2, -BodySize.Y/2);
	Points[2] = FVector2D(-BodySize.X/2, -BodySize.Y/2);
	Points[3] = FVector2D(-BodySize.X/2, BodySize.Y/2);
	Points[4] = Points[0].GetRotated(dYaw) + Shift;
	Points[5] = Points[1].GetRotated(dYaw) + Shift;
	Points[6] = Points[2].GetRotated(dYaw) + Shift;
	Points[7] = Points[3].GetRotated(dYaw) + Shift;

	double MinX = FLT_MAX, MaxX = -FLT_MAX, MinY = FLT_MAX, MaxY = -FLT_MAX;
	for(const auto & el : Points)
	{
		MinX = std::min(el.X, MinX);
		MinY = std::min(el.Y, MinY);
		MaxX = std::max(el.X, MaxX);
		MaxY = std::max(el.Y, MaxY);
	}

	FVector Center = ToVector(FVector2D((MinX + MaxX) / 2, (MinY + MaxY) / 2).GetRotated(Yaw)) + Pt1;
	FVector Size(MaxX - MinX + Border * 2, MaxY - MinY + Border * 2, ScanHeight);
	if(!Scan(Center, Size, VehicleKinematic.Curr.GlobalPose.Rotator().Yaw))
	{
		return false;
	}

	if(bDrawDebug)
	{
		float NextYaw = NextWorldPose.Rotator().Yaw;

		DrawDebugRect(GetWorld(), Pt1, FVector2D(BodySize.X/2, BodySize.Y/2), Yaw, FColor(255, 0, 0), false, -1.f, 0, 2.f);
		DrawDebugRect(GetWorld(), Pt2, FVector2D(BodySize.X/2, BodySize.Y/2), NextYaw, FColor(255, 0, 0), false, -1.f, 0, 2.f);
	}	

	return true;
}

bool UGroundScaner::GetHeight(float X, float Y, float & Height)
{
	std::unique_lock<std::mutex> Locker(Mutex);

	int X1 = (int)(X / MeshStepX - X0);
	int X2 = (int)(X / MeshStepX - X0 + 1.0);
	int Y1 = (int)(Y / MeshStepY - Y0);
	int Y2 = (int)(Y / MeshStepY - Y0 + 1.0);
	float nX = X / MeshStepX - X0;
	float nY = Y / MeshStepY - Y0;

	if( X1 < 0 || Y1 < 0 || X1 >= Width || Y1 >= Length || X2 < 0 || Y2 < 0 || X2 >= Width || Y2 >= Length)
	{
		return false;
	}

	auto & Q11 = Map[Y1 * Width + X1];
	auto & Q12 = Map[Y2 * Width + X1];
	auto & Q21 = Map[Y1 * Width + X2];
	auto & Q22 = Map[Y2 * Width + X2];

	if(Q11.Valid && Q12.Valid && Q21.Valid && Q22.Valid)
	{
		Height = BilinearInterpolation(Q11.Value.Z, Q12.Value.Z, Q21.Value.Z, Q22.Value.Z, X1, X2, Y1, Y2, nX, nY);
	}
	else
	{
		return false;
	}

	return true;
}