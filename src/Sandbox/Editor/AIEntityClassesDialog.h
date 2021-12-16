#ifndef _AI_ENTITY_CLASSES_DIALOG_H_
#define _AI_ENTITY_CLASSES_DIALOG_H_

#if _MSC_VER > 1000
#pragma once
#endif


class CAIEntityClassesDialog : public CDialog
{
	DECLARE_DYNAMIC(CAIEntityClassesDialog)

public:
	CAIEntityClassesDialog(CWnd* pParent);

	CString GetAIEntityClasses() const { return m_sAIEntityClasses; }
	void    SetAIEntityClasses(const CString& sAIEntityClasses) { m_sAIEntityClasses = sAIEntityClasses; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	CTreeCtrl m_TreeCtrl;		// CCheckListBox is tricky; using CTreeCtrl instead
	CEdit     m_description;
	CString   m_sAIEntityClasses;

	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	afx_msg void OnTVClick     (NMHDR*, LRESULT*);
	afx_msg void OnTVDblClk    (NMHDR*, LRESULT*);
	afx_msg void OnTVKeyDown   (NMHDR*, LRESULT*);
	afx_msg void OnTVSelChanged(NMHDR*, LRESULT*);

private:
	void UpdateList();
	void UpdateDescription();
	void UpdateAIEntityClassesString();
	
	void ToggleItemState(bool bInvokeSetCheck = false);
	
	std::set<CString>		m_setAIEntityClasses;
};

#endif	// #ifndef _AI_ENTITY_CLASSES_DIALOG_H_
