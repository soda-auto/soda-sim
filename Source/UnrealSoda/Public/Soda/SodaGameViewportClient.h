// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/GameViewportClient.h"
#include "Soda/SodaTypes.h"
#include "SceneManagement.h"
#include "Soda/UI/SSodaViewport.h"
#include "SodaGameViewportClient.generated.h"

class ASodaWidget;
class USodaSelection;
class APreviewActor;

DECLARE_DELEGATE_RetVal_OneParam(AActor*, FSpawnCastomActorDelegate, const FTransform&);

enum class EEditorMouseCaptureMode
{
	Default,
	Dragging,
	Capture
};


/**
* USodaGameViewportClient
*/
UCLASS(Within = Engine, transient, config = Engine)
class UNREALSODA_API USodaGameViewportClient : public UGameViewportClient, public FViewElementDrawer
{
	friend ASodaWidget;

	GENERATED_BODY()

public:
	UPROPERTY()
	USodaSelection * Selection;

public:
	/* override UGameViewportClient */
	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;
	virtual void Draw(FViewport* InViewport, FCanvas* SceneCanvas) override;
	virtual void FinalizeViews(class FSceneViewFamily* ViewFamily, const TMap<ULocalPlayer*, FSceneView*>& PlayerViewMap) override;
	virtual EMouseCursor::Type GetCursor(FViewport* Viewport, int32 X, int32 Y) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool InputKey(const FInputKeyEventArgs& InEventArgs) override;
	virtual bool InputAxis(FViewport* Viewport, FInputDeviceId InputDevice, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
	virtual EMouseCaptureMode GetMouseCaptureMode() const override;
	virtual bool LockDuringCapture() override;
	virtual bool ShouldAlwaysLockMouse() override;
	virtual bool HideCursorDuringCapture() const override;

public:
	/* override FViewElementDrawer */
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;

public:
	virtual void InitUI(const TWeakPtr<soda::SSodaViewport>& SodaViewportWidget);
	virtual void Invalidate();
	virtual soda::EUIMode GetGameMode() const;
	TSharedPtr < soda::SSodaViewport> GetSodaViewport() const { return SodaViewport.Pin(); }

	virtual soda::EWidgetMode GetWidgetMode() const;
	virtual void SetWidgetMode(soda::EWidgetMode WidgetMode);
	virtual soda::ECoordSystem GetWidgetCoordSystemSpace() const;
	virtual FMatrix GetLocalCoordinateSystem() const;
	virtual void SetWidgetCoordSystemSpace(soda::ECoordSystem CoordSystem);
	virtual void SetWidgetTarget(USceneComponent * WidgetTargetComponent);
	virtual USceneComponent* GetWidgetTarget() const { return WidgetTargetComponent; }

	void DrawBoundingBox(const FSceneView* View, FPrimitiveDrawInterface* PDI, const AActor* Actor, const FLinearColor& InColor);

	virtual bool UpdateDropPreviewActor(int32 MouseX, int32 MouseY);
	virtual void DestroyDropPreviewActor();
	virtual bool HasDropPreviewActor() const;
	virtual bool DropActorAtCoordinates(int32 MouseX, int32 MouseY, UClass* ActorClass, AActor** OutNewActor, bool bAddToSodaFactory);
	virtual bool DropActorAtCoordinates(int32 MouseX, int32 MouseY, const FSpawnCastomActorDelegate & SpawnDelegate, AActor** OutNewActor, bool bAddToSodaFactory);
	virtual bool DropPreviewActorAtCoordinates(int32 MouseX, int32 MouseY);

	void SetEditorMouseCaptureMode(EEditorMouseCaptureMode InMode) { EditorMouseCaptureMode = InMode; }
	EEditorMouseCaptureMode GetEditorMouseCaptureMode() const { return EditorMouseCaptureMode; }

protected:
	virtual FVector GetWidgetLocation() const;
	virtual FMatrix GetWidgetCoordSystem() const;
	virtual void SetCurrentWidgetAxis(EAxisList::Type AxisList);
	virtual EAxisList::Type GetCurrentWidgetAxis() const;
	virtual void TryStartTracking(const int32 InX, const int32 InY);
	virtual void StopTracking();
	virtual bool InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale);
	virtual void CheckAxisUnderCursor();
	virtual void UpdateMouseDelta();
	virtual bool IsDragging() const;
	void DrawAxes(FViewport* InViewport, FCanvas* Canvas, const FRotator* InRotation, EAxisList::Type InAxis);

protected:
	FVector Start;
	FVector End;

	uint32 CachedMouseX = 0;
	uint32 CachedMouseY = 0;

	bool bIsWidgetDragging = false;

	EEditorMouseCaptureMode EditorMouseCaptureMode = EEditorMouseCaptureMode::Default;
	bool bEditorCaptureModeDragging = false;

	soda::EWidgetMode TrackingWidgetMode;
	EAxisList::Type  CurrentAxis;
	soda::ECoordSystem CoordSystem;

	UPROPERTY()
	TObjectPtr<ASodaWidget> Widget;

	UPROPERTY()
	TObjectPtr<USceneComponent> WidgetTargetComponent = nullptr;

	UPROPERTY()
	TObjectPtr<APreviewActor> DropPreviewActor;

	TWeakPtr<soda::SSodaViewport> SodaViewport;
};
