#ifndef _AI_PF_PROPERTIES_LIST_DIALOG_H_
#define _AI_PF_PROPERTIES_LIST_DIALOG_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "Resource.h"

struct AgentPathfindingProperties;


class CAIPFPropertiesListDialog : public CDialog
{
	DECLARE_DYNAMIC(CAIPFPropertiesListDialog)

public:
	CAIPFPropertiesListDialog(CWnd* pParent);

	CString GetPFPropertiesList() { return m_sPFPropertiesList; }
	void    SetPFPropertiesList(CString& sPFPropertiesList) { m_sPFPropertiesList = sPFPropertiesList; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	CTreeCtrl m_TreeCtrl;		// CCheckListBox is tricky; using CTreeCtrl instead
	CEdit     m_description;
	CString   m_sPFPropertiesList;

	afx_msg void OnBnClickedEdit();
	afx_msg void OnBnClickedRefresh();
	
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	afx_msg void OnTVClick     (NMHDR*, LRESULT*);
	afx_msg void OnTVDblClk    (NMHDR*, LRESULT*);
	afx_msg void OnTVKeyDown   (NMHDR*, LRESULT*);
	afx_msg void OnTVSelChanged(NMHDR*, LRESULT*);

private:
	const char* m_scriptFileName;

	void UpdateList();
	void UpdateDescription();
	void UpdatePFPropertiesString();
	
	void ToggleItemState(bool bInvokeSetCheck = false);
	
	std::map<CString, const AgentPathfindingProperties*> m_mapPFProperties;
	std::set<CString>                                    m_setPFProperties;
};

#endif	// #ifndef _AI_PF_PROPERTIES_LIST_DIALOG_H_
