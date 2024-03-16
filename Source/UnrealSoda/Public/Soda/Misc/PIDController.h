// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UnrealSoda.h"
#include "PIDController.generated.h"

/**
* PID controller
*/
USTRUCT(BlueprintType)
struct UNREALSODA_API FPIDController
{
	GENERATED_BODY()

	/** PID FeedForward Coef*/
	UPROPERTY(EditAnywhere, Category = "PID controller", BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float FFCoef = 0.035f;

	/** PID Proportional Coef*/
	UPROPERTY(EditAnywhere, Category = "PID controller", BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float PropCoef = 0.03f;

	/** PID Integral Coef*/
	UPROPERTY(EditAnywhere, Category = "PID controller", BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float IntegrCoef = 0;

	/** PID Derivative Coef*/
	UPROPERTY(EditAnywhere, Category = "PID controller", BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float DiffCoef = 0.003;

	/** In case of low fps (under LowFPS limit) multiply proportional error on Coef to reduce oscillations*/
	UPROPERTY(EditAnywhere, Category = "PID controller", BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float LowFPSCoef = 0.2f;

	/** If time step per Tick exceeds limit - use LowFPSCoef [s]*/
	UPROPERTY(EditAnywhere, Category = "PID controller", BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	float LowFpsDtLimit = 0.05f;

	/** Put PID debug info to log*/
	UPROPERTY(EditAnywhere, Category = "PID controller", BlueprintReadWrite, SaveGame, meta = (EditInRuntime))
	bool bDebugOutput = false;

	inline float CalculatePID(float FFVal, float Error, float Time)
	{
		float Diff = (Error - PrevVal) / Time;
		Integrator += Error * Time;
		PrevVal = Error;
		if (Time > LowFpsDtLimit)
			Error *= LowFPSCoef;
		float Res = FFCoef * FFVal + PropCoef * Error + IntegrCoef * Integrator + DiffCoef * Diff;
		if (bDebugOutput)
		{
			UE_LOG(LogSoda, Log, TEXT("PID: FFVal=%f, Error=%f, Integ=%f, Diff=%f, (FF+Prop+Integ+Diff)=(%f+%f+%f+%f=%f)"), FFVal, Error, Integrator, Diff, FFCoef * FFVal, PropCoef * Error, IntegrCoef * Integrator, DiffCoef * Diff, Res);
		}

		return Res;
	}

	inline void Init(float FFVal)
	{
		PrevVal = FFVal;
	}

	inline void Reset()
	{
		Integrator = 0;
		PrevVal = 0;
	}

protected:
	float Integrator = 0;
	float PrevVal = 0;
};
