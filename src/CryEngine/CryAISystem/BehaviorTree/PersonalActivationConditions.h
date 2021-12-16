/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   PersonalActivationConditions.h
$Id$
$DateTime$
Description: This class manages the LUA table that stores the current
state of one puppet, so we will create one instance per puppet.
Everytime this table changes due to signals received, the Behavior
Tree will be re-evaluated.
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _PERSONAL_ACTIVATION_CONDITIONS_H_
#define _PERSONAL_ACTIVATION_CONDITIONS_H_

struct SSignalReaction;
struct SBTProfileUser;
class CActivationConditionTable;
class CActivationConditions;
class CDebugDrawContext;

class CPersonalActivationConditions
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CPersonalActivationConditions( const string& sName, CActivationConditions *pActivationConditions );
	virtual ~CPersonalActivationConditions();
	bool Init( TBSSProfileUserId userId, CActivationConditionTable const* pDefaults, const string& sProfile, const string& sTable );
	void SetProfileUserId( TBSSProfileUserId userId );
	void SetTableName( const string& sTable );
	void Reset();

	/*$1- Utils --------------------------------------------------------------*/
	bool ReceiveSignal( SmartScriptTable &pConditionsTable, const char* sSignal, const ScriptAnyValue &argValue ) const;
	bool ReceiveGlobalSignal( SSignalReaction const* pReaction, const char* sSignal, const ScriptAnyValue &argValue ) const;
	bool CheckCondition( int iPreCompiledCode, bool bDebug = false ) const;
	void Dump() const;
	void DebugDraw(CDebugDrawContext &dc, float fX, float fY) const;
	void TransformStartingValues( SmartScriptTable &pConditionsTable );
	bool TransformStartValue( SmartScriptTable &pConditionsTable, const char* sValue, ScriptAnyValue& Value );
	void CopyValues( CActivationConditionTable const* pValues );
	bool AddValues( SmartScriptTable &pTable ) const;
	bool GetConditionsTable( SmartScriptTable &pConditionsTable ) const;

	const char* GetName() const;
	TBSSProfileUserId GetUserId() const;

	CActivationConditions* GetActivationConditions();
	CActivationConditions const* GetActivationConditions() const;

private:
	bool ReceiveSignal( SSignalReaction const* pReaction, SmartScriptTable &pConditionsTable, const char* sSignal, const ScriptAnyValue &argValue ) const;
	bool ApplyReaction( int iPreCompiledCode, SmartScriptTable &pConditionsTable, const ScriptAnyValue &argValue, bool bDebug = false ) const;
	bool ApplyAllReactions( SSignalReaction const* pReaction, SmartScriptTable &pConditionsTable, const ScriptAnyValue &argValue, bool bDebug ) const;
	int	 PreCompileCode( const string& sCode ) const;
	bool ExecutePreCompiledCode( int iPreCompiledCode, SmartScriptTable &pConditionsTable, ScriptAnyValue *pResult = NULL, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA ) const;

	SBTProfileUser* GetUser() const;
	bool GetUserName(string& sUserName) const;

	virtual bool AssembleFinalCode( string const& sCode, string& sFinalCode ) const;

private:

	/*$1- Members ------------------------------------------------------------*/
	bool m_bInit;
	string m_sName;
	string m_sTable;
	string m_sProfile;
	TBSSProfileUserId m_userId;
	CActivationConditions *m_pActivationConditions;
	CActivationConditionTable *m_pValues;
	CActivationConditionTable const* m_pDefaults;

	/*$1- Static debug data --------------------------------------------------*/
	static TBSSProfileUserId sm_userCurrentlyDebugged;
	mutable std::list<string> m_lstRecentSignals;
};

#endif // _PERSONAL_ACTIVATION_CONDITIONS_H_
