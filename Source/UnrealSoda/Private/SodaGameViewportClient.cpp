// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaGameViewportClient.h"
#include "Engine/Canvas.h"
#include "SceneViewExtension.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Soda/SodaSubsystem.h"
#include "Editor/SodaWidget.h"
#include "Components/BillboardComponent.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/Editor/SodaSelection.h"
#include "Soda/Misc/EditorUtils.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/SodaSubsystem.h"
#include "Editor/PreviewActor.h"
#include "Soda/ISodaActor.h"
#include "Soda/ISodaVehicleComponent.h"
#include "GenericPlatform/GenericApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandList.h"
#include "Soda/LevelState.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/UnrealSodaVersion.h"
#include "Soda/SodaCommonSettings.h"

static void DrawSelectBox(FPrimitiveDrawInterface* PDI, const FBox& Box, float SizeK, const FLinearColor& Color, uint8 DepthPriority, float Thickness = 0.0f, float DepthBias = 0.0f, bool bScreenSpace = false)
{
	const float Size = Box.GetSize().GetMin() * SizeK;

	PDI->DrawLine(FVector(Box.Min.X, Box.Min.Y, Box.Min.Z), FVector(Box.Min.X + Size, Box.Min.Y, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Min.Y, Box.Min.Z), FVector(Box.Min.X, Box.Min.Y + Size, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Min.Y, Box.Min.Z), FVector(Box.Min.X, Box.Min.Y, Box.Min.Z + Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

	PDI->DrawLine(FVector(Box.Min.X, Box.Max.Y, Box.Min.Z), FVector(Box.Min.X + Size, Box.Max.Y, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Max.Y, Box.Min.Z), FVector(Box.Min.X, Box.Max.Y - Size, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Max.Y, Box.Min.Z), FVector(Box.Min.X, Box.Max.Y, Box.Min.Z + Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

	PDI->DrawLine(FVector(Box.Max.X, Box.Max.Y, Box.Min.Z), FVector(Box.Max.X - Size, Box.Max.Y, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Max.Y, Box.Min.Z), FVector(Box.Max.X, Box.Max.Y - Size, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Max.Y, Box.Min.Z), FVector(Box.Max.X, Box.Max.Y, Box.Min.Z + Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

	PDI->DrawLine(FVector(Box.Max.X, Box.Min.Y, Box.Min.Z), FVector(Box.Max.X - Size, Box.Min.Y, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Min.Y, Box.Min.Z), FVector(Box.Max.X, Box.Min.Y + Size, Box.Min.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Min.Y, Box.Min.Z), FVector(Box.Max.X, Box.Min.Y, Box.Min.Z + Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

	//

	PDI->DrawLine(FVector(Box.Min.X, Box.Min.Y, Box.Max.Z), FVector(Box.Min.X + Size, Box.Min.Y, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Min.Y, Box.Max.Z), FVector(Box.Min.X, Box.Min.Y + Size, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Min.Y, Box.Max.Z), FVector(Box.Min.X, Box.Min.Y, Box.Max.Z - Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

	PDI->DrawLine(FVector(Box.Min.X, Box.Max.Y, Box.Max.Z), FVector(Box.Min.X + Size, Box.Max.Y, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Max.Y, Box.Max.Z), FVector(Box.Min.X, Box.Max.Y - Size, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Min.X, Box.Max.Y, Box.Max.Z), FVector(Box.Min.X, Box.Max.Y, Box.Max.Z - Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

	PDI->DrawLine(FVector(Box.Max.X, Box.Max.Y, Box.Max.Z), FVector(Box.Max.X - Size, Box.Max.Y, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Max.Y, Box.Max.Z), FVector(Box.Max.X, Box.Max.Y - Size, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Max.Y, Box.Max.Z), FVector(Box.Max.X, Box.Max.Y, Box.Max.Z - Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

	PDI->DrawLine(FVector(Box.Max.X, Box.Min.Y, Box.Max.Z), FVector(Box.Max.X - Size, Box.Min.Y, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Min.Y, Box.Max.Z), FVector(Box.Max.X, Box.Min.Y + Size, Box.Max.Z), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
	PDI->DrawLine(FVector(Box.Max.X, Box.Min.Y, Box.Max.Z), FVector(Box.Max.X, Box.Min.Y, Box.Max.Z - Size), Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
}

//************************************************************************************
void USodaGameViewportClient::Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);

	Selection = NewObject<USodaSelection>(this);
	check(Selection);

	TrackingWidgetMode = soda::EWidgetMode::WM_Translate;
	CurrentAxis = EAxisList::None;
	CoordSystem = soda::COORD_Local;
}

void USodaGameViewportClient::InitUI(const TWeakPtr<soda::SSodaViewport>& SodaViewportWidget)
{
	SodaViewport = SodaViewportWidget;
	AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(SodaViewport.Pin().ToSharedRef()), 13);
}

soda::EUIMode USodaGameViewportClient::GetGameMode() const
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport.Pin()->GetUIMode();
	}
	else
	{
		return soda::EUIMode::Free;
	}
}

void USodaGameViewportClient::Draw(FViewport* InViewport, FCanvas* SceneCanvas)
{
	UGameViewportClient::Draw(InViewport, SceneCanvas);

	if (GetGameMode() == soda::EUIMode::Editing)
	{
		if (Widget)
		{
			Widget->DrawHUD(SceneCanvas);
		}
	}

	DrawAxes(InViewport, SceneCanvas, nullptr, EAxisList::XYZ);
}

void USodaGameViewportClient::PostRender(UCanvas* Canvas)
{
	Super::PostRender(Canvas);

	if (GetDefault<USodaCommonSettings>()->bIsDrawVehicleDebugPanel)
	{
		ASodaVehicle* Vehicle = nullptr;
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			Vehicle = PlayerController->GetPawn<ASodaVehicle>();
		}
		if (!Vehicle)
		{
			if (GetGameMode() == soda::EUIMode::Editing)
			{
				Vehicle = Cast< ASodaVehicle >(Selection->GetSelectedActor());
			}
		}
		if (Vehicle)
		{
			float YL = 10;
			float YPos = 10;
			Vehicle->DrawDebug(Canvas, YL, YPos);
		}
	}

	UFont* RenderFont = GEngine->GetSmallFont();
	Canvas->SetDrawColor(FColor::White);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ver: %s"), UNREALSODA_VERSION_STRING), Canvas->SizeX - 100, Canvas->SizeY - 20);
}

void USodaGameViewportClient::FinalizeViews(class FSceneViewFamily* ViewFamily, const TMap<ULocalPlayer*, FSceneView*>& PlayerViewMap)
{
	for (auto& It : PlayerViewMap)
	{
		check(!It.Value->Drawer);
		It.Value->Drawer = this;
	}
}

void USodaGameViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (GetGameMode() == soda::EUIMode::Editing)
	{
		if (Widget)
		{
			Widget->Render(View, PDI, this);
		}

		if (IsValid(DropPreviewActor))
		{
			DropPreviewActor->RenderTarget(PDI);
		}

		USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();

		for (auto& It : SodaSubsystem->GetSodaActors())
		{
			if (ISodaActor* SodaActor = Cast<ISodaActor>(It))
			{
				SodaActor->DrawVisualization(this, View, PDI);
			}
		}

		AActor* SelectedActor = Selection->GetSelectedActor();
		ISodaActor* SodaActor = Cast<ISodaActor>(SelectedActor);
		if (SelectedActor && (!SodaActor || SodaActor->ShowSelectBox()))
		{
			DrawBoundingBox(View, PDI, SelectedActor, FLinearColor::White);
		}

		AActor* HoveredActor = Selection->GetHoveredActor();
		if (HoveredActor/* && SodaSubsystem->GetSodaActorDescriptor(HoveredActor->GetClass()).bDrawDebugSelectBox*/)
		{
			DrawBoundingBox(View, PDI, HoveredActor, FLinearColor(0, 0.8, 1, 0.8));
		}
		
		if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(WidgetTargetComponent))
		{
			VehicleComponent->DrawSelection(View, PDI);
		}
	}

}

void USodaGameViewportClient::Tick(float DeltaTime)
{
	if (Viewport)
	{
		if (GetGameMode() == soda::EUIMode::Editing)
		{
			if (Widget)
			{
				Widget->TickWidget(this);

				if (!bIsWidgetDragging)
				{
					CheckAxisUnderCursor();
				}
				else
				{
					UpdateMouseDelta();
				}
			}

			if (EditorMouseCaptureMode == EEditorMouseCaptureMode::Default)
			{
				Selection->Tick(Viewport);
			}
		}

		if (WidgetTargetComponent && !IsValid(WidgetTargetComponent))
		{
			SetWidgetTarget(nullptr);
			StopTracking();
		}
	}
}

bool USodaGameViewportClient::InputKey(const FInputKeyEventArgs& InEventArgs)
{
	if (GetGameMode() == soda::EUIMode::Editing)
	{
		bool bIsLeftMouseButtonDown = InEventArgs.Viewport->KeyState(EKeys::LeftMouseButton);
		bool bIsLeftMouseButtonEvent = InEventArgs.Key == EKeys::LeftMouseButton;

		if (EditorMouseCaptureMode == EEditorMouseCaptureMode::Default)
		{
			if (bIsLeftMouseButtonDown && bIsLeftMouseButtonEvent && (InEventArgs.Event == IE_Pressed))
			{
				TryStartTracking(InEventArgs.Viewport->GetMouseX(), InEventArgs.Viewport->GetMouseY());
				if (bIsWidgetDragging)
				{
					return Super::InputKey(InEventArgs);
				}
			}

			// If we are tracking and no mouse button is down and this input event released the mouse button stop tracking and process any clicks if necessary
			if (bIsWidgetDragging && !bIsLeftMouseButtonDown && bIsLeftMouseButtonEvent)
			{
				// Stop tracking if no mouse button is down
				StopTracking();
				bIsWidgetDragging = false;
				return Super::InputKey(InEventArgs);
			}

			if (bIsWidgetDragging)
			{
				return Super::InputKey(InEventArgs);
			}

			if (EditorMouseCaptureMode == EEditorMouseCaptureMode::Default)
			{
				Selection->InputKey(InEventArgs);
			}
		}
		else if (EditorMouseCaptureMode == EEditorMouseCaptureMode::Dragging)
		{
			if (bIsLeftMouseButtonDown && bIsLeftMouseButtonEvent && (InEventArgs.Event == IE_Pressed))
			{
				bEditorCaptureModeDragging = true;
			}

			if (!bIsLeftMouseButtonDown && bIsLeftMouseButtonEvent)
			{
				bEditorCaptureModeDragging = false;
			}
		}
	}

	if (Super::InputKey(InEventArgs))
	{
		return true;
	}
	else
	{
		if (SodaViewport.IsValid())
		{
			FModifierKeysState KeyState = FSlateApplication::Get().GetModifierKeys();
			return SodaViewport.Pin()->GetCommandList()->ProcessCommandBindings(InEventArgs.Key, KeyState, (InEventArgs.Event == IE_Repeat));
		}
		UE_LOG(LogSoda, Error, TEXT("ASodaVehicle::InputKey(); %s "), *InEventArgs.Key.ToString());
		return false;
	}
}

bool USodaGameViewportClient::InputAxis(FViewport* InViewport, FInputDeviceId InputDevice, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (GetGameMode() == soda::EUIMode::Editing)
	{
		if (bIsWidgetDragging && Widget)
		{
			FVector Wk = FVector(Key == EKeys::MouseX ? Delta : 0, Key == EKeys::MouseY ? Delta : 0, 0);

			if (TrackingWidgetMode == soda::WM_Rotate)
			{
				const float RotateSpeedMultipler = 1.0f;
				Wk *= RotateSpeedMultipler;
			}
			else if (TrackingWidgetMode == soda::WM_Scale)
			{
				const float ScaleSpeedMultipler = 0.01f;
				Wk *= ScaleSpeedMultipler;
			}

			End += Wk;

			return true;
		}
		else if (bEditorCaptureModeDragging)
		{
			return true;
		}
		else
		{
			Selection->InputAxis(InViewport, InputDevice, Key, Delta, DeltaTime, NumSamples, bGamepad);
		}
	}

	return Super::InputAxis(InViewport, InputDevice, Key, Delta, DeltaTime, NumSamples, bGamepad);
}

EMouseCursor::Type USodaGameViewportClient::GetCursor(FViewport* InViewport, int32 X, int32 Y)
{
	CachedMouseX = X;
	CachedMouseY = Y;

	if (GetGameMode() == soda::EUIMode::Editing)
	{
		if (bIsWidgetDragging)
		{
			return EMouseCursor::GrabHand;
		}
		else if (GetCurrentWidgetAxis() != EAxisList::None)
		{
			return EMouseCursor::CardinalCross;
		}
		else
		{
			return EMouseCursor::Crosshairs;
		}
	}

	return Super::GetCursor(InViewport, X, Y);
}


EMouseCaptureMode USodaGameViewportClient::GetMouseCaptureMode() const
{
	return IsDragging() ? EMouseCaptureMode::NoCapture : Super::GetMouseCaptureMode();
}

bool USodaGameViewportClient::LockDuringCapture()
{
	return IsDragging() ? false : Super::LockDuringCapture();
}

bool USodaGameViewportClient::ShouldAlwaysLockMouse()
{
	return IsDragging() ? false : Super::ShouldAlwaysLockMouse();
}

bool USodaGameViewportClient::HideCursorDuringCapture() const
{
	return IsDragging() ? false : Super::HideCursorDuringCapture();
}

bool USodaGameViewportClient::IsDragging() const
{
	return (bIsWidgetDragging && GetWidgetMode() == soda::WM_Translate) || bEditorCaptureModeDragging;
}

soda::EWidgetMode USodaGameViewportClient::GetWidgetMode() const
{
	return TrackingWidgetMode;
}

void USodaGameViewportClient::SetWidgetMode(soda::EWidgetMode InWidgetMode)
{
	TrackingWidgetMode = InWidgetMode;
}

soda::ECoordSystem USodaGameViewportClient::GetWidgetCoordSystemSpace() const
{
	return CoordSystem;
}

void USodaGameViewportClient::SetWidgetCoordSystemSpace(soda::ECoordSystem InCoordSystem)
{
	CoordSystem = InCoordSystem;
}

FVector USodaGameViewportClient::GetWidgetLocation() const
{
	if (WidgetTargetComponent)
	{
		return WidgetTargetComponent->GetComponentTransform().GetTranslation();
	}
	else
	{
		return FVector::ZeroVector;
	}
}

FMatrix USodaGameViewportClient::GetLocalCoordinateSystem() const
{
	if (WidgetTargetComponent)
	{
		return FQuatRotationMatrix(WidgetTargetComponent->GetComponentTransform().GetRotation());
	}
	else
	{
		return FMatrix::Identity;
	}
}

FMatrix USodaGameViewportClient::GetWidgetCoordSystem() const
{
	if (WidgetTargetComponent)
	{

		switch (GetWidgetCoordSystemSpace())
		{
		case soda::COORD_Local:
			return FQuatRotationMatrix(WidgetTargetComponent->GetComponentTransform().GetRotation());

		case soda::COORD_World:
		default:
			return FMatrix::Identity;
		}
	}
	else
	{
		return FMatrix::Identity;
	}
}

void USodaGameViewportClient::SetCurrentWidgetAxis(EAxisList::Type AxisList)
{
	if (Widget)
	{
		Widget->SetCurrentAxis(AxisList);
		CurrentAxis = AxisList;
	}
}

EAxisList::Type USodaGameViewportClient::GetCurrentWidgetAxis() const
{
	return CurrentAxis;
}

void USodaGameViewportClient::CheckAxisUnderCursor()
{
	const EAxisList::Type SaveAxis = Widget->GetCurrentAxis();
	EAxisList::Type NewAxis = EAxisList::None;

	const bool LeftMouseButtonDown = Viewport->KeyState(EKeys::LeftMouseButton) ? true : false;
	const bool MiddleMouseButtonDown = Viewport->KeyState(EKeys::MiddleMouseButton) ? true : false;
	const bool RightMouseButtonDown = Viewport->KeyState(EKeys::RightMouseButton) ? true : false;
	const bool bMouseButtonDown = (LeftMouseButtonDown || MiddleMouseButtonDown || RightMouseButtonDown);
	
	if(!bMouseButtonDown)
	{
		// In the case of the widget mode being overridden we can have a hit proxy
		// from the previous mode with an inappropriate axis for rotation.
		EAxisList::Type ProxyAxis = ASodaWidget::GetAxisListAtScreenPos(GetWorld(), FVector2D(CachedMouseX, CachedMouseY));
		if (GetWidgetMode() != soda::WM_Rotate || ProxyAxis == EAxisList::X || ProxyAxis == EAxisList::Y || ProxyAxis == EAxisList::Z)
		{
			NewAxis = ProxyAxis;
		}
	}

	// If the current axis on the widget changed, repaint the viewport.
	if (NewAxis != SaveAxis)
	{
		SetCurrentWidgetAxis(NewAxis);
	}
}

void USodaGameViewportClient::SetWidgetTarget(USceneComponent* InWidgetTargetComponent)
{
	if (IsValid(InWidgetTargetComponent))
	{
		WidgetTargetComponent = InWidgetTargetComponent;
		if(!Widget) Widget = GetWorld()->SpawnActor<ASodaWidget>();
	}
	else
	{
		WidgetTargetComponent = nullptr;
		if (Widget)
		{
			Widget->Destroy();
			Widget = nullptr;
		}
	}
}

void USodaGameViewportClient::Invalidate()
{
	StopTracking();
	WidgetTargetComponent = nullptr;
	if (Widget)
	{
		Widget->Destroy();
		Widget = nullptr;
	}
	EditorMouseCaptureMode = EEditorMouseCaptureMode::Default;
	bEditorCaptureModeDragging = false;
}

void USodaGameViewportClient::UpdateMouseDelta()
{
	if (Widget)
	{
		FVector DragDelta = End - Start;

		// Convert the movement delta into drag/rotation deltas
		FVector Drag = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		FVector Scale = FVector::ZeroVector;
		EAxisList::Type CurrentAxis2 = Widget->GetCurrentAxis();

		FSceneViewFamilyContext DragStartViewFamily(FSceneViewFamily::ConstructionValues(
			Viewport,
			GetWorld()->Scene,
			EngineShowFlags)
			.SetRealtimeUpdate(true));

		FVector	 TmpViewLocation;
		FRotator TmpViewRotation;
		FSceneView* DragStartView = GetWorld()->GetFirstLocalPlayerFromController()->CalcSceneView(&DragStartViewFamily, TmpViewLocation, TmpViewRotation, Viewport);

		switch (GetWidgetMode())
		{
		case soda::EWidgetMode::WM_Translate:
			Widget->AbsoluteTranslationConvertMouseMovementToAxisMovement(DragStartView, this, GetWidgetLocation(), FVector2D(Viewport->GetMouseX(), Viewport->GetMouseY()), Drag, Rot, Scale);
			break;
		case soda::EWidgetMode::WM_Rotate:
		case soda::EWidgetMode::WM_Scale:
			Widget->ConvertMouseMovementToAxisMovement(DragStartView, this, false, DragDelta, Drag, Rot, Scale);
			Widget->ConvertMouseMovementToAxisMovement(DragStartView, this, false, DragDelta, Drag, Rot, Scale);
			break;
		default:
			break;
		}

		InputWidgetDelta(Viewport, CurrentAxis, Drag, Rot, Scale);

		if (!Rot.IsZero())
		{
			Widget->UpdateDeltaRotation();
		}

		// Clean up
		End -= DragDelta;
	}
}

void USodaGameViewportClient::StopTracking()
{
	if (bIsWidgetDragging && Widget)
	{
		Widget->SetDragging(false);
		Widget->ResetDeltaRotation();
		Start = End = FVector::ZeroVector;
		SetCurrentWidgetAxis(EAxisList::None);
		bIsWidgetDragging = false;
	}
}

void USodaGameViewportClient::TryStartTracking(const int32 InX, const int32 InY)
{
	if (Widget)
	{
		bIsWidgetDragging = (GetCurrentWidgetAxis() != EAxisList::None);

		Widget->ResetInitialTranslationOffset();
		Widget->SetDragStartPosition(FVector2D(InX, InY));
		Widget->SetDragging(bIsWidgetDragging);
		Widget->ResetDeltaRotation();

		Start = End = FVector(InX, InY, 0);
	}
}

bool USodaGameViewportClient::InputWidgetDelta(FViewport* InViewport, EAxisList::Type InCurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
	if (!WidgetTargetComponent)
	{
		return false;
	}
	
	FTransform WidgetTransform = WidgetTargetComponent->GetComponentTransform();
	WidgetTransform.AddToTranslation(Drag);
	WidgetTransform.SetRotation(Rot.Quaternion() * WidgetTransform.GetRotation());
	WidgetTransform.SetScale3D(WidgetTransform.GetScale3D() + Scale);

	if (ISodaActor* SodaActor = Cast<ISodaActor>(Selection->GetSelectedActor()))
	{
		SodaActor->InputWidgetDelta(WidgetTargetComponent, WidgetTransform);

		if (Selection->GetSelectedActor()->GetRootComponent() == WidgetTargetComponent)
		{
			if (ALevelState* LevelState = ALevelState::Get())
			{
				LevelState->MarkAsDirty();
			}
		}
		else
		{
			SodaActor->MarkAsDirty();
		}
	}

	WidgetTargetComponent->SetWorldTransform(WidgetTransform, false, nullptr, ETeleportType::ResetPhysics);
	
	return true;
}


void USodaGameViewportClient::DrawAxes(FViewport* InViewport, FCanvas* Canvas, const FRotator* InRotation, EAxisList::Type InAxis)
{
	FMatrix ViewTM = FMatrix::Identity;

	FSceneViewFamilyContext DragStartViewFamily(FSceneViewFamily::ConstructionValues(
		Viewport,
		GetWorld()->Scene,
		EngineShowFlags)
		.SetRealtimeUpdate(true));
	FVector	 TmpViewLocation;
	FRotator TmpViewRotation;
	FSceneView* DragStartView = GetWorld()->GetFirstLocalPlayerFromController()->CalcSceneView(&DragStartViewFamily, TmpViewLocation, TmpViewRotation, Viewport);
	ViewTM = FRotationMatrix(TmpViewRotation);

	/*
	if (bUsingOrbitCamera)
	{
		FViewportCameraTransform& ViewTransform = GetViewTransform();
		ViewTM = FRotationMatrix(ViewTransform.ComputeOrbitMatrix().InverseFast().Rotator());
	}
	else
	{
		ViewTM = FRotationMatrix(GetViewRotation());
	}
	*/


	if (InRotation)
	{
		ViewTM = FRotationMatrix(*InRotation);
	}

	const int32 SizeX = InViewport->GetSizeXY().X / Canvas->GetDPIScale();
	const int32 SizeY = InViewport->GetSizeXY().Y / Canvas->GetDPIScale();

	const FIntPoint AxisOrigin(30, SizeY - 30);
	const float AxisSize = 25.f;

	UFont* Font = GEngine->GetSmallFont();
	int32 XL, YL;
	StringSize(Font, XL, YL, TEXT("Z"));

	FVector AxisVec;
	FIntPoint AxisEnd;
	FCanvasLineItem LineItem;
	FCanvasTextItem TextItem(FVector2D::ZeroVector, FText::GetEmpty(), Font, FLinearColor::White);
	if ((InAxis & EAxisList::X) == EAxisList::X)
	{
		AxisVec = AxisSize * ViewTM.InverseTransformVector(FVector(1, 0, 0));
		AxisEnd = AxisOrigin + FIntPoint(AxisVec.Y, -AxisVec.Z);
		LineItem.SetColor(FLinearColor::Red);
		TextItem.SetColor(FLinearColor::Red);
		LineItem.Draw(Canvas, AxisOrigin, AxisEnd);
		TextItem.Text = FText::FromString("X");
		TextItem.Draw(Canvas, FVector2D(AxisEnd.X + 2, AxisEnd.Y - 0.5 * YL));
	}

	if ((InAxis & EAxisList::Y) == EAxisList::Y)
	{
		AxisVec = AxisSize * ViewTM.InverseTransformVector(FVector(0, 1, 0));
		AxisEnd = AxisOrigin + FIntPoint(AxisVec.Y, -AxisVec.Z);
		LineItem.SetColor(FLinearColor::Green);
		TextItem.SetColor(FLinearColor::Green);
		LineItem.Draw(Canvas, AxisOrigin, AxisEnd);
		TextItem.Text = FText::FromString("Y");
		TextItem.Draw(Canvas, FVector2D(AxisEnd.X + 2, AxisEnd.Y - 0.5 * YL));

	}

	if ((InAxis & EAxisList::Z) == EAxisList::Z)
	{
		AxisVec = AxisSize * ViewTM.InverseTransformVector(FVector(0, 0, 1));
		AxisEnd = AxisOrigin + FIntPoint(AxisVec.Y, -AxisVec.Z);
		LineItem.SetColor(FLinearColor::Blue);
		TextItem.SetColor(FLinearColor::Blue);
		LineItem.Draw(Canvas, AxisOrigin, AxisEnd);
		TextItem.Text = FText::FromString("Z");
		TextItem.Draw(Canvas, FVector2D(AxisEnd.X + 2, AxisEnd.Y - 0.5 * YL));
	}
}


void USodaGameViewportClient::DrawBoundingBox(const FSceneView* View, FPrimitiveDrawInterface* PDI, const AActor* Actor, const FLinearColor& Color)
{
	check(Actor != NULL);

	FBox ActorBox = Actor->GetComponentsBoundingBox(false);

	if (!ActorBox.IsValid)
	{
		if (UBillboardComponent* Sprite = Actor->FindComponentByClass<UBillboardComponent>())
		{
			ActorBox = Sprite->Bounds.GetBox();
		}
	}

	// If we didn't get a valid bounding box, just make a little one around the actor location
	if (!ActorBox.IsValid)
	{
		ActorBox = FBox(Actor->GetActorLocation(), Actor->GetActorLocation());
	}

	DrawSelectBox(PDI, ActorBox.ExpandBy(20), 0.4, Color, SDPG_Foreground);
}

bool USodaGameViewportClient::UpdateDropPreviewActor(int32 MouseX, int32 MouseY)
{
	if (!HasDropPreviewActor())
	{
		return false;
	}

	// If the mouse did not move, there is no need to update anything
	//if (MouseX == DropPreviewMouseX && MouseY == DropPreviewMouseY)
	//{
	//	return false;
	//}

	// Update the cached mouse position
	//DropPreviewMouseX = MouseX;
	//DropPreviewMouseY = MouseY;

	const FEditorUtils::FActorPositionTraceResult TraceResult = FEditorUtils::TraceWorldForPosition(GetWorld(), FVector2D(MouseX, MouseY));
	if (TraceResult.State != FEditorUtils::FActorPositionTraceResult::HitSuccess)
	{
		return false;
	}

	if (!TraceResult.HitActor.Get())
	{
		return false;
	}

	DropPreviewActor->SetActorLocation(TraceResult.Location);

	FVector Test = DropPreviewActor->GetActorLocation();
	
	return true;
}

void USodaGameViewportClient::DestroyDropPreviewActor()
{
	if (HasDropPreviewActor())
	{
		GetWorld()->DestroyActor(DropPreviewActor);
	}
}

bool USodaGameViewportClient::HasDropPreviewActor() const
{
	return IsValid(DropPreviewActor);
}

bool USodaGameViewportClient::DropActorAtCoordinates(int32 MouseX, int32 MouseY, UClass* ActorClass, AActor** OutNewActor, bool bAddToFactory)
{
	DestroyDropPreviewActor();

	if (!ActorClass)
	{
		return false;
	}

	const FEditorUtils::FActorPositionTraceResult TraceResult = FEditorUtils::TraceWorldForPosition(GetWorld(), FVector2D(MouseX, MouseY));
	if (TraceResult.State != FEditorUtils::FActorPositionTraceResult::HitSuccess)
	{
		return false;
	}

	AActor* NewActor = nullptr;

	if (bAddToFactory)
	{
		USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
		if (!SodaSubsystem)
		{
			return false;
		}
		ASodaActorFactory* ActorFactory = SodaSubsystem->GetActorFactory();
		NewActor = ActorFactory->SpawnActor(ActorClass, FTransform(FRotator::ZeroRotator, TraceResult.Location, FVector::OneVector));
	}
	else
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnInfo.bDeferConstruction = true;
		//SpawnInfo.Name = ASodaActorFactory::MakeUniqueObjectName(GetWorld()->GetCurrentLevel(), ActorClass);
		FTransform Transform(FRotator::ZeroRotator, TraceResult.Location, FVector::OneVector);
		NewActor = GetWorld()->SpawnActor(ActorClass, &Transform, SpawnInfo);
	}

	if (!IsValid(NewActor))
	{
		return false;
	}

	if (OutNewActor)
	{
		*OutNewActor = NewActor;
	}
	return true;
}


bool USodaGameViewportClient::DropActorAtCoordinates(int32 MouseX, int32 MouseY, const FSpawnCastomActorDelegate& SpawnDelegate, AActor** OutNewActor, bool bAddToFactory)
{
	DestroyDropPreviewActor();

	if (!SpawnDelegate.IsBound())
	{
		return false;
	}

	const FEditorUtils::FActorPositionTraceResult TraceResult = FEditorUtils::TraceWorldForPosition(GetWorld(), FVector2D(MouseX, MouseY));
	if (TraceResult.State != FEditorUtils::FActorPositionTraceResult::HitSuccess)
	{
		return false;
	}

	AActor* NewActor = SpawnDelegate.Execute(FTransform(FRotator::ZeroRotator, TraceResult.Location, FVector::OneVector));

	if (!NewActor)
	{
		return false;
	}

	if (bAddToFactory)
	{
		USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
		ASodaActorFactory* ActorFactory = SodaSubsystem->GetActorFactory();
		ActorFactory->AddActor(NewActor);
	}

	if (OutNewActor)
	{
		*OutNewActor = NewActor;
	}
	return true;
}

bool USodaGameViewportClient::DropPreviewActorAtCoordinates(int32 MouseX, int32 MouseY)
{
	DestroyDropPreviewActor();

	const FEditorUtils::FActorPositionTraceResult TraceResult = FEditorUtils::TraceWorldForPosition(GetWorld(), FVector2D(MouseX, MouseY));
	if (TraceResult.State == FEditorUtils::FActorPositionTraceResult::Failed)
	{
		return false;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.bDeferConstruction = true;
	FTransform Transform(FRotator::ZeroRotator, TraceResult.Location, FVector::OneVector);
	DropPreviewActor = GetWorld()->SpawnActor<APreviewActor>(APreviewActor::StaticClass(), Transform, SpawnInfo);
	// TODO
	/*
	if (DropPreviewActor.IsValid())
	{
		DropPreviewActor->SetPreviwActorClass(ActorClass);
	}
	*/
	
	return IsValid(DropPreviewActor);
}
