////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TVEventsDialog.h
//  Version:     v1.00
//  Created:     3/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __tveventsdialog_h__
#define __tveventsdialog_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "Resource.h"
#include "afxcmn.h"
#include <IMovieSystem.h>

// CTVEventsDialog dialog

class CTVEventsDialog : public CDialog
{
	DECLARE_DYNAMIC(CTVEventsDialog)

public:
	CTVEventsDialog(IAnimSequence *pTrack, CWnd* pParent = NULL);   // standard constructor
	virtual ~CTVEventsDialog();

// Dialog Data
	enum { IDD = IDD_TV_EVENTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonAddevent();
	afx_msg void OnBnClickedButtonRemoveevent();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	TrackEvents m_events;

private:
	IAnimSequence *m_pTrack;

	// list of events
	CListCtrl m_List;
};

#endif //__tveventsdialog_h__