/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
*************************************************************************/

#include "StdAfx.h"
#include "HUDOnScreenMessageDef.h"
#include <IItemSystem.h>

SOnScreenMessageDef::SOnScreenMessageDef()
{
	m_lifespan = 0.f;
	m_vanishSettings = 0;
}

void SOnScreenMessageDef::operator=(const SOnScreenMessageDef & fromHere)
{
	m_lifespan = fromHere.m_lifespan;
	m_vanishSettings = fromHere.m_vanishSettings;
	m_inputRenderInfo = fromHere.m_inputRenderInfo;
	m_onScreenMessageText = fromHere.m_onScreenMessageText;

//CryLog ("[HUD MESSAGE DEFINITION] Copied: '%s' lifespan=%f (type %d, '%s') vanish=%u", m_onScreenMessageText.c_str(), m_lifespan, m_inputRenderInfo.GetType(), m_inputRenderInfo.GetText(), m_vanishSettings);
}

void SOnScreenMessageDef::Read(const IItemParamsNode * xml)
{
	// Read on-screen message text
	m_onScreenMessageText = xml->GetAttribute("display");

	// Read prompt icon information
	const char * inputMapName = xml->GetAttribute("inputMapName");
	const char * inputName = xml->GetAttribute("inputName");
	CRY_ASSERT_MESSAGE((inputMapName == NULL) == (inputName == NULL), string().Format("Provided %s", inputMapName[0] ? "inputMapName but not inputName" : "inputName but not inputMapName"));
	if (inputMapName && inputName)
	{
		m_inputRenderInfo.CreateForInput(inputMapName, inputName);
	}

	// Read vanish settings and lifespan
	m_vanishSettings = CUIElement::MakeVanishSettingsFromString(xml->GetAttribute("vanishWhen"));
	xml->GetAttribute("lifespan", m_lifespan);

//CryLog ("[HUD MESSAGE DEFINITION] Read from XML: '%s' lifespan=%f (type %d, '%s') vanish=%u", m_onScreenMessageText.c_str(), m_lifespan, m_inputRenderInfo.GetType(), m_inputRenderInfo.GetText(), m_vanishSettings);
}
