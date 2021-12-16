/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Scree Layout tools.

TODO : Which screen space does this natively work at?

-------------------------------------------------------------------------
History:
- 22:09:2009: Created by Frank Harrison for console TRC compliance.

*************************************************************************/
#ifndef __ScreenLayoutManager_H__
#define __ScreenLayoutManager_H__

#include "IUIDraw.h" // For alignment flags.

typedef uint8 ScreenLayoutStates;

enum EScreenLayoutFlags // EUIDRAWFLAGS
{
	// Safe area handling
	eSLO_AdaptToSafeArea = 0, // always done unless explicitly requested.
	eSLO_DoNotAdaptToSafeArea = BIT(1),

	// Scale methods
	eSLO_ScaleMethod_None  = BIT(2),
	eSLO_ScaleMethod_WithY = BIT(3),
	eSLO_ScaleMethod_WithX = BIT(4),

	// Utility/common
	eSLO_Default = eSLO_AdaptToSafeArea|eSLO_ScaleMethod_WithY,
	eSLO_FullScreen = eSLO_DoNotAdaptToSafeArea|eSLO_ScaleMethod_None,
};


enum EHUDSafeAreaID
{
	eHSAID_fullscreen = 1,
	eHSAID_title,
	eHSAID_action,
	eHSAID_PS3_trc,
	eHSAID_360_tcr,
	eHSAID_custom,
	eHSAID_curent,
	eHSAID_END,
#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(LINUX64)
	eHSAID_default = eHSAID_fullscreen,
#elif defined(XENON)
	eHSAID_default = eHSAID_360_tcr,
#elif defined(PS3)
	eHSAID_default = eHSAID_PS3_trc,
#else
	// should have a compiler error as eHSAID_default won't be defined.
#endif
};

class ScreenLayoutManager 
{
public:

	 ScreenLayoutManager( );
	~ScreenLayoutManager();

	//-------------------------------------------------------------------
	// States
	//		Controls how the layout manager behaves on position and size
	//		data.
	void               SetState( ScreenLayoutStates flags );
	ScreenLayoutStates GetState( void ) const;

	//-------------------------------------------------------------------
	// Virtual Screen-space dimensions.
	ILINE float GetVirtualWidth( void ) const { return VIRTUAL_SCREEN_WIDTH; };
	ILINE float GetVirtualHeight( void ) const { return VIRTUAL_SCREEN_HEIGHT; };

	//-------------------------------------------------------------------
	// Safe areas
	void SetSafeArea( EHUDSafeAreaID eSafeAreaID );
	void SetSafeArea( const Vec2& safe_area_percentage );
	static void SetSafeArea( IConsoleCmdArgs* pArgs );

	ILINE EHUDSafeAreaID GetCurSafeAreaId( void ) const { return m_curSafeAreaID; };
	const Vec2 GetSafeAreaScreenProportion( EHUDSafeAreaID which = eHSAID_default ) const;
	const Vec2 GetSafeAreaBorderScreenProportion( EHUDSafeAreaID which = eHSAID_default ) const;

	//------------------------------------------------------
	// Screen space & safe area funcs
	void  AdjustToSafeArea( float *fX, float* fY, float* fSizeX = NULL, float* fSizeY = NULL,
	                        const EUIDRAWHORIZONTAL eUIDrawHorizontal = UIDRAWHORIZONTAL_LEFT,
	                        const EUIDRAWVERTICAL   eUIDrawVertical   = UIDRAWVERTICAL_TOP,
	                        const EUIDRAWHORIZONTAL eUIDrawHorizontalDocking = UIDRAWHORIZONTAL_LEFT,
	                        const EUIDRAWVERTICAL   eUIDrawVerticalDocking   = UIDRAWVERTICAL_TOP ) const;

	void AdjustToSafeAreaProportional( Vec2& p, Vec2& s, const float assetAspect,
	                                   const EUIDRAWHORIZONTAL eUIDrawHorizontal,        
	                                   const EUIDRAWVERTICAL   eUIDrawVertical,
	                                   const EUIDRAWHORIZONTAL eUIDrawHorizontalDocking, 
	                                   const EUIDRAWVERTICAL   eUIDrawVerticalDocking  ) const;

	void  ConvertFromVirtualToRenderScreenSpace( float* inout_x, float* inout_y ) const;
	void  ConvertFromRenderToVirtualScreenSpace( float* inout_x, float* inout_y ) const;
	void  ConvertFromVirtualToNormalisedScreenSpace( float* inout_x, float* inout_y ) const;

	//------------------------------------------------------
	// Triggers a HUD event.
	void UpdateHUDCanvasSize( void );

	void GetMemoryUsage(ICrySizer *pSizer) const
	{	
		pSizer->AddObject(this, sizeof(*this));
	}

private :
	// Get the aspect of the area to render into
	const float GetTagetSpaceAspect( void ) const;
	// Get the final size of the asset after the current scale method has been applied.
	// should be called before any aligning is done.
	const Vec2 GetAssetSizeAsProportionOfScreen( const float assetApect, const float canvasAspect, const Vec2& proportionalSize ) const;
	// Get the final position of the asset.
	// Output only alters based on the safe area flags.
	const Vec2 ScaleProportionalPositionInToSafeArea( const Vec2 virtualSpace ) const;

	// Aligns an object around it's pivot.
	// Vec2(out_x,out_y) is the new Top-Left point after adjustment.
	void AlignAroundPivot( float* out_x, float* out_y,
		const float posX, const float posY,
		const float dimX, const float dimY,
		const EUIDRAWHORIZONTAL	eUIDrawHorizontal,        
		const EUIDRAWVERTICAL    eUIDrawVertical ) const;

	// Aligns an object to the screen.
	// Vec2(out_x,out_y) is the point when moved from the defined edge.
	void AlignToScreen( float* out_x, float* out_y,
		const float posX, const float posY,
		const float screenWidth, const float screenHeight,
		const EUIDRAWHORIZONTAL eUIDrawHorizontalDocking, 
		const EUIDRAWVERTICAL   eUIDrawVerticalDocking ) const;

private :

	ScreenLayoutStates m_flags;

	EHUDSafeAreaID m_curSafeAreaID;
	Vec2           m_customSafeArea;

	//------------------------------------------------------
	static ScreenLayoutManager* s_inst;
	static float VIRTUAL_SCREEN_WIDTH;
	static float VIRTUAL_SCREEN_HEIGHT;

	static const float TITLE_SAFE_AREA;
	static const float SONY_PS3_TRC_SAFE_AREA;
	static const float MICROSOFT_360_TCR_SAFE_AREA;
	static const float ACTION_SAFE_AREA;
};

#endif // __ScreenLayoutManager_H__
