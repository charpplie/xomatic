/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Burst Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 26:10:2005   12:15 : Created by Márcio Martins

*************************************************************************/
#ifndef __BURST_H__
#define __BURST_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"

class CBurst : public CSingle
{
public:
	CBurst();
	virtual ~CBurst();

	// CSingle
	virtual void Update(float frameTime, uint32 frameId);
	virtual void GetMemoryUsage(ICrySizer * s) const
	{ 
		s->AddObject(this, sizeof(*this));	
		CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
	}

	virtual void Activate(bool activate);
	virtual bool CanFire(bool considerAmmo /* = true */) const;

	virtual void StartFire();
	virtual void StopFire();
	virtual const char *GetType() const;
	// ~CSingle

protected:
	int		m_burst_shot;
	bool	m_bursting;
	bool	m_fireTriggerDown;

	float	m_next_burst_dt;
	float	m_next_burst;

	bool  m_canShoot;

private:
	const ItemString& GetBurstFireAction(const int ammoCount) const;
	void SetBurstSoundParam(tSoundID burstSoundId, const int ammoCount) const;
};

#endif