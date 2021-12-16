////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   tvconsolekeydialog.h
//  Version:     v1.00
//  Created:     29/10/2009 by Paulo Zaffari.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef TVGotoKeyDialog_h__
#define TVGotoKeyDialog_h__

#pragma once

struct IAnimNode;
struct IAnimTrack;

#include "ikeydlg.h"

// CTVEntityKeyDialog dialog

class CTVGotoKeyDialog : public IKeyDlg
{
	DECLARE_DYNAMIC(CTVGotoKeyDialog)

public:
	CTVGotoKeyDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTVGotoKeyDialog();

	// Dialog Data
	enum { IDD = IDD_TV_GOTO_KEY };

	void SetKey( IAnimNode *node,IAnimTrack *track,int key );

protected:
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	IAnimTrack* m_track;
	IAnimNode* m_node;
	int m_key;
public:
	CNumberCtrl m_fTime;

	virtual BOOL OnInitDialog();
	///CButton m_hide;
	afx_msg void ControlsToKey();
};

#endif // TVGotoKeyDialog_h__
