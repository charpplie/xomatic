/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Hologram Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 18:03:2009   09:50 : Created by Tom Berry

*************************************************************************/
#ifndef __HOLOGRAM_H__
#define __HOLOGRAM_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"
#include "Audio/AudioSignalPlayer.h"

class CHologram :
	public CSingle
{
public:
	CHologram();
	virtual ~CHologram();

	// IFireMode
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void Activate(bool activate);
	virtual bool CanFire(bool considerAmmo = true) const;
	virtual void StartFire();
	virtual void StopFire();

	virtual void NetStartFire();
	virtual void NetStopFire();
	virtual void NetStartSecondaryFire() {}
	virtual const char *GetType() const { return "Hologram"; };
	//~IFireMode

	virtual void Serialize(TSerialize ser) {};

	virtual bool Shoot(bool resetAnimation, bool autoreload = true , bool isRemote=false );
	virtual void NetShoot(const Vec3 &hit, int predictionHandle);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle);

protected:
	void LaunchHologram(const Vec3 &pos, const Vec3 &dir, const Vec3 &targetPos);
	void CalculateTargetPos(Vec3& targetPosition);

	EntityId	m_lastHoloID;

	CAudioSignalPlayer m_attachmentLoop;
};


#endif //__HOLOGRAM_H__
