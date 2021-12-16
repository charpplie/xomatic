////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   cmdline.cpp
//  Version:     v1.00
//  Created:     5/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CmdLine.h"
#include "Config.h"

//////////////////////////////////////////////////////////////////////////
CmdLine::CmdLine()
{
	m_platform = ePlatform_UNKNOWN;
	m_config = 0;
}

//////////////////////////////////////////////////////////////////////////
void CmdLine::Parse( int argc, char **argv,Config *config )
{
	m_config = config;
	for (int i = 1; i < argc; i++)
	{
		const char *param = argv[i];
		bool bFlag = false;
		bool bLast = ((i + 1) == argc);
		if (param[0] == '-' || param[0] == '/')
		{
			// remove flag specifier
			bFlag = true;
			++param;
		}
		ParseParam(param,bFlag,bLast);
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdLine::ParseParam( const char* param, bool bFlag,bool bLast )
{
	if (!bFlag)
	{
		if (m_fileSpec.empty())
			m_fileSpec = param;
	}
	else
	{
		// Flag.
		// Split on key/value pair.
		string prm = param;
		int splitter = int(prm.find('='));
		if (splitter >= 0)
		{
			string key = prm.Mid(0,splitter);
			string value = prm.Mid(splitter+1);
			// Put key/value pair to config.
			if (!key.empty() && m_config)
				m_config->Set(eCP_PriorityCmdline,key.c_str(),value.c_str() );
		}
		else
		{
			m_config->Set (eCP_PriorityCmdline,prm.c_str(), "");
		}
	}
}
