////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   MultiMonHelper.cpp
//  Version:     v1.00
//  Created:     27/11/2012 by Andrew Copland.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//					- 
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MultiMonHelper.h"

////////////////////////////////////////////////////////////////////////////
void ClipOrCenterRectToMonitor(LPRECT prc, const UINT flags)
{
	MONITORINFO	mi;
	RECT				rc;
	const int		w = prc->right  - prc->left;
	const int		h = prc->bottom - prc->top;

	// get the nearest monitor to the passed rect. 
	const HMONITOR hMonitor = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST);

	// get the work area or entire monitor rect. 
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	if (flags & MONITOR_WORKAREA)
		rc = mi.rcWork;
	else
		rc = mi.rcMonitor;

	// center or clip the passed rect to the monitor rect 
	if (flags & MONITOR_CENTER)
	{
		prc->left   = rc.left + (rc.right  - rc.left - w) / 2;
		prc->top    = rc.top  + (rc.bottom - rc.top  - h) / 2;
		prc->right  = prc->left + w;
		prc->bottom = prc->top  + h;
	}
	else
	{
		prc->left   = max(rc.left, min(rc.right-w,  prc->left));
		prc->top    = max(rc.top,  min(rc.bottom-h, prc->top));
		prc->right  = prc->left + w;
		prc->bottom = prc->top  + h;
	}
}
