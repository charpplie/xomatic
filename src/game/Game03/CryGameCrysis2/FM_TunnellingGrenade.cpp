/*************************************************************************
Crytek Source File.
Copyright (C) Crytek Studios 2001-2004.
-------------------------------------------------------------------------
$Id: FM_TunnellingGrenade.cpp$
$DateTime$
Description: Fire mode for Tunnelling Grenade. Basically just the Single firemode with additional HUD info.

-------------------------------------------------------------------------
History:
- 07/05/2009 12:59:26: Created by tombe

*************************************************************************/

#include "StdAfx.h"
#include "FM_TunnellingGrenade.h"
#include "TunnelingGrenade.h"
#include "IRenderAuxGeom.h"
#include "Actor.h"
#include "WeaponSystem.h"
#include "GameCVars.h"
#include "WeaponSharedParams.h"


CTunnellingGrenadeFM::CTunnellingGrenadeFM()
	:
	BaseClass(),
	m_drillerPenetration(-1.f),
	m_blockedTime(0.0f),
	m_blocked(false)
{
}

CTunnellingGrenadeFM::~CTunnellingGrenadeFM()
{
}

void CTunnellingGrenadeFM::Activate(bool activate)
{
	if (activate && m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsClient())
	{
		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}

	BaseClass::Activate(activate);
}

void CTunnellingGrenadeFM::Update(float frameTime, uint32 frameId)
{
	BaseClass::Update(frameTime, frameId);

	if (m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsClient())
	{	
		assert(m_drillerPenetration > 0.f);

		float testLength = m_fireParams->drillerParams.blockTestRange + m_drillerPenetration;

		IMovementController * pMC = m_pWeapon->GetOwnerActor()->GetMovementController();
		bool successful = true;

		if (pMC)
		{ 
			SMovementState info;
			pMC->GetMovementState(info);

			Vec3 launchPos = info.weaponPosition;
			Vec3 launchDir = info.fireDirection;
		
			assert(launchDir.IsUnit());

			Vec3 testDir = testLength * launchDir;

			//--- Physics probe to get target pos, point beneath the surface, just ahead of us
			IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
			const int MAX_HITS = 5;
			ray_hit hits[MAX_HITS];
			int flags   = DRILLER_PROBE_FLAGS&~(rwi_pierceability_mask|rwi_ignore_back_faces|rwi_ignore_solid_back_faces);
			int numHits = pWorld->RayWorldIntersection(launchPos, testDir, DRILLER_PROBE_TYPES, flags|rwi_separate_important_hits, hits, MAX_HITS);
		
			if(g_pGame->GetCVars()->i_debug_projectiles)
			{
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(launchPos, ColorB(0,0,255), launchPos + testDir, ColorB(0,255,255));

				for (int i = 0; i < MAX_HITS; i++)
				{
					if (hits[i].dist >= 0)
					{
						gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(hits[i].pt,0.2f,ColorB(0, 255, 0));
					}		
				}
			}

			if(numHits && ((hits[0].dist >= 0 && hits[0].dist < m_fireParams->drillerParams.blockTestRange) 
											|| (hits[1].dist >= 0 && hits[1].dist < m_fireParams->drillerParams.blockTestRange))) // the first hit could be in 1 or 0 so check at least one of them is valid first
			{
				successful = false;
				CTunnellingGrenade::TSurfaceList surfaceList;
				CTunnellingGrenade::AnalyseSurfaces(hits, numHits, testDir, surfaceList, false);

				bool surfaceFound = (surfaceList.size() > 0);
				if(surfaceFound)
				{
					float depth = surfaceList.front().depth;
					successful = (depth > 0.0f && depth < m_drillerPenetration);
				}
				

				if (!successful && surfaceFound) //don't bother with the second linetest if no surfaces entered
				{
					assert(surfaceList.front().entryDist < m_fireParams->drillerParams.blockTestRange);

					Vec3 resizedTestLength = (launchDir * (surfaceList.front().entryDist + m_drillerPenetration));
					Vec3 testPos = launchPos + resizedTestLength;
					Vec3 reverseTestDir = launchDir * -m_drillerPenetration;

					numHits = pWorld->RayWorldIntersection(testPos, reverseTestDir, DRILLER_PROBE_TYPES, flags|rwi_separate_important_hits, hits, MAX_HITS);

					CTunnellingGrenade::AnalyseSurfaces(hits, numHits, resizedTestLength, surfaceList, true);

					float depth = surfaceList.front().depth;

					successful = (depth > 0.0f && depth < m_drillerPenetration);
				}
			}
		}

		if (!successful)
		{
			m_blockedTime = m_fireParams->drillerParams.blockMinDisplayTime;
		}
		else
		{
			m_blockedTime -= frameTime;
			if (m_blockedTime < 0.0f)
				m_blockedTime = 0.0f;
		}

		if (m_blocked != (m_blockedTime > 0.0f))
		{
			m_blocked = (m_blockedTime > 0.0f);

			// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
			const char *SOUND_NAME[2] = {"Sounds/crysiswars2:weapons:Driller/Unblocked", "Sounds/crysiswars2:weapons:Driller/Blocked"};
			_smart_ptr<ISound> pReflectedSound = gEnv->pSoundSystem->CreateSound(SOUND_NAME[m_blocked], FLAG_SOUND_EVENT);
			if (pReflectedSound)
			{
				pReflectedSound->Play();
			}
		}

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

void CTunnellingGrenadeFM::ResetParams(const struct IItemParamsNode *params)
{
	BaseClass::ResetParams(params);
	ResetDrillerParams();
}

void CTunnellingGrenadeFM::ResetDrillerParams()
{
	const SAmmoParams *pAmmoParams = g_pGame->GetWeaponSystem()->GetAmmoParams(GetAmmoType());
	const IItemParamsNode *grenadeParams = pAmmoParams->pItemParams->GetChild("params");
	CItemParamReader grenadeReader(grenadeParams);
	grenadeReader.Read("PenetrationDepth", m_drillerPenetration);
}