/************************************************************************/
/* UI BattleLog for Component HUD, Tim Furnish, 2009										*/
/************************************************************************/

#include "StdAfx.h"
#include "UIButtonPromptRegion.h"

#include "PerkDbgDisplay.h"

#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"
#include "HUD/ScreenLayoutManager.h"
#include "HUD/UIElementManager.h"
#include "HUD/HUDOnScreenMessageDef.h"

#include "Graphics/2DRenderUtils.h"

#if 1 && defined(USER_timf) && !defined(_RELEASE)
#define BUTTON_PROMPT_REGION_DEBUGGING		1
#else
#define BUTTON_PROMPT_REGION_DEBUGGING		0
#endif

static CUIButtonPromptRegion * s_firstButtonPromptRegion;
static CUIButtonPromptRegion * s_lastButtonPromptRegion;

#if BUTTON_PROMPT_REGION_DEBUGGING
static int s_numMessageCreated = 0;
static int s_numberLastPrintedDuringFrame = 0;
#endif

CSingleButtonPromptInfo::CSingleButtonPromptInfo(CButtonPromptInfoLinkedList * list, const char * text, float time, const CControllerInputRenderInfo * inputRenderInfo, TBitfield vanishSettings)
{
#if BUTTON_PROMPT_REGION_DEBUGGING
	s_numMessageCreated ++;
#endif

	m_list = list;
	m_string = text;
	SetTimeRemaining(time);
	m_vanishSettings = vanishSettings;
	m_visibility = 0.f;

	if (inputRenderInfo)
	{
		m_inputRenderInfo = *inputRenderInfo;
	}
	else
	{
		m_inputRenderInfo.Clear();
	}

	// Add to linked list
	m_next = list->m_firstInfo;
	list->m_firstInfo = this;
	m_prev = NULL;

	if (m_next)
	{
		m_next->m_prev = this;
	}
	else
	{
		list->m_lastInfo = this;
	}
}

void CSingleButtonPromptInfo::SetTimeRemaining(float f)
{
	m_reduceTime = (f <= 999.f);
	m_timeUntilVanish = f;
}

CSingleButtonPromptInfo::~CSingleButtonPromptInfo()
{
#if BUTTON_PROMPT_REGION_DEBUGGING
	s_numMessageCreated --;
#endif

	if (m_prev)
	{
		CRY_ASSERT(m_list->m_firstInfo != this);
		m_prev->m_next = m_next;
	}
	else
	{
		CRY_ASSERT(m_list->m_firstInfo == this);
		m_list->m_firstInfo = m_next;
	}

	if (m_next)
	{
		CRY_ASSERT(m_list->m_lastInfo != this);
		m_next->m_prev = m_prev;
	}
	else
	{
		CRY_ASSERT(m_list->m_lastInfo == this);
		m_list->m_lastInfo = m_prev;
	}

	m_prev = NULL;
	m_next = NULL;
}

//--------------------------------------------------------------------------------
CUIButtonPromptRegion::CUIButtonPromptRegion() :
	CUIElement()
{
#if BUTTON_PROMPT_REGION_DEBUGGING
	CryLog ("BUTTONPROMPT: Creating button prompt region with name '%s'", GetName());
#endif

	m_maxSimultaneous = 1;
	m_list.m_firstInfo = NULL;
	m_list.m_lastInfo = NULL;
	m_inSpeed = 1.f;
	m_outSpeed = 1.f;
	m_fontSize = 1.f;
	m_totalVisibility = 0.f;
	m_distanceApartX = 0.f;
	m_distanceApartY = 0.f;
	m_inputIconOffsetY = 0.f;
	m_inputIconSize = 10.f;
	m_captionOffsetY = 0.f;
	m_captionSize = 10.f;
	//m_defaultVanishSettings = 0.0f;
}

CUIButtonPromptRegion::~CUIButtonPromptRegion()
{
	ClearAll();

	if (m_prev)
	{
		CRY_ASSERT(s_firstButtonPromptRegion != this);
		m_prev->m_next = m_next;
	}
	else
	{
		CRY_ASSERT(s_firstButtonPromptRegion == this);
		s_firstButtonPromptRegion = m_next;
	}

	if (m_next)
	{
		CRY_ASSERT(s_lastButtonPromptRegion != this);
		m_next->m_prev = m_prev;
	}
	else
	{
		CRY_ASSERT(s_lastButtonPromptRegion == this);
		s_lastButtonPromptRegion = m_prev;
	}

	m_prev = NULL;
	m_next = NULL;
}

void CUIButtonPromptRegion::Initialize( const IItemParamsNode* xmlElement, IUIElement* parent )
{
	CUIElement::Initialize(xmlElement, parent);

	xmlElement->GetAttribute("maxSimultaneous", m_maxSimultaneous);
	xmlElement->GetAttribute("inSpeed", m_inSpeed);
	xmlElement->GetAttribute("outSpeed", m_outSpeed);
	xmlElement->GetAttribute("fontSize", m_fontSize);
	xmlElement->GetAttribute("distanceApartX", m_distanceApartX);
	xmlElement->GetAttribute("distanceApartY", m_distanceApartY);
	xmlElement->GetAttribute("inputIconOffsetY", m_inputIconOffsetY);
	xmlElement->GetAttribute("inputIconSize", m_inputIconSize);
	xmlElement->GetAttribute("captionOffsetY", m_captionOffsetY);
	xmlElement->GetAttribute("captionSize", m_captionSize);

	const char * vanishWhenStr = xmlElement->GetAttribute("vanishWhen");

	if(vanishWhenStr)
	{
		m_defaultVanishSettings = MakeVanishSettingsFromString(vanishWhenStr);
	}
	else
	{
		m_defaultVanishSettings = kHudElementsDrawnFlag_none;
	}


	// Add to linked list
	m_next = s_firstButtonPromptRegion;
	s_firstButtonPromptRegion = this;
	m_prev = NULL;

	if (m_next)
	{
		m_next->m_prev = this;
	}
	else
	{
		s_lastButtonPromptRegion = this;
	}
}

void CUIButtonPromptRegion::ClearAll()
{
	while (m_list.m_firstInfo)
	{
		CRY_ASSERT (m_list.m_lastInfo);
		delete m_list.m_firstInfo;
	};

	CRY_ASSERT (!m_list.m_lastInfo);
}

void CUIButtonPromptRegion::Update(float frameTime)
{
	TBitfield elementsBeingDrawn = g_pGame->GetHUD()->GetHUDImpl()->GetElementsBeingDrawn();

#if BUTTON_PROMPT_REGION_DEBUGGING
	int frame = gEnv->pRenderer->GetFrameID(false);

	if (s_numberLastPrintedDuringFrame != frame)
	{
		s_numberLastPrintedDuringFrame = frame;
		HudDbgDisplay("Number of on-screen messages: %d, drawing=%u", s_numMessageCreated, elementsBeingDrawn);
	}
	HudDbgDisplay("Ticking button prompt region %p=>%p=>%p \"%s\" (first=%p, defVanish=%u)", m_prev, this, m_next, GetName(), m_list.m_firstInfo, m_defaultVanishSettings);
#endif

	CRY_ASSERT(m_prev || (s_firstButtonPromptRegion == this));
	CRY_ASSERT(m_prev == NULL || m_prev->m_next == this);
	CRY_ASSERT(m_next || (s_lastButtonPromptRegion == this));
	CRY_ASSERT(m_next == NULL || m_next->m_prev == this);

	m_totalVisibility = 0.f;
	CSingleButtonPromptInfo * eachOne = m_list.m_firstInfo;

	while (eachOne)
	{
		CRY_ASSERT(eachOne->m_list == & m_list);

#if BUTTON_PROMPT_REGION_DEBUGGING
		HudDbgDisplay ("%s: \"%s\" vanish=%u (%f)", GetName(), eachOne->m_string.c_str(), eachOne->m_vanishSettings, eachOne->m_timeUntilVanish);
#endif

		if ((eachOne->m_vanishSettings & elementsBeingDrawn) || eachOne->m_timeUntilVanish < 0.f)
		{
			eachOne->SetTimeRemaining(-1.f);
			eachOne->m_visibility -= frameTime * m_outSpeed;
			if (eachOne->m_visibility <= 0.f)
			{
				CSingleButtonPromptInfo * nextOne = eachOne->m_next;
				delete eachOne;
				eachOne = nextOne;
				continue;
			}
		}
		else
		{
#if BUTTON_PROMPT_REGION_DEBUGGING
			if (eachOne->m_visibility <= 0.f)
			{
				CryLogAlways ("BUTTONPROMPT: Adding on-screen message text '%s' timeOut=%f vanishFlags=%u", eachOne->m_string.c_str(), eachOne->m_timeUntilVanish, eachOne->m_vanishSettings);
			}
#endif

			if (eachOne->m_reduceTime)
			{
				eachOne->m_timeUntilVanish -= frameTime;
			}
			eachOne->m_visibility = min(eachOne->m_visibility + frameTime * m_inSpeed, 1.f);
		}

		m_totalVisibility += HUDCurveFraction(eachOne->m_visibility);
		eachOne = eachOne->m_next;
	}

	CUIElement::Update(frameTime);
}

void CUIButtonPromptRegion::Draw(void) const
{
	float visibilitySoFar = 0.f;

	for (CSingleButtonPromptInfo * eachOne = m_list.m_firstInfo; eachOne; eachOne=eachOne->m_next)
	{
		float growVal = (1.f - eachOne->m_visibility);
		float curve = growVal * growVal;
		float size = (25.f - curve * 5.f) * m_fontSize;
		float mid = visibilitySoFar + visibilitySoFar + HUDCurveFraction(eachOne->m_visibility);
		float shiftAmount = mid - m_totalVisibility;
		ColorF whiteWithAlpha(1.f, 1.f, 1.f, eachOne->m_visibility * eachOne->m_visibility);
		float centreX = (GetPosX() + shiftAmount * m_distanceApartX);
		float centreY = (GetPosY() + shiftAmount * m_distanceApartY);

#if BUTTON_PROMPT_REGION_DEBUGGING
		HudDbgDisplay("Message region '%s' element %p: curve=%f growVal=%f m_fontSize=%f", GetName(), eachOne, curve, growVal, m_fontSize);
		HudDbgDisplay("Message region '%s' element %p: '%s' being drawn at %.2f %.2f size=%.2f", GetName(), eachOne, eachOne->m_string.c_str(), centreX, centreY, size);
#endif

		C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();

		pRenderUtils->DrawText(centreX, centreY, size, size, eachOne->m_string.c_str(), whiteWithAlpha, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER);

		switch (eachOne->m_inputRenderInfo.GetType())
		{
			case kCITV_icon:
			{
				// scaleY _is_ currently still needed to keep the width:height ratio of the texture correct! [TF]
				ScreenLayoutManager * layoutMgr = g_pGame->GetHUD()->GetLayoutManager();
				const float scaleUsingWidthValues = (gEnv->pRenderer->GetWidth() / layoutMgr->GetVirtualWidth()); 
				const float scaleUsingHeightValues = (gEnv->pRenderer->GetHeight() / layoutMgr->GetVirtualHeight());
				const float scaleY = scaleUsingWidthValues / scaleUsingHeightValues;

				const float iconWidth = (2.f + 1.f * curve) * m_inputIconSize;
				const float iconHeight = iconWidth * scaleY;
				const float iconCentreOffsetX = 0.f;
				const float iconCentreOffsetY = m_inputIconOffsetY * scaleY;

				gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);

				pRenderUtils->DrawImage(
					eachOne->m_inputRenderInfo.GetTexture()->GetTextureID(),
					centreX + iconCentreOffsetX - (iconWidth  * 0.5f),
					centreY + iconCentreOffsetY - (iconHeight * 0.5f),
					iconWidth, iconHeight, 0.0f, whiteWithAlpha);
			}
			break;

			case kCITV_text:
				pRenderUtils->DrawText( centreX, centreY + m_captionOffsetY, size * m_captionSize, size * m_captionSize,
				                        eachOne->m_inputRenderInfo.GetText(),
				                        whiteWithAlpha, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER );
			break;
		}

		visibilitySoFar += HUDCurveFraction(eachOne->m_visibility);
	}

	return;
}

CUIButtonPromptRegion * CUIButtonPromptRegion::GetInstanceWithName(const char * name)
{
	for (CUIButtonPromptRegion * eachOne = s_firstButtonPromptRegion; eachOne; eachOne = eachOne->m_next)
	{
		if (0 == strcmp(name, eachOne->GetName()))
		{
			return eachOne;
		}
	}

	return NULL;
}

void CUIButtonPromptRegion::SetOnScreenMessageText(const char * message, const CControllerInputRenderInfo * renderInfo, float timeOut, TBitfield vanishSettings)
{
	vanishSettings |= m_defaultVanishSettings;

	for (CSingleButtonPromptInfo * eachOne = m_list.m_firstInfo; eachOne; eachOne=eachOne->m_next)
	{
		if (0 == strcmp (eachOne->m_string, message))
		{
			eachOne->SetTimeRemaining(timeOut);
			return;
		}
	}

	CSingleButtonPromptInfo * newInfo = new CSingleButtonPromptInfo(& m_list, message, timeOut, renderInfo, vanishSettings);
	CRY_ASSERT(newInfo == m_list.m_firstInfo);
	CRY_ASSERT(newInfo->m_list == & m_list);
}

//------------------------------------------------------------------------------------------------------------------------------------------------

/*static*/ void CUIButtonPromptRegion::SetOnScreenMessageText(const char * queueName, const SOnScreenMessageDef & messageDef, const char * paramA, const char * paramB)
{
	SetOnScreenMessageText(queueName, messageDef.GetDisplayText(), paramA, paramB, messageDef.GetInputRenderInfo(), messageDef.GetLifespan(), messageDef.GetVanishSettings());
}

/*static*/ void CUIButtonPromptRegion::SetOnScreenMessageText(const char * queueName, const char * message, const char * paramA, const char * paramB, const CControllerInputRenderInfo * renderInfo, float timeOut, TBitfield vanishSettings)
{
	const char * localised = (message && message[0] == '@') ? g_pGame->GetHUD()->LocalizeString(message, paramA, paramB) : message;
	SetOnScreenMessageText(queueName, localised, renderInfo, timeOut, vanishSettings);
}

/*static*/ void CUIButtonPromptRegion::SetOnScreenMessageText(const char * queueName, const char * message, const CControllerInputRenderInfo * renderInfo, float timeOut, TBitfield vanishSettings)
{
	const char * localised = (message && message[0] == '@') ? g_pGame->GetHUD()->LocalizeString(message, NULL, NULL) : message;

	if (g_pGame->GetIGameFramework()->GetClientActor())
	{
		CUIButtonPromptRegion * region = CUIButtonPromptRegion::GetInstanceWithName(queueName);

		if (region)
		{
			region->SetOnScreenMessageText(localised, renderInfo, timeOut, vanishSettings);
		}
		else
		{
			CryLogAlways("No on-screen message region by the name of '%s'", queueName);
		}
	}
}
