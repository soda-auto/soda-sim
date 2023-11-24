// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

FORCEINLINE TSharedRef<FJsonObject>  MakeVectorJsonObject(const FVector& Vector)
{
	TSharedPtr<FJsonObject> VectorJson = MakeShareable(new FJsonObject());
	VectorJson->SetNumberField("x", Vector.X);
	VectorJson->SetNumberField("y", Vector.Y);
	VectorJson->SetNumberField("z", Vector.Z);
	return VectorJson.ToSharedRef();
}

FORCEINLINE TSharedRef<FJsonObject>  MakeRotatorJsonObject(const FRotator& Rotator)
{
	TSharedPtr<FJsonObject> RotaotrJson = MakeShareable(new FJsonObject());
	RotaotrJson->SetNumberField("yaw", Rotator.Yaw);
	RotaotrJson->SetNumberField("pitch", Rotator.Pitch);
	RotaotrJson->SetNumberField("roll", Rotator.Roll);
	return RotaotrJson.ToSharedRef();
}

FORCEINLINE bool JsonObjectToVector(const TSharedPtr< FJsonObject >& Object, FVector& Vector)
{
	double X = 0, Y = 0, Z = 0;
	if (Object->TryGetNumberField("x", X) && Object->TryGetNumberField("y", Y) && Object->TryGetNumberField("z", Z))
	{
		Vector = FVector(X, Y, Z);
		return true;
	}

	return false;
}

FORCEINLINE bool JsonObjectToRotator(const TSharedPtr< FJsonObject >& Object, FRotator& Rotator)
{
	double Pitch = 0, Yaw = 0, Roll = 0;
	if (Object->TryGetNumberField("yaw", Yaw) && Object->TryGetNumberField("pitch", Pitch) && Object->TryGetNumberField("roll", Roll))
	{
		Rotator = FRotator(Pitch, Yaw, Roll);
		return true;
	}

	return false;
}

class FJsonValueNumberSetter : public FJsonValueNumber
{
public:
	void SetValue(double Val) { Value = Val; }
};

class FJsonValueStringSetter : public FJsonValueString
{
public:
	void SetValue(const FString& Val) { Value = Val; }
};

class FJsonValueBooleanSetter : public FJsonValueBoolean
{
public:
	void SetValue(bool Val) { Value = Val; }
};

