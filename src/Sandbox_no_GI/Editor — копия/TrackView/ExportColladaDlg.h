////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  Created:    27/5/2010 by Xiaomao Wu.
//  Compiler:   Visual Studio 2008 Professional
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#if !defined __EXPORT_COLLADA_H__
#define __EXPORT_COLLADA_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

class CExportColladaDlg : public CDialog
{
public:
	enum {EXPORT_GLOBAL_POSE, EXPORT_LOCAL_POSE};
	CExportColladaDlg (const CString path, const CString fileName, CWnd* pParent = NULL);

	// Dialog Data
	enum {IDD = IDD_TRACKVIEW_EXPORT_COLLADA};
	float GetFPS() const {return fps;}
	int GetExportType() const {return exportType;}
	CString GetExportPath() const {return exportPath;}
	bool IsPathSet() const {return bPathSet;}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnChooseExportPath();
	afx_msg void OnExportPathChange();
	afx_msg void OnExportTypeChange();
	afx_msg void OnFPSChange();

	DECLARE_MESSAGE_MAP()

	CComboBox m_ExportType;
	CEdit m_ExportPath;
	CNumberCtrl m_FPS;

private:
	CString m_Path;
	CString m_FileName;

	bool bPathSet;
	float fps;
	int exportType;
	CString exportPath;

};

#endif