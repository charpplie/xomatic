#include "StdAfx.h"
#include "EMPStrike.h"
#include "Game.h"
#include "GameCVars.h"
#include <IRenderAuxGeom.h>
#include "Perk.h"

SStrikeParams CEMPStrike::s_empStrikeParams;

//------------------------------------------------------------------------
CEMPStrike::CEMPStrike()
{
	m_audioSignal = "StrikeEMP";
}

//------------------------------------------------------------------------
bool CEMPStrike::Init(IGameObject *pGameObject)
{
	if(CProjectile::Init(pGameObject))
	{
		// Only initialise static data once
		if(s_empStrikeParams.isInitialised == false)
		{
			s_empStrikeParams.effect = m_pAmmoParams->GetCachedEffect("emp_strike");
			s_empStrikeParams.isInitialised = true;
		}

		// Initialise instance data from static data
		m_explodeTime = CPerk::GetInstance()->GetVars()->perk_empStrikeImpactTime;
		m_effect			= s_empStrikeParams.effect;

		return true;
	}
	return false;
}

//------------------------------------------------------------------------
void CEMPStrike::UpdateEffect()
{
#ifdef _DEBUG
	static bool drawDebug = false;
	if(drawDebug)
	{
		//Currently just changes color but will eventually be completely different rendering
		ColorB color = ColorB(100, 100, 255);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(m_initial_pos, color, m_targetPos, color, 6.0f);

		float distanceTravelled = m_explodeTimer / m_explodeTime;
		const Vec3 down(0.0f, 0.0f, -1.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_initial_pos + ((down * m_distanceToTravel) * distanceTravelled), 1.0f, color);
	}
#endif
}
