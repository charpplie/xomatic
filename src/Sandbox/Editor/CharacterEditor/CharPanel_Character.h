#pragma once


// CCharacterPropertiesDlg dialog

class CModelViewportCE;
class ICharacterChangeListener;

class CCharacterPropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CCharacterPropertiesDlg)

public:
	CCharacterPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCharacterPropertiesDlg();

// Dialog Data
	enum { IDD = IDD_CHARACTER_EDITOR_CHARACTER };

	CModelViewportCE *m_pModelViewportCE;

	void SetOverrideMaterial();
	void SetMaterial(CMaterial* pMaterial);
	void SetCharacterChangeListener(ICharacterChangeListener* pListener);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual void OnOK();
	virtual BOOL OnInitDialog();

	void SendOnCharacterChanged();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedDefaultMtl();
	afx_msg void OnBnClickedApplyMaterial();
	afx_msg void OnBnClickedOverrideMaterial();
	afx_msg void OnBnClickedSelectMaterial();

	CEdit m_materialEdit;
	ICharacterChangeListener* pCharacterChangeListener;
	CMaterial* m_DefaultMaterial;
};
