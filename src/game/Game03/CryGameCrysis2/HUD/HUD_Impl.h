/************************************************************************/
/* UI class for Component HUD, Jan Müller, 2009													*/
/************************************************************************/

#ifndef COMP_HUD_IMPL_H
#define COMP_HUD_IMPL_H

#include "NanoSuitDefs.h"
#include "UI/UIElement.h"
#include "Perk.h"
#include "AutoEnum.h"
#include "GameRulesTypes.h"
#include "IUIDraw.h"
#include "Audio/AudioSignalPlayer.h"

class C2DRenderUtils;
class SafeAreaRenderer;
struct CControllerInputRenderInfo;
class CHUD;
class CUIElementManager;

#if !defined(PROFILE) && !defined(_RELEASE)
#define ENABLE_HUD_IMPL_DEBUGGING 1
#else
#define ENABLE_HUD_IMPL_DEBUGGING 0
#endif

#if ENABLE_HUD_IMPL_DEBUGGING
void HudLog(int level, const char* format, ...) PRINTF_PARAMS(2, 3);
#else
#define HudLog(...)		(void)(0)
#endif

enum EHUDDependencyType
{
	EHUDDT_UKNOWN,
	EHUDDT_INT,
	EHUDDT_STRING,
};

#define HideHUDReasonList(f)    \
	f(eHideHUDReason_General)     \
	f(eHideHUDReason_Menu)        \
	f(eHideHUDReason_Debug)				\

AUTOENUM_BUILDFLAGS_WITHZERO(HideHUDReasonList, eHideHUDReason_None);
typedef TBitfield EHideHUDReason;

struct SHUD_Dependencies 
{
	int m_gamemode_start_countdown_timer;
	int m_gamemode_start_required_players;
	int m_gamemode_start_required_team_players;

	SHUD_Dependencies()
		:m_gamemode_start_countdown_timer(-1),
		m_gamemode_start_required_players(0),
		m_gamemode_start_required_team_players(0)
	{
		// ...
	}
};


class CHUD_Impl
{

public:

	CHUD_Impl(CHUD* in_pHUD, C2DRenderUtils* p2DRenderUtils);
	~CHUD_Impl();

	void Clear( void );

	void Update(float fDeltaTime);

	SHUD_Dependencies& GetDependencies() { return m_dependencies; }
	ColorF             GetBaseColor() const;

	ILINE TBitfield GetElementsBeingDrawn() const
	{
		return m_mostRecentElementsDrawn;
	}

	void Unload();

	void ShowHUD(bool onOff, EHideHUDReason reason = eHideHUDReason_General);
	bool ShouldShowHUD();
	
	static void ConvertSecondsToTimerString( const int s, string* in_out_string, bool stripZeroElements=false );

	void GetMemoryUsage(ICrySizer *pSizer) const
	{	
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_soundSuitMercyTime);
		pSizer->AddObject(m_soundSuitMercyTimeEnd);
	}
private:

	ITexture *GetWeaponTexture(const char *pWeaponName, bool bMenuTexture = false);

	void Draw();
	void DrawRadar();

	void DrawSuitModeSpecifics();

	void DrawWeaponMenu();

#if 0 // Handled by HUD states
	void DrawElements( float frameTime ) const;
#endif
	void UpdateData(IActor * localActor, float fDeltaTime);

	void UpdateCrossHair();

	//TBitfield UpdateGameModeSpecifics();
	TBitfield UpdateSuitState();

	void SelectHudColor();

	float GetAspectRatio();


private:

	TBitfield m_mostRecentElementsDrawn;

	//fill this struct with all required data
	SHUD_Dependencies m_dependencies;

	uint32	m_hideHUDFlags;

	C2DRenderUtils	 *m_pUIDraw;
	SafeAreaRenderer *m_safeAreas;

	//hud state
	ENanoSuitMode m_eLastSuitMode;
	float m_fSuitModeChangedTimer;

	ENanoSuitState m_eLastSuitState;
	float m_fSuitStateChangedTimer;

	float m_lastHitPoints;
	float m_mercyTimeTimer;

	float m_screenRedValue;

	CAudioSignalPlayer m_soundSuitMercyTime;
	CAudioSignalPlayer m_soundSuitMercyTimeEnd;

	string m_sLastWeapon;
	float m_fWeaponSwitchTimer;

	ColorF m_hudColor;
	float  globalAlphaModificator;

	bool editorModeActive;

	CHUD* m_pHUD;

}; //CHUD_Impl

#endif
