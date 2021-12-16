/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: MikeBullet

-------------------------------------------------------------------------
History:
- 25:1:2010   : Created by Filipe Amim

*************************************************************************/
#pragma once

#ifndef __MIKE_BULLET_H__
#define __MIKE_BULLET_H__

#include "Bullet.h"


struct IParticleEmitter;


class CBurnEffectManager
{
private:
	struct SBurnPoint
	{
		IParticleEmitter* m_effect;
		IDecalRenderNode* m_pDecal;
		IMaterial* m_pDecalMaterial;
		Vec3 m_position;
		float m_radius;
		float m_accumulation;
	};

	typedef std::vector<SBurnPoint> TBurnPoints;

public:
	CBurnEffectManager();
	~CBurnEffectManager();

	void AddBurnPoint(const EventPhysCollision& pCollision);
	void Update(float deltaTime);
	void Reset();

private:

	TBurnPoints::iterator FindClosestBurnPoint(const Vec3& point);
	void CreateBurnEffect(const EventPhysCollision& pCollision, SBurnPoint* pBurnPoint);
	void UpdateBurnEffect(SBurnPoint* pBurnPoint);
	void DestroyBurnEffect(SBurnPoint* pBurnPoint);
	void DebugDraw();

	TBurnPoints m_burnPoints;
};



class CMikeBullet : public CBullet
{
public:
	virtual void HandleEvent(const SGameObjectEvent &);
};


#endif
