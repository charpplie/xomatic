////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TVTrackEventKeyDialog.h
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TVTRACKEVENTKEYDIALOG_H__
#define __TVTRACKEVENTKEYDIALOG_H__

#if _MSC_VER > 1000
#pragma once
#endif

struct IAnimNode;
struct IAnimTrack;

#include "ikeydlg.h"
#include "afxwin.h"

// CTVTrackEventKeyDialog dialog

class CTVTrackEventKeyDialog : public IKeyDlg
{
	DECLARE_DYNAMIC(CTVTrackEventKeyDialog)

public:
	CTVTrackEventKeyDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTVTrackEventKeyDialog();

// Dialog Data
	enum { IDD = IDD_TV_EVENT_KEY };

	void SetKey( IAnimNode *node,IAnimTrack *track,int key );
	void RefreshValueType();

protected:
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	IAnimTrack* m_track;
	IAnimNode* m_node;
	int m_key;
public:
	CComboBox m_event;
	CEdit m_eventValue;
	virtual BOOL OnInitDialog();
	///CButton m_hide;
	afx_msg void ControlsToKey();
	afx_msg void OnBnClickedEventEdit();
};

#endif // __TVTRACKEVENTKEYDIALOG_H__