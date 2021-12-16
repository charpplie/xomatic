////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   AssetResolverDialog.h
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AssetResolverDialog_H__
#define __AssetResolverDialog_H__

#include "AssetResolverReport.h"

class CMissingAssetDialog : public CXTResizeDialog
{
	DECLARE_DYNCREATE(CMissingAssetDialog)

public:
	CMissingAssetDialog( CWnd* pParent = NULL);   // standard constructor
	virtual ~CMissingAssetDialog();

	static void RegisterViewClass();

	static void Open();
	static void Close();
	static void Update();

// Dialog Data
	enum { IDD = IDD_ERROR_REPORT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();

	afx_msg void OnSize( UINT nType,int cx,int cy );
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	afx_msg void OnReportItemRClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportItemDblClick(NMHDR * pNotifyStruct, LRESULT * result);

	DECLARE_MESSAGE_MAP()

private:
	void Clear();
	void UpdateReport();
	CMissingAssetReport* GetReport();

	typedef std::vector<CMissingAssetRecord*> TRecords;
	void GetSelectedRecords(TRecords& records);

	void AcceptRecort(CMissingAssetRecord* pRecord, int idx = 0);
	void CancelRecort(CMissingAssetRecord* pRecord);
	void ResolveRecord(CMissingAssetRecord* pRecord);
	
private:
	static CMissingAssetDialog* m_instance;

	CXTPReportControl m_wndReport;
	CXTPReportSubListControl m_wndSubList;
	CXTPReportFilterEditControl m_wndFilterEdit;
	CImageList m_imageList;

	CString m_lastPath;
};

#endif // __AssetResolverDialog_H__
