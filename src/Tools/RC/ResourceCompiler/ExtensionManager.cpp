////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   extensionmanager.cpp
//  Version:     v1.00
//  Created:     5/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <time.h>
#include "ExtensionManager.h"
#include "IConvertor.h"
#include "IResCompiler.h"       // IResourceCompiler
#include "IRCLog.h"             // IRCLog
#include "StringHelpers.h"

//////////////////////////////////////////////////////////////////////////
ExtensionManager::ExtensionManager()
{
}

//////////////////////////////////////////////////////////////////////////
ExtensionManager::~ExtensionManager()
{
	UnregisterAll();
}

//////////////////////////////////////////////////////////////////////////
IConvertor* ExtensionManager::FindConvertor( EPlatform platform, const char *filename )  const
{
	const ExtVector& extVector = m_extVector[platform];
	string const strFilename = StringHelpers::MakeLowerCase(string(filename));

	for (size_t i = 0; i < extVector.size(); ++i)
	{
		if (StringHelpers::EndsWith(strFilename, extVector[i].first))
		{
			return extVector[i].second;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
void ExtensionManager::RegisterConvertor( IConvertor *conv, IResourceCompiler *rc )
{
	assert(conv);
	assert(rc);

	m_convertors.push_back( conv );

	string strExt;
	for (int i = 0; i < conv->GetNumExt(); ++i)
	{
		if (i)
		{
			strExt += ", ";
		}
		strExt += conv->GetExt(i);
	}

	RCLog("    Registered convertor for %s", strExt.c_str());
/*
	time_t nTime = conv->GetTimestamp();
	char* szTime = "unknown";
	if (nTime)
	{
		szTime = asctime(localtime(&nTime));
		szTime[strlen(szTime)-1] = '\0';  // erase '\n' at the end of the string
	}
	RCLog("      timestamp %s", szTime);
*/
	for (int k = 0; k < conv->GetNumPlatforms(); ++k)
	{
		EPlatform platform = conv->GetPlatform(k);
		for (int i = 0; i < conv->GetNumExt(); ++i)
		{
			const char* const ext = conv->GetExt(i);
			assert(ext && ext[0]);
			string const extToAdd = StringHelpers::MakeLowerCase(string('.') + ext);
			m_extVector[platform].push_back(ExtVector::value_type(extToAdd, conv));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void ExtensionManager::UnregisterAll()
{
	for (size_t i = 0; i < m_convertors.size(); ++i)
	{
		IConvertor *conv = m_convertors[i];
		conv->Release();
	}
	m_convertors.clear();

	for (size_t i = 0; i < ePlatform_LAST; ++i)
	{
		m_extVector[i].clear();
	}
}
