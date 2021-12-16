/************************************************************************/
/* UI PerkIconSet for Component HUD, Tim Furnish, 2009										*/
/************************************************************************/

#include "StdAfx.h"
#include "HUD/UI/UISimpleBar.h"
#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h" // TODO : Remove
#include "Game.h"
#include "PerkDbgDisplay.h"
#include "Graphics/2DRenderUtils.h"

static float k_warningFlashTime = 0.2f;

static CUISimpleBar * s_firstSimpleBarRegion;
static CUISimpleBar * s_lastSimpleBarRegion;

CUISimpleBar::CUISimpleBar() :
	CUIElement(),
	m_bgColour(0.3f,0.3f,0.3f,0.4f), /* default colours for health bar */
	m_fgColour(0.5f,1.0f,0.5f,0.5f),
	m_flashColour1(1.0f,0.0f,0.0f,1.0f),
	m_flashColour2(0.3f,0.3f,0.3f,1.0f)
{
	m_fractionFull = 0.5f;
	m_alpha = 0.f;
	m_setThisFrame = false;
	m_doingWarningFlash = false;
	m_warningFlash = 0.f;

	// Add to linked list
	m_next = s_firstSimpleBarRegion;
	s_firstSimpleBarRegion = this;
	m_prev = NULL;

	if (m_next)
	{
		m_next->m_prev = this;
	}
	else
	{
		s_lastSimpleBarRegion = this;
	}

	m_adjustPosY = false;
}

CUISimpleBar::~CUISimpleBar()
{
	if (m_prev)
	{
		CRY_ASSERT(s_firstSimpleBarRegion != this);
		m_prev->m_next = m_next;
	}
	else
	{
		CRY_ASSERT(s_firstSimpleBarRegion == this);
		s_firstSimpleBarRegion = m_next;
	}

	if (m_next)
	{
		CRY_ASSERT(s_lastSimpleBarRegion != this);
		m_next->m_prev = m_prev;
	}
	else
	{
		CRY_ASSERT(s_lastSimpleBarRegion == this);
		s_lastSimpleBarRegion = m_prev;
	}

	m_prev = NULL;
	m_next = NULL;
}

void CUISimpleBar::Initialize( const IItemParamsNode* xmlElement, IUIElement* parent )
{
	CUIElement::Initialize( xmlElement, parent );

	const char * adjustPosYStr = xmlElement->GetAttribute("adjustPosY");

	if(adjustPosYStr)
	{
		m_adjustPosY = atoi(adjustPosYStr) != 0;
	}
}

CUISimpleBar * CUISimpleBar::GetInstanceWithName(const char * name)
{
	for (CUISimpleBar * eachOne = s_firstSimpleBarRegion; eachOne; eachOne = eachOne->m_next)
	{
		if (0 == strcmp(name, eachOne->GetName()))
		{
			return eachOne;
		}
	}

	return NULL;
}

void CUISimpleBar::Set(float f, bool display)
{
	m_fractionFull = f;
	m_setThisFrame = display;
}

void CUISimpleBar::SetColours( const ColorF& bg, const ColorF& fg, const ColorF& flash1, const ColorF& flash2 )
{
	m_bgColour = bg;
	m_fgColour = fg;
	m_flashColour1 = flash1;
	m_flashColour2 = flash2;
}

void CUISimpleBar::TriggerWarningFlash()
{
	if (m_doingWarningFlash == false)
	{
		m_doingWarningFlash = true;
		m_warningFlash = k_warningFlashTime;
	}
}

void CUISimpleBar::Update(float frameTime)
{
	if (m_setThisFrame)
	{
		m_alpha = min(m_alpha + frameTime, 1.f);
		m_setThisFrame = false;
	}
	else
	{
		m_alpha = max(m_alpha - frameTime, 0.f);
	}

	if (m_doingWarningFlash)
	{
		m_warningFlash -= frameTime * 0.4f;
		if (m_warningFlash <= 0.f)
		{
			m_doingWarningFlash = false;
			m_warningFlash = 0.f;
		}
	}
}

void CUISimpleBar::Draw(void) const
{
	if (m_alpha > 0.f)
	{
		const float heightAdjustment = 600.f / gEnv->pRenderer->GetHeight();
		const float widthAdjustment = 800.f / gEnv->pRenderer->GetWidth();
		const float w = GetWidth() * widthAdjustment;
		const float h = GetHeight() * heightAdjustment;
		const float x = GetPosX() - w * 0.5f;
		const float y = (m_adjustPosY ? (heightAdjustment * GetPosY()) : GetPosY()) - h * 0.5f;

		const float barFraction = m_fractionFull;

		ColorF drawBGColour(m_bgColour);
		if (m_doingWarningFlash)
		{
			const float lerp = m_warningFlash / k_warningFlashTime;
			const float invlerp = 1-lerp;
			drawBGColour = m_flashColour1*lerp;
			drawBGColour += m_flashColour2*invlerp;
		}

		// BG Bar
		drawBGColour.a *= m_alpha;
		g_pGame->GetHUD()->Get2DRenderUtils()->DrawQuad(x, y, w, h, drawBGColour);

		if (barFraction > 0.f)
		{
			const float colMult = 1-(barFraction*0.5f);
			ColorF drawFGColour( m_fgColour.r*colMult, m_fgColour.g*colMult, m_fgColour.b*colMult, m_fgColour.a*m_alpha);
			HudDbgDisplay("CUISimpleBar::Draw '%s' %.2f alpha=%.2f col=[%.1f %.1f %.1f]", GetName(), m_fractionFull, m_alpha, drawFGColour.r, drawFGColour.g, drawFGColour.b);

			if (w > h)
			{
				g_pGame->GetHUD()->Get2DRenderUtils()->DrawQuad(x + 2, y + 2, (w - 4) * barFraction, h - 4, drawFGColour);
			}
			else
			{
				g_pGame->GetHUD()->Get2DRenderUtils()->DrawQuad(x + 2, y + 2 + (h - 4) * (1.f - barFraction), w - 4, (h - 4) * barFraction, drawFGColour);
			}
		}
	}

	return;
}
