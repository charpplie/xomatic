/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Throw Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 261:10:2005   15:45 : Created by Márcio Martins
-  18:07:2008		Slightly Refactored (cleaned-up): Benito G.R.

*************************************************************************/
#ifndef __THROW_H__
#define __THROW_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Single.h"

class CThrow : public CSingle
{
	struct ThrowAction;
	struct FinishAction;
	struct PrimeAction;
	struct HoldAction;
	struct ShowItemAction;
		
public:
	CThrow();
	virtual ~CThrow();

	virtual void Update(float frameTime, uint32 frameId);
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void SetProjectileThrowParams(const SProjectileThrowParams &throwParams);

	virtual void Activate(bool activate);

	virtual bool CanReload() const;

	virtual const char* GetType() const
	{
		return "Thrown";
	}

	virtual void StartFire();
	virtual void StopFire();

	virtual void NetStartFire();
	virtual void NetStopFire();
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle);
	virtual void ReplayShoot();

	virtual bool IsReadyToFire() const;
	virtual bool Shoot(bool resetAnimation, bool autoreload =true , bool isRemote=false );

	// If we need more of this secondary functions, we might consider create one common in IFireMode interface
	virtual void Prime();
	virtual bool IsReadyToThrow() const { return (m_firing && !m_throwing); }

protected:
	virtual void CheckNearMisses(const Vec3 &probableHit, const Vec3 &pos, const Vec3 &dir, float range, float radius);
	virtual bool CheckAmmo();
	virtual void DoThrow();
	virtual void DisplayTrajectory();

private:
	bool    m_throwing;
	bool		m_primed;
	float		m_primeTime;
	float   m_projectileLifeTime;

	float			m_startFireTime;
	tSoundID	m_primedLoopSound;
};

#endif 
