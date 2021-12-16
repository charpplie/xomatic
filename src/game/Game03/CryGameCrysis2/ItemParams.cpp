/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 30:8:2005   12:33 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Item.h"
#include "ItemSharedParams.h"
#include "ItemParamReader.h"
#include "Game.h"
#include <ISound.h>

//------------------------------------------------------------------------
void CItem::ReadProperties(IScriptTable *pProperties)
{
	if (pProperties)
	{
		GetEntityProperty("HitPoints", m_properties.hitpoints);
		GetEntityProperty("bPickable", m_properties.pickable);
		GetEntityProperty("bMounted", m_properties.mounted);
		GetEntityProperty("bPhysics", m_properties.physics);
		GetEntityProperty("bUsable", m_properties.usable);


		GetEntityProperty("Respawn", "bRespawn", m_respawnprops.respawn);
		GetEntityProperty("Respawn", "nTimer", m_respawnprops.timer);
		GetEntityProperty("Respawn", "bUnique", m_respawnprops.unique);
	}
}

//------------------------------------------------------------------------
void CItem::InitItemFromParams()
{
	InitGeometry();
	InitAccessories();
	InitDamageLevels();
}

//------------------------------------------------------------------------
void CItem::InitGeometry()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	int geomSlots = m_sharedparams->geometry.size();

	for(int i = 0; i < geomSlots; i++)
	{
		const SGeometryDef& pGeom = m_sharedparams->geometry[i];

		assert(pGeom.slot < eIGS_Last);

		if(pGeom.slot == eIGS_FirstPerson)
		{
			m_fpgeometry[pGeom.hand].name = pGeom.modelPath;
			m_fpgeometry[pGeom.hand].position = pGeom.pos;
			m_fpgeometry[pGeom.hand].angles = DEG2RAD(pGeom.angles);
			m_fpgeometry[pGeom.hand].scale = pGeom.scale;
		}
		else //First person is loaded upon entering first person view (is this necessary since we no longer have different models per hand?
		{
			SetGeometry(pGeom.slot, pGeom.modelPath, pGeom.pos, pGeom.angles, pGeom.scale, false);
		}

		PlayAction(m_idleAnimation[pGeom.slot], 0, true, (eIPAF_Default|eIPAF_NoBlend)&~eIPAF_OwnerAnimGraph);
	}

	ForceSkinning(true);
}

//-----------------------------------------------------------------------
void CItem::InitAccessories()
{
	m_initialSetup.resize(0);

	m_initialSetup = m_sharedparams->initialSetup;
}

//-----------------------------------------------------------------------
void CItem::InitDamageLevels()
{
	int numLevels = m_sharedparams->damageLevels.size();

	m_damageLevelEffects.clear();
	m_damageLevelEffects.reserve(numLevels);

	for(int i = 0; i < numLevels; i++)
	{
		m_damageLevelEffects[i] = -1;
	}
}