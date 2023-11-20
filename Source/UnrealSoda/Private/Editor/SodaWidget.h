// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HitProxies.h"
#include "UObject/GCObject.h"
#include "Soda/SodaTypes.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"
#include "SceneView.h"
#include "Math/UnrealMathUtility.h"
#include "SodaWidget.generated.h"

/*
* TODO: Need refact this shit-code
*/

class FCanvas;
class FPrimitiveDrawInterface;
class FSceneView;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class FMaterialRenderProxy;
class USodaGameViewportClient;

/**
 * IAxisInterface
 */
UINTERFACE(BlueprintType)
class UAxisInterface : public UInterface
{
	GENERATED_BODY()
};
class IAxisInterface
{
	GENERATED_BODY()

public:
	EAxisList::Type AxisList;
};

/**
 * UAxisSphereComponent
 */
UCLASS(ClassGroup = Soda)
class UAxisSphereComponent : public USphereComponent, public IAxisInterface
{
	GENERATED_BODY()
};

/**
 * UAxisBoxComponent
 */
UCLASS(ClassGroup = Soda)
class UAxisBoxComponent : public UBoxComponent, public IAxisInterface
{
	GENERATED_BODY()
};

/**
 * UAxisProceduralMeshComponent
 */
UCLASS(ClassGroup = Soda)
class UAxisProceduralMeshComponent : public UProceduralMeshComponent, public IAxisInterface
{
	GENERATED_BODY()

public:
	void CreateArc(const FVector& Axis0, const FVector& Axis1, const FMatrix & CoordSystem, const FVector& InDirectionToWidget, float InEndAngle, float InStartAngle, float InnerRadius, float OuterRadius);

protected:
	FVector StoreAxis0;
	FVector StoreAxis1;
	float StoreEndAngle;
	float StoreStartAngle;
	float StoreInnerRadius;
	float StoreOuterRadius;
};

/**
 * UAxisCapsuleComponent
 */
UCLASS(ClassGroup = Soda)
class UAxisCapsuleComponent : public UCapsuleComponent, public IAxisInterface
{
	GENERATED_BODY()
};


/*
 *  Simple struct used to create and group data related to the current window's / viewport's space,
 *  orientation, and scale.
 */
struct FSpaceDescriptor
{
	/*
	*  Creates a new FSpaceDescriptor.
	*
	*  @param View         The virtual view for the space.
	*  @param Viewport     The real viewport for the space.
	*  @param InLocation   The location of the camera in the virtual space.
	*  @param ExternalScale ExtraScale Factor to apply to the UniformScale to allow for the Widget To Get Scaled.
	*/
	FSpaceDescriptor(const FSceneView* View, const USodaGameViewportClient* Viewport, const FVector& InLocation, const float ExternalScale) :
		bIsPerspective(View->ViewMatrices.GetProjectionMatrix().M[3][3] < 1.0f),
		bIsLocalSpace(false/*Viewport->GetWidgetCoordSystemSpace() == soda::COORD_Local*/),
		bIsOrthoXY(!bIsPerspective && FMath::Abs(View->ViewMatrices.GetViewMatrix().M[2][2]) > 0.0f),
		bIsOrthoXZ(!bIsPerspective && FMath::Abs(View->ViewMatrices.GetViewMatrix().M[1][2]) > 0.0f),
		bIsOrthoYZ(!bIsPerspective && FMath::Abs(View->ViewMatrices.GetViewMatrix().M[0][2]) > 0.0f),
		UniformScale(ExternalScale* View->WorldToScreen(InLocation).W* (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0])),
		Scale(CreateScale())
	{
	}

	FSpaceDescriptor() {}

	// Wether or not the view is perspective.
	bool bIsPerspective;

	// Whether or not the view is in local space.
	bool bIsLocalSpace;

	// Whether or not the view is orthogonal to the XY plane.
	bool bIsOrthoXY;

	// Whether or not the view is orthogonal to the XZ plane.
	bool bIsOrthoXZ;

	// Whether or not the view is orthogonal to the YZ plane.
	bool bIsOrthoYZ;

	// The uniform scale for the space.
	float UniformScale;

	// The scale vector for the space based on orientation.
	FVector Scale;

	/*
	 *  Used to determine whether or not the X axis should be drawn.
	 *
	 *  @param AxisToDraw   The desired axis to draw.
	 *
	 *  @return True if the axis should be drawn. False otherwise.
	 */
	bool ShouldDrawAxisX(const EAxisList::Type AxisToDraw)
	{
		return ShouldDrawAxis(EAxisList::X, AxisToDraw, bIsOrthoYZ);
	}

	/*
	 *  Used to determine whether or not the Y axis should be drawn.
	 *
	 *  @param AxisToDraw   The desired axis to draw.
	 *
	 *  @return True if the axis should be drawn. False otherwise.
	 */
	bool ShouldDrawAxisY(const EAxisList::Type AxisToDraw)
	{
		return ShouldDrawAxis(EAxisList::Y, AxisToDraw, bIsOrthoXZ);
	}

	/*
	 *  Used to determine whether or not the Z axis should be drawn.
	 *
	 *  @param AxisToDraw   The desired axis to draw.
	 *
	 *  @return True if the axis should be drawn. False otherwise.
	 */
	bool ShouldDrawAxisZ(const EAxisList::Type AxisToDraw)
	{
		return ShouldDrawAxis(EAxisList::Z, AxisToDraw, bIsOrthoXY);
	}

private:

	/*
	 *  Creates a space scale vector from the determined orientation and uniform scale.
	 *
	 *  @return Space scale vector.
	 */
	FVector CreateScale()
	{
		if (bIsOrthoXY)
		{
			return FVector(UniformScale, UniformScale, 1.0f);
		}
		else if (bIsOrthoXZ)
		{
			return FVector(UniformScale, 1.0f, UniformScale);
		}
		else if (bIsOrthoYZ)
		{
			return FVector(1.0f, UniformScale, UniformScale);
		}
		else
		{
			return FVector(UniformScale, UniformScale, UniformScale);
		}
	}

	/*
	 *  Used to determine whether or not a specific axis should be drawn.
	 *
	 *  @param AxisToCheck  The axis to check.
	 *  @param AxisToDraw   The desired axis to draw.
	 *  @param bIsOrtho     Whether or not the axis to check is orthogonal to the viewing orientation.
	 *
	 *  @return True if the axis should be drawn. False otherwise.
	 */
	bool ShouldDrawAxis(const EAxisList::Type AxisToCheck, const EAxisList::Type AxisToDraw, const bool bIsOrtho)
	{
		return (AxisToCheck & AxisToDraw) && (bIsPerspective || bIsLocalSpace || !bIsOrtho);
	}
};


/**
 * ASodaWidget
 * TODO: Full refact this facking crutch. 
 * TODO: Move all redering functions to components render proxy
 */
UCLASS(ClassGroup = Soda)
class ASodaWidget : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	constexpr static float AXIS_LENGTH                         = 35.0f;
	constexpr static float TRANSLATE_ROTATE_AXIS_CIRCLE_RADIUS = 20.0f;
	constexpr static float TWOD_AXIS_CIRCLE_RADIUS             = 10.0f;
	constexpr static float INNER_AXIS_CIRCLE_RADIUS            = 48.0f;
	constexpr static float OUTER_AXIS_CIRCLE_RADIUS            = 56.0f;
	constexpr static float ROTATION_TEXT_RADIUS                = 75.0f;
	constexpr static int32 AXIS_CIRCLE_SIDES                   = 24;
	constexpr static float ARCALL_RELATIVE_INNER_SIZE          = 0.75f;
	constexpr static float AXIS_LENGTH_SCALE_OFFSET            = 5.0f;

	constexpr static float TransformWidgetSizeAdjustment = 0.0;
	constexpr static bool bAllowArcballRotate = false;
	constexpr static bool bAllowScreenRotate = false;
	constexpr static bool bRotGridEnabled = true;

	constexpr static float ExternalScale = 1.0;

	constexpr static uint8 LargeInnerAlpha = 0x3f;
	constexpr static uint8 SmallInnerAlpha = 0x0f;
	constexpr static uint8 LargeOuterAlpha = 0x7f;
	constexpr static uint8 SmallOuterAlpha = 0x0f;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void TickWidget(USodaGameViewportClient* ViewportClient);
	void DrawHUD(FCanvas* Canvas);
	void Render(const FSceneView* View, FPrimitiveDrawInterface* PDI, USodaGameViewportClient* ViewportClient);

	static EAxisList::Type GetAxisListAtScreenPos(UWorld * World, const FVector2D & ScreenPos);

public:
	/**
	 * Converts mouse movement on the screen to widget axis movement/rotation.
	 */
	void ConvertMouseMovementToAxisMovement(FSceneView* InView, USodaGameViewportClient* InViewportClient,
	                                        bool bInUsedDragModifier, FVector& InDiff, FVector& OutDrag,
	                                        FRotator& OutRotation, FVector& OutScale);

	/**
	 * Absolute Translation conversion from mouse movement on the screen to widget axis movement/rotation.
	 */
	void AbsoluteTranslationConvertMouseMovementToAxisMovement(FSceneView* InView,
	                                                          USodaGameViewportClient* InViewportClient,
	                                                           const FVector& InLocation,
	                                                           const FVector2D& InMousePosition, FVector& OutDrag,
	                                                           FRotator& OutRotation, FVector& OutScale);

	/** 
	 * Grab the initial offset again first time input is captured
	 */
	void ResetInitialTranslationOffset(void)
	{
		bAbsoluteTranslationInitialOffsetCached = false;
	}

	/** Only some modes support Absolute Translation Movement.  Check current mode */
	static bool AllowsAbsoluteTranslationMovement(soda::EWidgetMode WidgetMode);

	/** Only some modes support Absolute Rotation Movement.  Check current mode */
	static bool AllowsAbsoluteRotationMovement(soda::EWidgetMode WidgetMode, EAxisList::Type InAxisType);

	/**
	 * Sets the axis currently being moused over.  Typically called by FMouseDeltaTracker or FLevelEditorViewportClient.
	 *
	 * @param	InCurrentAxis	The new axis value.
	 */
	void SetCurrentAxis(EAxisList::Type InCurrentAxis)
	{
		CurrentAxis = InCurrentAxis;
	}

	/**
	 * @return	The axis currently being moused over.
	 */
	EAxisList::Type GetCurrentAxis() const
	{
		return CurrentAxis;
	}

	/** 
	 * @return	The widget origin in viewport space.
	 */
	FVector2D GetOrigin() const
	{
		return RenderData.Origin;
	}

	/**
	 * @return	The mouse drag start position in viewport space.
	 */
	void SetDragStartPosition(const FVector2D& Position)
	{
		DragStartPos = Position;
		LastDragPos  = DragStartPos;
	}

	/**
	 * Returns whether we are actively dragging
	 */
	bool IsDragging(void) const
	{
		return bDragging;
	}

	/**
	 * Sets if we are currently engaging the widget in dragging
	 */
	void SetDragging(const bool InDragging)
	{
		bDragging = InDragging;
	}

	/**
	 * Sets if we are currently engaging the widget in dragging
	 */
	void SetSnapEnabled(const bool InSnapEnabled)
	{
		bSnapEnabled = InSnapEnabled;
	}

	/**
	 * Gets the axis to draw based on the current widget mode
	 */
	EAxisList::Type GetAxisToDraw(soda::EWidgetMode WidgetMode) const;

	/** @return true if the widget is disabled */
	bool IsWidgetDisabled() const;

	/** Updates the delta rotation on the widget */
	void UpdateDeltaRotation();

	/** Resets the total delta rotation back to zero */
	void ResetDeltaRotation()
	{
		TotalDeltaRotation = 0;
	}

	/** @return the rotation speed of the widget */
	static float GetRotationSpeed()
	{
		return (2.f * (float)PI) / 360.f;
	}

private:
	void ConvertMouseToAxis_Translate(FVector2D DragDir, FVector& InOutDelta, FVector& OutDrag) const;
	void ConvertMouseToAxis_Rotate(FVector2D TangentDir, FVector2D DragDir, FSceneView* InView,
	                               USodaGameViewportClient* InViewportClient, FVector& InOutDelta, FRotator& OutRotation);
	void ConvertMouseToAxis_Scale(FVector2D DragDir, FVector& InOutDelta, FVector& OutScale);
	void ConvertMouseToAxis_TranslateRotateZ(FVector2D TangentDir, FVector2D DragDir, FVector& InOutDelta,
	                                         FVector& OutDrag, FRotator& OutRotation);
	void ConvertMouseToAxis_WM_2D(FVector2D TangentDir, FVector2D DragDir, FVector& InOutDelta, FVector& OutDrag,
	                              FRotator& OutRotation);

	struct FAbsoluteMovementParams
	{
		/** The normal of the plane to project onto */
		FVector PlaneNormal;
		/** A vector that represent any displacement we want to mute (remove an axis if we're doing axis movement)*/
		FVector NormalToRemove;
		/** The current position of the widget */
		FVector Position;

		//Coordinate System Axes
		FVector XAxis;
		FVector YAxis;
		FVector ZAxis;

		//true if camera movement is locked to the object
		bool bMovementLockedToCamera;

		//Direction in world space to the current mouse location
		FVector PixelDir;
		//Direction in world space of the middle of the camera
		FVector CameraDir;
		FVector EyePos;

		//whether to snap the requested positionto the grid
		bool bPositionSnapping;
	};

	void AbsoluteConvertMouseToAxis_Translate(FSceneView* InView, const FMatrix& InputCoordSystem, FAbsoluteMovementParams& InOutParams, FVector& OutDrag);
	/**
	 * Render helper functions
	 */
	void RenderGrid(const FSceneView* View, FPrimitiveDrawInterface* PDI, USodaGameViewportClient* ViewportClient);
	void Render_Axis(const FSceneView* View, FPrimitiveDrawInterface* PDI, EAxisList::Type InAxis, FMatrix& InMatrix,
	                 UMaterialInterface* InMaterial, const FLinearColor& InColor, FVector2D& OutAxisDir,
	                 const FVector& InScale, bool bCubeHead = false, float AxisLengthOffset = 0);
	void Render_Cube(FPrimitiveDrawInterface* PDI, const FMatrix& InMatrix, const UMaterialInterface* InMaterial, const FVector& InScale);

	void Render_Translate(const FSceneView* View, FPrimitiveDrawInterface* PDI, USodaGameViewportClient* ViewportClient, const FVector& InLocation);
	void Render_Rotate(const FSceneView* View, FPrimitiveDrawInterface* PDI, USodaGameViewportClient* ViewportClient, const FVector& InLocation);
	void Render_Scale(const FSceneView* View, FPrimitiveDrawInterface* PDI, USodaGameViewportClient* ViewportClient, const FVector& InLocation);

	struct FThickArcParams
	{
		FThickArcParams(FPrimitiveDrawInterface* InPDI, const FVector& InPosition, UMaterialInterface* InMaterial,
		                const float InInnerRadius, const float InOuterRadius)
		    : Position(InPosition)
		    , PDI(InPDI)
		    , Material(InMaterial)
		    , InnerRadius(InInnerRadius)
		    , OuterRadius(InOuterRadius)
		{}

		/** The current position of the widget */
		FVector Position;

		//interface for Drawing
		FPrimitiveDrawInterface* PDI;

		//Material to use to render
		UMaterialInterface* Material;

		//Radii
		float InnerRadius;
		float OuterRadius;
	};

	/**
	 * Returns the Delta from the current position that the absolute movement system wants the object to be at
	 * @param InParams - Structure containing all the information needed for absolute movement
	 * @return - The requested delta from the current position
	 */
	FVector GetAbsoluteTranslationDelta(const FAbsoluteMovementParams& InParams);
	/**
	 * Returns the offset from the initial selection point
	 */
	FVector GetAbsoluteTranslationInitialOffset(const FVector& InNewPosition, const FVector& InCurrentPosition);

	/**
	 * Returns true if we're in Local Space editing mode or editing BSP (which uses the World axes anyway
	 */
	bool IsRotationLocalSpace() const;

	/**
	 * Returns how far we have just rotated
	 */
	float GetDeltaRotation() const;

	/**
	 * If actively dragging, draws a ring representing the potential rotation of the selected objects, snap ticks, and "delta" markers
	 * If not actively dragging, draws a quarter ring representing the closest quadrant to the camera
	 * @param View - Information about the scene/camera/etc
	 * @param PDI - Drawing interface
	 * @param InAxis - Enumeration of axis to rotate about
	 * @param InLocation - The Origin of the widget
	 * @param Axis0 - The Axis that describes a 0 degree rotation
	 * @param Axis1 - The Axis that describes a 90 degree rotation
	 * @param InColor - The color associated with the axis of rotation
	 * @param InScale - Multiplier to maintain a constant screen size for rendering the widget
	 * @param OutAxisDir - Viewport-space direction of rotation arc chord is placed here
	 */
	void DrawRotationArc(const FSceneView* View, FPrimitiveDrawInterface* PDI, EAxisList::Type InAxis,
	                     const FVector& InLocation, const FVector& Axis0, const FVector& Axis1,
	                     const FColor& InColor, const float InScale,
	                     FVector2D& OutAxisEnd);

	/**
	 * If actively dragging, draws a ring representing the potential rotation of the selected objects, snap ticks, and "delta" markers
	 * If not actively dragging, draws a quarter ring representing the closest quadrant to the camera
	 * @param View - Information about the scene/camera/etc
	 * @param PDI - Drawing interface
	 * @param InAxis - Enumeration of axis to rotate about
	 * @param InLocation - The Origin of the widget
	 * @param Axis0 - The Axis that describes a 0 degree rotation
	 * @param Axis1 - The Axis that describes a 90 degree rotation
	 * @param InStartAngle - The starting angle about (Axis0^Axis1) to render the arc
	 * @param InEndAngle - The ending angle about (Axis0^Axis1) to render the arc
	 * @param InColor - The color associated with the axis of rotation
	 * @param InScale - Multiplier to maintain a constant screen size for rendering the widget
	 */
	void DrawPartialRotationArc(const FSceneView* View, FPrimitiveDrawInterface* PDI, EAxisList::Type InAxis,
	                            const FVector& InLocation, const FVector& Axis0, const FVector& Axis1,
	                            const float InStartAngle, const float InEndAngle, const FColor& InColor,
	                            const float InScale);

	/**
	 * Renders a portion of an arc for the rotation widget
	 * @param InParams - Material, Radii, etc
	 * @param InStartAxis - Start of the arc
	 * @param InEndAxis - End of the arc
	 * @param InColor - Color to use for the arc
	 */
	void DrawThickArc(const FThickArcParams& InParams, const FVector& Axis0, const FVector& Axis1,
	                  const float InStartAngle, const float InEndAngle, const FColor& InColor, bool bIsOrtho);

	/**
	 * Draws protractor like ticks where the rotation widget would snap too.
	 * Also, used to draw the wider axis tick marks
	 * @param PDI - Drawing interface
	 * @param InLocation - The Origin of the widget
	 * @param Axis0 - The Axis that describes a 0 degree rotation
	 * @param Axis1 - The Axis that describes a 90 degree rotation
	 * @param InAngle - The Angle to rotate about the axis of rotation, the vector (Axis0 ^ Axis1)
	 * @param InColor - The color to use for line/poly drawing
	 * @param InScale - Multiplier to maintain a constant screen size for rendering the widget
	 * @param InWidthPercent - The percent of the distance between the outer ring and inner ring to use for tangential thickness
	 * @param InPercentSize - The percent of the distance between the outer ring and inner ring to use for radial distance
	 */
	void DrawSnapMarker(FPrimitiveDrawInterface* PDI, const FVector& InLocation, const FVector& Axis0,
	                    const FVector& Axis1, const FColor& InColor, const float InScale,
	                    const float InWidthPercent = 0.0f, const float InPercentSize = 1.0f);

	/**
	 * Draw Start/Stop Marker to show delta rotations along the arc of rotation
	 * @param PDI - Drawing interface
	 * @param InLocation - The Origin of the widget
	 * @param Axis0 - The Axis that describes a 0 degree rotation
	 * @param Axis1 - The Axis that describes a 90 degree rotation
	 * @param InAngle - The Angle to rotate about the axis of rotation, the vector (Axis0 ^ Axis1)
	 * @param InColor - The color to use for line/poly drawing
	 * @param InScale - Multiplier to maintain a constant screen size for rendering the widget
	 */
	void DrawStartStopMarker(FPrimitiveDrawInterface* PDI, const FVector& InLocation, const FVector& Axis0,
	                         const FVector& Axis1, const float InAngle, const FColor& InColor, const float InScale);

	/**
	 * Caches off HUD text to display after 3d rendering is complete
	 * @param View - Information about the scene/camera/etc
	 * @param PDI - Drawing interface
	 * @param InLocation - The Origin of the widget
	 * @param Axis0 - The Axis that describes a 0 degree rotation
	 * @param Axis1 - The Axis that describes a 90 degree rotation
	 * @param AngleOfAngle - angle we've rotated so far (in degrees)
	 * @param InScale - Multiplier to maintain a constant screen size for rendering the widget
	 */
	void CacheRotationHUDText(const FSceneView* View, FPrimitiveDrawInterface* PDI, const FVector& InLocation,
	                          const FVector& Axis0, const FVector& Axis1, const float AngleOfChange,
	                          const float InScale);

	/**
	 * Gets the axis to use when converting mouse movement, accounting for Ortho views.
	 *
	 * @param InDiff Difference vector to determine dominant axis.
	 * @param ViewportType Type of viewport for ortho checks.
	 *
	 * @return Index of the dominant axis.
	 */
	uint32 GetDominantAxisIndex(const FVector& InDiff, USodaGameViewportClient* ViewportClient) const;


	void DrawColoredSphere(FPrimitiveDrawInterface* PDI, const FVector& Center, const FRotator& Orientation,
	                       FColor Color, const FVector& Radii, int32 NumSides, int32 NumRings,
	                       const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriority,
	                       bool bDisableBackfaceCulling);

	/** The axis currently being moused over */
	EAxisList::Type CurrentAxis;


	/** Viewport space direction vectors of the axes on the widget */
	FVector2D XAxisDir, YAxisDir, ZAxisDir;
	/** Drag start position in viewport space */
	FVector2D DragStartPos;
	/** Last mouse position in viewport space */
	FVector2D LastDragPos;
	enum
	{
		AXIS_ARROW_SEGMENTS = 16
	};

	/** Materials and colors to be used when drawing the items for each axis */
	UMaterialInterface* TransparentPlaneMaterialXY;
	UMaterialInterface* GridMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialX;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialY;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialZ;

	UPROPERTY()
	UMaterialInstanceDynamic* CurrentAxisMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* OpaquePlaneMaterialXY;

	UPROPERTY()
	TArray<UPrimitiveComponent*> WidgetAxisComponents;

	FLinearColor AxisColorX, AxisColorY, AxisColorZ;
	FLinearColor ScreenAxisColor;
	FColor PlaneColorXY, ScreenSpaceColor, CurrentColor;
	FColor ArcBallColor;

	// Data computed in the Render() func
	struct FRenderData
	{
		FMatrix CoordSystem = FMatrix::Identity;
		soda::ECoordSystem CoordSystemSpace = soda::COORD_World;
		float UniformScale = 1.0;
		FVector WidgetLocation;
		soda::EWidgetMode WidgetMode = soda::EWidgetMode::WM_None;
		FVector DirectionToWidget;
		FSpaceDescriptor Space;

		/** Viewport space origin location of the widget */
		FVector2D Origin = FVector2D::ZeroVector;

		bool bUpdated = false;
	};

	FRenderData RenderData;


	bool bPrevMirrorAxis0 = false;
	bool bPrevMirrorAxis1 = false;
	bool bPrevMirrorAxis2 = false;

	//location in the viewport to render the hud string
	FVector2D HUDInfoPos;
	//string to be displayed on top of the viewport
	FString HUDString;

	/** Whether Absolute Translation cache position has been captured */
	bool bAbsoluteTranslationInitialOffsetCached;
	/** The initial offset where the widget was first clicked */
	FVector InitialTranslationOffset;
	/** The initial position of the widget before it was clicked */
	FVector InitialTranslationPosition;
	/** Whether or not the widget is actively dragging */
	bool bDragging;
	/** Whether or not snapping is enabled for all actors */
	bool bSnapEnabled;
	/** Whether we are drawing the full ring in rotation mode (ortho viewports only) */
	bool bIsOrthoDrawingFullRing;

	/** Total delta rotation applied since the widget was dragged */
	float TotalDeltaRotation;

	/** Current delta rotation applied to the rotation widget */
	float CurrentDeltaRotation;
};
