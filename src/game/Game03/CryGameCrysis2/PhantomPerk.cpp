#include "StdAfx.h"
#include "PhantomPerk.h"

#include "Item.h"
#include "Player.h"
#include "Perk.h"
#include "NanoSuitDefs.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

PhantomPerk::PhantomPerk(const float* suitEnegyScale1, const float* suitEnegyScale2, const float* suitEnegyScale3)
{
	m_suitEnergyScale[eTierOne] = suitEnegyScale1;
	m_suitEnergyScale[eTierTwo] = suitEnegyScale2;
	m_suitEnergyScale[eTierThree] = suitEnegyScale3;
}

void PhantomPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_SetCloak:
		{
			bool *pEnable = static_cast<bool*>(data);
			bool enabled = (*pEnable);
			SetPhantom(enabled);
			break;
		}
	case EPE_OverrideStealthEnergyParams:
		{
			SNanoSuitEnergyParams *pEnergyParams = static_cast<SNanoSuitEnergyParams*>(data);

			const float suitEnergyScale = (*m_suitEnergyScale[GetTier()]);
			pEnergyParams->consumptionRateMin *= suitEnergyScale;
			pEnergyParams->consumptionRateMax *= suitEnergyScale;

			break;
		}
	default:
		{
			IPerk::HandleEvent (perkEvent, data);
			break;
		}
	}
}


void PhantomPerk::SetPhantom(bool enable)
{
	IInventory *pInventory = m_ownerPlayer->GetInventory();
	if (enable)
	{
		CCCPOINT(Perk_Phantom_Enter);
		CAudioSignalPlayer::JustPlay("Perk_Phantom_Enter", m_ownerPlayer->GetEntityId());
	}
	else
	{
		CCCPOINT(Perk_Phantom_Exit);
	}

	SetEntityPhantom(enable, m_ownerPlayer->GetEntity());

	if(pInventory)
	{
		for (int i = 0; i < pInventory->GetCount(); i++)
		{
			EntityId id = pInventory->GetItem(i);
			CItem *pItem=(CItem*)gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(id);
			if(pItem)
			{
				SetEntityPhantom(enable, pItem->GetEntity());
				const CItem::TAccessoryMap *accessories=pItem->GetAttachedAccessories();
				for(CItem::TAccessoryMap::const_iterator it = accessories->begin();it!=accessories->end();it++)
				{
					CItem* pAccesory = static_cast<CItem*>(gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(it->second));
					SetEntityPhantom(enable, pAccesory->GetEntity());
				}
			}
		}
	}
}


void PhantomPerk::SetEntityPhantom(bool enable, IEntity* pEntity)
{
	if(pEntity)
	{
		if (enable)
		{
			pEntity->ClearFlags(ENTITY_FLAG_CASTSHADOW);
		}
		else
		{
			pEntity->AddFlags(ENTITY_FLAG_CASTSHADOW);
		}
	}
}
