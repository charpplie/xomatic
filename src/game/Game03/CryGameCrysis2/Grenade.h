/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Grenades

-------------------------------------------------------------------------
History:
- 11:12:2009   10:30 : Created by Claire Allan

*************************************************************************/
#ifndef __GRENADE_H__
#define __GRENADE_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Projectile.h"

class CGrenade : public CProjectile
{
public:
	
	typedef CProjectile BaseClass;

	CGrenade();
	virtual ~CGrenade();

	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale /*=1.0f*/);
	virtual void HandleEvent(const SGameObjectEvent &event);

protected:
	virtual bool ShouldKnockTarget() const;
};

#endif
