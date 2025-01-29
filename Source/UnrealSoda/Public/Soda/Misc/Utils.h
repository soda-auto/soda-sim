// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <math.h>
#include <cmath>

#include "Soda/SodaTypes.h"
#include "Math/Color.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/RotationMatrix.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ANG2RPM (30.0 / M_PI)

inline float Rgba2Float(void* BGRA8Ptr, int DestStride, int row, int col)
{
	unsigned char* sBuf = &((unsigned char*)BGRA8Ptr)[row * DestStride + col * 4];
	return (float(sBuf[0]) / 65025.0 +  //B
		float(sBuf[1]) / 255.0 +	//G
		float(sBuf[2]) / 1.0 +		//R
		float(sBuf[3]) / 16581375.0 //A
		) / 255.0;
}

inline float Rgba2Float(FColor Pixel)
{
	return (float(Pixel.B) / 65025.0 + 
		float(Pixel.G) / 255.0 +
		float(Pixel.R) / 1.0 +
		float(Pixel.A) / 16581375.0
		) / 255.0;
}

inline float NormAngRad(float a)
{
	return (a > M_PI) ? (a - 2.0 * M_PI) : ((a < -M_PI) ? (a + 2 * M_PI) : a);
}

inline float NormAngDeg(float a)
{
	return (a > 180.f) ? (a - 360.f) : ((a < -180.f) ? (a + 360.f) : a);
}

inline FVector NormAngRad(const FVector& v)
{
	return FVector(
		(v.X > M_PI) ? (v.X - 2.0 * M_PI) : ((v.X < -M_PI) ? (v.X + 2 * M_PI) : v.X),
		(v.Y > M_PI) ? (v.Y - 2.0 * M_PI) : ((v.Y < -M_PI) ? (v.Y + 2 * M_PI) : v.Y),
		(v.Z > M_PI) ? (v.Z - 2.0 * M_PI) : ((v.Z < -M_PI) ? (v.Z + 2 * M_PI) : v.Z)
	);
}

inline FVector NormAngDeg(const FVector& v)
{
	return FVector(
		(v.X > M_PI) ? (v.X - 360.f) : ((v.X < -180.f) ? (v.X + 360.f) : v.X),
		(v.Y > M_PI) ? (v.Y - 360.f) : ((v.Y < -180.f) ? (v.Y + 360.f) : v.Y),
		(v.Z > M_PI) ? (v.Z - 360.f) : ((v.Z < -180.f) ? (v.Z + 360.f) : v.Z)
	);
}

inline int Floor(float a)
{
	return (a >= 0) ? (a + 0.5) : (a - 0.5);
}

inline float Sqr(float a)
{
	return a * a;
}

/**
 * See https://helloacm.com/cc-function-to-compute-the-bilinear-interpolation/
 * */
inline float BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y)
{
	float x2x1, y2y1, x2x, y2y, yy1, xx1;
	x2x1 = x2 - x1;
	y2y1 = y2 - y1;
	x2x = x2 - x;
	y2y = y2 - y;
	yy1 = y - y1;
	xx1 = x - x1;
	return 1.0 / (x2x1 * y2y1) * (q11 * x2x * y2y + q21 * xx1 * y2y + q12 * x2x * yy1 + q22 * xx1 * yy1);
}

inline bool Intersection(const FVector2D& Pt11, const FVector2D& Pt12, const FVector2D& Pt21, const FVector2D& Pt22, FVector2D* Intersection)
{
	FVector2D dir1 = Pt12 - Pt11;
	FVector2D dir2 = Pt22 - Pt21;

	float a1 = -dir1.Y;
	float b1 = +dir1.X;
	float d1 = -(a1 * Pt11.X + b1 * Pt11.Y);

	float a2 = -dir2.Y;
	float b2 = +dir2.X;
	float d2 = -(a2 * Pt21.X + b2 * Pt21.Y);

	float seg1_line2_start = a2 * Pt11.X + b2 * Pt11.Y + d2;
	float seg1_line2_end = a2 * Pt12.X + b2 * Pt12.Y + d2;

	float seg2_line1_start = a1 * Pt21.X + b1 * Pt21.Y + d1;
	float seg2_line1_end = a1 * Pt22.X + b1 * Pt22.Y + d1;


	if (seg1_line2_start * seg1_line2_end >= 0 || seg2_line1_start * seg2_line1_end >= 0)
		return false;

	float u = seg1_line2_start / (seg1_line2_start - seg1_line2_end);
	if (Intersection)*Intersection = Pt11 + u * dir1;

	return true;
}

template<typename VectorType>
bool IsClockwise2D(const TArray<VectorType>& Polyline)
{
	float Area = 0;
	for (int i = 0; i < (Polyline.Num()); i++)
	{
		int j = (i + 1) % Polyline.Num();
		Area += Polyline[i].X * Polyline[j].Y;
		Area -= Polyline[j].X * Polyline[i].Y;
	}
	return (Area < 0);
}

template<typename VectorType>
bool IsClockwise2D(const TArray<VectorType>& Cloud, const TArray<int32>& Hull)
{
	float Area = 0;
	for (int i = 0; i < (Hull.Num()); i++)
	{
		int j = (i + 1) % Hull.Num();
		Area += Cloud[Hull[i]].X * Cloud[Hull[j]].Y;
		Area -= Cloud[Hull[j]].X * Cloud[Hull[i]].Y;
	}
	return (Area < 0);
}

inline int FindNearestPoint2D(const FVector& Point, const TArray<FVector>& Points)
{
	FVector2D Point2D(Point);
	check(Points.Num() > 0);
	int Ret = 0;
	float MinLength = (FVector2D(Points[0]) - Point2D).Size();
	for (int i = 1; i < Points.Num(); ++i)
	{
		float Length = (FVector2D(Points[i]) - Point2D).Size();
		if (Length < MinLength)
		{
			MinLength = Length;
			Ret = i;
		}
	}
	return Ret;
}

template <class T>
inline static T CubicInterpolate(const T& p0, const T& p1, const T& p2, const T& p3, double x)
{
	return p1 + 0.5 * x * (p2 - p0 + x * (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3 + x * (3.0 * (p1 - p2) + p3 - p0)));
}

inline static FVector ComputeNormal2D(const FVector& Pt0, const FVector& Pt1, const FVector& Pt2)
{
	return FRotationMatrix(((Pt2 - Pt1).GetSafeNormal() + (Pt1 - Pt0).GetSafeNormal()).GetSafeNormal2D().Rotation()).GetScaledAxis(EAxis::Y);
}

inline float ZeroThreshold(float F, float Threshold)
{
	if (F > Threshold || F < -Threshold)
	{
		return ((F >= 0) ? (F - Threshold) : (F + Threshold)) / (1.0 - Threshold);
	}
	else
	{
		return  0;
	}
}

inline float Distance2D(float X1, float Y1, float X2, float Y2)
{
	return std::sqrt(Sqr(X1 - X2) + Sqr(Y1 - Y2));
}

inline FQuat QExp(const FVector & V)
{
	double Vn = V.Size();
	if (Vn <= SMALL_NUMBER) 
		return FQuat(0., 0., 0., 1.);
	FVector H = V * std::sin(Vn / 2.) / Vn;
	return FQuat(H.X, H.Y, H.Z, std::cos(Vn / 2.));
}

inline FVector QLog(const FQuat& Q)
{
	FVector V(Q.X, Q.Y, Q.Z);
	double Vn = V.Size();
	if (Vn < SMALL_NUMBER)
		return FVector::ZeroVector;
	double Psi = 2.0 * asin(Vn);
	if (Q.W < 0.0)
		Psi *= -1.0;
	V *= Psi / Vn;
	return V;
}



inline float LinInterp1(float x, float x0, float x1, float y0, float y1)
{


	if (x > x1)
	{
		x = x1;
	}
	else if (x < x0)
	{
		x = x0;
	}
	else if (FMath::Abs(x1 - x0) < 0.00001)
	{
		return y0;
	}
	return y0 + (x - x0) * (y1 - y0) / (x1 - x0);

}

inline float LookupTable1d(float X, const TArray<float>& BreakPoints, const TArray<float>& Values)
{
	if (BreakPoints.Num() != Values.Num() || (BreakPoints.Num() == 0))
	{
		return 0;
	}

	// Check for clipping

	if (X <= BreakPoints[0])
	{
		return Values[0];
	}
	else if (X >= BreakPoints[BreakPoints.Num() - 1])
	{
		return Values[Values.Num() - 1];
	}

	for (int32 Idx = 0; Idx <= BreakPoints.Num() - 2; Idx++)
	{

		if (X >= BreakPoints[Idx] && X < BreakPoints[Idx + 1])
		{
			return LinInterp1(X, BreakPoints[Idx], BreakPoints[Idx + 1], Values[Idx], Values[Idx + 1]);
		}

	}

	return 0.0;

}





namespace soda
{

/** [in bytes] */
UNREALSODA_API uint8 GetDataTypeSize(EDataType CVType);


}
