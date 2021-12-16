/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 26:02:2010   Created by Filipe Amim

*************************************************************************/
#ifndef __SNIPER_SINGLE_H__
#define __SNIPER_SINGLE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"


class CSniperSingle : public CSingle
{
private:
	class CPumpAction
	{
	public:
		CPumpAction();
		void Update(const CSniperSingle& pSniperSingle, float frameTime);
		void Schedule();
		bool IsRunning() const {return m_running;}

	private:
		float Pump(const CSniperSingle& pSniperSingle) const;

		bool m_running;
		bool m_waiting;
		float m_timeCounter;
	};

public:
	CSniperSingle();

	void GetMemoryUsage(ICrySizer *s ) const
	{
		s->AddObject(this, sizeof(*this));	
		CSingle::GetInternalMemoryUsage(s);		 // collect memory of parent class
	}
	
private:
	virtual void Activate(bool activate);
	virtual void Update(float frameTime, uint32 slot);
	virtual bool Shoot(bool resetAnimation, bool autoreload/* =true */, bool isRemote/* =false */);
	virtual void StartFire();
	virtual bool CanZoom() const;

	void RestartZoom() const;
	bool IsWeaponSelected() const;
	bool IsWeaponZoomed() const;
	bool IsWeaponFullyZoomed() const;

	void Reset();

	CPumpAction m_pumpAction;
};


#endif
