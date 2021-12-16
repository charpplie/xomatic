/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   SlotGroup.h
$Id$
$DateTime$
Description: Every Slot belongs to a SlotGroup. All the resources 
throughout a SlotGroup are unique when they are assigned to run in
a certain slot.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _SLOT_GROUP_H_
#define _SLOT_GROUP_H_

#include "Slot.h"

class CSlot;

class CSlotGroup
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CSlotGroup();
	~CSlotGroup();

	/*$1- Actions ------------------------------------------------------------*/
	CSlot* AddSlot();
	bool AssignResources();
	bool DeAssignResources( bool bOnlyAssigned = false );

	/*$1- Queries ------------------------------------------------------------*/
	bool HaveCandidates() const;
	bool HaveEnoughCandidates() const;
	bool HaveEnoughAssigned() const;
	bool HaveEnoughRunning() const;
	uint32 GetMinResourcesCount() const;
	void GetCandidateResources( TSlotTypeList &vecResources ) const;
	void GetNewAssignedResources( TSlotTypeList &vecResources ) const;

protected:
	friend class CSlot;

	/*$1-  -------------------------------------------------------------------*/
	void AssignResource( CSlot const* pSlot, TSlotType typeId );
	bool IsResourceAssigned( TSlotType typeId, CSlot const* pSkip ) const;

private:

	/*$1- Members ------------------------------------------------------------*/
	std::vector<CSlot*>  m_vecSlots;
};

#endif // _SLOT_GROUP_H_
