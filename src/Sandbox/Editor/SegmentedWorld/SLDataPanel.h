////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2014.
// -------------------------------------------------------------------------
//  File name:   CSLPanel.h
//  Version:     v1.00
//  Created:     27/4/2014 by Allen Chen
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SLDATA_PANEL_H__
#define __SLDATA_PANEL_H__

#include "Controls/TreeCtrlReport.h"

enum ESCMMenu
{
	MENU_UNDEFINED = 0,
	MENU_SCM_ADD,
	MENU_SCM_CHECK_IN,
	MENU_SCM_CHECK_OUT,
	MENU_SCM_UNDO_CHECK_OUT,
	MENU_SCM_GET_LATEST,
	MENU_SCM_HISTORY,
};

class CSLBaseItemRecord : public CTreeItemRecord
{
public:
	void UpdateStatus();

	virtual void CreateItems() = 0;
	virtual uint32 GetFileSCMAttributes() = 0;
	virtual void GetFilename(std::vector<CString> &filenames) const = 0;
};

class CSLDataTreeCtrl : public CTreeCtrlReport
{
public:
	CSLDataTreeCtrl();

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
};

class CSLDataPanel : public CXTResizeDialog
{
public:
	CSLDataPanel(const UINT nID, CWnd* pParent) : CXTResizeDialog(nID, pParent) {}

	void ShowContextMenu(CSLBaseItemRecord *pRecord, const CPoint &pos);
	void DoSourceControlOp(CSLBaseItemRecord *pRecord, int scmOp);

protected:
	virtual void OnOK() {}
	virtual void OnCancel() {}
	virtual BOOL OnInitDialog();

	CSLDataTreeCtrl m_tree;
};

#endif // __SLDATA_PANEL_H__