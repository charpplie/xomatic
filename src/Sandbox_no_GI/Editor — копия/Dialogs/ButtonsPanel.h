#if !defined(__ButtonsPanel__)
#define __ButtonsPanel__

#pragma once

class CToolButton;

/////////////////////////////////////////////////////////////////////////////
// Panel with custom auto arranged buttons
class CButtonsPanel : public CXTResizeDialog
{
public:
	struct SButtonInfo
	{
		CString name;
		CString toolClassName;
		CString toolUserDataKey;
		CString toolUserData;
		CRuntimeClass *pToolClass;

		SButtonInfo() : pToolClass(0) {};
	};

	CButtonsPanel();
	virtual ~CButtonsPanel();

	enum { IDD = IDD_MAINTOOLS };

	virtual void AddButton( const SButtonInfo &button );
	virtual void AddButton( CString name,CString toolClass );
	virtual void AddButton( CString name,CRuntimeClass *pToolClass );
	virtual void ClearButtons();

	virtual void OnButtonPressed( const SButtonInfo &button ) {};
	virtual void UncheckAll();

protected:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

protected:
	void RecalcButtonsLayout();
	void ReleaseGuiButtons();

	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	struct SButton
	{
		SButtonInfo info;
		CToolButton *pButton;
		SButton() : pButton(0) {};
	};

	std::vector<SButton> m_buttons;
};

#endif // !defined(__ButtonsPanel__)
