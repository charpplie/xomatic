/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
*************************************************************************/

#ifndef __HUD_ONSCREENMESSAGEDEF_H__
#define __HUD_ONSCREENMESSAGEDEF_H__

#include "HUDControllerInputIcons.h"
#include "HUD/UI/UIElement.h"

struct SOnScreenMessageDef
{
	public:
	SOnScreenMessageDef();
	void Read(const IItemParamsNode * xml);
	void operator=(const SOnScreenMessageDef & fromHere);

	ILINE const bool empty() const
	{
		return m_onScreenMessageText.empty();
	}

	ILINE const char * GetDisplayText() const
	{
		return m_onScreenMessageText.c_str();
	}

	ILINE const CControllerInputRenderInfo * GetInputRenderInfo() const
	{
		return & m_inputRenderInfo;
	}

	ILINE const float GetLifespan() const
	{
		return m_lifespan;
	}

	ILINE const TBitfield GetVanishSettings() const
	{
		return m_vanishSettings;
	}

	private:
	string                          m_onScreenMessageText;
	CControllerInputRenderInfo      m_inputRenderInfo;
	TBitfield                       m_vanishSettings;
	float                           m_lifespan;
};

#endif	// __HUD_ONSCREENMESSAGEDEF_H__
