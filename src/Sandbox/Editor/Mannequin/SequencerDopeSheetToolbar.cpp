////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   SequencerDopeSheetToolbar.cpp
//  Version:     v1.00
//  Created:     2014-01-24 by Timothy Brookes.
//  Description: A toolbar which contains basic playback functionality for use
//    in the mannequin editor
// 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

// CSequenceDialogToolbar.cpp implementation file
//

#include "StdAfx.h"
#include "SequencerDopeSheetToolbar.h"

CSequencerDopeSheetToolbar::CSequencerDopeSheetToolbar()
	: CDlgToolBar()
{
	m_lastTime = -1;
}

CSequencerDopeSheetToolbar::~CSequencerDopeSheetToolbar()
{
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetToolbar::InitToolbar()
{
	// Set up time display
	CRect rc(0,0,0,0);
	int index = CommandToIndex(ID_TV_CURSORPOS);
	SetButtonInfo(index, ID_TV_CURSORPOS, TBBS_SEPARATOR, 100);
	GetItemRect(index, &rc);
	++rc.top;
	m_timeWindow.Create("0.000", WS_CHILD|WS_VISIBLE|SS_CENTER|SS_CENTERIMAGE|SS_SUNKEN,rc,this,IDC_STATIC);
	m_timeWindow.SetFont(CFont::FromHandle((HFONT)gSettings.gui.hSystemFontBold));
	m_timeWindow.SetParent(this);
}

//////////////////////////////////////////////////////////////////////////
void CSequencerDopeSheetToolbar::SetTime(float fTime, float fFps)
{
	if (fTime == m_lastTime)
		return;

	m_lastTime = fTime;
	
	int nMins = (int)(fTime / 60.0f);
	fTime -= (float)(nMins * 60);
	int nSecs = (int)fTime;
	fTime -= (float)nSecs;
	int nMillis = fTime * 100.0f;
	int nFrames = (int)(fTime / (1.0f / CLAMP(fFps, FLT_EPSILON, FLT_MAX)));

	CString sText;
	sText.Format("%02d:%02d:%02d (%02d)", nMins, nSecs, nMillis, nFrames);
	m_timeWindow.SetWindowText(sText);
}