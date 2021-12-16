/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Throw Fire Mode Extension (for objects, NPCs...)
-------------------------------------------------------------------------
History:
- 18:07:2008 - Benito G.R.

*************************************************************************/
#ifndef __THROWEX_H__
#define __THROWEX_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"

class CThrowEx : public CSingle
{
	struct ThrowAction;

public:
	CThrowEx();
	virtual ~CThrowEx();

	virtual void Update(float frameTime, uint32 frameId);
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void Activate(bool activate);

	virtual bool CanFire(bool considerAmmo) const;
	virtual bool CanReload() const;

	virtual const char* GetType() const
	{
		return "ThrownEx";
	}

	virtual bool IsReadyToFire() const;
	virtual void StartFire();
	virtual void StopFire();

	//virtual void NetStartFire();
	//virtual void NetStopFire();

	virtual void SetThrowable(EntityId entityId, ISchedulerAction *action);
	virtual bool Shoot(bool resetAnimation, bool autoreload = true , bool isRemote=false );
	
	virtual void RequestDrop() { m_dropRequested = true; };
	virtual void   ThrowLivingEntity(IEntity* pEntity, IPhysicalEntity* pPE, bool isDrop);


protected:

	virtual void DoThrow();
	virtual void DoDrop();

private:

	void   ThrowObject(IEntity* pEntity, IPhysicalEntity* pPE, bool isDrop);
	void   ApplyThrowEffects();
	bool   CheckForIntersections(IPhysicalEntity* heldEntity, Vec3 &dir);

private:

	bool m_throwing;
	bool m_dropRequested;

	ISchedulerAction	*m_throwableAction;
};

#endif 
