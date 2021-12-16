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
	void SyncSliderValueToEditControl();
	void ApplyDistValueToSlider();
	void ApplyDistValueToEdit();
	void ApplySliderControlToDistValue();
	void ApplyEditControlToDistValue();
	void RecalcSliderSize();
	void RegisterChangeCallback(const char* szVariableName,ConsoleVarFunc fnCallbackFunction);

	static CPanelDisplayStereo*& GetCurrentDisplayStereo();
	static void OnDisplayOptionChanged(ICVar* piDisplayModeVariable);
	void OnDisplayOptionChanged();
	void SetupCallbacks();

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	// Generated message map functions
	//{{AFX_MSG(CPanelDisplayStereo)
	virtual BOOL OnInitDialog();
	afx_msg void OnHideAll();
	afx_msg void OnHideNone();
	afx_msg void OnHideInvert();
	afx_msg void OnChangeHideMask();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnCbnSelchangeStereoModeCombo();
	afx_msg void OnCbnSelchangeStereoOutputCombo();
	afx_msg void OnBnClickedDontFlipRadio();
	afx_msg void OnBnClickedFlipRadio();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEnKillfocusEyeDistEdit();
	afx_msg void OnEnKillfocusScreenDistEdit();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	CSliderCtrlCustomDraw m_eyeDistSlider;
	CSliderCtrlCustomDraw m_screenDistSlider;
	CComboBox m_stereoModeCombo;
	CComboBox m_stereoOutputCombo;

	CEdit m_eyeDistEdit;
	CEdit m_screenDistEdit;
	CButton m_dontFlip;
	CButton m_flip;

	TDVariableNameToConsoleFunction		m_cVariableNameToConsoleFunction;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PANELDISPLAYSTEREO_H__5CFD89B5_08F6_42D0_B8DA_AF3949C77A60__INCLUDED_)
