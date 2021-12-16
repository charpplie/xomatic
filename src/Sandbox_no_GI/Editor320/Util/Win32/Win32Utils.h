/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: Win32Utils.h,v 1.1 2008/11/26 14:26:18 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file should contain a set of functions which wrap
WIN32 functions in a more convenient fashion.
-------------------------------------------------------------------------
History:
- 16:11:2008   14:26: Created by Paulo Zaffari
*************************************************************************/

#ifndef Win32Utils_h__
#define Win32Utils_h__

#pragma once

namespace NWin32Utils
{
	string GetWindowErrorMassage(DWORD nWindowErrorCode);
	string GetCurrentWindowErrorMassage();	
};

#endif // Win32Utils_h__
