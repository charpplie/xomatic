/************************************************************************/
/* New damage indicator UI element, Tim Furnish, 2009  									*/
/************************************************************************/

#include "StdAfx.h"
#include "UINewDamageIndicator.h"

#include "PerkDbgDisplay.h"

#include "HUD/HUD.h"
#include "HUD/ScreenLayoutManager.h"
#include "HUD/UIElementManager.h"
#include "HUD/HUDOnScreenMessageDef.h"

#include "Graphics/2DRenderUtils.h"
#include "ItemParams.h"
#include "Utility/CryWatch.h"
#include "IWorldQuery.h"
#include "GameRules.h"

//--------------------------------------------------------------------------------
CUINewDamageIndicator::CUINewDamageIndicator()
{
	m_numDamageSegmentsToDisplay = 0;
	m_drawNonDirectionalDamage = false;
	m_preventDirectionalDamage = false;
	m_maxNonDirectionalAlphaWhenTakingDirectionalDamage = 0.5f;
	m_filteredMaxAlpha = 0.f;

	CHUDEventDispatcher::AddHUDEventListener(this, "OnShowHitIndicator");
	CHUDEventDispatcher::AddHUDEventListener(this, "OnBlind");
	CHUDEventDispatcher::AddHUDEventListener(this, "OnEndBlind");
}

//--------------------------------------------------------------------------------
CUINewDamageIndicator::~CUINewDamageIndicator()
{
	CHUDEventDispatcher::RemoveHUDEventListener(this);
}

//--------------------------------------------------------------------------------
void CUINewDamageIndicator::Initialize( const IItemParamsNode* xmlElement, IUIElement* parent )
{
	CUIElementManager * mgr = g_pGame->GetHUD()->GetElementManager();
	CUIElement::Initialize(xmlElement, parent);

	const char * textureName = xmlElement->GetAttribute("texture");
	assert (textureName);
	ITexture * texture = mgr->GetTexture(textureName);
	assert (texture);
	m_segmentTextureId = texture->GetTextureID();

	const char * nonDirTextureName = xmlElement->GetAttribute("nonDirectionalTexture");
	assert (nonDirTextureName);
	ITexture * nonDirTexture = mgr->GetTexture(nonDirTextureName);
	assert (nonDirTexture);
	m_nonDirectionalTextureId = nonDirTexture->GetTextureID();

	m_colour.Read(xmlElement);

#if IS_HUD_DEV
	CryLog ("[HUD] Creating new damage indicator... segmentTexture='%s' (ID=%d) nonDirectionalTexture='%s' (ID=%d)", textureName, m_segmentTextureId, nonDirTextureName, m_nonDirectionalTextureId);
#endif //IS_HUD_DEV
}

//--------------------------------------------------------------------------------
void CUINewDamageIndicator::Update(float frameTime)
{
	int i = 0;
	float reduceAlpha = frameTime;
	float newMaxAlpha = 0.f;

	while (i < m_numDamageSegmentsToDisplay)
	{
		SDamageIndicatorSegment * seg = m_segments + i;

		if (seg->m_drawn)
		{
			if (seg->m_alpha < reduceAlpha)
			{
				-- m_numDamageSegmentsToDisplay;

				if (m_numDamageSegmentsToDisplay != i)
				{
					memcpy (seg, m_segments + m_numDamageSegmentsToDisplay, sizeof(SDamageIndicatorSegment));
				}

				continue;
			}
			else
			{
				seg->m_alpha -= reduceAlpha;
			}
		}
		else
		{
			seg->m_drawn = true;
		}

		newMaxAlpha = max(seg->m_alpha, newMaxAlpha);
		++ i;
	}

	float changeAllowed = frameTime * 2.5f;
	m_filteredMaxAlpha = (float)__fsel (newMaxAlpha - m_filteredMaxAlpha - changeAllowed, m_filteredMaxAlpha + changeAllowed, newMaxAlpha);

	if (m_drawNonDirectionalDamage)
	{
		if (m_nonDirectionalDamageAlpha < reduceAlpha)
		{
			m_drawNonDirectionalDamage = false;
		}
		else
		{
			m_nonDirectionalDamageAlpha -= reduceAlpha;
		}
	}

	CUIElement::Update(frameTime);

#if 0
	if (Random(0.f, 1.f) < 0.02f)
	{
		AddDirectionalDamage (Random(-3.14159f, 3.14159f));
	}
	else if (Random(0.f, 1.f) < 0.02f)
	{
		m_nonDirectionalDamageAlpha = 1.f;
		m_drawNonDirectionalDamage = true;
	}
#endif
}

//--------------------------------------------------------------------------------
void CUINewDamageIndicator::AddDirectionalDamage(float angleRadians)
{
	// TODO: Overwrite closest/oldest if out of room in list?
	if (m_numDamageSegmentsToDisplay < kMaxSegments)
	{
		m_segments[m_numDamageSegmentsToDisplay].m_alpha = 1.f;
		m_segments[m_numDamageSegmentsToDisplay].m_angleRadians = angleRadians;
		m_segments[m_numDamageSegmentsToDisplay].m_drawn = false;
		++ m_numDamageSegmentsToDisplay;
	}
}

//--------------------------------------------------------------------------------
void CUINewDamageIndicator::OnHUDEvent(const SHUDEvent& event)
{
	switch(event.eventType)
	{
		case eHUDEvent_OnShowHitIndicator:
		{
			Vec3 dir(event.GetData(0).GetFloat(), event.GetData(1).GetFloat(), event.GetData(2).GetFloat());
			dir.z = 0.f;

			if (m_preventDirectionalDamage || (dir.len2() < 0.05f))
			{
				m_nonDirectionalDamageAlpha = 1.f;
				m_drawNonDirectionalDamage = true;
			}
			else
			{
				AddDirectionalDamage (cry_atan2f(dir.x, dir.y));
			}

			break;
		}

		case eHUDEvent_OnBlind:
			m_preventDirectionalDamage = true;
			break;

		case eHUDEvent_OnEndBlind:
			m_preventDirectionalDamage = false;
			break;

		default:
			CRY_ASSERT_MESSAGE(0,"UINewDamageIndicator received unrecognised event");
			break;
	}
}

//--------------------------------------------------------------------------------
void CUINewDamageIndicator::Draw(void) const
{
	TParent_CUINewDamageIndicator::Draw();

	if (m_numDamageSegmentsToDisplay || m_drawNonDirectionalDamage)
	{
		IActor * clientActor = g_pGame->GetIGameFramework()->GetClientActor();
		if (clientActor)
		{
			assert (clientActor->GetGameObject());
			assert (clientActor->GetGameObject()->GetWorldQuery());
			const Vec3 & eyeDir = clientActor->GetGameObject()->GetWorldQuery()->GetDir();
			const float playerRotationAngle = cry_atan2f(eyeDir.x, eyeDir.y);
			const float posX = GetPosX();
			const float posY = GetPosY();

			C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
			ScreenLayoutManager * layoutMgr = g_pGame->GetHUD()->GetLayoutManager();

			const float iconWidth = GetWidth();
			const float iconHeight = GetHeight();

			gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
			CRY_TODO( 16,12,2009, "HUD: Fix new hit indicators so that they adpat with safe areas!");
			layoutMgr->SetState(eSLO_AdaptToSafeArea|eSLO_ScaleMethod_WithY);

			for (int i = 0; i < m_numDamageSegmentsToDisplay; ++ i)
			{
				const float alpha = m_segments[i].m_alpha;
				pRenderUtils->DrawImage( m_segmentTextureId,
				                         posX, posY,
				                         iconWidth, iconHeight, 
				                         RAD2DEG(m_segments[i].m_angleRadians - playerRotationAngle), 
				                         m_colour * alpha,
																 0.0f,0.0f,1.0f,1.0f,
				                         UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER );
			}

			if (m_drawNonDirectionalDamage)
			{
				const float nonDirectionalAlpha = m_nonDirectionalDamageAlpha * (1.f - m_filteredMaxAlpha * (1.f - m_maxNonDirectionalAlphaWhenTakingDirectionalDamage));
				pRenderUtils->DrawImage( m_nonDirectionalTextureId,
				                         posX, posY,
				                         iconWidth, iconHeight,
				                         0.f, 
				                         m_colour * nonDirectionalAlpha,
																 0.0f,0.0f,1.0f,1.0f,
				                         UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER );
			}
		}
	}
}
