////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   GroupPanel.h
//  Version:     v1.00
//  Created:     23/06/2009 by Sergiy Shaykin.
//  Compilers:   Visual C++.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Grouppanel_h__
#define __Grouppanel_h__
#pragma once

#include "Controls\ToolButton.h"

class CGroup;

// CGroupPanel dialog
class CGroupPanel : public CDialog, public IEditorNotifyListener
{
	DECLARE_DYNAMIC(CGroupPanel)

public:
	CGroupPanel( CGroup * obj, CWnd* pParent = NULL);   // standard constructor
	virtual ~CGroupPanel();

// Dialog Data
	enum { IDD = IDD_PANEL_GROUP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	afx_msg void OnGroup();
	afx_msg void OnOpen();
	afx_msg void OnAttach();
	afx_msg void OnUngroup();
	afx_msg void OnCloseGroup();
	afx_msg void OnDetach();

	void UpdateButtons();

	virtual void OnOK() {};
	virtual void OnCancel() {};

	DECLARE_MESSAGE_MAP()

	CGroup * m_obj;

	CCustomButton m_GroupBn;
	CCustomButton m_OpenBn;
	CCustomButton m_AttachBn;
	CCustomButton m_UngroupBn;
	CCustomButton m_CloseBn;
	CCustomButton m_DetachBn;
};


#endif // __Grouppanel_h__