/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   Tactic.h
$Id$
$DateTime$
Description: A tactic is just a container of subtactics. A tactic is
only marked as "running" when all its subtactics are ready to run.
It will also monitoize the resources on it and stop itself if needed.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _TACTIC_H_
#define _TACTIC_H_

#include "TacticsManager.h"
#include "ProfileDictionary.h"

class CTacticInfo;
class CSubTactic;
class CSlotGroup;
class CTacticsManager;
struct SSubTacticInfo;

enum ETacticState
{
	ETS_WAITING_FOR_MORE_CANDIDATES = 0,
	ETS_WAITING_FOR_ACKNOWELDGE,
	ETS_RUNNING
};

class CTactic
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CTactic( CTacticInfo const* pInfo, CTacticsManager* pManager, bool bIsInstance );
	~CTactic();
	void Reset();
	void Update();
	uint32 DebugDraw( float xPos, float yPos ) const;

	/*$1- Util ---------------------------------------------------------------*/
	const string& GetName() const;
	CTacticInfo const* GetInfo() const;
	bool IsActive() const;
	CSubTactic* GetSubTactic( const string& sName );
	CSubTactic* GetSubTactic( const SSubTacticInfo* pSubInfo );
	bool DeleteResource( TBSSProfileUserId userId , bool bOnlyIfNotInLastRun = false );
	ETacticState GetState() const;
	bool IsInstance() const;
	bool IsObsolete() const;
	bool NeedNewInstance() const;
	void SetRequestNewInstance( bool bRequest, TBSSProfileUserId requestingUser = g_uBTUserId_Invalid );
	CSubTactic* FindResource( TBSSProfileUserId userId ) const;
	void RunTreeForMyResources( bool bOnlyNewAssigned = false, TBSSProfileUserId ignoreUser = g_uBTUserId_Invalid );
	TTacticGroupId GetCurrentGroup() const;
	bool CheckUserGroup( TBSSProfileUserId userId );
	bool ExceedsMaxGroupInstances( uint32 uNumInstances ) const;

private:

	/*$1- Members ------------------------------------------------------------*/
	CSubTactic* CreateSubTactic( SSubTacticInfo const* pSubTacticInfo );
	void ChangeState( ETacticState NewState );
	bool CheckIfBestResources();

	/*$1- Members ------------------------------------------------------------*/
	bool m_bIsNewlyCreated;
	bool m_bIsInstance;
	bool m_bObsolete;
	bool m_bNeedNewInstance;
	ETacticState m_State;
	TTacticGroupId m_CurrentGroup;
	CTacticInfo const* m_pInfo;
	CTacticsManager *m_pManager;
	std::vector<CSubTactic *> m_vecSubTactics;
	CSlotGroup *m_pSlots;
};

#endif // _TACTIC_H_
