/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Here the actual input implementation gets chosen for the
							different platforms.
-------------------------------------------------------------------------
History:
- Dec 05,2005:	Created by Marco Koegler

*************************************************************************/
#ifndef __CRYINPUT_H__
#define __CRYINPUT_H__
#pragma once


#if !defined(DEDICATED_SERVER)
	#if defined(LINUX)
		// Linux client (not dedicated server)
		#define USE_LINUXINPUT
		#include "LinuxInput.h"










	#else
		// Win32
		#define USE_DXINPUT
		#include "DXInput.h"
	#endif
	#include "InputCVars.h"
#endif

#endif //__CRYINPUT_H__

