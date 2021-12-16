/************************************************************************/
/* Entire-screen-covering UI element, Tim Furnish, 2009								  */
/************************************************************************/

#include "StdAfx.h"
#include "HUD/UI/UICoverEntireScreen.h"
#include "HUD/HUD_Impl.h" // TODO: Remove
#include "HUD/HUD.h"
#include "Game.h"
#include "PerkDbgDisplay.h"
#include "Graphics/2DRenderUtils.h"
#include "HUD/ScreenLayoutManager.h"
#include "HUD/UIElementManager.h"

static CUICoverEntireScreen * s_firstCoverEntireScreenRegion;
static CUICoverEntireScreen * s_lastCoverEntireScreenRegion;

CUICoverEntireScreen::CUICoverEntireScreen() :
	CUIElement()
{
	m_targetVisibility = 0.0f;
	m_filteredVisibility = 0.0f;
	m_filterRateAppear = 10.f;
	m_filterRateVanish = 10.f;
	m_maxAlpha = 1.f;
	m_red = 1.f;
	m_green = 1.f;
	m_blue = 1.f;
	m_textureID = 0;

	// Add to linked list
	m_next = s_firstCoverEntireScreenRegion;
	s_firstCoverEntireScreenRegion = this;
	m_prev = NULL;

	if (m_next)
	{
		m_next->m_prev = this;
	}
	else
	{
		s_lastCoverEntireScreenRegion = this;
	}
}

CUICoverEntireScreen::~CUICoverEntireScreen()
{
	if (m_prev)
	{
		CRY_ASSERT(s_firstCoverEntireScreenRegion != this);
		m_prev->m_next = m_next;
	}
	else
	{
		CRY_ASSERT(s_firstCoverEntireScreenRegion == this);
		s_firstCoverEntireScreenRegion = m_next;
	}

	if (m_next)
	{
		CRY_ASSERT(s_lastCoverEntireScreenRegion != this);
		m_next->m_prev = m_prev;
	}
	else
	{
		CRY_ASSERT(s_lastCoverEntireScreenRegion == this);
		s_lastCoverEntireScreenRegion = m_prev;
	}

	m_prev = NULL;
	m_next = NULL;
}

void CUICoverEntireScreen::Initialize(const IItemParamsNode* xmlElement, IUIElement* parent)
{
	CUIElement::Initialize(xmlElement, parent);

	xmlElement->GetAttribute("red", m_red);
	xmlElement->GetAttribute("green", m_green);
	xmlElement->GetAttribute("blue", m_blue);
	xmlElement->GetAttribute("maxAlpha", m_maxAlpha);
	xmlElement->GetAttribute("fadeInSpeed", m_filterRateAppear);
	xmlElement->GetAttribute("fadeOutSpeed", m_filterRateAppear);

	const char * textureName = xmlElement->GetAttribute("texture");
	if (textureName)
	{
		ITexture * texture = g_pGame->GetHUD()->GetElementManager()->GetTexture(textureName);
		CRY_ASSERT_MESSAGE(texture, string().Format("No '%s' texture found for use in '%s' HUD element", textureName, GetName()));
		if (texture)
		{
			m_textureID = texture->GetTextureID();
		}
	}
}

CUICoverEntireScreen * CUICoverEntireScreen::GetInstanceWithName(const char * name)
{
	for (CUICoverEntireScreen * eachOne = s_firstCoverEntireScreenRegion; eachOne; eachOne = eachOne->m_next)
	{
		if (0 == strcmp(name, eachOne->GetName()))
		{
			return eachOne;
		}
	}

	return NULL;
}

void CUICoverEntireScreen::Set(float f)
{
	m_targetVisibility = f;
}

void CUICoverEntireScreen::Update(float frameTime)
{
	if (m_targetVisibility > m_filteredVisibility)
	{
		m_filteredVisibility = min(m_filteredVisibility + frameTime * m_filterRateAppear, m_targetVisibility);
	}
	else
	{
		m_filteredVisibility = max(m_filteredVisibility - frameTime * m_filterRateVanish, m_targetVisibility);
	}
}

void CUICoverEntireScreen::Draw(void) const
{
	if (m_filteredVisibility > 0.f)
	{
		ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();
		ScreenLayoutStates prevSate = pLayoutManager->GetState();

		pLayoutManager->SetState(eSLO_FullScreen);

		HudDbgDisplay("%s visibility = %f", GetName(), m_filteredVisibility);
		ColorF noColour(0.f, 0.f, 0.f, 0.f);
		g_pGame->GetHUD()->Get2DRenderUtils()->DrawQuad( 0.f, 0.f, 800.f, 600.f,
			                                               ColorF(m_red, m_green, m_blue, m_filteredVisibility * m_maxAlpha), 
#if C2DRU_USE_DVN_VB
																										 noColour, noColour, noColour, noColour,
#endif
																								     m_textureID );
		
		pLayoutManager->SetState(prevSate);
	}

	return;
}
