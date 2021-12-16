#ifndef __ELEMENTFACTORY_H__
#define __ELEMENTFACTORY_H__

#include "HUD/UI/IUIElement.h"

enum EUISubElementType
{
	eSET_EMPTY = 0,
	eSET_TEXT,
	eSET_TEXT_WITH_INT_DATA,
	eSET_TEXT_WITH_STRING_DATA,
	eSET_TEXTURE,
	eSET_TEXTURE_SWITCH,
	eSET_SLIDER,
	eSET_BATTLELOG,
	eSET_TAGNAMES,
	eSET_PERKICONSET,
	eSET_TEXTMESSAGEREGION,
	eSET_MISSIONOBJECTIVES,
	eSET_SIMPLEBAR,
	eSET_COVERENTIRESCREEN,
	eSET_NEWDAMAGEINDICATOR,
	eSET_HITINDICATORS,
	eSET_SCOREPOPUP,
	eSET_DEATHCAM,
	eSET_PREMATCH,
	eSET_MINIMAPTEXTURE,
	eSET_SPECTATORCAM,
	eSET_UNKOWN
};

//---------------------------------------------------------------------------------
// Non-Flash HUDObject interface.
class ElementFactory
{
public :
	static IUIElement*       GetClassInstance(const char* className);
	static EUISubElementType GetElementType(const char *tagName);
};

#endif // __ELEMENTFACTORY_H__