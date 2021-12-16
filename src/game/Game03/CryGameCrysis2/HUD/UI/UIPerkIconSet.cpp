/************************************************************************/
/* UI PerkIconSet for Component HUD, Tim Furnish, 2009										*/
/************************************************************************/

#include "StdAfx.h"
#include "HUD/UI/UIPerkIconSet.h"

#include "HUD/HUD.h"
#include "Game.h"
#include "PerkDbgDisplay.h"
#include "Graphics/2DRenderUtils.h"
#include "Player.h"
#include "PerkIconData.h"

CUIPerkIconSet::CUIPerkIconSet() :
CUIElement(),
m_distanceApart(0.f),
m_pulseSize(1.f),
m_alignHorizontal(0.5f),
m_alignVertical(0.5f)
{
	// ...
}

void CUIPerkIconSet::Initialize( const IItemParamsNode* xmlElement, IUIElement* parent )
{
	CUIElement::Initialize( xmlElement, parent);

	xmlElement->GetAttribute("distanceApart", m_distanceApart);
	CRY_ASSERT_MESSAGE (m_distanceApart != 0.f, "Distance between HUD perk icons shouldn't be zero! Check the XML!");

	xmlElement->GetAttribute("pulseSize", m_pulseSize);
	xmlElement->GetAttribute("alignHorizontal", m_alignHorizontal);
	xmlElement->GetAttribute("alignVertical", m_alignVertical);

	const char * strShowPerksWithFlags = xmlElement->GetAttribute("onlyShowPerksWithFlags");
	m_onlyShowPerksWithFlags = AutoEnum_GetBitfieldFromString(strShowPerksWithFlags, SPerkData::s_perkDataFlagNames, SPerkData::PerkDataFlagList_numBits);

	const char * strSkipPerksWithFlags = xmlElement->GetAttribute("skipPerksWithFlags");
	m_skipPerksWithFlags = AutoEnum_GetBitfieldFromString(strSkipPerksWithFlags, SPerkData::s_perkDataFlagNames, SPerkData::PerkDataFlagList_numBits);

	assert (m_onlyShowPerksWithFlags >= 0 && m_onlyShowPerksWithFlags < BIT(SPerkData::PerkDataFlagList_numBits));
	assert (m_skipPerksWithFlags >= 0 && m_skipPerksWithFlags < BIT(SPerkData::PerkDataFlagList_numBits));

#if IS_HUD_DEV
	CryLog ("Creating CUIPerkIconSet '%s' (showFlags=%u skipFlags=%u)", GetName(), m_onlyShowPerksWithFlags, m_skipPerksWithFlags);
#endif //IS_HUD_DEV

	m_activeBrightColour.Read(xmlElement->GetChild("ActiveBrightColour"));
	m_activeDrainedColour.Read(xmlElement->GetChild("ActiveDrainedColour"));
	m_inactiveBrightColour.Read(xmlElement->GetChild("InactiveBrightColour"));
	m_inactiveDrainedColour.Read(xmlElement->GetChild("InactiveDrainedColour"));
	m_availableColour.Read(xmlElement->GetChild("AvailableColour"));
	m_unavailableColour.Read(xmlElement->GetChild("UnavailableColour"));
}

void CUIPerkIconSet::Draw(void) const
{
	CUIElement::Draw();

#if 1
	// TODO: Add a 'TBitfield m_vanishWhen' member var to the CUIElement class and check it elsewhere [TF]
	CActor  *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());
	//assert (pActor);
	if (!pActor || pActor->GetSpectatorMode() != CActor::eASM_None)
	{
		return;
	}
#endif

	HudDbgDisplay("CUIPerkIconSet::Draw '%s' (requireFlags=%d skipWithFlags=%d)", GetName(), m_onlyShowPerksWithFlags, m_skipPerksWithFlags);
	int perksActive = 0;

	const CPerkIconData * perkIcons = CPerkIconData::GetLocalDataToRender();

	struct SPerkTempInfo
	{
		int theTexture;
		const CPerkIconData::SSinglePerkIconInfo * storedData;
	};

	SPerkTempInfo tempPerkInfo[k_numberOfPerkIcons];
	CPerk * perkInstance = CPerk::GetInstance();
	float totalWidth = 0.f;

	for(int i = 0 ; i < k_numberOfPerkIcons; i++)
	{
		EPerks perkID = perkIcons->m_icons[i].m_perkID;
		if(perkID != ePerk_Null)
		{
			CryFixedStringT<32> textureName;
			const SPerkData * perkData = perkInstance->GetPerkData(perkID);

			if(perkData->m_iconTexture != 0 && ((perkData->m_flag & m_onlyShowPerksWithFlags) == m_onlyShowPerksWithFlags) && ((perkData->m_flag & m_skipPerksWithFlags) == 0))
			{
				tempPerkInfo[perksActive].theTexture = perkData->m_iconTexture;
				tempPerkInfo[perksActive].storedData = & perkIcons->m_icons[i];
				perksActive++;
				totalWidth += HUDCurveFraction(perkIcons->m_icons[i].m_alpha);
			}
		}
	}

	const float scaleUsingWidthValues = (gEnv->pRenderer->GetWidth()/800.f); 
	const float scaleUsingHeightValues = (gEnv->pRenderer->GetHeight()/600.f);

	const float fullWidth = GetWidth();
	const float fullHeight = GetHeight();// * scaleUsingWidthValues / scaleUsingHeightValues;
	const float centreX = GetPosX();
	const float centreY = GetPosY();

	float acrossSoFar = 0.f;

	if (perksActive)
	{
		IActor * localActor = g_pGame->GetIGameFramework()->GetClientActor();
		ASSERT_IS_NOT_NULL(localActor);

		CPlayer * player = (CPlayer *)localActor;
		ASSERT_IS_NOT_NULL(player);

		C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
		ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();
		pLayoutManager->SetState(eSLO_ScaleMethod_WithY);

		for (int i = 0; i < perksActive; ++ i)
		{
			const SPerkTempInfo * iconInfo = & tempPerkInfo[i];
			const CPerkIconData::SSinglePerkIconInfo * storedData = iconInfo->storedData;
			const int textureID = iconInfo->theTexture;
			const float alpha = HUDCurveFraction(storedData->m_alpha);
			float width = fullWidth * alpha;
			float height = fullHeight * alpha;

			if(storedData->m_pulse > 0.0f)
			{
				float pulseTimeSquared = storedData->m_pulse * storedData->m_pulse;
				float pulseScale = pulseTimeSquared * (1.f - pulseTimeSquared);
				float pulseSize = 1.f + (pulseScale * m_pulseSize);
				width *= pulseSize;
				height *= pulseSize;
			}

			const float x = centreX + (0.5f * alpha + acrossSoFar - totalWidth * m_alignHorizontal) * m_distanceApart;
			const float y = centreY;

			acrossSoFar += alpha;

			//	HudDbgDisplay("Drawing draining perk icon at %.2f %.2f alpha=%f", x, y, alpha);

			const ColorF & tint = player->IsPerkActive(storedData->m_perkID) ? m_availableColour : m_unavailableColour;
			ColorF iconColour, darkenColour;

			// TODO: Do perk icon in red if in wrong suit mode...			
			if (storedData->m_flags & CPerkIconData::SSinglePerkIconInfo::eSSPIIF_Usable)
			{
				iconColour = tint * m_activeBrightColour;
				darkenColour = tint * m_activeDrainedColour;
			}
			else
			{
				iconColour = tint * m_inactiveBrightColour;
				darkenColour = tint * m_inactiveDrainedColour;
			}

			if (storedData->m_drainAmount < 1.f)
			{
				pRenderUtils->DrawImage( 
					textureID, 
					x, y + height * storedData->m_drainAmount,
					width, height * (1.f - storedData->m_drainAmount),
					0.0f, 
					iconColour,
					0.f, 1.f - storedData->m_drainAmount, 1.f, 0.f,
					UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP);
			}

			if (storedData->m_drainAmount > 0.f)
			{
				pRenderUtils->DrawImage( 
					textureID, 
					x, y, 
					width, height * storedData->m_drainAmount, 
					0.0f, 
					darkenColour,
					0.f, 1.f, 1.f, 1.f-storedData->m_drainAmount,
					UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP );
			}

#if PERK_ICONS_DO_BUTTON_PROMPTS
			switch (storedData->m_promptInfo.GetType())
			{
			case kCITV_icon:
				{
					const ITexture* iconTexture = storedData->m_promptInfo.GetTexture();
					pRenderUtils->DrawImage( 
						iconTexture->GetTextureID(),
						x + width * 0.5f, y - height * 0.125f, 
						width * 0.5f, height * 0.5f, 
						0.0f, 
						ColorF( 1.f, 1.f, 1.f, alpha ), 
						0.f, 1.f, 1.f, 0.f,
						UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP);
				}
				break;
			}
#endif
		}
	}

	return;
}

#if 0
//----------------------------------------------------------------------
void CUIPerkIconSet::GetBounds( vector2f* out_pos, vector2f* out_size ) const
{
	assert(out_pos);
	assert(out_size);

	const float scaleUsingWidthValues = (gEnv->pRenderer->GetWidth()/800.f);
	const float scaleUsingHeightValues = (gEnv->pRenderer->GetHeight()/600.f);

	const float width = GetWidth();
	const float height = GetHeight() * scaleUsingWidthValues / scaleUsingHeightValues;
	const float centreX = GetPosX();
	const float centreY = GetPosY();

	out_pos->x = GetPosX() - (width * 0.5f);
	out_pos->y = GetPosY() - (height * 0.5f);

	out_size->x = width;
	out_size->y = height;
}
#endif
