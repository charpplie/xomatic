/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   PersonalBehaviorTree.h
$Id$
$DateTime$
Description: As Behavior Trees are stored only once in memory, they
are read-only. We use this class per puppet to track down the current
position in the tree and manages the changes. It is also able to
manage profile changes. It's also responsible of resending signals
to the activation conditions and check if the state changed. If so, 
behavior tree will be called.
---------------------------------------------------------------------
History:
- 01:07:2008 : Now BT is only evaluated once per tick
- 10:06:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _BEHAVIOR_TREE_ACTOR_H_
#define _BEHAVIOR_TREE_ACTOR_H_

#include "IPersonalBehaviorTree.h"

class CPersonalActivationConditions;
class CActivationConditionTable;
class CBehaviorTree_node;
class CAIActor;
class CSubTactic;
struct SBTProfile;
struct SBTProfileUser;

class CPersonalBehaviorTree : public IPersonalBehaviorTree
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CPersonalBehaviorTree();
	~CPersonalBehaviorTree();
	void Reset();
	bool Update();

	/*$1- Util ---------------------------------------------------------------*/
	bool SetProfile( SBTProfile* pProfile, TBSSProfileUserId userId );
	bool SetProfile( const char* sProfile, TBSSProfileUserId userId );
	bool ReloadProfile();
	bool ReceiveSignal( const char* sSignal, const ScriptAnyValue &argValue );
	void AllowNodeChange();
	bool RunTree();
	bool RunTreeNow();
	void Enable();
	void Disable();

	bool Enabled() const { return m_bDisabled == false; }

	/*$1- Queries ------------------------------------------------------------*/
	SBTProfile* GetProfile();
	SBTProfile const* GetProfile() const;
	bool IsReseted() const;
	bool IsWaitingForNodeChange() const;
	const char* GetNextNode() const;
	const char* GetCurrentNode() const;
	const char* GetPreviousNode() const;
	void GetNextNodePath( string& sPath ) const;
	void GetCurrentNodePath( string& sPath ) const;
	void GetPreviousNodePath( string& sPath ) const;
	bool CheckCondition( int iPreCompiledCode, bool bDebug = false ) const;
	bool HandleReceiveSignalCondition( const char* sSignal, const ScriptAnyValue &argValue );
	CSubTactic* GetAssignedTactic( const char* sTactic = NULL ) const;
	uint32 GetUsersInMyTactic( std::vector<TBSSProfileUserId> *pListToFill, const char *sTactic = NULL ) const;
	void DebugDraw();
	void SetBlackBoardSignalData(const ScriptAnyValue &argValue) const;
	bool GetBlackBoardSignalData(ScriptAnyValue &argValue) const;
	bool GetBlackBoardSignalData(IAISignalExtraData* &pData) const;
	bool CreateConditionsTable( SmartScriptTable &pTable ) const;

private:

	/*$1- Util ---------------------------------------------------------------*/
	bool SetNextChildNode( CBehaviorTree_node* pNewNode );
	void InitPersonalActivationConditions();
	void DeletePersonalActivationConditions();
	void GetNodePath( CBehaviorTree_node const* pNode, string& sPath ) const;

	SBTProfileUser* GetUser() const;
	const char* GetUserName() const;

private:

	typedef std::vector<CPersonalActivationConditions*> TPersonalACVec;

	/*$1- Members ------------------------------------------------------------*/
	bool m_bReseted;
	bool m_bDisabled;
	bool m_bNeedUpdate;
	TBSSProfileUserId m_userId;
	SBTProfile *m_pProfile;
	ScriptAnyValue m_currentSignalData;
	CBehaviorTree_node *m_pPrevious;
	CBehaviorTree_node *m_pCurrent;
	CBehaviorTree_node *m_pNext;

	TPersonalACVec m_vecPersonalACs;

	/*$1- Static debug data --------------------------------------------------*/
	static CPersonalBehaviorTree const* sm_pCurrentlyDebugged;
	static std::list<string> sm_lstRecentNodes;
};

#endif // _BEHAVIOR_TREE_ACTOR_H_
