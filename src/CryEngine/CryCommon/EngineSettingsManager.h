////////////////////////////////////////////////////////////////////////////
//
//  CryENGINE Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   EnigneSettingsManager.h
//  Version:     v1.00
//  Created:     12/07/2004 by Benjamin Peters
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __ENGINESETTINGSMANAGER_H__
#define __ENGINESETTINGSMANAGER_H__
#pragma once

#if defined(PS3) || defined(LINUX)
	//require tchar definition at least, currently char
//	typedef wchar_t TCHAR;	typedef string tstring;	#define _tcsclen wcslen
	typedef char TCHAR;
	typedef string tstring;	
	#define _tcsclen strlen
	#define _T tstring
	#define _tcsicmp strcasecmp
#else

#include "CryModuleDefs.h"												// ECryModule

#ifdef NOT_USE_CRY_STRING
	#include <string>																// STL string
	typedef std::string string;
#else
	#include <platform.h>														// string
#endif

#if !defined(TSTRING_DEFINED)
#include <tchar.h>
typedef string tstring;
#endif //!defined(TSTRING_DEFINED)


#include <map>


#if defined(WIN32) || defined(WIN64)


//////////////////////////////////////////////////////////////////////////
// Manages storage and loading of all information for tools and CryENGINE, by either registry or an INI file.
// Information can be read and set by key-to-value functions.
// Specific information can be set by a dialog application called by this class.
// If the engine root path is not found, a fall-back dialog is opened.
class CEngineSettingsManager
{
public:
	// prepares CEngineSettingsManager to get requested information either from registry or an INI file,
	// if existent as a file with name an directory equal to the module, or from registry.
	CEngineSettingsManager(const TCHAR* moduleName=NULL, const TCHAR* iniFileName=NULL);

	void RestoreDefaults();

	// stores/loads user specific information for modules to/from registry or INI file
	template <typename T> bool GetModuleSpecificEntry(const tstring& key, T& value);
	template <typename T> bool SetModuleSpecificEntry(const tstring& key, const T& value);

	bool HasKey(const tstring& key);
	void GetValueByRef(const tstring& key, tstring& value);
	void GetValueByRef(const tstring& key, int& value);
	template <typename T> T GetValue(const tstring& key);
	void SetKey(const tstring& key, const TCHAR* value);
	void SetKey(const tstring& key, const tstring& value);
	void SetKey(const tstring& key, bool value);
	void SetKey(const tstring& key, int value);

	bool StoreData();
	void CallSettingsDialog(void* hParent);
	void CallRootPathDialog(void* hParent);

	void SetRootPath( const tstring& szRootPath );

	// returns path determined either by registry or by INI file
	tstring GetRootPath();
	void	SetParentDialog(unsigned long window);

private:
	typedef std::map<tstring,tstring> TKeyValueMap;

	tstring				m_sModuleName;					// name to store key-value pairs of modules in (registry) or to identify INI file
	tstring				m_sModuleFileName;				// used in case of data being loaded from INI file
	bool					m_bGetDataFromRegistry;
	TKeyValueMap	m_keyToValueMap;

	void*					m_hBtnBrowse;
	unsigned long				m_hWndParent;

private:
	tstring Trim(tstring& str);

	void LoadEngineSettingsFromRegistry();
	bool StoreEngineSettingsToRegistry();

	// parses a file and stores all flags in a private key-value-map
	bool LoadValuesFromConfigFile(const TCHAR* szFileName);

	bool SetRegValue(void* key, const tstring& valueName, const tstring& value);
	bool SetRegValue(void* key, const tstring& valueName, const TCHAR* value);
	bool SetRegValue(void* key, const tstring& valueName, bool value);
	bool SetRegValue(void* key, const tstring& valueName, int value);
	bool GetRegValue(void* key, const tstring& valueName, tstring& value);
	bool GetRegValue(void* key, const tstring& valueName, bool& value);
	bool GetRegValue(void* key, const tstring& valueName, int& value);

	class RegKey
	{
	public:
		RegKey(const tstring& key, bool writeable);
		~RegKey();
		void* pKey;
	};

public:
	long HandleProc(void* pWnd, long uMsg, long wParam, long lParam);
};

template <typename T> bool CEngineSettingsManager::GetModuleSpecificEntry(const tstring& key, T& value)
{
	if (!m_bGetDataFromRegistry)
	{
		if (!HasKey(key))
			return false;
		value = GetValue<T>(key);
		return true;
	}

	RegKey superKey(tstring(_T("Software\\Crytek\\Settings\\"))+m_sModuleName, false);
	bool result = false;
	if (superKey.pKey)
	{
		T regValue;
		if (result = GetRegValue(superKey.pKey, key, regValue))
		{
			value = regValue;
			result = true;
		}
	}

	return result;
}

template <typename T> bool CEngineSettingsManager::SetModuleSpecificEntry(const tstring& key, const T& value)
{
	SetKey(key, value);
	if (!m_bGetDataFromRegistry)
		return StoreData();

	RegKey superKey(tstring(_T("Software\\Crytek\\Settings\\"))+m_sModuleName, true);
	if (superKey.pKey)
		return SetRegValue(superKey.pKey, key, value);
	return false;
}

template <typename T> T CEngineSettingsManager::GetValue(const tstring& key)
{
	T value;
	GetValueByRef(key, value);
	return value;
}

#endif // defined(WIN32) || defined(WIN64)
#endif//PS3

#endif // __RESOURCECOMPILERHELPER_H__
