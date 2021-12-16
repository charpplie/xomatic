#include "StdAfx.h"
#include "BitmapUi.h"

#include "IGameFramework.h"
#include "IRenderer.h"
#include "UIDraw/UIDraw.h"
#include "GameCVars.h"
#include "Player.h"
#include "Weapon.h"

static const float UI_WIDTH			= 800.0f;
static const float UI_HEIGHT		= 600.0f;
static const float UI_CENTRE_X	= ( UI_WIDTH * 0.5f );
static const float UI_CENTRE_Y	= ( UI_HEIGHT * 0.5f );

//texture sizes
static const float CROSSHAIR_TEXTURE_WIDTH				=	64.0f;
static const float CROSSHAIR_TEXTURE_HEIGHT				=	CROSSHAIR_TEXTURE_WIDTH;

#define FULLSCREEN_TEXTURE_WIDTH	gEnv->pConsole->GetCVar("r_Width")->GetFVal()	//gEnv->pRenderer->CV_r_Width
#define FULLSCREEN_TEXTURE_HEIGHT	gEnv->pConsole->GetCVar("r_Height")->GetFVal()	//gEnv->pRenderer->CV_r_Height

//texture files
static const char* CROSSHAIR_TEXTURE			=	"Textures\\Gui\\Crosshair.tif";
static const char* PAUSESCREEN_TEXTURE			=	"Textures\\Gui\\loading_screen.tif";		// "Textures\\Gui\\start_screen.tif"

#if defined(WIN32) || defined(WIN64)
static const char* LOADINGSCREEN_TEXTURE		=	"Textures\\Gui\\loading_screen.tif";
static const char* STARTSCREEN_TEXTURE			=	"Textures\\Gui\\loading_screen.tif";		// "Textures\\Gui\\start_screen.tif"
static const char* CREDITSSCREEN_TEXTURE		=	"Textures\\Gui\\credits_screen.tif";
#else //console optimized textures: save on memory
static const char* LOADINGSCREEN_TEXTURE		=	"Textures\\Gui\\loading_screen_con.tif";
static const char* STARTSCREEN_TEXTURE			=	"Textures\\Gui\\loading_screen_con.tif";	//"Textures\\Gui\\start_screen_con.tif"
static const char* CREDITSSCREEN_TEXTURE		=	"Textures\\Gui\\credits_screen_con.tif"; 
#endif //WIN32 || WIN64

//-------------------------------
CBitmapUi::CBitmapUi() : 
	m_pGameFramework( NULL ),
	m_pUiDraw( NULL ),
	m_iTexCrosshair( 0 ),
	m_iTexLoadingScreen( 0 ),
	m_iTexStartScreen( 0 ),
	m_iTexCreditsScreen( 0 ),
	m_iTexPauseScreen( 0 )
{
	m_pGameFramework = GetISystem()->GetIGame()->GetIGameFramework();
	if ( m_pGameFramework == NULL )
	{
		return;
	}

	m_pUiDraw = m_pGameFramework->GetIUIDraw();
	if ( m_pUiDraw == NULL )
	{
		return;
	}

	gEnv->pConsole->SetLoadingImage( LOADINGSCREEN_TEXTURE );
	m_iTexCrosshair			= m_pUiDraw->CreateTexture( CROSSHAIR_TEXTURE, false );
	m_iTexLoadingScreen	= m_pUiDraw->CreateTexture( LOADINGSCREEN_TEXTURE, false );
	m_iTexStartScreen		= m_pUiDraw->CreateTexture( STARTSCREEN_TEXTURE, false );
	m_iTexCreditsScreen	= m_pUiDraw->CreateTexture( CREDITSSCREEN_TEXTURE, false );
	m_iTexPauseScreen		= m_pUiDraw->CreateTexture( PAUSESCREEN_TEXTURE, false );

	m_pGameFramework->GetILevelSystem()->AddListener( this );

	if ( ! gEnv->IsEditor() )
	{
		g_pGameCVars->g_show_fullscreen_info = eSS_START;
	}
}

//-------------------------------
CBitmapUi::~CBitmapUi()
{
	if ( m_pUiDraw == NULL )
	{
		return;
	}

	m_pGameFramework->GetILevelSystem()->RemoveListener( this );

	m_pUiDraw->DeleteTexture( m_iTexCrosshair );
	m_pUiDraw->DeleteTexture( m_iTexLoadingScreen );
	m_pUiDraw->DeleteTexture( m_iTexStartScreen );
	m_pUiDraw->DeleteTexture( m_iTexCreditsScreen );
	m_pUiDraw->DeleteTexture( m_iTexPauseScreen );
}

//-------------------------------
void CBitmapUi::OnLoadingProgress(ILevelInfo* pLevel, int progressAmount)
{
	if ( ! CanDraw() || g_pGameCVars->g_flash_mainmenu_state != 0)
	{
		return;
	}

	bool isStandAlone = ( gEnv->pRenderer->EF_Query(EFQ_RecurseLevel) <= 0 );
	if (isStandAlone)
	{
		gEnv->pSystem->RenderBegin();
	}

	Draw();

	if (isStandAlone)
	{
		gEnv->pSystem->RenderEnd();
	}
}

//-------------------------------
bool	CBitmapUi::CanDraw() const
{
	if ( m_pGameFramework == NULL )
	{	return false;	}
	
	bool showBitmapUi = ( g_pGameCVars->g_showBitmapUi == 1 );
	if ( ! showBitmapUi )
	{	return false;	}

	if ( m_pUiDraw == NULL )
	{	return false;	}

	return true;
}

//-------------------------------
bool	CBitmapUi::CanDrawCrosshair() const
{
	if (!CanDraw())	
	{	return false;	}

	if (!g_pGameCVars->g_show_crosshair || g_pGameCVars->g_flash_hud_enable)
	{	return false;	}

	CPlayer* pPlayer = static_cast< CPlayer* >( m_pGameFramework->GetClientActor() );
	if ( pPlayer == NULL )
	{	return false;	}

	EntityId mountedWeaponId = pPlayer->GetInventory()->GetCurrentItem();
	CWeapon* pWeapon = pPlayer->GetWeapon( mountedWeaponId );
	if ( pWeapon == NULL )
	{	return false;	}

	bool carryingMeleeWeapon = pWeapon->CanMeleeAttack();
	if ( carryingMeleeWeapon )
	{	return false;	}

	bool usingWeaponSightForAiming = pPlayer->IsThirdPerson() ? 
		!g_pGameCVars->g_show_crosshair_tp : 
		pWeapon->IsZoomed();
	if ( usingWeaponSightForAiming )
	{	return false;	}

	if ( pPlayer->IsDead() )
		return false;

	return true;
}

//-------------------------------
void	CBitmapUi::DrawCrosshair() const
{
	if ( !CanDrawCrosshair() )
	{	return;	}

	DrawCentered(m_iTexCrosshair, CROSSHAIR_TEXTURE_WIDTH, CROSSHAIR_TEXTURE_HEIGHT);
}

//-------------------------------
void	CBitmapUi::DrawFullscreen() const
{
	if ( !CanDraw())
	{	return;	}

	switch( g_pGameCVars->g_show_fullscreen_info )	
	{
	case eSS_LOADING:
		DrawCentered(m_iTexLoadingScreen, FULLSCREEN_TEXTURE_WIDTH, FULLSCREEN_TEXTURE_HEIGHT);
		break;
	case eSS_START:
		DrawCentered(m_iTexStartScreen, FULLSCREEN_TEXTURE_WIDTH, FULLSCREEN_TEXTURE_HEIGHT);
		break;
	case eSS_CREDITS:
		DrawCentered(m_iTexCreditsScreen, FULLSCREEN_TEXTURE_WIDTH, FULLSCREEN_TEXTURE_HEIGHT);
		break;
	default:
		break;
	}	
}

//-------------------------------
void	CBitmapUi::DrawPauseScreen() const
{
	if ( !CanDraw() )
	{	return;	}

	DrawCentered(m_iTexPauseScreen, FULLSCREEN_TEXTURE_WIDTH, FULLSCREEN_TEXTURE_HEIGHT);
}

//-------------------------------
void	CBitmapUi::Draw() const
{
	if ( !CanDraw() )
	{	return;	}

	UpdateCVars();

	m_pUiDraw->PreRender();
	if ( m_pGameFramework->IsGamePaused() )
	{
		DrawPauseScreen();
	}
	else
	{
		DrawCrosshair();
		DrawFullscreen();
	}	
	m_pUiDraw->PostRender();
}

//-------------------------------
void	CBitmapUi::DrawCentered(int texId, float width, float height) const
{
	assert( m_pUiDraw != NULL );
	IRenderer* pRenderer = gEnv->pRenderer;
	if ( pRenderer == NULL )
	{	return;	}

	float screenCentreX = pRenderer->GetWidth()	* 0.5f;
	float screenCentreY = pRenderer->GetHeight()* 0.5f;

	float offsetX = screenCentreX - width * 0.5f;
	float offsetY = screenCentreY - height * 0.5f;

	m_pUiDraw->DrawQuadSimple( offsetX, offsetY, width, height, 0xFFFFFFFF, texId);
}

//-------------------------------
//this function updates the related GameCVars
//functionality is based on a state machine controlled by CVars

void	CBitmapUi::UpdateCVars() const
{
	if(!m_pGameFramework)
		return;

	if(		m_pGameFramework->IsInLevelLoad() 
		||	m_pGameFramework->IsLoadingSaveGame()
		)
	{	g_pGameCVars->g_show_fullscreen_info = eSS_LOADING;	}
	//other conditions
	else if(	g_pGameCVars->g_show_fullscreen_info == eSS_START	)
	{
		if ( m_pGameFramework->IsGameStarted() )
		{
			g_pGameCVars->g_show_fullscreen_info = eSS_NONE;
		}
	}
	else if(	g_pGameCVars->g_show_fullscreen_info == eSS_CREDITS	)
	{	//empty for now
	}
	//finally
	else
	{	g_pGameCVars->g_show_fullscreen_info = eSS_NONE;	}

}