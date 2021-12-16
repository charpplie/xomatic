/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   DefaultActivationConditions.h
$Id$
$DateTime$
Description: Manages the LUA table with the default values for the state
of each puppet that wants to use the Behavior Tree.
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _DEFAULT_ACTIVATION_CONDITIONS_H_
#define _DEFAULT_ACTIVATION_CONDITIONS_H_

class CActivationConditionTable;

// Scope levels
enum EActCondScopes
{
	eACS_Global,	// Global scope - Every tree shares these
	eACS_Actor,		// Actor scope - Unique (cloned) per tree instance
	eACS_COUNT,
};

class CDefaultActivationConditions
{

public:

	/*$1-Basics --------------------------------------------------------------*/
	CDefaultActivationConditions( XmlNodeRef& xmlTable, const string& sProfile, EActCondScopes eScope );
	virtual ~CDefaultActivationConditions();

	/*$1-Utils ---------------------------------------------------------------*/
	bool IsInit() const
	{
		return( m_bInit );
	}

	CActivationConditionTable const* GetValuesFromScope( const char* sScope ) const;
	CActivationConditionTable const* GetValuesFromScope( EActCondScopes eScope ) const;

private:
	bool Reload( XmlNodeRef& xmlTable );
	CActivationConditionTable* GetValuesFromScope( const string& sScope );
	void CreateEntry( const string& sName, const string& sScope, const string& sDefValue, const char* sParentTable = NULL );
	void Shutdown();

	/*$1- Members --------------------------------------------------------------*/
private:
	bool m_bInit;
	EActCondScopes m_eDefScope;
	string m_sProfile;
	std::vector<CActivationConditionTable *> m_vecScopes;
};

#endif // _DEFAULT_ACTIVATION_CONDITIONS_H_
