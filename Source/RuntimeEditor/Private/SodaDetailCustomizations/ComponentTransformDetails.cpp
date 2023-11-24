// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaDetailCustomizations/ComponentTransformDetails.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Textures/SlateIcon.h"
#include "SodaStyleSet.h"
#include "RuntimePropertyEditor/IDetailChildrenBuilder.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
#include "UObject/UnrealType.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/ConfigCacheIni.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SCheckBox.h"
//#include "Editor/UnrealEdEngine.h"
//#include "Kismet2/ComponentEditorUtils.h"
//#include "Editor.h"
//#include "UnrealEdGlobals.h"
#include "RuntimePropertyEditor/DetailLayoutBuilder.h"
#include "RuntimePropertyEditor/DetailCategoryBuilder.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Input/SRotatorInputBox.h"
//#include "ScopedTransaction.h"
#include "RuntimePropertyEditor/IPropertyUtilities.h"
#include "Math/UnitConversion.h"
#include "Widgets/Input/NumericUnitTypeInterface.inl"
//#include "Settings/EditorProjectSettings.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Algo/Transform.h"
#include "Misc/NotifyHook.h"

#include "SodaDetailCustomizations/ComponentEditorUtils.h"
#include "IEditableObject.h"

#define LOCTEXT_NAMESPACE "FComponentTransformDetails"

namespace soda
{

/*
class FScopedSwitchWorldForObject
{
public:
	FScopedSwitchWorldForObject( UObject* Object )
		: PrevWorld( NULL )
	{
		bool bRequiresPlayWorld = false;
		if( GUnrealEd->PlayWorld && !GIsPlayInEditorWorld )
		{
			UPackage* ObjectPackage = Object->GetOutermost();
			bRequiresPlayWorld = ObjectPackage->HasAnyPackageFlags(PKG_PlayInEditor);
		}

		if( bRequiresPlayWorld )
		{
			PrevWorld = SetPlayInEditorWorld( GUnrealEd->PlayWorld );
		}
	}

	~FScopedSwitchWorldForObject()
	{
		if( PrevWorld )
		{
			RestoreEditorWorld( PrevWorld );
		}
	}

private:
	UWorld* PrevWorld;
};
*/

static USceneComponent* GetSceneComponentFromDetailsObject(UObject* InObject)
{
	AActor* Actor = Cast<AActor>(InObject);
	if(Actor)
	{
		return Actor->GetRootComponent();
	}
	
	return Cast<USceneComponent>(InObject);
}

FComponentTransformDetails::FComponentTransformDetails( const TArray< TWeakObjectPtr<UObject> >& InSelectedObjects, /*const FSelectedActorInfo& InSelectedActorInfo,*/ IDetailLayoutBuilder& DetailBuilder)
	: TNumericUnitTypeInterface(EUnit::Centimeters)
	//, SelectedActorInfo( InSelectedActorInfo )
	, SelectedObjects( InSelectedObjects )
	, NotifyHook( DetailBuilder.GetPropertyUtilities()->GetNotifyHook() )
	, bPreserveScaleRatio( false )
	, bEditingRotationInUI( false )
	, bIsSliderTransaction( false )
	, HiddenFieldMask( 0 )
{
	GConfig->GetBool(TEXT("SelectionDetails"), TEXT("PreserveScaleRatio"), bPreserveScaleRatio, GEditorPerProjectIni);
	//FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FComponentTransformDetails::OnObjectsReplaced);
}

FComponentTransformDetails::~FComponentTransformDetails()
{
	//FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
}

TSharedRef<SWidget> FComponentTransformDetails::BuildTransformFieldLabel( ETransformField::Type TransformField )
{
	FText Label;
	switch( TransformField )
	{
	case ETransformField::Rotation:
		Label = LOCTEXT( "RotationLabel", "Rotation");
		break;
	case ETransformField::Scale:
		Label = LOCTEXT( "ScaleLabel", "Scale" );
		break;
	case ETransformField::Location:
	default:
		Label = LOCTEXT("LocationLabel", "Location");
		break;
	}

	FMenuBuilder MenuBuilder( true, NULL, NULL );

	FUIAction SetRelativeLocationAction
	(
		FExecuteAction::CreateSP( this, &FComponentTransformDetails::OnSetAbsoluteTransform, TransformField, false ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &FComponentTransformDetails::IsAbsoluteTransformChecked, TransformField, false )
	);

	FUIAction SetWorldLocationAction
	(
		FExecuteAction::CreateSP( this, &FComponentTransformDetails::OnSetAbsoluteTransform, TransformField, true ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &FComponentTransformDetails::IsAbsoluteTransformChecked, TransformField, true )
	);

	MenuBuilder.BeginSection( TEXT("TransformType"), FText::Format( LOCTEXT("TransformType", "{0} Type"), Label ) );

	MenuBuilder.AddMenuEntry
	(
		FText::Format( LOCTEXT( "RelativeLabel", "Relative"), Label ),
		FText::Format( LOCTEXT( "RelativeLabel_ToolTip", "{0} is relative to its parent"), Label ),
		FSlateIcon(),
		SetRelativeLocationAction,
		NAME_None, 
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry
	(
		FText::Format( LOCTEXT( "WorldLabel", "World"), Label ),
		FText::Format( LOCTEXT( "WorldLabel_ToolTip", "{0} is relative to the world"), Label ),
		FSlateIcon(),
		SetWorldLocationAction,
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.EndSection();

	TSharedRef<SWidget> NameContent =
		SNew(SComboButton)
		.ContentPadding(0)
		.MenuContent()
		[
			MenuBuilder.MakeWidget()
		]
		.ButtonContent()
		[
			SNew( SBox )
			.Padding( FMargin( 0.0f, 0.0f, 2.0f, 0.0f ) )
			.MinDesiredWidth(50.f)
			[
				SNew(STextBlock)
				.Text(this, &FComponentTransformDetails::GetTransformFieldText, TransformField)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];

	if(TransformField == ETransformField::Scale)
	{
		NameContent =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				NameContent
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
			[
				// Add a checkbox to toggle between preserving the ratio of x,y,z components of scale when a value is entered
				SNew(SCheckBox)
				.IsChecked(this, &FComponentTransformDetails::IsPreserveScaleRatioChecked)
				.IsEnabled(this, &FComponentTransformDetails::GetIsEnabled)
				.OnCheckStateChanged(this, &FComponentTransformDetails::OnPreserveScaleRatioToggled)
				.Style(FSodaStyle::Get(), "TransparentCheckBox")
				.ToolTipText(LOCTEXT("PreserveScaleToolTip", "When locked, scales uniformly based on the current xyz scale values so the object maintains its shape in each direction when scaled"))
				[
					SNew(SImage)
					.Image(this, &FComponentTransformDetails::GetPreserveScaleRatioImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			];
	}

	return NameContent;
}

FText FComponentTransformDetails::GetTransformFieldText( ETransformField::Type TransformField ) const
{
	switch (TransformField)
	{
	case ETransformField::Location:
		return GetLocationText();
		break;
	case ETransformField::Rotation:
		return GetRotationText();
		break;
	case ETransformField::Scale:
		return GetScaleText();
		break;
	default:
		return FText::GetEmpty();
		break;
	}
}

bool FComponentTransformDetails::OnCanCopy( ETransformField::Type TransformField ) const
{
	// We can only copy values if the whole field is set.  If multiple values are defined we do not copy since we are unable to determine the value
	switch (TransformField)
	{
	case ETransformField::Location:
		return CachedLocation.IsSet();
		break;
	case ETransformField::Rotation:
		return CachedRotation.IsSet();
		break;
	case ETransformField::Scale:
		return CachedScale.IsSet();
		break;
	default:
		return false;
		break;
	}
}

void FComponentTransformDetails::OnCopy( ETransformField::Type TransformField )
{
	CacheTransform();

	FString CopyStr;
	switch (TransformField)
	{
	case ETransformField::Location:
		CopyStr = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), CachedLocation.X.GetValue(), CachedLocation.Y.GetValue(), CachedLocation.Z.GetValue());
		break;
	case ETransformField::Rotation:
		CopyStr = FString::Printf(TEXT("(Pitch=%f,Yaw=%f,Roll=%f)"), CachedRotation.Y.GetValue(), CachedRotation.Z.GetValue(), CachedRotation.X.GetValue());
		break;
	case ETransformField::Scale:
		CopyStr = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), CachedScale.X.GetValue(), CachedScale.Y.GetValue(), CachedScale.Z.GetValue());
		break;
	default:
		break;
	}

	if( !CopyStr.IsEmpty() )
	{
		FPlatformApplicationMisc::ClipboardCopy( *CopyStr );
	}
}

void FComponentTransformDetails::OnPaste( ETransformField::Type TransformField )
{
	FString PastedText;
	FPlatformApplicationMisc::ClipboardPaste(PastedText);

	switch (TransformField)
	{
		case ETransformField::Location:
		{
			FVector Location;
			if (Location.InitFromString(PastedText))
			{
				//FScopedTransaction Transaction(LOCTEXT("PasteLocation", "Paste Location"));
				OnSetTransform(ETransformField::Location, EAxisList::All, Location, false, true);
			}
		}
		break;
	case ETransformField::Rotation:
		{
			FRotator Rotation;
			PastedText.ReplaceInline(TEXT("Pitch="), TEXT("P="));
			PastedText.ReplaceInline(TEXT("Yaw="), TEXT("Y="));
			PastedText.ReplaceInline(TEXT("Roll="), TEXT("R="));
			if (Rotation.InitFromString(PastedText))
			{
				//FScopedTransaction Transaction(LOCTEXT("PasteRotation", "Paste Rotation"));
				OnSetTransform(ETransformField::Rotation, EAxisList::All, Rotation.Euler(), false, true);
			}
		}
		break;
	case ETransformField::Scale:
		{
			FVector Scale;
			if (Scale.InitFromString(PastedText))
			{
				//FScopedTransaction Transaction(LOCTEXT("PasteScale", "Paste Scale"));
				OnSetTransform(ETransformField::Scale, EAxisList::All, Scale, false, true);
			}
		}
		break;
	default:
		break;
	}
}

FUIAction FComponentTransformDetails::CreateCopyAction( ETransformField::Type TransformField ) const
{
	return
		FUIAction
		(
			FExecuteAction::CreateSP(const_cast<FComponentTransformDetails*>(this), &FComponentTransformDetails::OnCopy, TransformField ),
			FCanExecuteAction::CreateSP(const_cast<FComponentTransformDetails*>(this), &FComponentTransformDetails::OnCanCopy, TransformField )
		);
}

FUIAction FComponentTransformDetails::CreatePasteAction( ETransformField::Type TransformField ) const
{
	return 
		 FUIAction( FExecuteAction::CreateSP(const_cast<FComponentTransformDetails*>(this), &FComponentTransformDetails::OnPaste, TransformField ) );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FComponentTransformDetails::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	UClass* SceneComponentClass = USceneComponent::StaticClass();
		
	FSlateFontInfo FontInfo = IDetailLayoutBuilder::GetDetailFont();

	const bool bHideLocationField = ( HiddenFieldMask & ( 1 << ETransformField::Location ) ) != 0;
	const bool bHideRotationField = ( HiddenFieldMask & ( 1 << ETransformField::Rotation ) ) != 0;
	const bool bHideScaleField = ( HiddenFieldMask & ( 1 << ETransformField::Scale ) ) != 0;

	// Location
	if(!bHideLocationField)
	{
		TSharedPtr<INumericTypeInterface<FVector::FReal>> TypeInterface;
		if( FUnitConversion::Settings().ShouldDisplayUnits() )
		{
			TypeInterface = SharedThis(this);
		}

		ChildrenBuilder.AddCustomRow( LOCTEXT("LocationFilter", "Location") )
		.RowTag("Location")
		.CopyAction( CreateCopyAction( ETransformField::Location ) )
		.PasteAction( CreatePasteAction( ETransformField::Location ) )
		.OverrideResetToDefault(FResetToDefaultOverride::Create(TAttribute<bool>(this, &FComponentTransformDetails::GetLocationResetVisibility), FSimpleDelegate::CreateSP(this, &FComponentTransformDetails::OnLocationResetClicked)))
		.PropertyHandleList({ GeneratePropertyHandle(USceneComponent::GetRelativeLocationPropertyName(), ChildrenBuilder) })
		.NameContent()
		.VAlign(VAlign_Center)
		[
			BuildTransformFieldLabel( ETransformField::Location )
		]
		.ValueContent()
		.MinDesiredWidth(125.0f * 3.0f)
		.MaxDesiredWidth(125.0f * 3.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SNumericVectorInputBox<FVector::FReal>)
			.X(this, &FComponentTransformDetails::GetLocationX)
			.Y(this, &FComponentTransformDetails::GetLocationY)
			.Z(this, &FComponentTransformDetails::GetLocationZ)
			.bColorAxisLabels(true)
			.IsEnabled(this, &FComponentTransformDetails::GetIsEnabled)
			.OnXChanged(this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Location, EAxisList::X, false)
			.OnYChanged(this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Location, EAxisList::Y, false)
			.OnZChanged(this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Location, EAxisList::Z, false)
			.OnXCommitted(this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Location, EAxisList::X, true)
			.OnYCommitted(this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Location, EAxisList::Y, true)
			.OnZCommitted(this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Location, EAxisList::Z, true)
			.Font(FontInfo)
			.TypeInterface(TypeInterface)
			.AllowSpin(SelectedObjects.Num() == 1)
			.SpinDelta(1)
			.OnBeginSliderMovement(this, &FComponentTransformDetails::OnBeginLocationSlider)
			.OnEndSliderMovement(this, &FComponentTransformDetails::OnEndLocationSlider)
		];
	}
	
	// Rotation
	if(!bHideRotationField)
	{
		TSharedPtr<INumericTypeInterface<FRotator::FReal>> TypeInterface;
		if( FUnitConversion::Settings().ShouldDisplayUnits() )
		{
			TypeInterface = MakeShareable( new TNumericUnitTypeInterface<FRotator::FReal>(EUnit::Degrees) );
		}

		ChildrenBuilder.AddCustomRow( LOCTEXT("RotationFilter", "Rotation") )
		.RowTag("Rotation")
		.CopyAction( CreateCopyAction(ETransformField::Rotation) )
		.PasteAction( CreatePasteAction(ETransformField::Rotation) )
		.OverrideResetToDefault(FResetToDefaultOverride::Create(TAttribute<bool>(this, &FComponentTransformDetails::GetRotationResetVisibility), FSimpleDelegate::CreateSP(this, &FComponentTransformDetails::OnRotationResetClicked)))
		.PropertyHandleList({ GeneratePropertyHandle(USceneComponent::GetRelativeRotationPropertyName(), ChildrenBuilder) })
		.NameContent()
		.VAlign(VAlign_Center)
		[
			BuildTransformFieldLabel(ETransformField::Rotation)
		]
		.ValueContent()
		.MinDesiredWidth(125.0f * 3.0f)
		.MaxDesiredWidth(125.0f * 3.0f)
		.VAlign(VAlign_Center)
		[
			SNew( SNumericRotatorInputBox<FRotator::FReal> )
			.AllowSpin( SelectedObjects.Num() == 1 ) 
			.Roll( this, &FComponentTransformDetails::GetRotationX )
			.Pitch( this, &FComponentTransformDetails::GetRotationY )
			.Yaw( this, &FComponentTransformDetails::GetRotationZ )
			.bColorAxisLabels( true )
			.IsEnabled( this, &FComponentTransformDetails::GetIsEnabled )
			.OnBeginSliderMovement( this, &FComponentTransformDetails::OnBeginRotationSlider )
			.OnEndSliderMovement( this, &FComponentTransformDetails::OnEndRotationSlider )
			.OnRollChanged( this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Rotation, EAxisList::X, false )
			.OnPitchChanged( this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Rotation, EAxisList::Y, false )
			.OnYawChanged( this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Rotation, EAxisList::Z, false )
			.OnRollCommitted( this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Rotation, EAxisList::X, true )
			.OnPitchCommitted( this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Rotation, EAxisList::Y, true )
			.OnYawCommitted( this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Rotation, EAxisList::Z, true )
			.TypeInterface( TypeInterface )
			.Font( FontInfo )
		];
	}
	
	// Scale
	if(!bHideScaleField)
	{
		ChildrenBuilder.AddCustomRow( LOCTEXT("ScaleFilter", "Scale") )
		.RowTag("Scale")
		.CopyAction( CreateCopyAction(ETransformField::Scale) )
		.PasteAction( CreatePasteAction(ETransformField::Scale) )
		.OverrideResetToDefault(FResetToDefaultOverride::Create(TAttribute<bool>(this, &FComponentTransformDetails::GetScaleResetVisibility), FSimpleDelegate::CreateSP(this, &FComponentTransformDetails::OnScaleResetClicked)))
		.PropertyHandleList({ GeneratePropertyHandle(USceneComponent::GetRelativeScale3DPropertyName(), ChildrenBuilder) })
		.NameContent()
		.VAlign(VAlign_Center)
		[
			BuildTransformFieldLabel(ETransformField::Scale)
		]
		.ValueContent()
		.MinDesiredWidth(125.0f * 3.0f)
		.MaxDesiredWidth(125.0f * 3.0f)
		.VAlign(VAlign_Center)
		[
			SNew( SNumericVectorInputBox<FVector::FReal> )
			.X( this, &FComponentTransformDetails::GetScaleX )
			.Y( this, &FComponentTransformDetails::GetScaleY )
			.Z( this, &FComponentTransformDetails::GetScaleZ )
			.bColorAxisLabels( true )
			.IsEnabled( this, &FComponentTransformDetails::GetIsEnabled )
			.OnXChanged( this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Scale, EAxisList::X, false )
			.OnYChanged( this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Scale, EAxisList::Y, false )
			.OnZChanged( this, &FComponentTransformDetails::OnSetTransformAxis, ETextCommit::Default, ETransformField::Scale, EAxisList::Z, false )
			.OnXCommitted( this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Scale, EAxisList::X, true )
			.OnYCommitted( this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Scale, EAxisList::Y, true )
			.OnZCommitted( this, &FComponentTransformDetails::OnSetTransformAxis, ETransformField::Scale, EAxisList::Z, true )
			.ContextMenuExtenderX( this, &FComponentTransformDetails::ExtendXScaleContextMenu )
			.ContextMenuExtenderY( this, &FComponentTransformDetails::ExtendYScaleContextMenu )
			.ContextMenuExtenderZ( this, &FComponentTransformDetails::ExtendZScaleContextMenu )
			.Font( FontInfo )
			.AllowSpin( SelectedObjects.Num() == 1 )
			.SpinDelta( 0.0025f )
			.OnBeginSliderMovement( this, &FComponentTransformDetails::OnBeginScaleSlider )
			.OnEndSliderMovement(this, &FComponentTransformDetails::OnEndScaleSlider)
		];
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FComponentTransformDetails::Tick( float DeltaTime ) 
{
	CacheTransform();
	if (!FixedDisplayUnits.IsSet())
	{
		CacheCommonLocationUnits();
	}
}

void FComponentTransformDetails::CacheCommonLocationUnits()
{
	float LargestValue = 0.f;
	if (CachedLocation.X.IsSet() && CachedLocation.X.GetValue() > LargestValue)
	{
		LargestValue = CachedLocation.X.GetValue();
	}
	if (CachedLocation.Y.IsSet() && CachedLocation.Y.GetValue() > LargestValue)
	{
		LargestValue = CachedLocation.Y.GetValue();
	}
	if (CachedLocation.Z.IsSet() && CachedLocation.Z.GetValue() > LargestValue)
	{
		LargestValue = CachedLocation.Z.GetValue();
	}

	SetupFixedDisplay(LargestValue);
}

TSharedPtr<IPropertyHandle> FComponentTransformDetails::GeneratePropertyHandle(FName PropertyName, IDetailChildrenBuilder& ChildrenBuilder)
{
	// Try finding the property handle in the details panel's property map first.
	IDetailLayoutBuilder& LayoutBuilder = ChildrenBuilder.GetParentCategory().GetParentLayout();
	TSharedPtr<IPropertyHandle> PropertyHandle = LayoutBuilder.GetProperty(PropertyName, USceneComponent::StaticClass());
	if (!PropertyHandle || !PropertyHandle->IsValidHandle())
	{
		// If it wasn't found, add a collapsed row which contains the property node.
		TArray<UObject*> SceneComponents;
		Algo::Transform(SelectedObjects, SceneComponents, [](TWeakObjectPtr<UObject> Obj) { return GetSceneComponentFromDetailsObject(Obj.Get()); });
		PropertyHandle = LayoutBuilder.AddObjectPropertyData(SceneComponents, PropertyName);
		CachedHandlesObjects.Append(SceneComponents);
	}

	PropertyHandles.Add(PropertyHandle);
	return PropertyHandle;
}

void FComponentTransformDetails::UpdatePropertyHandlesObjects(const TArray<UObject*> NewSceneComponents)
{
	// Cached the old handles objects.
	CachedHandlesObjects.Reset(NewSceneComponents.Num());
	Algo::Transform(NewSceneComponents, CachedHandlesObjects, [](UObject* Obj) { return TWeakObjectPtr<UObject>(Obj); });

	for (TSharedPtr<IPropertyHandle>& Handle : PropertyHandles)
	{
		if (Handle && Handle->IsValidHandle())
		{
			Handle->ReplaceOuterObjects(NewSceneComponents);
		}
	}
}

bool FComponentTransformDetails::GetIsEnabled() const
{
	return true; //!GEditor->HasLockedActors() || SelectedActorInfo.NumSelected == 0;
}

const FSlateBrush* FComponentTransformDetails::GetPreserveScaleRatioImage() const
{
	return bPreserveScaleRatio ? FSodaStyle::GetBrush( TEXT("Icons.Lock") ) : FSodaStyle::GetBrush( TEXT("Icons.Unlock") ) ;
}

ECheckBoxState FComponentTransformDetails::IsPreserveScaleRatioChecked() const
{
	return bPreserveScaleRatio ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FComponentTransformDetails::OnPreserveScaleRatioToggled( ECheckBoxState NewState )
{
	//bPreserveScaleRatio = (NewState == ECheckBoxState::Checked) ? true : false;
	//GConfig->SetBool(TEXT("SelectionDetails"), TEXT("PreserveScaleRatio"), bPreserveScaleRatio, GEditorPerProjectIni);
}

FText FComponentTransformDetails::GetLocationText() const
{
	return bAbsoluteLocation ? LOCTEXT( "AbsoluteLocation", "Absolute Location" ) : LOCTEXT( "Location", "Location" );
}

FText FComponentTransformDetails::GetRotationText() const
{
	return bAbsoluteRotation ? LOCTEXT( "AbsoluteRotation", "Absolute Rotation" ) : LOCTEXT( "Rotation", "Rotation" );
}

FText FComponentTransformDetails::GetScaleText() const
{
	return bAbsoluteScale ? LOCTEXT( "AbsoluteScale", "Absolute Scale" ) : LOCTEXT( "Scale", "Scale" );
}

void FComponentTransformDetails::OnSetAbsoluteTransform(ETransformField::Type TransformField, bool bAbsoluteEnabled)
{
	FBoolProperty* AbsoluteProperty = nullptr;
	FText TransactionText;

	switch (TransformField)
	{
	case ETransformField::Location:
		AbsoluteProperty = FindFProperty<FBoolProperty>(USceneComponent::StaticClass(), USceneComponent::GetAbsoluteLocationPropertyName());
		TransactionText = LOCTEXT("ToggleAbsoluteLocation", "Toggle Absolute Location");
		break;
	case ETransformField::Rotation:
		AbsoluteProperty = FindFProperty<FBoolProperty>(USceneComponent::StaticClass(), USceneComponent::GetAbsoluteRotationPropertyName());
		TransactionText = LOCTEXT("ToggleAbsoluteRotation", "Toggle Absolute Rotation");
		break;
	case ETransformField::Scale:
		AbsoluteProperty = FindFProperty<FBoolProperty>(USceneComponent::StaticClass(), USceneComponent::GetAbsoluteScalePropertyName());
		TransactionText = LOCTEXT("ToggleAbsoluteScale", "Toggle Absolute Scale");
		break;
	default:
		return;
	}

	bool bBeganTransaction = false;
	TArray<UObject*> ModifiedObjects;
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		TWeakObjectPtr<UObject> ObjectPtr = SelectedObjects[ObjectIndex];
		if (ObjectPtr.IsValid())
		{
			UObject* Object = ObjectPtr.Get();
			USceneComponent* SceneComponent = GetSceneComponentFromDetailsObject(Object);
			if (SceneComponent)
			{
				bool bOldValue = TransformField == ETransformField::Location ? SceneComponent->IsUsingAbsoluteLocation() : (TransformField == ETransformField::Rotation ? SceneComponent->IsUsingAbsoluteRotation() : SceneComponent->IsUsingAbsoluteScale());

				if (bOldValue == bAbsoluteEnabled)
				{
					// Already the desired value
					continue;
				}

				if (!bBeganTransaction)
				{
					// NOTE: One transaction per change, not per actor
					//GEditor->BeginTransaction(TransactionText);
					bBeganTransaction = true;
				}

				//FScopedSwitchWorldForObject WorldSwitcher(Object);

				if (SceneComponent->HasAnyFlags(RF_DefaultSubObject))
				{
					// Default subobjects must be included in any undo/redo operations
					SceneComponent->SetFlags(RF_Transactional);
				}

				//SceneComponent->PreEditChange(AbsoluteProperty);
				if (IEditableObject* EditableObject = Cast<IEditableObject>(SceneComponent))
				{
					EditableObject->RuntimePreEditChange(AbsoluteProperty);
				}

				if (NotifyHook)
				{
					NotifyHook->NotifyPreChange(AbsoluteProperty);
				}

				switch (TransformField)
				{
				case ETransformField::Location:
					SceneComponent->SetUsingAbsoluteLocation(bAbsoluteEnabled);

					// Update RelativeLocation to maintain/stabilize position when switching between relative and world.
					if (SceneComponent->GetAttachParent())
					{
						if (SceneComponent->IsUsingAbsoluteLocation())
						{
							SceneComponent->SetRelativeLocation_Direct(SceneComponent->GetComponentTransform().GetTranslation());
						}
						else
						{
							FTransform ParentToWorld = SceneComponent->GetAttachParent()->GetSocketTransform(SceneComponent->GetAttachSocketName());
							FTransform RelativeTM = SceneComponent->GetComponentTransform().GetRelativeTransform(ParentToWorld);
							SceneComponent->SetRelativeLocation_Direct(RelativeTM.GetTranslation());
						}
					}
					break;
				case ETransformField::Rotation:
					SceneComponent->SetUsingAbsoluteRotation(bAbsoluteEnabled);
					break;
				case ETransformField::Scale:
					SceneComponent->SetUsingAbsoluteScale(bAbsoluteEnabled);
					break;
				}

				ModifiedObjects.Add(Object);
			}
		}
	}

	if (bBeganTransaction)
	{
		FPropertyChangedEvent PropertyChangedEvent(AbsoluteProperty, EPropertyChangeType::ValueSet, MakeArrayView(ModifiedObjects));

		for (UObject* Object : ModifiedObjects)
		{
			USceneComponent* SceneComponent = GetSceneComponentFromDetailsObject(Object);
			if (SceneComponent)
			{
				//SceneComponent->PostEditChangeProperty(PropertyChangedEvent);
				if (IEditableObject* EditableObject = Cast<IEditableObject>(SceneComponent))
				{
					EditableObject->RuntimePostEditChangeProperty(PropertyChangedEvent);
				}

				// If it's a template, propagate the change out to any current instances of the object
				if (SceneComponent->IsTemplate())
				{
					bool NewValue = bAbsoluteEnabled;
					bool OldValue = !NewValue;
					TSet<USceneComponent*> UpdatedInstances;
					FComponentEditorUtils::PropagateDefaultValueChange(SceneComponent, AbsoluteProperty, OldValue, NewValue, UpdatedInstances);
				}
			}
		}

		if (NotifyHook)
		{
			NotifyHook->NotifyPostChange(PropertyChangedEvent, AbsoluteProperty);
		}

		//GEditor->EndTransaction();

		//GUnrealEd->RedrawLevelEditingViewports();
	}
}

bool FComponentTransformDetails::IsAbsoluteTransformChecked(ETransformField::Type TransformField, bool bAbsoluteEnabled) const
{
	switch (TransformField)
	{
	case ETransformField::Location:
		return bAbsoluteLocation == bAbsoluteEnabled;
		break;
	case ETransformField::Rotation:
		return bAbsoluteRotation == bAbsoluteEnabled;
		break;
	case ETransformField::Scale:
		return bAbsoluteScale == bAbsoluteEnabled;
		break;
	default:
		return false;
		break;
	}
}

struct FGetRootComponentArchetype
{
	static USceneComponent* Get(UObject* Object)
	{
		auto RootComponent = Object ? GetSceneComponentFromDetailsObject(Object) : nullptr;
		return RootComponent ? Cast<USceneComponent>(RootComponent->GetArchetype()) : nullptr;
	}
};

bool FComponentTransformDetails::GetLocationResetVisibility() const
{
	const USceneComponent* Archetype = FGetRootComponentArchetype::Get(SelectedObjects[0].Get());
	const FVector Data = Archetype ? Archetype->GetRelativeLocation() : FVector::ZeroVector;

	// unset means multiple differing values, so show "Reset to Default" in that case
	return CachedLocation.IsSet() && CachedLocation.X.GetValue() == Data.X && CachedLocation.Y.GetValue() == Data.Y && CachedLocation.Z.GetValue() == Data.Z ? false : true;
}

void FComponentTransformDetails::OnLocationResetClicked()
{
	const FText TransactionName = LOCTEXT("ResetLocation", "Reset Location");
	//FScopedTransaction Transaction(TransactionName);

	const USceneComponent* Archetype = FGetRootComponentArchetype::Get(SelectedObjects[0].Get());
	const FVector Data = Archetype ? Archetype->GetRelativeLocation() : FVector::ZeroVector;

	OnSetTransform(ETransformField::Location, EAxisList::All, Data, false, true);
}

bool FComponentTransformDetails::GetRotationResetVisibility() const
{
	const USceneComponent* Archetype = FGetRootComponentArchetype::Get(SelectedObjects[0].Get());
	const FVector Data = Archetype ? Archetype->GetRelativeRotation().Euler() : FVector::ZeroVector;

	// unset means multiple differing values, so show "Reset to Default" in that case
	return CachedRotation.IsSet() && CachedRotation.X.GetValue() == Data.X && CachedRotation.Y.GetValue() == Data.Y && CachedRotation.Z.GetValue() == Data.Z ? false : true;
}

void FComponentTransformDetails::OnRotationResetClicked()
{
	const FText TransactionName = LOCTEXT("ResetRotation", "Reset Rotation");
	//FScopedTransaction Transaction(TransactionName);

	const USceneComponent* Archetype = FGetRootComponentArchetype::Get(SelectedObjects[0].Get());
	const FVector Data = Archetype ? Archetype->GetRelativeRotation().Euler() : FVector::ZeroVector;

	OnSetTransform(ETransformField::Rotation, EAxisList::All, Data, false, true);
}

bool FComponentTransformDetails::GetScaleResetVisibility() const
{
	const USceneComponent* Archetype = FGetRootComponentArchetype::Get(SelectedObjects[0].Get());
	const FVector Data = Archetype ? Archetype->GetRelativeScale3D() : FVector(1.0f);

	// unset means multiple differing values, so show "Reset to Default" in that case
	return CachedScale.IsSet() && CachedScale.X.GetValue() == Data.X && CachedScale.Y.GetValue() == Data.Y && CachedScale.Z.GetValue() == Data.Z ? false : true;
}

void FComponentTransformDetails::OnScaleResetClicked()
{
	const FText TransactionName = LOCTEXT("ResetScale", "Reset Scale");
	//FScopedTransaction Transaction(TransactionName);

	const USceneComponent* Archetype = FGetRootComponentArchetype::Get(SelectedObjects[0].Get());
	const FVector Data = Archetype ? Archetype->GetRelativeScale3D() : FVector(1.0f);

	OnSetTransform(ETransformField::Scale, EAxisList::All, Data, false, true);
}

void FComponentTransformDetails::ExtendXScaleContextMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection( "ScaleOperations", LOCTEXT( "ScaleOperations", "Scale Operations" ) );
	MenuBuilder.AddMenuEntry( 
		LOCTEXT( "MirrorValueX", "Mirror X" ),  
		LOCTEXT( "MirrorValueX_Tooltip", "Mirror scale value on the X axis" ), 
		FSlateIcon(), 		
		FUIAction( FExecuteAction::CreateSP( this, &FComponentTransformDetails::OnXScaleMirrored ), FCanExecuteAction() )
	);
	MenuBuilder.EndSection();
}

void FComponentTransformDetails::ExtendYScaleContextMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection( "ScaleOperations", LOCTEXT( "ScaleOperations", "Scale Operations" ) );
	MenuBuilder.AddMenuEntry( 
		LOCTEXT( "MirrorValueY", "Mirror Y" ),  
		LOCTEXT( "MirrorValueY_Tooltip", "Mirror scale value on the Y axis" ), 
		FSlateIcon(), 		
		FUIAction( FExecuteAction::CreateSP( this, &FComponentTransformDetails::OnYScaleMirrored ), FCanExecuteAction() )
	);
	MenuBuilder.EndSection();
}

void FComponentTransformDetails::ExtendZScaleContextMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection( "ScaleOperations", LOCTEXT( "ScaleOperations", "Scale Operations" ) );
	MenuBuilder.AddMenuEntry( 
		LOCTEXT( "MirrorValueZ", "Mirror Z" ),  
		LOCTEXT( "MirrorValueZ_Tooltip", "Mirror scale value on the Z axis" ), 
		FSlateIcon(), 		
		FUIAction( FExecuteAction::CreateSP( this, &FComponentTransformDetails::OnZScaleMirrored ), FCanExecuteAction() )
	);
	MenuBuilder.EndSection();
}

void FComponentTransformDetails::OnXScaleMirrored()
{
	FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Mouse);
	//FScopedTransaction Transaction(LOCTEXT("MirrorActorScaleX", "Mirror actor scale X"));
	OnSetTransform(ETransformField::Scale, EAxisList::X, FVector(1.0f), true, true);
}

void FComponentTransformDetails::OnYScaleMirrored()
{
	FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Mouse);
	//FScopedTransaction Transaction(LOCTEXT("MirrorActorScaleY", "Mirror actor scale Y"));
	OnSetTransform(ETransformField::Scale, EAxisList::Y, FVector(1.0f), true, true);
}

void FComponentTransformDetails::OnZScaleMirrored()
{
	FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Mouse);
	//FScopedTransaction Transaction(LOCTEXT("MirrorActorScaleZ", "Mirror actor scale Z"));
	OnSetTransform(ETransformField::Scale, EAxisList::Z, FVector(1.0f), true, true);
}

void FComponentTransformDetails::CacheTransform()
{
	FVector CurLoc;
	FRotator CurRot;
	FVector CurScale;

	for( int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex )
	{
		TWeakObjectPtr<UObject> ObjectPtr = SelectedObjects[ObjectIndex];
		if( ObjectPtr.IsValid() )
		{
			UObject* Object = ObjectPtr.Get();
			USceneComponent* SceneComponent = GetSceneComponentFromDetailsObject( Object );

			FVector Loc;
			FRotator Rot;
			FVector Scale;
			if( SceneComponent )
			{
				Loc = SceneComponent->GetRelativeLocation();
				FRotator* FoundRotator = ObjectToRelativeRotationMap.Find(SceneComponent);
				Rot = (bEditingRotationInUI && !Object->IsTemplate() && FoundRotator) ? *FoundRotator : SceneComponent->GetRelativeRotation();
				Scale = SceneComponent->GetRelativeScale3D();

				if( ObjectIndex == 0 )
				{
					// Cache the current values from the first actor to see if any values differ among other actors
					CurLoc = Loc;
					CurRot = Rot;
					CurScale = Scale;

					CachedLocation.Set( Loc );
					CachedRotation.Set( Rot );
					CachedScale.Set( Scale );

					bAbsoluteLocation = SceneComponent->IsUsingAbsoluteLocation();
					bAbsoluteScale = SceneComponent->IsUsingAbsoluteScale();
					bAbsoluteRotation = SceneComponent->IsUsingAbsoluteRotation();
				}
				else if( CurLoc != Loc || CurRot != Rot || CurScale != Scale )
				{
					// Check which values differ and unset the different values
					CachedLocation.X = Loc.X == CurLoc.X && CachedLocation.X.IsSet() ? Loc.X : TOptional<FVector::FReal>();
					CachedLocation.Y = Loc.Y == CurLoc.Y && CachedLocation.Y.IsSet() ? Loc.Y : TOptional<FVector::FReal>();
					CachedLocation.Z = Loc.Z == CurLoc.Z && CachedLocation.Z.IsSet() ? Loc.Z : TOptional<FVector::FReal>();

					CachedRotation.X = Rot.Roll == CurRot.Roll && CachedRotation.X.IsSet() ? Rot.Roll : TOptional<FRotator::FReal>();
					CachedRotation.Y = Rot.Pitch == CurRot.Pitch && CachedRotation.Y.IsSet() ? Rot.Pitch : TOptional<FRotator::FReal>();
					CachedRotation.Z = Rot.Yaw == CurRot.Yaw && CachedRotation.Z.IsSet() ? Rot.Yaw : TOptional<FRotator::FReal>();

					CachedScale.X = Scale.X == CurScale.X && CachedScale.X.IsSet() ? Scale.X : TOptional<FVector::FReal>();
					CachedScale.Y = Scale.Y == CurScale.Y && CachedScale.Y.IsSet() ? Scale.Y : TOptional<FVector::FReal>();
					CachedScale.Z = Scale.Z == CurScale.Z && CachedScale.Z.IsSet() ? Scale.Z : TOptional<FVector::FReal>();

					// If all values are unset all values are different and we can stop looking
					const bool bAllValuesDiffer = !CachedLocation.IsSet() && !CachedRotation.IsSet() && !CachedScale.IsSet();
					if( bAllValuesDiffer )
					{
						break;
					}
				}
			}
		}
	}
}

FVector FComponentTransformDetails::GetAxisFilteredVector(EAxisList::Type Axis, const FVector& NewValue, const FVector& OldValue)
{
	return FVector((Axis & EAxisList::X) ? NewValue.X : OldValue.X,
		(Axis & EAxisList::Y) ? NewValue.Y : OldValue.Y,
		(Axis & EAxisList::Z) ? NewValue.Z : OldValue.Z);
}

void FComponentTransformDetails::OnSetTransform(ETransformField::Type TransformField, EAxisList::Type Axis, FVector NewValue, bool bMirror, bool bCommitted)
{
	if (!bCommitted && SelectedObjects.Num() > 1)
	{
		// Ignore interactive changes when we have more than one selected object
		return;
	}

	FText TransactionText;
	FProperty* ValueProperty = nullptr;
	FProperty* AxisProperty = nullptr;
	
	switch (TransformField)
	{
	case ETransformField::Location:
		TransactionText = LOCTEXT("OnSetLocation", "Set Location");
		ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeLocationPropertyName());
		
		// Only set axis property for single axis set
		if (Axis == EAxisList::X)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, X));
		}
		else if (Axis == EAxisList::Y)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Y));
		}
		else if (Axis == EAxisList::Z)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Z));
		}
		break;
	case ETransformField::Rotation:
		TransactionText = LOCTEXT("OnSetRotation", "Set Rotation");
		ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeRotationPropertyName());
		
		// Only set axis property for single axis set
		if (Axis == EAxisList::X)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FRotator>::Get(), GET_MEMBER_NAME_CHECKED(FRotator, Roll));
		}
		else if (Axis == EAxisList::Y)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FRotator>::Get(), GET_MEMBER_NAME_CHECKED(FRotator, Pitch));
		}
		else if (Axis == EAxisList::Z)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FRotator>::Get(), GET_MEMBER_NAME_CHECKED(FRotator, Yaw));
		}
		break;
	case ETransformField::Scale:
		TransactionText = LOCTEXT("OnSetScale", "Set Scale");
		ValueProperty = FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeScale3DPropertyName());

		// If keep scale is set, don't set axis property
		if (!bPreserveScaleRatio && Axis == EAxisList::X)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, X));
		}
		else if (!bPreserveScaleRatio && Axis == EAxisList::Y)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Y));
		}
		else if (!bPreserveScaleRatio && Axis == EAxisList::Z)
		{
			AxisProperty = FindFProperty<FFloatProperty>(TBaseStructure<FVector>::Get(), GET_MEMBER_NAME_CHECKED(FVector, Z));
		}
		break;
	default:
		return;
	}

	bool bBeganTransaction = false;
	TArray<UObject*> ModifiedObjects;

	FPropertyChangedEvent PropertyChangedEvent(ValueProperty, !bCommitted ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet, MakeArrayView(ModifiedObjects));
	FEditPropertyChain PropertyChain;

	if (AxisProperty)
	{
		PropertyChain.AddHead(AxisProperty);
	}
	PropertyChain.AddHead(ValueProperty);
	FPropertyChangedChainEvent PropertyChangedChainEvent(PropertyChain, PropertyChangedEvent);

	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		TWeakObjectPtr<UObject> ObjectPtr = SelectedObjects[ObjectIndex];
		if (ObjectPtr.IsValid())
		{
			UObject* Object = ObjectPtr.Get();
			USceneComponent* SceneComponent = GetSceneComponentFromDetailsObject(Object);
			if (SceneComponent)
			{
				AActor* EditedActor = SceneComponent->GetOwner();
				const bool bIsEditingTemplateObject = Object->IsTemplate();

				FVector OldComponentValue;
				FVector NewComponentValue;

				switch (TransformField)
				{
				case ETransformField::Location:
					OldComponentValue = SceneComponent->GetRelativeLocation();
					break;
				case ETransformField::Rotation:
					// Pull from the actual component or from the cache
					OldComponentValue = SceneComponent->GetRelativeRotation().Euler();
					if (bEditingRotationInUI && !bIsEditingTemplateObject && ObjectToRelativeRotationMap.Find(SceneComponent))
					{
						OldComponentValue = ObjectToRelativeRotationMap.Find(SceneComponent)->Euler();
					}
					break;
				case ETransformField::Scale:
					OldComponentValue = SceneComponent->GetRelativeScale3D();
					break;
				}

				// Set the incoming value
				if (bMirror)
				{
					NewComponentValue = GetAxisFilteredVector(Axis, -OldComponentValue, OldComponentValue);
				}
				else
				{
					NewComponentValue = GetAxisFilteredVector(Axis, NewValue, OldComponentValue);
				}

				// If we're committing during a slider transaction then we need to force it, in order that PostEditChangeChainProperty be called.
				// Note: this will even happen if the slider hasn't changed the value.
				if (OldComponentValue != NewComponentValue || (bCommitted && bIsSliderTransaction))
				{
					if (!bBeganTransaction && bCommitted)
					{
						// NOTE: One transaction per change, not per actor
						//GEditor->BeginTransaction(TransactionText);
						bBeganTransaction = true;
					}

					//FScopedSwitchWorldForObject WorldSwitcher(Object);

					if (bCommitted)
					{
						if (!bIsEditingTemplateObject)
						{
							/*
							// Broadcast the first time an actor is about to move
							GEditor->BroadcastBeginObjectMovement(*SceneComponent);
							if (EditedActor && EditedActor->GetRootComponent() == SceneComponent)
							{
								GEditor->BroadcastBeginObjectMovement(*EditedActor);
							}
							*/
						}

						if (SceneComponent->HasAnyFlags(RF_DefaultSubObject))
						{
							// Default subobjects must be included in any undo/redo operations
							SceneComponent->SetFlags(RF_Transactional);
						}
					}

					// Have to downcast here because of function overloading and inheritance not playing nicely
					/*
					((UObject*)SceneComponent)->PreEditChange(PropertyChain);
					if (EditedActor && EditedActor->GetRootComponent() == SceneComponent)
					{
						((UObject*)EditedActor)->PreEditChange(PropertyChain);
					}
					*/

					if (IEditableObject* EditableObject = Cast<IEditableObject>((UObject*)SceneComponent))
					{
						EditableObject->RuntimePreEditChange(PropertyChain);
					}
					if (EditedActor && EditedActor->GetRootComponent() == SceneComponent)
					{
						if (IEditableObject* EditableObject = Cast<IEditableObject>((UObject*)EditedActor))
						{
							EditableObject->RuntimePreEditChange(PropertyChain);
						}
					}

					if (NotifyHook)
					{
						NotifyHook->NotifyPreChange(ValueProperty);
					}

					switch (TransformField)
					{
					case ETransformField::Location:
						{
							if (!bIsEditingTemplateObject)
							{
								// Update local cache for restoring later
								ObjectToRelativeRotationMap.FindOrAdd(SceneComponent) = SceneComponent->GetRelativeRotation();
							}

							SceneComponent->SetRelativeLocation(NewComponentValue);

							// Also forcibly set it as the cache may have changed it slightly
							SceneComponent->SetRelativeLocation_Direct(NewComponentValue);

							// If it's a template, propagate the change out to any current instances of the object
							if (bIsEditingTemplateObject)
							{
								TSet<USceneComponent*> UpdatedInstances;
								FComponentEditorUtils::PropagateDefaultValueChange(SceneComponent, ValueProperty, OldComponentValue, NewComponentValue, UpdatedInstances);
							}

							break;
						}
					case ETransformField::Rotation:
						{
							FRotator NewRotation = FRotator::MakeFromEuler(NewComponentValue);

							if (!bIsEditingTemplateObject)
							{
								// Update local cache for restoring later
								ObjectToRelativeRotationMap.FindOrAdd(SceneComponent) = NewRotation;
							}

							SceneComponent->SetRelativeRotationExact(NewRotation);

							// If it's a template, propagate the change out to any current instances of the object
							if (bIsEditingTemplateObject)
							{
								TSet<USceneComponent*> UpdatedInstances;
								FComponentEditorUtils::PropagateDefaultValueChange(SceneComponent, ValueProperty, FRotator::MakeFromEuler(OldComponentValue), NewRotation, UpdatedInstances);
							}

							break;
						}
					case ETransformField::Scale:
						{
							if (bPreserveScaleRatio)
							{
								// If we set a single axis, scale the others
								float Ratio = 0.0f;

								switch (Axis)
								{
								case EAxisList::X:
									if (bIsSliderTransaction)
									{
										Ratio = SliderScaleRatio.X == 0.0f ? SliderScaleRatio.Y : (SliderScaleRatio.Y / SliderScaleRatio.X);
										NewComponentValue.Y = NewComponentValue.X * Ratio;

										Ratio = SliderScaleRatio.X == 0.0f ? SliderScaleRatio.Z : (SliderScaleRatio.Z / SliderScaleRatio.X);
										NewComponentValue.Z = NewComponentValue.X * Ratio;
									}
									else
									{
										Ratio = OldComponentValue.X == 0.0f ? NewComponentValue.Z : NewComponentValue.X / OldComponentValue.X;
										NewComponentValue.Y *= Ratio;
										NewComponentValue.Z *= Ratio;
									}
									break;
								case EAxisList::Y:
									if (bIsSliderTransaction)
									{
										Ratio = SliderScaleRatio.Y == 0.0f ? SliderScaleRatio.X : (SliderScaleRatio.X / SliderScaleRatio.Y);
										NewComponentValue.X = NewComponentValue.Y * Ratio;

										Ratio = SliderScaleRatio.Y == 0.0f ? SliderScaleRatio.Z : (SliderScaleRatio.Z / SliderScaleRatio.Y);
										NewComponentValue.Z = NewComponentValue.Y * Ratio;
									}
									else
									{
										Ratio = OldComponentValue.Y == 0.0f ? NewComponentValue.Z : NewComponentValue.Y / OldComponentValue.Y;
										NewComponentValue.X *= Ratio;
										NewComponentValue.Z *= Ratio;
									}
									break;
								case EAxisList::Z:
									if (bIsSliderTransaction)
									{
										Ratio = SliderScaleRatio.Z == 0.0f ? SliderScaleRatio.X : (SliderScaleRatio.X / SliderScaleRatio.Z);
										NewComponentValue.X = NewComponentValue.Z * Ratio;

										Ratio = SliderScaleRatio.Z == 0.0f ? SliderScaleRatio.Y : (SliderScaleRatio.Y / SliderScaleRatio.Z);
										NewComponentValue.Y = NewComponentValue.Z * Ratio;
									}
									else
									{
										Ratio = OldComponentValue.Z == 0.0f ? NewComponentValue.Z : NewComponentValue.Z / OldComponentValue.Z;
										NewComponentValue.X *= Ratio;
										NewComponentValue.Y *= Ratio;
									}
									break;
								default:
									// Do nothing, this set multiple axis at once
									break;
								}
							}

							SceneComponent->SetRelativeScale3D(NewComponentValue);

							// If it's a template, propagate the change out to any current instances of the object
							if (bIsEditingTemplateObject)
							{
								TSet<USceneComponent*> UpdatedInstances;
								FComponentEditorUtils::PropagateDefaultValueChange(SceneComponent, ValueProperty, OldComponentValue, NewComponentValue, UpdatedInstances);
							}

							break;
						}
					}

					ModifiedObjects.Add(Object);
				}
			}
		}
	}

	if (ModifiedObjects.Num())
	{
		for (UObject* Object : ModifiedObjects)
		{
			USceneComponent* SceneComponent = GetSceneComponentFromDetailsObject(Object);
			USceneComponent* OldSceneComponent = SceneComponent;

			if (SceneComponent)
			{
				AActor* EditedActor = SceneComponent->GetOwner();
				FString SceneComponentPath = SceneComponent->GetPathName(EditedActor);
				
				// This can invalidate OldSceneComponent
				//OldSceneComponent->PostEditChangeChainProperty(PropertyChangedChainEvent);
				if (IEditableObject* EditableObject = Cast<IEditableObject>(OldSceneComponent))
				{
					EditableObject->RuntimePostEditChangeChainProperty(PropertyChangedChainEvent);
				}

				if (!bCommitted)
				{
					const FProperty* ConstValueProperty = ValueProperty;
					SnapshotTransactionBuffer(OldSceneComponent, MakeArrayView(&ConstValueProperty, 1));
				}

				SceneComponent = FindObject<USceneComponent>(EditedActor, *SceneComponentPath);

				if (EditedActor && EditedActor->GetRootComponent() == SceneComponent)
				{
					//EditedActor->PostEditChangeChainProperty(PropertyChangedChainEvent);
					if (IEditableObject* EditableObject = Cast<IEditableObject>(EditedActor))
					{
						EditableObject->RuntimePostEditChangeChainProperty(PropertyChangedChainEvent);
					}

					SceneComponent = FindObject<USceneComponent>(EditedActor, *SceneComponentPath);

					if (!bCommitted && OldSceneComponent != SceneComponent)
					{
						const FProperty* ConstValueProperty = ValueProperty;
						SnapshotTransactionBuffer(SceneComponent, MakeArrayView(&ConstValueProperty, 1));
					}
				}
				
				if (!Object->IsTemplate())
				{
					if (TransformField == ETransformField::Rotation || TransformField == ETransformField::Location)
					{
						FRotator* FoundRotator = ObjectToRelativeRotationMap.Find(OldSceneComponent);

						if (FoundRotator)
						{
							FQuat OldQuat = FoundRotator->GetDenormalized().Quaternion();
							FQuat NewQuat = SceneComponent->GetRelativeRotation().GetDenormalized().Quaternion();

							if (OldQuat.Equals(NewQuat))
							{
								// Need to restore the manually set rotation as it was modified by quat conversion
								SceneComponent->SetRelativeRotation_Direct(*FoundRotator);
							}
						}
					}

					if (bCommitted)
					{
						/*
						// Broadcast when the actor is done moving
						GEditor->BroadcastEndObjectMovement(*SceneComponent);
						if (EditedActor && EditedActor->GetRootComponent() == SceneComponent)
						{
							GEditor->BroadcastEndObjectMovement(*EditedActor);
						}
						*/
					}
				}
			}
		}

		if (NotifyHook)
		{
			NotifyHook->NotifyPostChange(PropertyChangedEvent, ValueProperty);
		}
	}

	if (bCommitted && bBeganTransaction)
	{
		//GEditor->EndTransaction();
		CacheTransform();
	}

	//GUnrealEd->UpdatePivotLocationForSelection();
	//GUnrealEd->SetPivotMovedIndependently(false);
	// Redraw
	//GUnrealEd->RedrawLevelEditingViewports();
}

void FComponentTransformDetails::OnSetTransformAxis(FVector::FReal NewValue, ETextCommit::Type CommitInfo, ETransformField::Type TransformField, EAxisList::Type Axis, bool bCommitted)
{
	FVector NewVector = GetAxisFilteredVector(Axis, FVector(NewValue), FVector::ZeroVector);
	OnSetTransform(TransformField, Axis, NewVector, false, bCommitted);
}

void FComponentTransformDetails::BeginSliderTransaction(FText ActorTransaction, FText ComponentTransaction) const
{
	bool bBeganTransaction = false;
	for (TWeakObjectPtr<UObject> ObjectPtr : SelectedObjects)
	{
		if (ObjectPtr.IsValid())
		{
			UObject* Object = ObjectPtr.Get();

			// Start a new transaction when a slider begins to change
			// We'll end it when the slider is released
			// NOTE: One transaction per change, not per actor
			if (!bBeganTransaction)
			{
				if (Object->IsA<AActor>())
				{
					//GEditor->BeginTransaction(ActorTransaction);
				}
				else
				{
					//GEditor->BeginTransaction(ComponentTransaction);
				}

				bBeganTransaction = true;
			}

			USceneComponent* SceneComponent = GetSceneComponentFromDetailsObject(Object);
			if (SceneComponent)
			{
				//FScopedSwitchWorldForObject WorldSwitcher(Object);

				if (SceneComponent->HasAnyFlags(RF_DefaultSubObject))
				{
					// Default subobjects must be included in any undo/redo operations
					SceneComponent->SetFlags(RF_Transactional);
				}

				// Call modify but not PreEdit, we don't do the proper "Edit" until it's committed
				SceneComponent->Modify();
			}
		}
	}

	// Just in case we couldn't start a new transaction for some reason
	if (!bBeganTransaction)
	{
		//GEditor->BeginTransaction(ActorTransaction);
	}
}

void FComponentTransformDetails::OnBeginRotationSlider()
{
	FText ActorTransaction = LOCTEXT("OnSetRotation", "Set Rotation");
	FText ComponentTransaction = LOCTEXT("OnSetRotation_ComponentDirect", "Modify Component(s)");
	BeginSliderTransaction(ActorTransaction, ComponentTransaction);

	bEditingRotationInUI = true;
	bIsSliderTransaction = true;

	for (TWeakObjectPtr<UObject> ObjectPtr : SelectedObjects)
	{
		if (ObjectPtr.IsValid())
		{
			UObject* Object = ObjectPtr.Get();

			USceneComponent* SceneComponent = GetSceneComponentFromDetailsObject(Object);
			if (SceneComponent)
			{
				//FScopedSwitchWorldForObject WorldSwitcher(Object);

				// Add/update cached rotation value prior to slider interaction
				ObjectToRelativeRotationMap.FindOrAdd(SceneComponent) = SceneComponent->GetRelativeRotation();
			}
		}
	}
}

void FComponentTransformDetails::OnEndRotationSlider(FRotator::FReal NewValue)
{
	// Commit gets called right before this, only need to end the transaction
	bEditingRotationInUI = false;
	bIsSliderTransaction = false;
	//GEditor->EndTransaction();
}

void FComponentTransformDetails::OnBeginLocationSlider()
{
	bIsSliderTransaction = true;
	FText ActorTransaction = LOCTEXT("OnSetLocation", "Set Location");
	FText ComponentTransaction = LOCTEXT("OnSetLocation_ComponentDirect", "Modify Component Location");
	BeginSliderTransaction(ActorTransaction, ComponentTransaction);
}

void FComponentTransformDetails::OnEndLocationSlider(FVector::FReal NewValue)
{
	bIsSliderTransaction = false;
	//GEditor->EndTransaction();
}

void FComponentTransformDetails::OnBeginScaleSlider()
{
	// Assumption: slider isn't usable if multiple objects are selected
	SliderScaleRatio.X = CachedScale.X.GetValue();
	SliderScaleRatio.Y = CachedScale.Y.GetValue();
	SliderScaleRatio.Z = CachedScale.Z.GetValue();

	bIsSliderTransaction = true;
	FText ActorTransaction = LOCTEXT("OnSetScale", "Set Scale");
	FText ComponentTransaction = LOCTEXT("OnSetScale_ComponentDirect", "Modify Component Scale");
	BeginSliderTransaction(ActorTransaction, ComponentTransaction);
}

void FComponentTransformDetails::OnEndScaleSlider(FVector::FReal NewValue)
{
	bIsSliderTransaction = false;
	//GEditor->EndTransaction();
}

void FComponentTransformDetails::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	TArray<UObject*> NewSceneComponents;
	for (const TWeakObjectPtr<UObject> Obj : CachedHandlesObjects)
	{
		if (UObject* Replacement = ReplacementMap.FindRef(Obj.GetEvenIfUnreachable()))
		{
			NewSceneComponents.Add(Replacement);
		}
	}

	if (NewSceneComponents.Num())
	{
		UpdatePropertyHandlesObjects(NewSceneComponents);
	}
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
