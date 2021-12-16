#pragma once

class CJointComboBoxManager;




class CProxiesDlg : public CDialog
{

public:
	DECLARE_DYNAMIC(CProxiesDlg)

	CProxiesDlg( CWnd* pParent = NULL )	: CDialog(CProxiesDlg::IDD, pParent)	
	{
		m_ProxyType=0;
		m_pJointComboBoxManager=0;
	}

	~CProxiesDlg();

	// Dialog Data
	enum { IDD = IDD_CHARACTER_EDITOR_PROXIES };

	class CModelViewportCE *m_pModelViewportCE;
	void InitFromJointNameList( const std::vector< CString >& boneNames );
	void ClearBones();
	void SelectBone( const CString &bone );
	void UpdateList();
	CString GetBonenameFromWindow();

	afx_msg void OnClicked_AlignJointWithProxy();
	afx_msg void OnJointSelect();
	afx_msg void OnProxySelect();
	afx_msg void OnBnClicked_NEW();
	afx_msg void OnBnClicked_RENAME();
	afx_msg void OnBnClicked_REMOVE();
	afx_msg void OnBnClicked_IMPORT();
	afx_msg void OnBnClicked_EXPORT();
	afx_msg void OnChange_ProxyParameters();

	CComboBox m_strJointName;
	CButton m_ButtonAlignBoneAttachment;

	CListBox m_ProxyList;
	CButton m_ButtonRENAME;
	CButton m_ButtonREMOVE;
	CButton m_ButtonIMPORT;
	CButton m_ButtonEXPORT;

	CNumberCtrl m_Proxy_Radius;
	CNumberCtrl m_Proxy_XAxis;
	CNumberCtrl m_Proxy_YAxis;
	CNumberCtrl m_Proxy_ZAxis;

	CComboBox m_Proxy_Purpose;

	int m_ProxyType;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	struct IProxy* GetSelectedIProxy();
	CJointComboBoxManager* m_pJointComboBoxManager;

private:
	virtual void OnOK() override {}

	DECLARE_MESSAGE_MAP()
};


