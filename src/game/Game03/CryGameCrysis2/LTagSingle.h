/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Description: 

Ltag version of single firemode. Requires some extra functionality
for animation, projectile spawning, etc

-------------------------------------------------------------------------
History:
- 15:09:09   Benito Gangoso Rodriguez

*************************************************************************/

#pragma once

#ifndef _LTAG_SINGLE_H_
#define _LTAG_SINGLE_H_

#include "Single.h"
#include "LTAGGrenade.h"

class CLTagSingle : public CSingle
{
private:
	struct EndCockingAction;
	struct ScheduleReload;

public:
	CLTagSingle();
	virtual ~CLTagSingle();

	//CSingle
	virtual void Activate(bool activate);
	virtual void UpdateFPView(float frameTime);

	virtual bool Shoot(bool resetAnimation, bool autoreload=true, bool isRemote=false);
	virtual const char *GetType() const;
	//~CSingle

	void NextGrenadeType();
	//bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	//uint NetGetGrenadeType() const { return static_cast<uint>(m_grenadeType); }
	//void NetSetGrenadeType(uint type);
	void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle);

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));	
		CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
	}
protected:
	virtual const char* GetBestReloadAction(int ammoCount);

private:
	typedef CSingle inherited;
	
	const char* GetFireAction(int ammoCount) const;

	void SwitchGrenades ();
	void UpdateGrenadeAttachment(ICharacterInstance* pCharacter, const char* attachmentName, const char* model);

	ELTAGGrenadeType m_grenadeType;
	bool	m_fpModelInitialised;
};

#endif
