/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   Behavior_Tree.h
$Id$
$DateTime$
Description: Behavior Tree class using CDAG as base. A Behavior Tree is
a formal, tree-like graphical form that represents behavior of individual
or networks of entities which realize or change states, make decisions,
respond-to/cause events, and interact by exchanging information and/or
passing control.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _BEHAVIOR_TREE_H_
#define _BEHAVIOR_TREE_H_

#include "dag.h"
#include "IGameFramework.h"
#include "IFileChangeMonitor.h"
#include "BehaviorTree_node.h"

class CBehaviorTree_node;
class CTacticsManager;
class CTactic;
class CSubTactic;
enum EChildSelectionMethod;

class CBehaviorTree : public CDag, public IFileChangeListener
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CBehaviorTree( const char* sRootName, uint32 uProfileId );
	~CBehaviorTree();
	void Init();
	bool Reload( const string& sFile );
	bool LoadTactics( CTacticsManager* pTacticsManager );
	void Reset();
	void UpdateDraw();

	/*$1- Inherited functions ------------------------------------------------*/
	void OnFileChange( const char* sFilename );
	void OnEditorSetGameMode( bool bGameMode );

	/*$1- Utils --------------------------------------------------------------*/
	CBehaviorTree_node* Run( TBSSProfileUserId userId ) const;
	bool DisableResource( TBSSProfileUserId userId );
	void OnActorChangeNode( TBSSProfileUserId userId, CBehaviorTree_node* pOld, CBehaviorTree_node* pNew );
	EChildSelectionMethod GetChildSelectionMethodByName( const char* sName ) const;
	void SetDebugTree( bool bDebug );
	bool GetDebugTree() const;
	CSubTactic* FindRunningTacticForResource( TBSSProfileUserId userId, const char* sTactic = NULL ) const;

protected:
	friend class CTacticsManager;

	/*$1- Members ------------------------------------------------------------*/
	void AddInstancedTactic( CTactic* pTactic );
	void DeleteSubTactics( CTactic const* pTactic );

private:
	typedef CDag TParent;

	/*$1- Members ------------------------------------------------------------*/
	bool LoadSheet( const string& sName, uint32 uRows, uint32 uColumns, XmlNodeRef& xml );
	bool LoadRows( XmlNodeRef& xmlTable );
	bool LoadNodeProperties_Row( XmlNodeRef& xmlTable, uint32 uRow );
	bool LoadCell( CBehaviorTree_node* pNode, uint32 uCell, const char* sData );
	void LoadingFinished();
	void AssignMethodNames();
	void AssignTactics();
	void Register();
	void UnRegister();
	bool CheckInfiteLoops();

private:

	/*$1- Props --------------------------------------------------------------*/
	bool m_bDebugTree;
	bool m_bRegistered;
	bool m_bReloadOnGameModeExit;
	uint32 m_uProfileId;
	std::vector<string> m_vecMethodNames;
	CTacticsManager *m_pTacticsManager;
};

#endif // _BEHAVIOR_TREE_H_
