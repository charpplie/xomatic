/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   ActivationConditionsTable.h
$Id$
$DateTime$
Description: Manages the LUA table that stores the state of each puppet
that is used to select the proper node to use in the Behavior Tree.
---------------------------------------------------------------------
History:
- 06:07:2007 : Created by Ricardo Pillosu
- 20 Feb 2009			 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _ACTIVATION_CONDITIONS_TABLE_H_
#define _ACTIVATION_CONDITIONS_TABLE_H_

#include "IVisualLog.h"

/*$1- Logging facility -------------------------------------------------------*/
class CACTDump : public IScriptTableDumpSink
{

public:
	CACTDump( IScriptTable* obj, const string& sTableName, const string& sMostParentName );
	~CACTDump();
	void OnElementFound( const char* sName, ScriptVarType type );
	void OnElementFound( int nIdx, ScriptVarType type );

	IScriptTable *m_pScriptObject;
	string m_sTableName;
	string m_sMostParentName;
};

class CACTDebugDrawDump : public IScriptTableDumpSink
{

public:
	CACTDebugDrawDump( IScriptTable* obj, float x, float y );

	void OnElementFound( const char* sName, ScriptVarType type );
	void OnElementFound( int nIdx, ScriptVarType type );

	float m_fX;
	float m_fY;
	IScriptTable *m_pScriptObject;
	IVisualLog *m_pVisualLog;
	SVisualLogParams m_visLogParams;
};

/*$1- Main Class -------------------------------------------------------------*/
class CActivationConditionTable
{

public:

	/*$1-Basics --------------------------------------------------------------*/
	CActivationConditionTable( const char* sName, const char* sDefaultsName, const string& sProfile );
	void Reset();

	/*$1-Utils ---------------------------------------------------------------*/
	SmartScriptTable& GetTable();
	const SmartScriptTable& GetTable() const;
	bool GetValue( const char* sName, ScriptAnyValue& Value ) const;
	bool GetValue( int iIndex, ScriptAnyValue& Value ) const;
	void SetValue( const char* sName, ScriptAnyValue& Value ) const;
	void SetValue( int iIndex, ScriptAnyValue& Value ) const;
	void CreateValue( const string& sName, const string& sValue );
	void Dump( IScriptTableDumpSink* pSink = NULL ) const;
	const string& GetName() const;
	const string& GetParentName() const;
	const string& GetMostParentName() const;

	/*$1- Members --------------------------------------------------------------*/
private:
	string m_sName;
	string m_sDefaults;
	string m_sProfile;
	SmartScriptTable m_pTable;
	SmartScriptTable m_pParent;
	SmartScriptTable m_pDefaults;
};

#endif // _ACTIVATION_CONDITIONS_TABLE_H_
