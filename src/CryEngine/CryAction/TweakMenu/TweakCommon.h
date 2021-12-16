/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakCommon, a base class for Tweak menu components

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/


#ifndef __CTWEAKCOMMON_H__
#define __CTWEAKCOMMON_H__

#pragma once

//-------------------------------------------------------------------------
// Forward declarations

struct IScriptTable;

//-------------------------------------------------------------------------
// BC for common elements of a Tweak entry

class CTweakCommon {
public:

	enum ETweakType {
		eTT_Metadata,		// A metadata item
		eTT_ItemSimple,	// A simple item implemented in CPP (for simple debugging items and such)
		eTT_Menu,				// A menu
		eTT_Broken			// Any item that failed in construction
	};

	// Common tweak constructor
	CTweakCommon() { m_sDescription = "N/A"; }

	// Common tweak destructor
	virtual ~CTweakCommon() {};

	// Tweak constructor used for components that could not be constructed
	CTweakCommon(const string& sErrorMessage) { m_sName = sErrorMessage; }

	// Get the type of this tweak
	virtual ETweakType GetType() { return eTT_Broken; } 

	// Get the name of this tweak component
	const string &GetName(void) const { return m_sName; }

	// Get the description of this tweak component
	const string &GetDescription(void) const { return m_sDescription; }

	// Get if this tweak component is hidden
	bool IsHidden(void) const { return m_bHidden; }

	// Identify the type of tweak a table represents, return an instance
	static CTweakCommon *GetNewTweak( IScriptTable *pTable );

	// Cause a recursive save of changed values - for now, only LUA values
	virtual void StoreChanges( IScriptTable *pTable ) {} ;

protected:

	// Wraps fetching a Lua table entry
	string FetchStringValue(IScriptTable *pTable, const char *sKey) const;

	// The name of this tweak component
	string m_sName;

	// The description of this tweak component
	string m_sDescription;

	// Is this component hidden?
	bool m_bHidden;

	// If there are non-fatal errors, they can be reported here
	string m_sError;
};

#endif // __CTWEAKCOMMON_H__