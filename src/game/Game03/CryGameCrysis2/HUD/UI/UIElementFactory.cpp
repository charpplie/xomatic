
#include "StdAfx.h"

#include "HUD/UI/UIElementFactory.h"

#include "HUD/UI/UIBattleLog.h"
#include "HUD/UI/UITagNames.h"
#include "HUD/UI/UIPerkIconSet.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "HUD/UI/UIObjectives.h"
#include "HUD/UI/UISimpleBar.h"
#include "HUD/UI/UICoverEntireScreen.h"
#include "HUD/UI/UIHitIndicators.h"
#include "HUD/UI/UIScorePopup.h"
#include "HUD/UI/UIDeathCam.h"
#include "HUD/UI/UIPrematch.h"
#include "HUD/UI/UIMinimapTextureElement.h"
#include "HUD/UI/UINewDamageIndicator.h"
#include "HUD/UI/UISpectatorCam.h"

//----------------------------------------------------------------------------
EUISubElementType ElementFactory::GetElementType(const char *tagName)
{
	if(!stricmp(tagName, "Texture"))
		return eSET_TEXTURE;
	else if(!stricmp(tagName, "TextureSwitch"))
		return eSET_TEXTURE_SWITCH;
	else if(!stricmp(tagName, "Text"))
		return eSET_TEXT;
	else if(!stricmp(tagName, "Slider"))
		return eSET_SLIDER;
	else if(!stricmp(tagName, "BattleLog"))
		return eSET_BATTLELOG;
	else if(!stricmp(tagName, "TagNames"))
		return eSET_TAGNAMES;
	else if(!stricmp(tagName, "PerkIconSet"))
		return eSET_PERKICONSET;
	else if(!stricmp(tagName, "TextMessageRegion"))
		return eSET_TEXTMESSAGEREGION;
	else if(!stricmp(tagName, "SimpleBar"))
		return eSET_SIMPLEBAR;
	else if(!stricmp(tagName, "CoverEntireScreen"))
		return eSET_COVERENTIRESCREEN;
	else if(!stricmp(tagName, "MissionObjectives"))
		return eSET_MISSIONOBJECTIVES;
	else if(!stricmp(tagName, "HitIndicators"))
		return eSET_HITINDICATORS;	
	else if(!stricmp(tagName, "NewDamageIndicator"))
		return eSET_NEWDAMAGEINDICATOR;
	else if(!stricmp(tagName, "ScorePopup"))
		return eSET_SCOREPOPUP;
	else if(!stricmp(tagName, "DeathCam"))
		return eSET_DEATHCAM;
	else if(!stricmp(tagName, "Prematch"))
		return eSET_PREMATCH;
	else if(!stricmp(tagName, "MinimapTexture"))
		return eSET_MINIMAPTEXTURE;
	else if(!stricmp(tagName, "SpectatorCam"))
		return eSET_SPECTATORCAM;


	return eSET_UNKOWN;

}

//----------------------------------------------------------------------------
IUIElement* ElementFactory::GetClassInstance(const char* className)
{
	EUISubElementType eType = GetElementType(className);

	if (eType != eSET_UNKOWN)
	{
#if IS_HUD_DEV
		CryLog ("[HUD] Creating UI element: class=\"%s\" type=%d", className, eType);
#endif //IS_HUD_DEV
	}

	switch(eType)
	{
	case eSET_BATTLELOG:
		return new CUIBattleLog();

	case eSET_TAGNAMES:
		return new CUITagNames();

	case eSET_PERKICONSET:
		return new CUIPerkIconSet();

	case eSET_TEXTMESSAGEREGION:
		return new CUIButtonPromptRegion();

	case eSET_SIMPLEBAR:
		return new CUISimpleBar();

	case eSET_COVERENTIRESCREEN:
		return new CUICoverEntireScreen();

	case eSET_MISSIONOBJECTIVES:
		return new CUIMissionObjectives();

	case eSET_EMPTY:
		return new CUIElement();

	case eSET_HITINDICATORS:
		return new UIHitIndicators();

	case eSET_NEWDAMAGEINDICATOR:
		return new CUINewDamageIndicator();

	case eSET_SCOREPOPUP:
		return new UIScorePopup();

	case eSET_DEATHCAM:
		return new UIDeathCam();

	case eSET_PREMATCH:
		return new UIPrematch();
		
	case eSET_MINIMAPTEXTURE:
		return new CUIMinimapTextureElement();

	case eSET_SPECTATORCAM :
		return new UISpectatorCam();

	default :
		//CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format( "[HUD_Impl] Completely unknown UIElement type '%s'.", className ) );
		return NULL;
	}
}