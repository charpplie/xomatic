////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   MFXSoundEffect.cpp
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: Sound effect
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "MFXSoundEffect.h"

CMFXSoundEffect::CMFXSoundEffect()
{
}

CMFXSoundEffect::~CMFXSoundEffect()
{
}

void CMFXSoundEffect::Execute(SMFXRunTimeEffectParams &params)
{
  FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	if (!(params.playflags & MFX_PLAY_SOUND))
		return;

	_smart_ptr<ISound> snd = 0;
	ISoundSystem *ssystem = gEnv->pSoundSystem;

	CryFixedStringT<256> sFixSoundName(m_soundParams.name.c_str());

	if (params.playSoundFP)
	{
		sFixSoundName += "_fp";
	}

	if (params.soundProxyEntityId)
	{
		IEntity *pEnt = gEnv->pEntitySystem->GetEntity(params.soundProxyEntityId);
		if (pEnt)
		{
			IEntitySoundProxy *pSoundProxy = (IEntitySoundProxy*)pEnt->GetProxy(ENTITY_PROXY_SOUND);
			if (!pSoundProxy)
				pSoundProxy = (IEntitySoundProxy*)pEnt->CreateProxy(ENTITY_PROXY_SOUND);
			if (pSoundProxy)
			{
				ISound *pSound = ssystem->CreateSound(sFixSoundName.c_str(), (params.soundNoObstruction || params.playSoundFP) ? 0 : FLAG_SOUND_DEFAULT_3D);
				
				if (pSound)
				{
					EntityId SkipEntIDs[2];
					SkipEntIDs[0] = params.src;
					SkipEntIDs[1] = params.trg;

					pSound->SetPhysicsToBeSkipObstruction(SkipEntIDs, 2);
					pSound->SetSemantic(params.soundSemantic);

					if (params.soundDistanceMult< 1.0f && !m_soundParams.bIgnoreDistMult)
						pSound->SetDistanceMultiplier(params.soundDistanceMult);

					for (int i=0; i<params.MAX_SOUND_PARAMS; ++i)
					{
						const char* soundParamName = params.soundParams[i].paramName;
						if (soundParamName && *soundParamName)
						{
							pSound->SetParam(soundParamName, params.soundParams[i].paramValue, false);
						}
					}

					pSound->GetInterfaceExtended()->SetVolume(params.soundScale);
					pSoundProxy->PlaySound(pSound, params.soundProxyOffset, FORWARD_DIRECTION);
				}
			}
		}
	}
	else
	{
		snd = ssystem->CreateSound(sFixSoundName.c_str(), (params.soundNoObstruction || params.playSoundFP) ? 0 : FLAG_SOUND_DEFAULT_3D);
		if (snd)
		{
			EntityId SkipEntIDs[2];
			SkipEntIDs[0] = params.src;
			SkipEntIDs[1] = params.trg;

			snd->SetPhysicsToBeSkipObstruction(SkipEntIDs, 2);
			snd->SetSemantic(params.soundSemantic);
			snd->SetPosition(params.pos);

			if (params.soundDistanceMult< 1.0f && !m_soundParams.bIgnoreDistMult)
				snd->SetDistanceMultiplier(params.soundDistanceMult);
			snd->GetInterfaceExtended()->SetVolume(params.soundScale);
			
			for (int i=0; i<params.MAX_SOUND_PARAMS; ++i)
			{
				const char* soundParamName = params.soundParams[i].paramName;
				if (soundParamName && *soundParamName)
					snd->SetParam(soundParamName, params.soundParams[i].paramValue, false);
			}
			snd->Play();
		}
	}
}

void CMFXSoundEffect::ReadXMLNode(XmlNodeRef &node)
{
	IMFXEffect::ReadXMLNode(node);
	m_soundParams.bIgnoreDistMult = false;
	XmlNodeRef nameNode = node->findChild("Name");
	if (nameNode)
	{
		m_soundParams.name = nameNode->getContent();
		node->getAttr("bIgnoreDistMult", m_soundParams.bIgnoreDistMult);
	}
}

IMFXEffectPtr CMFXSoundEffect::Clone()
{
	CMFXSoundEffect *clone = new CMFXSoundEffect();
	clone->m_soundParams = m_soundParams;
	clone->m_effectParams = m_effectParams;
	return clone;
}

void CMFXSoundEffect::GetResources(SMFXResourceList &rlist)
{
	SMFXSoundListNode *listNode = SMFXSoundListNode::Create();
	listNode->m_soundParams.name = m_soundParams.name.c_str();

  SMFXSoundListNode* next = rlist.m_soundList;

  if (!next)
    rlist.m_soundList = listNode;
  else
  { 
    while (next->pNext)
      next = next->pNext;

    next->pNext = listNode;
  }  
}
