/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   Slot.h
$Id$
$DateTime$
Description: Slots deal with resources (in this case, entities) and
distribute them following some rules and steps:
1. You can add / remove "candidates" to enter a certain slot
2. Each slot have a Minimum, Maximum, and Minimum-to-run candidates
3. Once there are enough candidates, they will be "assigned" and
the slot will be considered to be "running". The resources will
then markes as "used". No other slot can use them.
4. The resources will be sorted before assigned, using a user-specified
LUA code and a simple qsort.
5. If the resources in the slot go below "Min-to-run" the slot will
"stop" and "free" the resources.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _SLOT_H_
#define _SLOT_H_

class CSlotData;
class CSlotGroup;

typedef TBSSProfileUserId TSlotType;
typedef std::list<TSlotType> TSlotTypeList;
typedef std::vector<TSlotType> TSlotTypeVector;

class CSlot
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CSlot( CSlotGroup* pGroup );
	~CSlot();
	void Reset();

	/*$1- Actions ------------------------------------------------------------*/
	bool AddCandidate( TSlotType typeId );
	bool Delete( TSlotType typeId );
	bool DeleteAssigned( TSlotType typeId );
	bool AssignMaxResources();
	bool AssignMinResources();
	bool AssignForcedResources();
	bool DeAssignResources( bool bOnlyAssigned = false );
	bool SetResourceRunning( TSlotType typeId );
	bool RemoveResourceRunning( TSlotType typeId );
	void SetSortScriptCode( const string& sCode );
	void SetMinMax( uint32 uMin, uint32 uMax );
	void StartAssignProcess();
	bool EndAssignProcess();

	/*$1- Queries ------------------------------------------------------------*/
	bool HaveCandidates() const;
	bool HaveEnoughCandidates() const;
	bool HaveEnoughAssigned() const;
	bool HaveEnoughRunning() const;
	bool CandidateExists( TSlotType typeId ) const;
	bool AssignedExists( TSlotType typeId ) const;
	bool RunningExists( TSlotType typeId ) const;
	void GetCandidatesAsString( string& sString ) const;
	void GetIdealCandidatesAsString( string& sString ) const;
	void GetAssignedAsString( string& sString ) const;
	void GetRunningAsString( string& sString ) const;
	void GetNewAssignedResources( TSlotTypeList &vecResources ) const;
	void GetCandidateResources( TSlotTypeList &vecResources ) const;
	void GetAssignedResources( TSlotTypeVector &vecResources ) const;
	uint32 GetMin() const;
	uint32 GetMax() const;

private:

	/*$1- Private Actions ----------------------------------------------------*/
	bool AssignCandidates( uint32 uAmount );
	bool SortCandidates();

private:

	/*$1- Members ------------------------------------------------------------*/
	uint32 m_uMin;
	uint32 m_uMax;
	int m_iSortCode;
	bool m_bCandidatesOrdered;
	CSlotGroup *m_pGroup;
	CSlotData *m_pCandidates;
	CSlotData *m_pIdealCandidates;
	CSlotData *m_pAssigned;
	CSlotData *m_pRunning;
};

#endif // _SLOT_H_
