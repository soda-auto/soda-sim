// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/TelemetryGraph.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "GlobalRenderResources.h"

FFontRenderInfo FontRenderInfo;

void FTelemetryGraph::Init(const FString & InTitle, int InWidth, int InHeight, float InMinY, float InMaxY, int InNumSamples, const TArray<FColor>& InColors)
{
	check(InColors.Num());

	Title = InTitle;
	Width = InWidth;
	Height = InHeight;
	MinY = InMinY;
	MaxY = InMaxY;
	NumSamples = InNumSamples;
	Colors = InColors;
	Points.SetNum(InColors.Num());

	FontRenderInfo.bClipText = true;
}

void FTelemetryGraph::AddPoint(float Value, int Ind)
{
	Points[Ind].push_back(Value);
	Points[Ind].pop_front();
}

float FTelemetryGraph::Draw(UCanvas* Canvas, float & XPos, float YPos)
{
	const float YPos0 = YPos;

	for (auto& Pts : Points)
	{
		while (Pts.size() < NumSamples) Pts.push_back(MinY);
	}

	//inline float GetLastPoint() const { return Points.back(); }

	FString Label = Title + FString::Printf(TEXT("[%.2f,%.2f]" /*%.3f*/),  MinY, MaxY/*, GetLastPoint()*/);
	Canvas->SetDrawColor(FColor(255, 255, 0));
	UFont *Font = GEngine->GetSmallFont();
	Canvas->DrawText(Font, Label, XPos, YPos, 1.f, 1.f, FontRenderInfo);

	float XL, YL;
	Canvas->TextSize(Font, Label, XL, YL);
	YPos += YL + 1;

	for (int j = 0; j < Points.Num(); ++j)
	{
		Canvas->SetDrawColor(Colors[j]);
		Canvas->DrawText(Font, FString::Printf(TEXT("%.3f"), Points[j].back()), XPos + 3, YPos + (YL * j + 1) + 3, 1.f, 1.f, FontRenderInfo);
		//YPos += YL + 1;
	}

	YPos += 3;
	FVector2D GraphPos = FVector2D(XPos, YPos);

	FCanvasTileItem TileItem(GraphPos, GWhiteTexture, FVector2D(Width, Height), FLinearColor(0.0f, 0.125f, 0.0f, 0.25f));
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);

	FCanvasLineItem LineAsix( GraphPos + FVector2D(0, Height /2), GraphPos + FVector2D(Width, Height /2));
	LineAsix.SetColor(FLinearColor(0.0f, 0.5f, 0.0f, 1.0f));
	LineAsix.Draw(Canvas->Canvas);
	Canvas->SetDrawColor(FColor(0, 32, 0, 128));

	for (int j = 0; j < Points.Num(); ++j)
	{
		float PrevY = 0;
		for (int i = 0; i < Width; ++i)
		{
			float x = float(NumSamples * i) / Width;
			int x0 = int(x);
			int x1 = int(x + 1.0);
			float y0 = Points[j][x0];
			float y1 = Points[j][x1];
			float y = y0 + (y1 - y0) / float(x1 - x0) * float(x - x0);

			y = (y - MinY) / (MaxY - MinY) * float(Height);
			y = FMath::Clamp(y, 0.0f, float(Height));

			if (i != 0)
			{
				FCanvasLineItem LineItem(
					GraphPos + FVector2D(i - 1, Height - PrevY),
					GraphPos + FVector2D(i, Height - y));
				LineItem.SetColor(Colors[j]);
				LineItem.Draw(Canvas->Canvas);
			} 

			PrevY = y;
		}
	}

	YPos += Height;

	XPos = GraphPos.X + Width + 10;
	return YPos - YPos0 + 10.0f;
}

void FTelemetryGraphGrid::InitGrid(int Rows, int Cols, int InCellWidth, int InCellHeight, int InNumSamples)
{
	CellWidth = InCellWidth; 
	CellHeight = InCellHeight; 
	NumSamples = InNumSamples;

	Grid.SetNum(Rows);
	for (auto & Row : Grid) Row.SetNum(Cols);
}

FTelemetryGraphGrid::FCell & FTelemetryGraphGrid::InitCell(int Row, int Col, float MinY, float MaxY, const FString & Title, const FString & RowTitle, const TArray<FColor>& Colors, int OffsetX)
{
	check(Colors.Num());
	FCell& Cell = Grid[Row][Col];
	Cell.Graph.Init(Title, CellWidth, CellHeight, MinY, MaxY, NumSamples, Colors);
	Cell.Caption = RowTitle;
	Cell.Visible = true;
	Cell.OffsetX = OffsetX;
	return Cell;
}

void FTelemetryGraphGrid::AddPoint(int Row, int Col, float Value, int Ind)
{
	Grid[Row][Col].Graph.AddPoint(Value, Ind);
}

float FTelemetryGraphGrid::Draw(UCanvas* InCanvas, float & InXPos, float InYPos)
{
	UFont* RenderFont = GEngine->GetSmallFont();

	float XPosMax = InXPos;
	float XPos = InXPos;
	float YPos = InYPos;
	for(int i = 0; i < Grid.Num(); ++i )
	{
		XPos = InXPos;
		float CaptionY = YPos; 
		float GraphY = YPos;
		float GraphShiftY = 0;
		for(int j = 0; j < Grid[i].Num(); ++j )
		{
			if(Grid[i][j].Visible)
			{
				XPos += Grid[i][j].OffsetX;
				InCanvas->SetDrawColor(FColor::White);
				GraphY = CaptionY + InCanvas->DrawText(RenderFont, *Grid[i][j].Caption, XPos, CaptionY, 1.f, 1.f, FontRenderInfo);
				GraphShiftY = Grid[i][j].Graph.Draw(InCanvas, XPos, GraphY);
			}
		}
		YPos = GraphY + GraphShiftY;
		XPosMax = std::max(XPosMax, XPos);
	}

	InXPos = XPosMax;
	return (YPos - InYPos);
}