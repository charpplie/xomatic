/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: SuitDisruptionField

-------------------------------------------------------------------------
History:
- 26:05:2009   11:50 : Created by Tom Berry.

*************************************************************************/
#include "StdAfx.h"
#include "SuitDisruptionField.h"

#include <IRenderAuxGeom.h>
#include "Actor.h"
#include "GameCVars.h"
#include "Player.h"
#include "GameRules.h"


CSuitDisruptionField::CSuitDisruptionField(EntityId id)
:
	m_teamID(-1),
	m_active(false),
	m_outerRadius(14.0f),
	m_innerRadius(6.0f),
	m_maxEffect(150.0f),
	m_pos(0.0f, 0.0f, 0.0f),
	m_entityId(id)
{
	const char *effectName = "cw2_weapon_effects.NanoSuitDisruptor";
	m_activeEffect = gEnv->pParticleManager->FindEffect(effectName);
	IEntity *owner = gEnv->pEntitySystem->GetEntity(m_entityId);
	m_effectSlot = owner->LoadParticleEmitter(-1, m_activeEffect);
}

CSuitDisruptionField::~CSuitDisruptionField()
{
}

//----------------------------------------------
void CSuitDisruptionField::Update(float timePassed)
{
	SEntityProximityQuery query;
	query.box = AABB(Vec3(m_pos.x-m_outerRadius,m_pos.y-m_outerRadius,m_pos.z-m_outerRadius),Vec3(m_pos.x+m_outerRadius,m_pos.y+m_outerRadius,m_pos.z+m_outerRadius));
	int count=gEnv->pEntitySystem->QueryProximity(query);

	for (int i=0; i<count; i++)
	{
		bool sameTeam = (m_teamID == g_pGame->GetGameRules()->GetTeam(query.pEntities[i]->GetId()));

		IActor *pIActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(query.pEntities[i]->GetId());

		float distance = query.pEntities[i]->GetPos().GetDistance(m_pos);
		if (pIActor && (distance < m_outerRadius) && !sameTeam)
		{
			float factor = (distance > m_innerRadius) ? 1.0f - ((distance - m_innerRadius) / (m_outerRadius - m_innerRadius)) : 1.0f;

			if (pIActor->GetActorClass() == CPlayer::GetActorClassType())
			{
				CActor *pActor=static_cast<CActor *>(pIActor);

				SNanoSuitEvent event;
				event.event = eNanoSuitEvent_ENERGY_DRAIN;
				event.fParam = factor * m_maxEffect * timePassed;
				pActor->SendActorSuitEvent(event);
			}
		}
	}

	bool bDebug = g_pGameCVars->i_debug_projectiles > 0;
	if (bDebug)
	{
		static ColorB colourInner(RGBA8(0xff,0x00,0x00,0xaa));
		static ColorB colourOuter(RGBA8(0xff,0x00,0xff,0x70));
		static float SPHERE_RADIUS = 0.25f;
		SAuxGeomRenderFlags renderFlags;
		renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
		renderFlags.SetDepthWriteFlag(e_DepthWriteOff);
		renderFlags.SetDepthTestFlag(e_DepthTestOn);
		gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(renderFlags);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( m_pos, m_innerRadius, colourInner );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( m_pos, m_outerRadius, colourOuter );
	}
}

void CSuitDisruptionField::SetActive(bool active)
{
	CRY_ASSERT(m_entityId != 0);

	IEntity *owner = gEnv->pEntitySystem->GetEntity(m_entityId);

	if (active != m_active)
	{
		m_active = active;
		owner->GetParticleEmitter(m_effectSlot)->Activate(m_active);
		if (!active)
		{
			//--- Flush entity list
			for (TEntityList::iterator iter = m_effectedEntities.begin(); iter != m_effectedEntities.end(); iter++)
			{
				IEntity *ent = gEnv->pEntitySystem->GetEntity((*iter).id);
			}
			m_effectedEntities.clear();
		}
	}
}
