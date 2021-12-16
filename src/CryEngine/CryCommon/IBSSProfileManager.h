////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2007.
// -------------------------------------------------------------------------
//  File name:   IBSSProfileManager.h
//  Description: Interface functions for BSS Profile Manager
//    Aggregates all available profiles for the Behavior Selection System
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include DEVIRTUALIZE_HEADER_FIX(IBSSProfileManager.h)

#ifndef _IBSS_PROFILE_MANAGER_H_
#define _IBSS_PROFILE_MANAGER_H_

#include "IPersonalBehaviorTree.h"

struct IAISignalExtraData;
struct IScriptTable;
struct IBlackBoard;

typedef uint32 TBSSProfileUserId;
static const TBSSProfileUserId g_uBTUserId_Invalid = 0;

// Nil (empty) signal data
#define NULL_BSS_SIGNAL_DATA (ScriptAnyValue(ANY_TNIL))

// BT Profile User Events
enum EBTUserEvents
{
	eBTUE_INVALID = 0,

	eBTUE_OnNodeSignal,			// Sent when tree has come across a node signal (node was traversed through)
	eBTUE_OnNewLeafNode,		// Sent when tree has traversed to a new leaf node (sent each frame until you allow the node change!)
	eBTUE_OnDecisionMade,		// Sent when tree has made a decision (node change was approved)
};

// Profile User - Inherit this from a class to describe something that can use a BT
UNIQUE_IFACE struct IBSSProfileUser
{
	virtual ~IBSSProfileUser() {}

	virtual bool GetBTUserName(string &sOut) const { return false; }
	virtual IScriptTable* GetBTUserTable() const { return NULL; }
	virtual IBlackBoard* GetBTUserBlackBoard() const { return NULL; }
	
	// Return TRUE if the tree is allowed to run now
	virtual bool CanBTUserTreeRun() const = 0;

	// Notify you of events that happen with the tree/system
	virtual void OnBTUserEvent(EBTUserEvents event, IPersonalBehaviorTree *pTree, const string& sSignal, const ScriptAnyValue* pArgValue = NULL) = 0;
};

UNIQUE_IFACE struct IBSSProfileManager
{
	virtual bool SendSignalToAllActors(const char* szProfileName, const char* szSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA) = 0;
	virtual bool SendGlobalSignal(const char* szSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA) = 0;

	// User management
	virtual bool InitNewUser(IBSSProfileUser *pUser, const char* szProfileName, TBSSProfileUserId &outUserId) = 0;
	virtual bool SwitchUserProfile(TBSSProfileUserId &userId, const char* szProfileName) = 0;
	virtual void RemoveUser(TBSSProfileUserId userId) = 0;
	virtual void ResetUser(TBSSProfileUserId userId) = 0;
	virtual void ResetAllUsers() = 0;
	virtual void EnableUser(TBSSProfileUserId userId, bool bEnable) = 0;
	virtual void RunUserTree(TBSSProfileUserId userId, bool bForce) = 0;

	// Send input to user trees
	virtual bool SendUserInput(TBSSProfileUserId userId, const char* szSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA) = 0;
};

#endif // _IBSS_PROFILE_MANAGER_H_
