#pragma once

class CDashboardDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDashboardDlg)

public:
	CDashboardDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDashboardDlg();

// Dialog Data
	enum { IDD = IDD_DASHBOARD };

	const CString& GetLevelPath();
	void SetRecentFileList(CRecentFileList* pList);

protected:
	CString m_levelPath;
	CRecentFileList* m_pRecentList;
	CFont m_listFont;
	CCustomButton	m_openLevelButton;
	CCustomButton	m_newLevelButton;
	CCustomButton	m_closeButton;
	CCustomButton m_openDocumentationButton;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	typedef std::pair<CString, CString> TNamePathPair;
	typedef std::vector<TNamePathPair> TNameFullPathArray;

	CListCtrl m_lstRecentLevels;
	TNameFullPathArray m_levels;

	void FillRecentLevels();
	void RemoveLevelEntry(int index);

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCmdOpenLevel();
	afx_msg void OnBnClickedCmdCreateNewLevel();
	afx_msg void OnBnClickedCmdDocumentation();
	afx_msg void OnBnClickedCheckDontShowAgain();
	afx_msg void OnNMClickListRecentLevels(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnRecentLevelsGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckAutoloadLastLevel();
	afx_msg void OnNMRClickListRecentLevels(NMHDR *pNMHDR, LRESULT *pResult);
};
