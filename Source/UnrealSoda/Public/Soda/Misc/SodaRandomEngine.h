// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <random>

#include "SodaRandomEngine.generated.h"

UCLASS(Blueprintable, BlueprintType)
class UNREALSODA_API USodaRandomEngine : public UObject
{
	GENERATED_BODY()

public:
	// ===========================================================================
	/// @name Generate Ids
	// ===========================================================================
	/// @{

	/// Generate a non-deterministic random id.
	static uint64 GenerateRandomId();

	/// @}
	// ===========================================================================
	/// @name Seed
	// ===========================================================================
	/// @{

	/// Generate a non-deterministic random seed.
	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	static int32 GenerateRandomSeed();

	/// Generate a seed derived from previous seed.
	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	int32 GenerateSeed();

	/// Seed the random engine.
	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	void Seed(int32 InSeed)
	{
		Engine.seed(InSeed);
	}

	/// @}
	// ===========================================================================
	/// @name Uniform distribution
	// ===========================================================================
	/// @{

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	float GetUniformFloat()
	{
		return std::uniform_real_distribution< float >()(Engine);
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	float GetUniformFloatInRange(float Minimum, float Maximum)
	{
		return std::uniform_real_distribution< float >(Minimum, Maximum)(Engine);
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	int32 GetUniformIntInRange(int32 Minimum, int32 Maximum)
	{
		return std::uniform_int_distribution< int32 >(Minimum, Maximum)(Engine);
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	bool GetUniformBool()
	{
		return (GetUniformIntInRange(0, 1) == 1);
	}

	/// @}
	// ===========================================================================
	/// @name Other distributions
	// ===========================================================================
	/// @{

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	bool GetBernoulliDistribution(float P)
	{
		return std::bernoulli_distribution(P)(Engine);
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	int32 GetBinomialDistribution(int32 T, float P)
	{
		return std::binomial_distribution< int32 >(T, P)(Engine);
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	int32 GetPoissonDistribution(float Mean)
	{
		return std::poisson_distribution< int32 >(Mean)(Engine);
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	float GetExponentialDistribution(float Lambda)
	{
		return std::exponential_distribution< float >(Lambda)(Engine);
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	float GetNormalDistribution(float Mean, float StandardDeviation)
	{
		return std::normal_distribution< float >(Mean, StandardDeviation)(Engine);
	}

	/// @}
	// ===========================================================================
	/// @name Sampling distributions
	// ===========================================================================
	/// @{

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	bool GetBoolWithWeight(float Weight)
	{
		return (Weight >= GetUniformFloat());
	}

	UFUNCTION(Category = "Traffic Routes", BlueprintCallable)
	int32 GetIntWithWeight(const TArray< float >& Weights)
	{
		return std::discrete_distribution< int32 >(
			Weights.GetData(),
			Weights.GetData() + Weights.Num())(Engine);
	}

	/// @}
	// ===========================================================================
	/// @name Elements in TArray
	// ===========================================================================
	/// @{

	template < typename T >
	auto& PickOne(const TArray< T >& Array)
	{
		check(Array.Num() > 0);
		return Array[GetUniformIntInRange(0, Array.Num() - 1)];
	}

	template < typename T >
	void Shuffle(TArray< T >& Array)
	{
		std::shuffle(Array.GetData(), Array.GetData() + Array.Num(), Engine);
	}

	/// @}

private:
	std::minstd_rand Engine;
};
