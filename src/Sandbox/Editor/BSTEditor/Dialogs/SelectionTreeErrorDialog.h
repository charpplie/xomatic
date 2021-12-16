////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeErrorDialog.h
//  Version:     v1.00
//  Created:     14/9/2011 by Paul Reindell
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SelectionTreeErrorDialog_H__
#define __SelectionTreeErrorDialog_H__

#include "BSTEditor/SelectionTreeErrorReport.h"

class CSelectionTreeErrorDialog : public CXTResizeDialog
{
	DECLARE_DYNCREATE(CSelectionTreeErrorDialog)

public:
	CSelectionTreeErrorDialog( CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectionTreeErrorDialog();

	static void CSelectionTreeErrorDialog::RegisterViewClass();

	static void Open( CSelectionTreeErrorReport *pReport );
	static void Close();
	static void Clear();
	static void Reload();
	void CopyToClipboard();

// Dialog Data
	enum { IDD = IDD_ERROR_REPORT };

protected:
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	void SetReport(CSelectionTreeErrorReport *report) {m_pErrorReport = report;}
	void UpdateErrors();

	virtual void PostNcDestroy();
	afx_msg void OnNMDblclkErrors(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSelectObjects();
	afx_msg void OnSize( UINT nType,int cx,int cy );
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	afx_msg void OnReportItemClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportItemRClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportColumnRClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportItemDblClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportKeyDown(NMHDR * pNotifyStruct, LRESULT * result);


	void ReloadErrors();

	DECLARE_MESSAGE_MAP()

	CSelectionTreeErrorReport *m_pErrorReport;
	CImageList m_imageList;

	static CSelectionTreeErrorDialog* m_instance;

	std::vector<CSelectionTreeErrorRecord> m_errorRecords;

	CXTPReportControl m_wndReport;
	CXTPReportSubListControl m_wndSubList;
	CXTPReportFilterEditControl m_wndFilterEdit;
};

#endif // __SelectionTreeErrorDialog_H__
