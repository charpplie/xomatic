#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios
// -------------------------------------------------------------------------
//  Created:     29 Feb 2012 by Sergiy Shaykin
//  Description: Modeless dialog for list of errors. 
//               To avoid interuption on start Editor time and on load level time.
//               Using: To add messages from any part of CryEngine you can use this style:
//               gEnv->pSystem->ShowMessage("Text", "Caption", MB_OK);
//
////////////////////////////////////////////////////////////////////////////

class CErrorsDlg : public CDialog
{
	DECLARE_DYNAMIC(CErrorsDlg)

public:
	CErrorsDlg(CWnd* pParent = NULL);
	virtual ~CErrorsDlg();
	void AddMessage(const CString& text, const CString& caption);

	enum { IDD = IDD_ERRORS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnOK() {};
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	void ProcessingMessages();

private:
	CStatic m_errorIconCtrl;
	CRichEditCtrl m_richEdit;
	bool m_bFirstMessage;
};
