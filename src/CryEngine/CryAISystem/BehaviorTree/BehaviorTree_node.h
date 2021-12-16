/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   BehaviorTree_node.h
$Id$
$DateTime$
Description: Behavior Tree node (leaf or not) that include it's own
conditions and a callback to any tactics to add candidates to them
if needed. It uses CDagNode as base.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _DECISION_TREE_NODE_H_
#define _DECISION_TREE_NODE_H_

#include "dag_node.h"

class CTacticsManager;
class CTactic;
class CSubTactic;

enum EChildSelectionMethod
{
	ECSM_INVALID = -1,
	ECSM_PRIORITIZED,
	ECSM_RANDOM,
	ECSM_RANDOM_NON_REPEAT,
	ECSM_SEQUENTIAL,
	ECSM_SEQUENTIAL_LOOP,
	ECSM_COUNT
};

class CBehaviorTree_node : public CDagNode
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CBehaviorTree_node( CBehaviorTree_node* pParent, const char* sName, uint32 uProfileId );
	~CBehaviorTree_node();
	CBehaviorTree_node* Run( TBSSProfileUserId userId );
	void Reset();

	/*$1- Util ---------------------------------------------------------------*/
	CDagNode* AddChild( const char* sName );
	void DestroyAllChilds();
	void SetPriority( uint32 uPriority );
	void SetChildSelectionMethod( EChildSelectionMethod eMethod );
	void SetSignal( const string& sSignal );
	void SetSubTactic( const string& sSubTactic );
	void SetImpulseTo( CBehaviorTree_node* pNodeTo );
	void AddActivationCondition( const string& sCondition, bool bEraseBeforeAdd = false );
	uint32 GetPriority() const;
	EChildSelectionMethod GetChildSelectionMethod() const;
	const string& GetSignal() const;
	const char*	 GetDebugText() const;
	bool IsLeafNode() const;
	CBehaviorTree_node* GetImpulse() const;
	bool CheckConditions( TBSSProfileUserId userId ) const;
	void GetYourSubTactic( CTacticsManager* pTacticManager );

protected:
	friend class CBehaviorTree;

	/*$1- Members ------------------------------------------------------------*/
	void OnResourceEnter( TBSSProfileUserId userId );
	void OnResourceLeave( TBSSProfileUserId userId );
	void InstanceTactic( CTactic* pTactic );
	void DeleteSubTactics( CTactic const* pTactic );
	CBehaviorTree_node const* CheckInfiteLoops();

private:
	typedef CDagNode TParent;

	/*$1- Members ------------------------------------------------------------*/
	CBehaviorTree_node* RunChilds( TBSSProfileUserId userId ) const;
	bool RunSubTactics( TBSSProfileUserId userId );
	bool ExploreTree( CBehaviorTree_node const* pOrigin );
	void SendSignal( TBSSProfileUserId userId ) const;

private:

	/*$1- Members ------------------------------------------------------------*/
	bool m_bActive;
	uint32 m_uPriority;
	uint32 m_uProfileId;
	EChildSelectionMethod m_eChildSelectionMethod;
	string m_sSignal;
	string m_sSubTactic;
	std::vector<int> m_vecPreCompiledConditions;
	CBehaviorTree_node *m_pImpulseTo;

	typedef std::vector<CSubTactic *> TSubTacticsVec;
	TSubTacticsVec m_vecSubTactics;
};

#endif // _DECISION_TREE_NODE_H_
