/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Dogtags that are dropped by players

-------------------------------------------------------------------------
History:
- 04:12:2009   10:30 : Created by Ben Parbury

*************************************************************************/
#ifndef __DOGTAG_H__
#define __DOGTAG_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Item.h"

class CDogtag: public CItem
{
public:
	struct STagInfo
	{
		char  m_ownerName[17];			// dunno what the actual length limit for names is (if any), but there's only going to be so much that can be fit onto the dogtag anyway...!
		char  m_clanName[8];				// (ditto for clan names)
		EntityId	m_ownerId;				// nb. won't always resolve, eg. the owner might've rage quit after being killed
		int8  m_rank;								// 7 bits, rank (max 80)
		int8  m_progressTactical;		// 5 bits, Tactical progression (max 20)
		int8  m_progressStealth;		// 5 bits, Stealth progression (max 20)
		int8  m_progressArmour;			// 5 bits, Armour progression (max 20)
		int8  m_progressPower;			// 5 bits, Power progression (max 20)
		STagInfo() {}
		void Clear();
		void AssignFromEntity(EntityId eid);
	};

public:
	CDogtag();
	virtual ~CDogtag();

	virtual void PostInit( IGameObject * pGameObject );

	virtual bool IsPickable() const { return true; }
	virtual bool CanPickUp(EntityId userId) const { return true;}
	virtual bool CanUse(EntityId userId) const { return false; }
	virtual bool CanDrop() const { return false; }

	virtual void PickUp(EntityId pickerId, bool sound=true, bool select=false, bool keepHistory=true, const char *setup=0);
	virtual void PickUpAmmo(EntityId pickerId);

	virtual void Update( SEntityUpdateContext& ctx, int );

	static void SpawnDogtag(EntityId shooterId, EntityId victimId);

	void GetMemoryUsage(ICrySizer *pSizer )const
	{
		pSizer->AddObject(this, sizeof(*this));
		CItem::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
protected:
	static bool ShouldSpawnDogtag(EntityId shooterId, EntityId victimId);

public:
	STagInfo  m_info;
	int				m_effectSlot;
	float			m_timer;
	bool			m_pickedUp;
};


#endif //__DOGTAG_H__
