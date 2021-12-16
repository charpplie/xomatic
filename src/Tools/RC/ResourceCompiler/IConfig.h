////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   iconfig.h
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __iconfig_h__
#define __iconfig_h__
#pragma once

class Config;

enum EConfigPriority
{
	eCP_PriorityLowest,						// internally used

	// lowest priories first
	eCP_PriorityFile,							// per file settings
	eCP_PriorityPreset,						// preset settings (from rc_presets_...), if preset was used
	eCP_PriorityPlatform,					// rc.ini
	eCP_PriorityForce,						// e.g. used by autoptimize
	eCP_PriorityCmdline,					// rc.exe command line
	eCP_PriorityJob,					    // Job configuration

	eCP_PriorityNotFile,					// any setting that is not per file (means the per file setting should be greyed out)

	// highest priorities last
	eCP_PriorityHighest,					// internally used, when used to mask out certain priorities this is not masking
};


struct IConfigSink
{
	// Arguments:
	//   value - can be 0 to delete a key, "" can be used to set a key without a special value (e.g. /refresh)
	virtual void Set( const EConfigPriority ePri, const char *key,const char* value ) = 0;
};

/** Configuration options interface.
*/
struct IConfig :public IConfigSink
{
	// Delete instance of configuration class.
	virtual void Release() = 0;

	// Check if configuration has unknown keys. Returns false and outputs error message into log if an unknown key found.
	virtual bool CheckForUnknownKeys() const = 0;

	// Merge configuration.
	// Arguments:
	//   pCfg - must not be 0
	virtual void Merge( const IConfig *pCfg ) = 0;

	// Check if configuration has this key.
	virtual bool HasKey( const char *key ) const = 0;

	// Get value of key. the value isn't modified if the key isn't found
	// Returns:
	//   true if option found, false if not.
	virtual bool GetImpl( const char *key, float &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const = 0;
	virtual bool GetImpl( const char *key, int &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const = 0;
	// sum of all values specified for EConfigPriority
	// Returns:
	//   0 if not value was set
	virtual int GetSum( const char *key ) const = 0;
	virtual bool GetImpl( const char *key, bool &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const = 0;
	virtual bool GetImpl( const char *key, const char* &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const = 0;

	virtual uint32 ClearPriorityUsage( const EConfigPriority ePri )=0;

	// convenience method
	virtual void SetToConfig( const EConfigPriority ePri, IConfigSink *pDestConfig )=0;

	virtual const Config *GetInternalRepresentation() const = 0;

	bool Get( const char *key, float &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const {return GetImpl(key, value, ePriMask);}
	bool Get( const char *key, int &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const {return GetImpl(key, value, ePriMask);}
	bool Get( const char *key, bool &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const {return GetImpl(key, value, ePriMask);}
	bool Get( const char *key, string &value, const EConfigPriority ePriMask=eCP_PriorityHighest ) const
	{
		const char* s;
		if (GetImpl(key, s, ePriMask))
		{
			value = s;
			return true;
		}
		return false;
	}

	template <typename T>
	T GetAs (const char* key, T defaultValue)
	{
		Get (key, defaultValue);
		return defaultValue;
	}
};


struct IConfigKeyRegistry
{
	virtual void VerifyKeyRegistration( const char *szKey )=0;
	virtual bool VerifyKeyRegistration2( const char *szKey )=0;
};


#endif // __iconfig_h__
