/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   ProfileDictionary.h
$Id$
$DateTime$
Description: Every puppet can select (using an Scriptbind) his own
Behavior Tree + Activation Conditions profile to use. All this data
is loaded here (and reloaded when needed)
---------------------------------------------------------------------
History:
- 06:09:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _PROFILE_DICTIONARY_H_
#define _PROFILE_DICTIONARY_H_

#include "IBSSProfileManager.h"

class CBehaviorTree;
class CActivationConditions;
class CGlobalActivationConditions;
class CPersonalBehaviorTree;
class CTacticsManager;
struct SSignalReaction;
struct SBTProfile;

// Invalid BehaviorTree Profile Id
static const uint32 g_uBTProfileId_Invalid = 0;

// Helpers to handle profile user ids
#define MAKE_PROFILE_USERID(profileId, userCount) (((profileId)<<16) | ((userCount)&0xFF))
#define GET_PROFILE_FROM_USERID(userId) ((userId)>>16)

struct SBTProfileUser
{
	SBTProfileUser();
	~SBTProfileUser();

	bool Init(IBSSProfileUser *pUser, SBTProfile *pProfile, uint32 uUserCount);
	bool OnNewProfile(SBTProfile *pProfile, uint32 uUserCount);
	void PostInit();

	void SendEvent(EBTUserEvents event, const string& sData, const ScriptAnyValue *pArgValue = NULL) const;

	bool CanTreeRun() const;
	void RunTree(bool bForce = false);
	bool CheckCondition(int iPreCompiledCode, bool bDebug = false) const;
	bool CreateConditionsTable( SmartScriptTable &pTable ) const;

	/*$1- Members --------------------------------------------------------------*/
	CPersonalBehaviorTree *m_pPersonalTree;
	IBSSProfileUser *m_pUser;
	SBTProfile *m_pProfile;
	TBSSProfileUserId m_userId;
};

struct SBTProfile
{
	SBTProfile (uint32 uProfileId);
	~SBTProfile();

	void Init();
	bool LoadBehaviorTree( const string& sFile );
	bool LoadActivationConditions( XmlNodeRef &pNode );
	bool LoadTactics( CTacticsManager *pTacticsManager );
	void AddPersonalBT( CPersonalBehaviorTree* pPersonal );
	void DelPersonalBT( CPersonalBehaviorTree* pPersonal );
	void RunAllTrees( bool bForceRealUpdate = false );
	
	bool InitNewUser(IBSSProfileUser *pUser, TBSSProfileUserId &outUserId);
	void RemoveUser(TBSSProfileUserId userId);
	void ResetUser(TBSSProfileUserId userId);
	void ResetAllUsers();
	void EnableUser(TBSSProfileUserId userId, bool bEnable);
	void RunUserTree(TBSSProfileUserId userId, bool bForce);
	SBTProfileUser* GetUser(TBSSProfileUserId userId) const;

	bool SendUserInput(TBSSProfileUserId userId, const char* szSignal, const ScriptAnyValue &argValue);

	void SetTimeForBTUpdate( const CTimeValue& timeslice )
	{
		m_BTTimeSlice = timeslice;
	};
	const CTimeValue& GetTimeForBTUpdate() const
	{
		return( m_BTTimeSlice );
	}

	// Check all personal behavior trees and run them if necessary
	void Update();
	void ResetUser(SBTProfileUser *pUser);
	SBTProfileUser* GetUser(IBSSProfileUser *pProfileUser) const;

	// Ownership swapping for profile switching
	bool AddProfileUser(SBTProfileUser* &pProfileUser, TBSSProfileUserId &outUserId);
	SBTProfileUser* ReleaseProfileUser(TBSSProfileUserId userId);

	/*$1- Members --------------------------------------------------------------*/
	struct SActivationConditionDef
	{
		SActivationConditionDef();

		bool Init(XmlNodeRef &pNode);

		bool m_bMustReset;
		string m_sName;
		string m_sTable;
	};

	typedef std::list<CPersonalBehaviorTree*> TPersonalList;
	typedef std::vector<SActivationConditionDef> TActivationConditionsVec;
	typedef std::map<TBSSProfileUserId, SBTProfileUser*> TUserMap;

	uint32 m_uProfileId;
	uint32 m_uUserCountGen;
	CBehaviorTree *m_pBehaviorTree;
	CTimeValue m_BTTimeSlice;
	TActivationConditionsVec m_vecActivationConditions;
	TUserMap m_Users;
	TPersonalList m_listPersonals;
	TPersonalList::iterator	m_sNextOne;
};

class CProfileDictionary : public IBSSProfileManager
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CProfileDictionary();
	~CProfileDictionary();
	void InitProfiles();
	bool LoadData();
	void OnEditorSetGameMode( bool bMode );

	/*$1- Interfaced methods -------------------------------------------------*/
	bool SendSignalToAllActors(const char* szProfileName, const char* szSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA);
	bool SendGlobalSignal(const char* szSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA);

	bool InitNewUser(IBSSProfileUser *pUser, const char* szProfileName, TBSSProfileUserId &outUserId);
	bool SwitchUserProfile(TBSSProfileUserId &userId, const char* szProfileName);
	void RemoveUser(TBSSProfileUserId userId);
	void ResetUser(TBSSProfileUserId userId);
	void ResetAllUsers();
	void EnableUser(TBSSProfileUserId userId, bool bEnable);
	void RunUserTree(TBSSProfileUserId userId, bool bForce);
	SBTProfileUser* GetProfileUser(TBSSProfileUserId userId) const;

	bool SendUserInput(TBSSProfileUserId userId, const char* szSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA);

	/*$1- Utils --------------------------------------------------------------*/
	CTacticsManager* GetTacticsManager() const { return m_pTacticsManager; }
	CGlobalActivationConditions const* GetGlobalActivationConditions() const { return m_pGlobalConditions; }
	CActivationConditions* GetActivationConditions( const string& sName );
	CActivationConditions const* GetActivationConditions( const string& sName ) const;
	SBTProfile*	GetProfile( const string& sProfile ) const;
	SBTProfile*	GetProfile( uint32 uProfileId ) const;
	uint32 GetProfileId( const string& sProfile ) const;
	bool GetProfileName( uint32 uProfileId, string& sProfile ) const;
	void Update( float fDelta );
	void UpdateDraw();
	bool IsEditing() const { return m_bIsEditing; }

	/*$1- Debug utils --------------------------------------------------------*/
	void SetDebugTree( const char* sProfile );
	void SetDebugTactics( bool bDebug );
	void SetDebugActivationConditions( const char* sProfile );

private:

	/*$1- Members ------------------------------------------------------------*/
	bool LoadActivationConditions( const string& sDir );
	bool LoadProfiles( const string& sDir );
	bool LoadProfile( const string& sFile );
	bool LoadGlobals( const string& sFile );
	bool LoadTactics( const string& sFile );

private:
	typedef std::map<string, SBTProfile*> TProfileMap;
	typedef std::map<string, CActivationConditions*> TActivationConditionsMap;

	/*$1- Props --------------------------------------------------------------*/
	bool m_bIsEditing;
	bool m_bProfilesInit;
	bool m_bDebugTactics;
	uint32 m_uProfileIdGen;
	float m_fUpdateTacticsTime;

	CTacticsManager* m_pTacticsManager;
	CGlobalActivationConditions* m_pGlobalConditions;

	TProfileMap m_Profiles;
	TActivationConditionsMap m_ActivationConditions;

};
#endif // _BEHAVIOR_TREE_H_
