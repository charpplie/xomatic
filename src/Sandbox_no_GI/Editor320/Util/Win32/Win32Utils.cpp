/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: Win32Utils.cpp,v 1.1 2008/11/26 14:26:18 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file should contain a set of functions which wrap
WIN32 functions in a more convenient fashion.
-------------------------------------------------------------------------
History:
- 16:11:2008   14:26: Created by Paulo Zaffari
*************************************************************************/

#include "stdafx.h"
#include "Win32Utils.h"

using namespace NWin32Utils;

//////////////////////////////////////////////////////////////////////////
string GetWindowErrorMassage(DWORD nWindowErrorCode)
{
	char szBuffer[65535]="";
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,nWindowErrorCode,0,szBuffer,65534,NULL);
	return (string)szBuffer;
}
//////////////////////////////////////////////////////////////////////////
string GetCurrentWindowErrorMassage()
{
	char szBuffer[65535]="";
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),0,szBuffer,65534,NULL);
	return (string)szBuffer;
}
//////////////////////////////////////////////////////////////////////////
