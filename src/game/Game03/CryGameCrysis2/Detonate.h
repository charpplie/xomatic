/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Detonation Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 11:9:2004   15:00 : Created by Márcio Martins

*************************************************************************/
#ifndef __DETONATE_H__
#define __DETONATE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"

class CDetonate :
	public CSingle
{
	struct ExplodeAction;
protected:
public:
	CDetonate();
	virtual ~CDetonate();

	//IFireMode
	virtual void Update(float frameTime, uint32 frameId);
	virtual void GetMemoryUsage(ICrySizer * s) const
	{ 
		s->AddObject(this, sizeof(*this));	
		CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
	}

	virtual void Activate(bool activate);

	virtual bool CanReload() const;
	virtual const char *GetType() const { return "Detonate"; }

	virtual bool CanFire(bool considerAmmo = true) const;
	virtual void StartFire();

	virtual void NetShoot(const Vec3 &hit, int ph);
	//~IFireMode

	virtual void SetCanDetonate(bool canDet) { m_canDetonate = canDet; }
	virtual const char *GetCrosshair() const;
protected:
	bool Detonate(bool net=false);
	void SelectLast();

	float	m_detonationTimer;
	bool	m_canDetonate;
};


#endif //__DETONATE_H__