////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   PerforcePlugin.cpp
//  Version:     v1.00
//  Created:     21 Sen 2004 by Sergiy Shaykin.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "PerforcePlugin.h"
#include "Include/ISourceControl.h"


void CPerforcePlugin::Release()
{
	delete this;
}

void CPerforcePlugin::ShowAbout()
{
}

const char * CPerforcePlugin::GetPluginGUID()
{
	return 0;
}

DWORD CPerforcePlugin::GetPluginVersion()
{
	return PERFORCE_PLUGIN_CURRENT_VERSION;
}

const char * CPerforcePlugin::GetPluginName()
{
	return PERFORCE_PLUGIN_NAME;
}

bool CPerforcePlugin::CanExitNow()
{
	return true;
}

void CPerforcePlugin::Serialize(FILE *hFile, bool bIsStoring)
{

}

void CPerforcePlugin::ResetContent()
{

}

bool CPerforcePlugin::CreateUIElements()
{
	return true;
}

bool CPerforcePlugin::ExportDataToGame(const char * pszGamePath)
{
	return false;
}