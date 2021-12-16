#include "StdAfx.h"
#include "Controls/FeedbackButton.h"
#include "Dialogs/FeedbackFormDlg.h"

// uncomment this if you want to hide all feedback buttons in Sandbox
#define NO_SANDBOX_FEEDBACK_BUTTONS

IMPLEMENT_DYNAMIC(CFeedbackButton, CStatic)

CFeedbackButton::CFeedbackButton()
{
}

CFeedbackButton::~CFeedbackButton()
{
}

BEGIN_MESSAGE_MAP(CFeedbackButton, CStatic)
	ON_CONTROL_REFLECT(STN_CLICKED, OnBnClicked)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

void CFeedbackButton::Init(const char* pFeatureName)
{
	const int kButtonSize = 18;
	HICON hIcon;

	m_featureName = pFeatureName;

#ifdef NO_SANDBOX_FEEDBACK_BUTTONS
	int showStyle = 0;
#else
	int showStyle = WS_VISIBLE;
#endif
	
	ModifyStyle(
		0xffffffff,
		WS_CHILD | showStyle | SS_NOTIFY | SS_ICON | SS_REALSIZEIMAGE | SS_CENTERIMAGE);
	SetWindowPos(0, 0, 0, kButtonSize, kButtonSize, SWP_NOMOVE);
	SetWindowText("");
	hIcon = (HICON)LoadImage(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDI_FEEDBACK), IMAGE_ICON, 16, 16, 0);
	SetIcon(hIcon);
	EnableToolTips();
	m_toolTip.Create(this);
	m_toolTip.AddTool(this, ("Send us feedback about " + m_featureName));
	RedrawWindow();
}

void CFeedbackButton::OnBnClicked()
{
	CFeedbackFormDlg dlg;

	dlg.SetFeatureName(m_featureName);
	dlg.DoModal();
}

BOOL CFeedbackButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor(AfxGetApp()->LoadCursor(IDC_HAND_INTERNAL));
	return TRUE;
}

BOOL CFeedbackButton::PreTranslateMessage(MSG* pMsg)
{
	m_toolTip.RelayEvent(pMsg);
	return CStatic::PreTranslateMessage(pMsg);
}
