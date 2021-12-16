////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   TrackViewNewSequenceDialog.h
//  Version:     v1.00
//  Created:     31/5/2012 by Konrad.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once

class CTVNewSequenceDialog : public CDialog
{
	DECLARE_DYNAMIC(CTVNewSequenceDialog)

public:
	CTVNewSequenceDialog(CWnd* pParent);   // standard constructor
	virtual ~CTVNewSequenceDialog();

	const CString& GetLayerName()const {return m_layerName;};
	const CString& GetSequenceName()const {return m_sequenceName;};

	// Dialog Data
	enum { IDD = IDD_TV_NEWSEQ_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnAddNewLayerCheckbox();

private:
	CEdit m_nameEdit;
	CButton m_createNewLayerChk;
	CEdit m_newLayerEdit;

	CString m_layerName;
	CString m_sequenceName;
};
