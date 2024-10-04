// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/StyleDefaults.h"
#include "Styling/ISlateStyle.h"
#include "Styling/AppStyle.h"

struct FSlateDynamicImageBrush;

/**
 * A collection of named properties that guide the appearance of Slate.
 */
class SODASTYLE_API FSodaStyle
{
public:

	/** 
	* @return the Application Style 
	*
	* NOTE: Until the Editor can be fully updated, calling FSodaStyle::Get() or any of its 
	* static convenience functions will will return the AppStyle instead of the style definied in this class.  
	*
	* Using the AppStyle is preferred in most cases as it allows the style to be changed 
	* on an application level.
	*
	* In cases requiring explicit use of the SodaStyle where a Slate Widget should not take on
	* the appearance of the rest of the application, use FSodaStyle::GetSodaStyle().
	*
	*/

	static const ISlateStyle& Get()
	{
		if (Instance) return *Instance.Get();
		return FAppStyle::Get();
	}

	template< class T >            
	static const T& GetWidgetStyle( FName PropertyName, const ANSICHAR* Specifier = NULL  ) 
	{
		return Get().GetWidgetStyle< T >( PropertyName, Specifier );
	}

	static float GetFloat( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetFloat( PropertyName, Specifier );
	}

	static FVector2D GetVector( FName PropertyName, const ANSICHAR* Specifier = NULL ) 
	{
		return Get().GetVector( PropertyName, Specifier );
	}

	static const FLinearColor& GetColor( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetColor( PropertyName, Specifier );
	}

	static const FSlateColor GetSlateColor( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetSlateColor( PropertyName, Specifier );
	}

	static const FMargin& GetMargin( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetMargin( PropertyName, Specifier );
	}

	static const FSlateBrush* GetBrush( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetBrush( PropertyName, Specifier );
	}

	static const TSharedPtr< FSlateDynamicImageBrush > GetDynamicImageBrush( FName BrushTemplate, FName TextureName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetDynamicImageBrush( BrushTemplate, TextureName, Specifier );
	}

	static const TSharedPtr< FSlateDynamicImageBrush > GetDynamicImageBrush( FName BrushTemplate, const ANSICHAR* Specifier, class UTexture2D* TextureResource, FName TextureName )
	{
		return Get().GetDynamicImageBrush( BrushTemplate, Specifier, TextureResource, TextureName );
	}

	static const TSharedPtr< FSlateDynamicImageBrush > GetDynamicImageBrush( FName BrushTemplate, class UTexture2D* TextureResource, FName TextureName )
	{
		return Get().GetDynamicImageBrush( BrushTemplate, TextureResource, TextureName );
	}

	static const FSlateSound& GetSound( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetSound( PropertyName, Specifier );
	}
	
	static FSlateFontInfo GetFontStyle( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return Get().GetFontStyle( PropertyName, Specifier );
	}

	static const FSlateBrush* GetDefaultBrush()
	{
		return Get().GetDefaultBrush();
	}

	static const FSlateBrush* GetNoBrush()
	{
		return FStyleDefaults::GetNoBrush();
	}

	static const FSlateBrush* GetOptionalBrush( FName PropertyName, const ANSICHAR* Specifier = NULL, const FSlateBrush* const DefaultBrush = FStyleDefaults::GetNoBrush() )
	{
		return Get().GetOptionalBrush( PropertyName, Specifier, DefaultBrush );
	}

	static void GetResources( TArray< const FSlateBrush* >& OutResources )
	{
		return Get().GetResources( OutResources );
	}

	static const FName& GetStyleSetName()
	{
		return Instance->GetStyleSetName();
	}

	/**
	 * Concatenates two FNames.e If A and B are "Path.To" and ".Something"
	 * the result "Path.To.Something".
	 *
	 * @param A  First FName
	 * @param B  Second name
	 *
	 * @return New FName that is A concatenated with B.
	 */
	static FName Join( FName A, const ANSICHAR* B )
	{
		if( B == NULL )
		{
			return A;
		}
		else
		{
			return FName( *( A.ToString() + B ) );
		}
	}

	static void ResetToDefault();


protected:

	static void SetStyle( const TSharedRef< class ISlateStyle >& NewStyle );

private:

	/** Singleton instance of the slate style */
	static TSharedPtr< class ISlateStyle > Instance;
};
