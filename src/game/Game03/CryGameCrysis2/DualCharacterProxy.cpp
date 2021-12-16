/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Dual character proxy, to play animations on fp character and shadow character

-------------------------------------------------------------------------
History:
- 09-11-2009		Benito G.R. - Extracted from Player

*************************************************************************/

#include "StdAfx.h"
#include "DualCharacterProxy.h"

#include "RecordingSystem.h"
#include "GameCVars.h"

#include "StringUtils.h"

static float GetTPSpeedMul(IAnimationSet *animSet, int animIDFP, int animIDTP)
{
	float TPSpeedMul = 1.0f;

	if ((animIDFP >= 0) && (animIDTP >= 0))
	{
		float durationFP = animSet->GetDuration_sec(animIDFP);
		float durationTP = animSet->GetDuration_sec(animIDTP);
		TPSpeedMul = (durationTP / durationFP);
	}

	return TPSpeedMul;
}

std::vector<CAnimationProxyDualCharacterBase::S1PMapping> CAnimationProxyDualCharacterBase::m_1PMapping;

void CAnimationProxyDualCharacterBase::RegisterAnimSet(IAnimationSet *animSet)
{
	for (int i=0; i<m_1PMapping.size(); i++)
	{
		if (m_1PMapping[i].animSet == animSet)
		{
			return;
		}
	}

	uint32 numPairs = 0;
	uint32 numMissingPairs = 0;
	uint32 numAnims = animSet->GetAnimationCount();

	int mapID = m_1PMapping.size();
	m_1PMapping.resize(mapID+1);
	S1PMapping &mappingSet = m_1PMapping[mapID];
	mappingSet.animSet = animSet;
	mappingSet.mappings = new uint16[numAnims];
	for (uint32 anm = 0; anm < numAnims; anm++)
	{
		int animID3P = -1;
		const char *name = animSet->GetNameByAnimID(anm);
		if (strlen(name) >= 255)
		{
			CRY_ASSERT_MESSAGE(0, string().Format("[CAnimationProxyDualCharacterBase::RegisterAnimSet] Animname %s overruns buffer", name));
			CryLogAlways("[CAnimationProxyDualCharacterBase::RegisterAnimSet] Animname %s overruns buffer", name);
			continue;
		}
		const char *pos = CryStringUtils::stristr(name, "_1p");
		if (pos)
		{
			char name3P[256];
			strcpy(name3P, name);
			name3P[(int)pos + 1 - (int)name] = '3';
			animID3P = animSet->GetAnimIDByName(name3P);
			if (animID3P < 0)
			{
				numMissingPairs++;
			}
		}

		if (animID3P < 0)
		{
			mappingSet.mappings[anm] = anm;
		}
		else
		{
			mappingSet.mappings[anm] = animID3P;
			numPairs++;
		}
	}

	CryLogAlways("Linking 1P3P pairs NumAnims:%d Matched:%d Missing:%d", numAnims, numPairs, numMissingPairs);
}

void CAnimationProxyDualCharacterBase::ReleaseBuffers()
{
	for (int i=0; i<m_1PMapping.size(); i++)
	{
		delete [] m_1PMapping[i].mappings;
	}
	m_1PMapping.resize(0);
}

int CAnimationProxyDualCharacterBase::Get3PAnimID(IAnimationSet *animSet, int animID)
{
	if (animID >= 0)
	{
		for (int i=0; i<m_1PMapping.size(); i++)
		{
			if (m_1PMapping[i].animSet == animSet)
			{
				return m_1PMapping[i].mappings[animID];
			}
		}
	}

	return animID;
}

CAnimationProxyDualCharacterBase::CAnimationProxyDualCharacterBase()
:
	m_characterMain(0),
	m_characterShadow(5),
	m_firstPersonMode(true)
{
}

void CAnimationProxyDualCharacterBase::GetPlayParams(int animID, float speedMul, IAnimationSet *animSet, SPlayParams &params)
{
	params.animIDFP = animID;
	params.animIDTP = Get3PAnimID(animSet, animID);
	params.speedFP  = speedMul;
	params.speedTP  = speedMul;

	if (params.animIDFP != params.animIDTP)
	{
		params.speedTP *= GetTPSpeedMul(animSet, params.animIDFP, params.animIDTP);
	}
}

bool CAnimationProxyDualCharacterBase::StartAnimation(IEntity *entity, const char* szAnimName, const CryCharAnimationParams& Params, float speedMultiplier)
{
	ICharacterInstance* pICharacter = entity->GetCharacter(m_characterMain);
	int animID = pICharacter->GetIAnimationSet()->GetAnimIDByName(szAnimName);

#if !defined(_RELEASE)
		if (g_pGameCVars->g_animatorDebug)
		{
			static const ColorF col (1.0f, 0.0f, 0.0f, 1.0f);
			if (animID < 0) 
			{
				g_pGame->GetIGameFramework()->GetIPersistantDebug()->Add2DText(string().Format("Missing %s", szAnimName).c_str(), 1.0f, col, 10.0f);
			}
		}
#endif //!defined(_RELEASE)

	return StartAnimationById(entity, animID, Params, speedMultiplier);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CAnimationProxyDualCharacter::CAnimationProxyDualCharacter()
{
	m_firstPersonMode = true;
	m_killMixInFirst = true;
}

bool CAnimationProxyDualCharacter::StartAnimationById(IEntity *entity, int animId, const CryCharAnimationParams& Params, float speedMultiplier)
{
	ICharacterInstance* pICharacter = entity->GetCharacter(m_characterMain);
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;
	IAnimationSet* pAnimSet = pICharacter->GetIAnimationSet();

	SPlayParams playParams;
	GetPlayParams(animId, speedMultiplier, pAnimSet, playParams);

	if (pAnimSet && (playParams.animIDTP >= 0) && g_pGame->GetRecordingSystem())
	{
		SRecording_Animation a;
		a.type = eTPP_ActorAnim;
		a.eid = entity->GetId();
		a.animID = playParams.animIDTP;
		a.speedMultiplier = speedMultiplier;
		a.animparams = Params;
		g_pGame->GetRecordingSystem()->QueueAddPacket(a);
	}

	bool shadowLocomotion = !(((Params.m_nFlags & CA_DISABLE_MULTILAYER) || (Params.m_nLayerID != 0)) || !m_killMixInFirst);
	bool bAnimStarted = false;
	bool bShouldPlayShadow = true;
	if (!m_firstPersonMode || !shadowLocomotion)
	{
		bAnimStarted = pISkeletonAnim->StartAnimationById(playParams.animIDTP, Params);
		if (bAnimStarted)
		{
			pISkeletonAnim->SetAdditiveWeight(Params.m_nLayerID, 1.0f);
			pISkeletonAnim->SetLayerUpdateMultiplier(Params.m_nLayerID, m_firstPersonMode ? playParams.speedFP : playParams.speedTP);
		}
		else
			bShouldPlayShadow = false;
	}

	if (bShouldPlayShadow && pIShadowCharacter)
	{
		ISkeletonAnim* pIShadowSkeletonAnim = pIShadowCharacter->GetISkeletonAnim();
		if (pIShadowSkeletonAnim->StartAnimationById(playParams.animIDTP, Params))
		{
			bAnimStarted = true;

			pIShadowSkeletonAnim->SetAdditiveWeight(Params.m_nLayerID, 1.0f);
			pIShadowSkeletonAnim->SetLayerUpdateMultiplier(Params.m_nLayerID, playParams.speedTP);
		}
	}

	if (bAnimStarted && (Params.m_nLayerID == 0))
	{
		m_allowsMix = (Params.m_nFlags & CA_DISABLE_MULTILAYER) == 0;
	}

	return bAnimStarted;
}

bool CAnimationProxyDualCharacter::StopAnimationInLayer(IEntity *entity, int32 nLayer, f32 BlendOutTime)
{
	ICharacterInstance* pICharacter = entity->GetCharacter(m_characterMain);
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;

	if (pIShadowCharacter)
	{
		ISkeletonAnim* pIShadowSkeletonAnim = pIShadowCharacter->GetISkeletonAnim();

		bool stopped = pIShadowSkeletonAnim->StopAnimationInLayer(nLayer, BlendOutTime);
		if (m_allowsMix)
		{
			return stopped;
		}
	}

	return pISkeletonAnim->StopAnimationInLayer(nLayer, BlendOutTime);
}

bool CAnimationProxyDualCharacter::RemoveAnimationInLayer(IEntity *entity, int32 nLayer, uint32 token)
{
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;
	ICharacterInstance* pICharacter = pIShadowCharacter ? pIShadowCharacter : entity->GetCharacter(m_characterMain);
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();

	if (pIShadowCharacter)
	{
		ISkeletonAnim* pIShadowSkeletonAnim = pIShadowCharacter->GetISkeletonAnim();
		int nAnimsInFIFO = pIShadowSkeletonAnim->GetNumAnimsInFIFO(nLayer);
		for (int i=0; i<nAnimsInFIFO; ++i)
		{
			const CAnimation& anim = pIShadowSkeletonAnim->GetAnimFromFIFO(nLayer, i);
			if (anim.m_AnimParams.m_nUserToken == token)
			{
				pIShadowSkeletonAnim->RemoveAnimFromFIFO(nLayer, i);
			}
		}
	}

	int nAnimsInFIFO = pISkeletonAnim->GetNumAnimsInFIFO(nLayer);
	for (int i=0; i<nAnimsInFIFO; ++i)
	{
		const CAnimation& anim = pISkeletonAnim->GetAnimFromFIFO(nLayer, i);
		if (anim.m_AnimParams.m_nUserToken == token)
		{
			return pISkeletonAnim->RemoveAnimFromFIFO(nLayer, i);
		}
	}

	return false;
}

const CAnimation *CAnimationProxyDualCharacter::GetAnimation(IEntity *entity, int32 layer)
{
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;
	ICharacterInstance* pICharacter = pIShadowCharacter ? pIShadowCharacter : entity->GetCharacter(m_characterMain);
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();

	int nAnimsInFIFO = pISkeletonAnim->GetNumAnimsInFIFO(layer);
	if (nAnimsInFIFO > 0)
	{
		return &pISkeletonAnim->GetAnimFromFIFO(layer, nAnimsInFIFO-1);
	}

	return NULL;
}

const CAnimation *CAnimationProxyDualCharacter::GetAnimation(IEntity *entity, int32 layer, uint32 token)
{
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;
	ICharacterInstance* pICharacter = pIShadowCharacter ? pIShadowCharacter : entity->GetCharacter(m_characterMain);
	if (pICharacter)
	{
		ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();

		int nAnimsInFIFO = pISkeletonAnim->GetNumAnimsInFIFO(layer);
		if (nAnimsInFIFO == 0)
		{
			return NULL;
		}
		if (token == INVALID_ANIMATION_TOKEN)
		{
			return &pISkeletonAnim->GetAnimFromFIFO(layer, 0);
		}
		else
		{
			return pISkeletonAnim->FindAnimInFIFO(token, layer, true);
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CAnimationProxyDualCharacterUpper::CAnimationProxyDualCharacterUpper()
{
	m_characterMain = 0;
	m_characterShadow = 5;
	m_firstPersonMode = true;
	m_killMixInFirst = true;
}

bool CAnimationProxyDualCharacterUpper::StartAnimationById(IEntity *entity, int animId, const CryCharAnimationParams& Params, float speedMultiplier)
{
	ICharacterInstance* pICharacter = entity->GetCharacter(m_characterMain);
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;
	IAnimationSet* pAnimSet = pICharacter->GetIAnimationSet();

	SPlayParams playParams;
	GetPlayParams(animId, speedMultiplier, pAnimSet, playParams);

	if (g_pGame->GetRecordingSystem() && pAnimSet && (playParams.animIDTP >= 0))
	{
		SRecording_Animation a;
		a.type = eTPP_ActorUpperAnim;
		a.eid = entity->GetId();
		a.animID = playParams.animIDTP;
		a.speedMultiplier = speedMultiplier;
		a.animparams = Params;
		g_pGame->GetRecordingSystem()->QueueAddPacket(a);
	}

	bool startedMain = false;
	if (m_firstPersonMode)
	{
		if (Params.m_nLayerID != 0)
		{
			startedMain = pISkeletonAnim->StartAnimationById(playParams.animIDFP, Params);
		}
	}
	else
	{
		startedMain = pISkeletonAnim->StartAnimationById(playParams.animIDTP, Params);
	}

	if (startedMain)
	{
		pISkeletonAnim->SetAdditiveWeight(Params.m_nLayerID, 1.0f);
		pISkeletonAnim->SetLayerUpdateMultiplier(Params.m_nLayerID, m_firstPersonMode ? playParams.speedFP : playParams.speedTP);
	}

	if (pIShadowCharacter && (Params.m_nLayerID != 0))
	{
		ISkeletonAnim* pIShadowSkeletonAnim = pIShadowCharacter->GetISkeletonAnim();

		if (pIShadowSkeletonAnim->StartAnimationById(playParams.animIDTP, Params))
		{
			pIShadowSkeletonAnim->SetAdditiveWeight(Params.m_nLayerID, 1.0f);
			pIShadowSkeletonAnim->SetLayerUpdateMultiplier(Params.m_nLayerID, playParams.speedTP);
		}
	}

	return startedMain;
}

bool CAnimationProxyDualCharacterUpper::StopAnimationInLayer(IEntity *entity, int32 nLayer, f32 BlendOutTime)
{
	ICharacterInstance* pICharacter = entity->GetCharacter(m_characterMain);
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;

	if (pIShadowCharacter)
	{
		ISkeletonAnim* pIShadowSkeletonAnim = pIShadowCharacter->GetISkeletonAnim();

		pIShadowSkeletonAnim->StopAnimationInLayer(nLayer, BlendOutTime);
	}

	return pISkeletonAnim->StopAnimationInLayer(nLayer, BlendOutTime);
}

bool CAnimationProxyDualCharacterUpper::RemoveAnimationInLayer(IEntity *entity, int32 nLayer, uint32 token)
{
	ICharacterInstance* pIShadowCharacter = m_firstPersonMode ? entity->GetCharacter(m_characterShadow) : NULL;
	ICharacterInstance* pICharacter = pIShadowCharacter ? pIShadowCharacter : entity->GetCharacter(m_characterMain);
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();

	if (pIShadowCharacter)
	{
		ISkeletonAnim* pIShadowSkeletonAnim = pIShadowCharacter->GetISkeletonAnim();
		int nAnimsInFIFO = pIShadowSkeletonAnim->GetNumAnimsInFIFO(nLayer);
		for (int i=0; i<nAnimsInFIFO; ++i)
		{
			const CAnimation& anim = pIShadowSkeletonAnim->GetAnimFromFIFO(nLayer, i);
			if (anim.m_AnimParams.m_nUserToken == token)
			{
				pIShadowSkeletonAnim->RemoveAnimFromFIFO(nLayer, i);
			}
		}
	}

	int nAnimsInFIFO = pISkeletonAnim->GetNumAnimsInFIFO(nLayer);
	for (int i=0; i<nAnimsInFIFO; ++i)
	{
		const CAnimation& anim = pISkeletonAnim->GetAnimFromFIFO(nLayer, i);
		if (anim.m_AnimParams.m_nUserToken == token)
		{
			return pISkeletonAnim->RemoveAnimFromFIFO(nLayer, i);
		}
	}

	return false;
}