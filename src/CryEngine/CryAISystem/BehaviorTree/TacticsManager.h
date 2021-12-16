/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   TacticsManager.h
$Id$
$DateTime$
Description: Main interface to load tactics and get them by name. 
A tactic is just a filter added to certain Behavior Tree nodes that is
executed AFTER the conditions are met for a certain puppet. The filter
can:
1. Ask for a minimum or maximum of resources to mark the node as
valid.
2. Check if some OTHER tactic is running before marking this node
as valid.
3. Keep monitoring of the resources (entities) allocated in one node
and mark it invalid if less than "min-to-run" resources are running
this node.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _TACTICS_MANAGER_H_
#define _TACTICS_MANAGER_H_

#include "dag.h"
#include "IFileChangeMonitor.h"

class CTacticInfo;
class CTactic;
class CSubTactic;
class CBehaviorTree;
struct SSubTacticInfo;

// Tactics
typedef uint32 TTacticGroupId;
static TTacticGroupId NULL_TACTIC_GROUP = 0;
static TTacticGroupId FIRST_TACTIC_GROUP = NULL_TACTIC_GROUP+1;

class CTacticsManager : public IFileChangeListener
{
	typedef std::vector<CTactic *> TTacticsVec;
	typedef std::vector<CTacticInfo *> TTacticInfoVec;
	typedef std::map<TBSSProfileUserId, TTacticGroupId> TUserGroupIdMap;

public:

	/*$1- Basics -------------------------------------------------------------*/
	CTacticsManager( CProfileDictionary *pDictionary );
	~CTacticsManager();
	void Init();
	bool ReLoad( const string& sFile );
	void Shutdown();
	void Reset();
	void Update();
	void DebugDraw() const;

	/*$1- Inherited functions ------------------------------------------------*/
	void OnFileChange( const char* sFilename );
	void OnEditorSetGameMode( bool bGameMode );

	/*$1- Utils --------------------------------------------------------------*/
	CTactic* GetTactic( const string& sName );
	CSubTactic* GetSubTactic( const string& sName );
	bool DeleteResource( TBSSProfileUserId userId, bool bOnlyIfNotInLastRun = false );
	CSubTactic* FindResource( TBSSProfileUserId userId, const char* sTactic = NULL ) const;

	TTacticGroupId GenerateUniqueGroupId();
	TTacticGroupId GetUserGroupId( TBSSProfileUserId userId ) const;
	bool SetUserGroupid( TBSSProfileUserId userId, TTacticGroupId GroupId );
	bool RemoveUserGroupid( TBSSProfileUserId userId );

protected:
	friend class CBehaviorTree;
	friend class CTactic;

	/*$1- Members ------------------------------------------------------------*/
	bool LoadSheet( const string& sName, XmlNodeRef& xml );
	bool LoadRow( XmlNodeRef& xmlRow, bool bIsRequirements );
	bool NotifyTacticRunning( CTacticInfo const* pInfo );

	bool CreateTacticInstance( CTacticInfo const* pInfo, bool bForce = false );
	void DestroyTacticInstance( CTactic const* pTactic );

private:
	CTactic* CreateTactic( CTacticInfo const* pInfo, bool bInstance = false );
	void DeleteTactic( CTactic* pTactic );
	CTactic* CheckIfTacticExists( CTacticInfo const* pInfo );
	CTacticInfo const* GetTacticInfo( const string& sName ) const;
	SSubTacticInfo const* GetSubTacticInfo( const string& sName ) const;
	uint32 GetNumOfRunningInstances( CTacticInfo const* pInfo );
	uint32 GetRunningGroupInstancesCount( CTacticInfo const* pInfo, TTacticGroupId groupId ) const;
	bool NeedToCreateInstances( CTacticInfo const* pInfo, CTactic const* pSkip = NULL );

	void Register();
	void UnRegister();

	void FindOrCreateTacticsInfo( const string& sName, CTacticInfo *& pOutTacticInfo );

private:

	/*$1- Props --------------------------------------------------------------*/
	bool m_bReloadOnGameModeExit;
	bool m_bRegistered;
	string m_sFile;

	TTacticGroupId m_uUniqueTacticGroupId;
	TUserGroupIdMap m_mapUserGroupId;

	TTacticsVec m_vecTactics;
	TTacticsVec m_vecNewTactics;
	TTacticInfoVec m_vecTacticsInfo;
	CTacticInfo *m_pLastInfoLoaded;
	CProfileDictionary *m_pDictionary;
};

#endif // _TACTICS_MANAGER_H_
