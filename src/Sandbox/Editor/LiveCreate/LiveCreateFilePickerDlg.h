#pragma once

#ifndef NO_LIVECREATE

namespace LiveCreate
{
	class CBGTask_ScanDirectory;
}

class CLiveCreateFilePickerDlg : public CDialog
{
	DECLARE_DYNAMIC(CLiveCreateFilePickerDlg)

private:
	struct STreeItem
	{
		LiveCreate::IPlatformHandler* m_pPlatform;
		const STreeItem* m_pParent;
		CString m_name;
		bool m_bIsExecutable;
		bool m_bWasScanned;
		bool m_bIsConfirmed;
		bool m_bIsExpaned;
		HTREEITEM m_hItem;
		std::vector<STreeItem*> m_pSubDirs;
		LiveCreate::CBGTask_ScanDirectory* m_pScanTask;

		STreeItem(const STreeItem* parent, const CString& name, LiveCreate::IPlatformHandler* pPlatform);
		~STreeItem();

		CString FormatFullPath() const;
		void Scan();
		bool Update(CTreeCtrl& treeCtrl);
		void CreateRootItem(CTreeCtrl& treeCtrl);
		bool CreateItems(CTreeCtrl& treeCtrl);
		bool CreateItemsRecursive(CTreeCtrl& treeCtrl);
		STreeItem* CreateSubDirectory(const CString& name, bool bFromScan);
		STreeItem* CreateExecutableFile(const CString& name, bool bFromScan);
		bool OnItemExpanded(CTreeCtrl& treeCtrl, HTREEITEM hItem);
		STreeItem* FindItem(HTREEITEM hItem);
	};

public:
	CLiveCreateFilePickerDlg(CWnd* pParent = NULL);
	virtual ~CLiveCreateFilePickerDlg();

	void Setup(LiveCreate::IPlatformHandler* pPlatform, const CString& executableName);

	ILINE const CString& GetSelectedDirectory() const
	{
		return m_selectedDirectory;
	}

	ILINE const CString& GetSelectedExecutable() const
	{
		return m_selectedExecutable;
	}

	// Dialog Data
	enum { IDD = IDD_LIVECREATE_PICKER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOk();
	afx_msg void OnTreeItemExpand(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTreeItemSelected(NMHDR* pNMHDR, LRESULT* pResult);

	void UpdateOKButton();

private:
	static const UINT_PTR kTimerRefresh = 12345;
	static const uint kRefreshTimerPeriod = 200;

	LiveCreate::IPlatformHandler* m_pPlatform;
	bool m_bAllowExecutables;
	CString m_selectedDirectory;
	CString m_selectedExecutable;
	CTreeCtrl m_treeView;
	STreeItem* m_pRoot;
	STreeItem* m_pSelected;
	CImageList m_cImageList;
	CButton m_btnOK;
};

#endif