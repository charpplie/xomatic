////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   SequencerDopeSheetToolbar.h
//  Version:     v1.00
//  Created:     2014-01-24 by Timothy Brookes.
//  Description: A toolbar which contains a time display, for use in 
//    mannequin editor.
// 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SequencerDialogToolbar_h__
#define __SequencerDialogToolbar_h__
#pragma once

class CSequencerDopeSheetToolbar;

#include "../Controls/DlgBars.h"
#include "SequencerDopeSheetBase.h"

class CSequencerDopeSheetToolbar : public CDlgToolBar
{
	
public:
	CSequencerDopeSheetToolbar();
	virtual ~CSequencerDopeSheetToolbar();
	virtual void InitToolbar();
	virtual void SetTime(float fTime, float fFps);

protected:
	CStatic m_timeWindow;
	float m_lastTime;

};

#endif // __SequencerDialogToolbar_h__