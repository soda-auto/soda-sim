// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <deque>

//#include "TelemetryGraph.generated.h"

class UCanvas;

struct UNREALSODA_API FTelemetryGraph
{
	void Init(const FString & InTitle, int InWidth, int InHeight, float InMinY, float InMaxY, int InNumSamples, const TArray<FColor>& Colors = { {255, 255, 0, 128} });
	void AddPoint(float Value, int Ind = 0);
	float Draw(UCanvas* InCanvas, float & XPos, float YPos);

protected:
	FString Title;
	int Width = 100;
	int Height = 100;
	float MinY = 0;
	float MaxY = 1;
	int NumSamples = 300;
	TArray<FColor> Colors = { {255, 255, 0, 128} };
	TArray<std::deque<float>> Points;
};

struct UNREALSODA_API FTelemetryGraphGrid
{
	struct UNREALSODA_API FCell
	{
		FString Caption = "";
		bool Visible = false;
		int OffsetX = 0;
		FTelemetryGraph Graph;
	};

	void InitGrid(int Rows, int Cols, int InCellWidth = 100, int InCellHeight = 100, int InNumSamples = 300);
	FCell& InitCell(int Row, int Col, float MinY, float MaxY, const FString& Title = "", const FString& RowTitle = "", const TArray<FColor>& Colors = {{255, 255, 0, 128}}, int OffsetX = 0);
	void AddPoint(int Row, int Col, float Value, int Ind = 0);
	float Draw(UCanvas* InCanvas, float & XPos, float YPos);

protected:
	int CellWidth = 100; 
	int CellHeight = 100; 
	int NumSamples = 300;

	TArray<TArray<FCell>> Grid;
};