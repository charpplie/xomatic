/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Dual character proxy, to play animations on fp character and shadow character

-------------------------------------------------------------------------
History:
- 09-11-2009		Benito G.R. - Extracted from Player

*************************************************************************/

#pragma once

#ifndef _DUALCHARACTER_PROXY_H_
#define _DUALCHARACTER_PROXY_H_

#include <IAnimationGraph.h>

class CAnimationProxyDualCharacterBase : public CAnimationPlayerProxy
{
public:

	static void RegisterAnimSet(IAnimationSet *animSet);
	static void ReleaseBuffers();

	CAnimationProxyDualCharacterBase();
	virtual bool StartAnimation(IEntity *entity, const char* szAnimName, const CryCharAnimationParams& Params, float speedMultiplier = 1.0f);

protected:

	struct SPlayParams
	{
		int animIDFP;
		int animIDTP;
		float speedFP;
		float speedTP;
	};

	void GetPlayParams(int animID, float speedMul, IAnimationSet *animSet, SPlayParams &params);

	static int Get3PAnimID(IAnimationSet *animSet, int animID);

	struct S1PMapping
	{
		IAnimationSet *animSet;
		uint16 *mappings;
	};

	static std::vector<S1PMapping> m_1PMapping;

	int m_characterMain;
	int m_characterShadow;
	bool m_firstPersonMode;
};

class CAnimationProxyDualCharacter : public CAnimationProxyDualCharacterBase
{
public:

	CAnimationProxyDualCharacter();

	virtual bool StartAnimationById(IEntity *entity, int animId, const CryCharAnimationParams& Params, float speedMultiplier = 1.0f);
	virtual bool StopAnimationInLayer(IEntity *entity, int32 nLayer, f32 BlendOutTime);
	virtual bool RemoveAnimationInLayer(IEntity *entity, int32 nLayer, uint32 token);
	virtual const CAnimation *GetAnimation(IEntity *entity, int32 layer);
	virtual const CAnimation *GetAnimation(IEntity *entity, int32 layer, uint32 token);

	void SetFirstPerson(bool FP)
	{
		m_firstPersonMode = FP;
	}

	bool CanMixUpperBody() const
	{
		return m_allowsMix;
	}

private:
	bool m_killMixInFirst;
	bool m_allowsMix;
};

class CAnimationProxyDualCharacterUpper : public CAnimationProxyDualCharacterBase
{
public:

	CAnimationProxyDualCharacterUpper();

	virtual bool StartAnimationById(IEntity *entity, int animId, const CryCharAnimationParams& Params, float speedMultiplier = 1.0f);
	virtual bool StopAnimationInLayer(IEntity *entity, int32 nLayer, f32 BlendOutTime);
	virtual bool RemoveAnimationInLayer(IEntity *entity, int32 nLayer, uint32 token);

	void SetFirstPerson(bool FP)
	{
		m_firstPersonMode = FP;
	}

private:
	bool m_killMixInFirst;
};

#endif //_DUALCHARACTER_PROXY_H_