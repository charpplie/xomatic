/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Rapid Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 26:10:2005   14:15 : Created by Márcio Martins

*************************************************************************/
#ifndef __RAPID_H__
#define __RAPID_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"

class CRapid : public CSingle
{
public:

	enum ERapidFlag
	{
		eRapidFlag_none					 = (0),
		eRapidFlag_accelerating  = (1 << 0),
		eRapidFlag_decelerating  = (1 << 1),
		eRapidFlag_netShooting	 = (1 << 2),
		eRapidFlag_rapidFiring	 = (1 << 3),
		eRapidFlag_startedFiring = (1 << 4)
	};

	CRapid();
	virtual ~CRapid();

	// CSingle
	virtual void Update(float frameTime, uint32 frameId);

	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void Activate(bool activate);

	virtual void StartReload(int zoomed);

	virtual void StartFire();
	virtual void StopFire();
  virtual bool IsFiring() const { return m_firing || (m_rapidFlags & eRapidFlag_accelerating); };

	virtual void NetStartFire();
	virtual void NetStopFire();

	virtual float GetSpinUpTime() const;
	virtual float GetSpinDownTime() const;

	virtual bool AllowZoom() const;

	virtual const char *GetType() const;
	virtual int PlayActionSAFlags(int flags);

	virtual void OnZoomStateChanged();
	// ~CSingle

protected:
	virtual void Accelerate(float acc);
	virtual void Firing(bool firing);
	virtual void UpdateRotation(float frameTime);
	virtual void UpdateSound(float frameTime);
  virtual void FinishDeceleration();

  CTimeValue m_startFiringTime;

	float	m_speed;
	float	m_acceleration;
	float m_rotation_angle;

	uint32 m_rapidFlags;
	uint32	m_fireLoopSoundId;
	uint32	m_spinUpSoundId;
};

#endif