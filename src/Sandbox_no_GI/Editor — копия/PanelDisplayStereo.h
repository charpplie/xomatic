#if !defined(AFX_PANELDISPLAYSTEREO_H__5CFD89B5_08F6_42D0_B8DA_AF3949C77A60__INCLUDED_)
#define AFX_PANELDISPLAYSTEREO_H__5CFD89B5_08F6_42D0_B8DA_AF3949C77A60__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PanelDisplatHide.h : header file
//

#include "Controls/SliderCtrlEx.h"

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayStereo dialog

class CPanelDisplayStereo : public CDialog, public IEditorNotifyListener
{
// Construction
public:
	typedef std::map<string,ConsoleVarFunc>		TDVariableNameToConsoleFunction;
public:
	CPanelDisplayStereo(CWnd* pParent = NULL);   // standard constructor
	~CPanelDisplayStereo();

// Dialog Data
	//{{AFX_DATA(CPanelDisplayStereo)
	enum { IDD = IDD_PANEL_DISPLAY_STEREO };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPanelDisplayStereo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void OnOK() {};
	virtual void OnCancel() {};
	
	void	SetCheckButtons();
	uint32 m_mask;
	void SetMask();
	void SetControls();
	void ApplyDistValueToEdit();
	void ApplyEditControlToDistValue();
	void RegisterChangeCallback(const char* szVariableName,ConsoleVarFunc fnCallbackFunction);

	static CPanelDisplayStereo*& GetCurrentDisplayStereo();
	static void OnDisplayOptionChanged(ICVar* piDisplayModeVariable);
	void OnDisplayOptionChanged();
	void SetupCallbacks();
	void SendCvarToConsole(CString sVarName, float sVarValue);

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	// Generated message map functions
	//{{AFX_MSG(CPanelDisplayStereo)
	virtual BOOL OnInitDialog();
	afx_msg void OnHideAll();
	afx_msg void OnHideNone();
	afx_msg void OnHideInvert();
	afx_msg void OnChangeHideMask();
	afx_msg void OnScreenDistSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEyeDistSpin(NMHDR *pNMHDR, LRESULT *pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnCbnSelchangeStereoModeCombo();
	afx_msg void OnCbnSelchangeStereoOutputCombo();
	afx_msg void OnCbnSelChangedFlipCheck();
	afx_msg void OnBnClickedPCCheck();
	afx_msg void OnBnClickedConsoleCheck();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNumber_ScreenDist();
	afx_msg void OnNumber_EyeDist();

private:
	CComboBox	m_stereoModeCombo;
	CComboBox	m_stereoOutputCombo;

	CNumberCtrl m_eyeDistEdit;
	CNumberCtrl m_screenDistEdit;
	CComboBox	m_flipCombo;
	CButton		m_enablePC;
	CButton		m_enableConsole;
	int			m_nOldPCStereoMode;

	TDVariableNameToConsoleFunction		m_cVariableNameToConsoleFunction;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PANELDISPLAYSTEREO_H__5CFD89B5_08F6_42D0_B8DA_AF3949C77A60__INCLUDED_)
