/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   SubTactic.h
$Id$
$DateTime$
Description: Subtatic is basically a CSlot (see Slot.cpp) with its
own resources and a configuration (CTacticInfo).
---------------------------------------------------------------------
History:
- 13:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _SUB_TACTIC_H_
#define _SUB_TACTIC_H_

#include "Tactic.h"

class CTactic;
class CSlot;
struct SSubTacticInfo;
enum ETacticState;

typedef std::map<uint32, CSlot*> TSlotMap;

class CSubTactic
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CSubTactic( CTactic* pTactic, SSubTacticInfo const* pInfo, TSlotMap& Slots );
	~ CSubTactic();
	void Reset();
	bool Run( TBSSProfileUserId userId, bool &bMatchesGroup );
	uint32 DebugDraw( float xPos, float yPos ) const;

	/*$1- Util ---------------------------------------------------------------*/
	void ChangeState( ETacticState ENewState );
	bool AddCandidate( TBSSProfileUserId userId, bool &bMatchesGroup );
	bool DeleteResource( TBSSProfileUserId userId, bool &bHasCandidates, bool bOnlyIfNotInLastRun = false );
	const string& GetName() const;
	SSubTacticInfo const* GetInfo() const;
	bool UsesGroup() const;
	CTactic const* GetTactic() const;
	void OnResourceLeave( TBSSProfileUserId userId );
	void OnResourceEnter( TBSSProfileUserId userId );
	bool IsResourceAssigned( TBSSProfileUserId userId ) const;
	bool IsRunning() const;
	uint32 GetAssignedUsers( std::vector<TBSSProfileUserId> *pListToFill ) const;
	void RequestNewTacticInstance( TBSSProfileUserId userId ) const;

protected:

	bool CheckUserGroup( TBSSProfileUserId userId );

private:

	/*$1- Members ------------------------------------------------------------*/
	CTactic *m_pTactic;
	SSubTacticInfo const* m_pInfo;
	TSlotMap m_Slots;
	TBSSProfileUserId m_LastRun;
};

#endif // _SubTactic_H_
