// PanelDisplatHide.cpp : implementation file
//

#include "StdAfx.h"
#include "PanelDisplayStereo.h"
#include "DisplaySettings.h"

const int MaxDistValue = 100;

//////////////////////////////////////////////////////////////////////////

static float ConvertSliderPosToFloatValue(int value)
{
	return (float)(value-MaxDistValue)/MaxDistValue;
}

static int ConvertFloatValueToSliderPos(float value)
{
	return (value*MaxDistValue)+MaxDistValue;
}

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayStereo dialog


CPanelDisplayStereo::CPanelDisplayStereo(CWnd* pParent /*=NULL*/)
	: CDialog(CPanelDisplayStereo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPanelDisplayStereo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	assert(GetCurrentDisplayStereo()==NULL);
	GetCurrentDisplayStereo()=this;

	SetupCallbacks();
	Create( IDD,pParent );
	GetIEditor()->RegisterNotifyListener(this);
}

CPanelDisplayStereo::~CPanelDisplayStereo()
{
	GetIEditor()->UnregisterNotifyListener(this);
	GetCurrentDisplayStereo()=NULL;
}


void CPanelDisplayStereo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STEREO_MODE_COMBO, m_stereoModeCombo);
	DDX_Control(pDX, IDC_STEREO_OUTPUT_COMBO, m_stereoOutputCombo);
	DDX_Control(pDX, IDC_STEREO_EYEDIST_SLIDER, m_eyeDistSlider);
	DDX_Control(pDX, IDC_STEREO_EYEDIST_EDIT, m_eyeDistEdit);
	DDX_Control(pDX, IDC_STEREO_SCREENDIST_SLIDER, m_screenDistSlider);
	DDX_Control(pDX, IDC_STEREO_SCREENDIST_EDIT, m_screenDistEdit);
	DDX_Control(pDX, IDC_STEREO_FLIPEYES_RADIO, m_dontFlip);
	DDX_Control(pDX, IDC_STEREO_FLIPEYES_RADIO2, m_flip);

	//
	//IDC_STEREO_FLIPEYES_RADIO2

	//{{AFX_DATA_MAP(CPanelDisplayStereo)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPanelDisplayStereo, CDialog)
	//{{AFX_MSG_MAP(CPanelDisplayStereo)
	ON_CBN_SELCHANGE(IDC_STEREO_MODE_COMBO, &CPanelDisplayStereo::OnCbnSelchangeStereoModeCombo)
	ON_CBN_SELCHANGE(IDC_STEREO_OUTPUT_COMBO, &CPanelDisplayStereo::OnCbnSelchangeStereoOutputCombo)
	ON_BN_CLICKED(IDC_STEREO_FLIPEYES_RADIO, &CPanelDisplayStereo::OnBnClickedDontFlipRadio)
	ON_BN_CLICKED(IDC_STEREO_FLIPEYES_RADIO2, &CPanelDisplayStereo::OnBnClickedFlipRadio)
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_EN_KILLFOCUS(IDC_STEREO_EYEDIST_EDIT, &CPanelDisplayStereo::OnEnKillfocusEyeDistEdit)
	ON_EN_KILLFOCUS(IDC_STEREO_SCREENDIST_EDIT, &CPanelDisplayStereo::OnEnKillfocusScreenDistEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayStereo message handlers

BOOL CPanelDisplayStereo::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_mask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
	
	SetCheckButtons();

	m_eyeDistSlider.SetRange(0, MaxDistValue*2);
	m_screenDistSlider.SetRange(0, MaxDistValue*2);

	//SetResize(IDC_STEREO_EYEDIST_SLIDER, SZ_HORRESIZE(1));
	
	SetControls();
	RecalcSliderSize();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayStereo::SetMask()
{
	GetIEditor()->GetDisplaySettings()->SetObjectHideMask( m_mask );
	GetIEditor()->GetObjectManager()->InvalidateVisibleList();
	GetIEditor()->UpdateViews( eUpdateObjects );
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayStereo::OnHideAll() 
{
	m_mask = 0xFFFFFFFF;
	SetCheckButtons();
	SetMask();
}

void CPanelDisplayStereo::OnHideNone() 
{
	m_mask = 0;
	SetCheckButtons();
	SetMask();
}

void CPanelDisplayStereo::OnHideInvert() 
{
	m_mask = ~m_mask;
	SetCheckButtons();
	SetMask();
}

void CPanelDisplayStereo::SetCheckButtons()
{
	// Check or uncheck buttons.	
}

void CPanelDisplayStereo::OnChangeHideMask() 
{
	// TODO: Add your control notification handler code here
	m_mask = 0;
	
	// Check or uncheck buttons.	
	SetCheckButtons();

	SetMask();
}

void CPanelDisplayStereo::OnCbnSelchangeStereoModeCombo()
{
	CString text;
	m_stereoModeCombo.GetWindowText(text);
	int curSel = m_stereoModeCombo.GetCurSel();

	ICVar		*piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoMode");
	if (piVariable)
	{
		piVariable->Set(curSel);
	}
}
void CPanelDisplayStereo::OnCbnSelchangeStereoOutputCombo()
{
	CString text;
	m_stereoOutputCombo.GetWindowText(text);
	int curSel = m_stereoOutputCombo.GetCurSel();

	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoOutput");
	if (piVariable)
	{
		piVariable->Set(curSel);
	}
}

void CPanelDisplayStereo::OnBnClickedDontFlipRadio()
{
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoFlipEyes");
	if (piVariable)
	{
		piVariable->Set(0);
	}
}

void CPanelDisplayStereo::OnBnClickedFlipRadio()
{
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoFlipEyes");
	if (piVariable)
	{
		piVariable->Set(1);
	}
}

void CPanelDisplayStereo::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	SyncSliderValueToEditControl();
	ApplySliderControlToDistValue();
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPanelDisplayStereo::SetControls()
{
	ICVar		*piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoMode");
	if (piVariable)
	{
		m_stereoModeCombo.SetCurSel(piVariable->GetIVal());
	}
	piVariable=gEnv->pConsole->GetCVar("r_StereoOutput");
	if (piVariable)
	{
		m_stereoOutputCombo.SetCurSel(piVariable->GetIVal());
	}

	ApplyDistValueToSlider();
	ApplyDistValueToEdit();
	piVariable=gEnv->pConsole->GetCVar("r_StereoFlipEyes");
	if (piVariable)
	{
		if (0 == piVariable->GetIVal())
		{
			m_dontFlip.SetCheck(1);
			m_flip.SetCheck(0);
		}
		else
		{
			m_dontFlip.SetCheck(0);
			m_flip.SetCheck(1);
		}
	}
}

void CPanelDisplayStereo::SyncSliderValueToEditControl()
{
	int posValue = m_eyeDistSlider.GetPos();

	CString valueStr;
	valueStr.Format( "%d cm", posValue );
	m_eyeDistEdit.SetWindowText(valueStr);

	posValue = m_screenDistSlider.GetPos();
	valueStr.Format("%d cm", posValue);
	m_screenDistEdit.SetWindowText(valueStr);
}

void CPanelDisplayStereo::ApplyDistValueToSlider()
{
	CRect sliderRect;
	ICVar		*piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoEyeDist");
	if (piVariable)
	{
		m_eyeDistSlider.SetPos(ConvertFloatValueToSliderPos(piVariable->GetFVal()));
		m_eyeDistSlider.GetClientRect(sliderRect);
		m_eyeDistSlider.InvalidateRect(sliderRect);
	}
	piVariable=gEnv->pConsole->GetCVar("r_StereoScreenDist");
	if (piVariable)
	{
		m_screenDistSlider.SetPos(ConvertFloatValueToSliderPos(piVariable->GetFVal()));
		m_screenDistSlider.GetClientRect(sliderRect);
		m_screenDistSlider.InvalidateRect(sliderRect);
	}
}

void CPanelDisplayStereo::ApplyDistValueToEdit()
{
	CString valueStr;	
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoEyeDist");

	if (piVariable)
	{
		valueStr.Format("%.2f cm", piVariable->GetFVal());		
		m_eyeDistEdit.SetWindowText(valueStr);
	}
	piVariable=gEnv->pConsole->GetCVar("r_StereoScreenDist");
	if (piVariable)
	{
		valueStr.Format("%.2f cm", piVariable->GetFVal());
		m_screenDistEdit.SetWindowText(valueStr);
	}
}

void CPanelDisplayStereo::ApplySliderControlToDistValue()
{
	int posValue = m_eyeDistSlider.GetPos();
	float fValue = ConvertSliderPosToFloatValue(posValue);
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoEyeDist");
	if (piVariable)
	{
		piVariable->Set(fValue);
	}
	posValue = m_screenDistSlider.GetPos();
	fValue = ConvertSliderPosToFloatValue(posValue);
	piVariable=gEnv->pConsole->GetCVar("r_StereoScreenDist");
	if (piVariable)
	{
		piVariable->Set(fValue);
	}
}

void CPanelDisplayStereo::ApplyEditControlToDistValue()
{
	CString editText;
	m_eyeDistEdit.GetWindowText(editText);
	float fValue = atof(editText.GetString());
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoEyeDist");
	if (piVariable)
	{
		piVariable->Set(fValue);
	}

	m_screenDistEdit.GetWindowText(editText);
	fValue = atof(editText.GetString());
	piVariable=gEnv->pConsole->GetCVar("r_StereoScreenDist");
	if (piVariable)
	{
		piVariable->Set(fValue);
	}
}

void CPanelDisplayStereo::RecalcSliderSize()
{
	if (m_eyeDistSlider.GetSafeHwnd())
	{
		CRect dlgRc;
		GetClientRect(dlgRc);
		CRect sliderRect;
		m_eyeDistSlider.GetWindowRect(sliderRect);
		ScreenToClient(sliderRect);
		m_eyeDistSlider.MoveWindow(
			CRect(dlgRc.left,sliderRect.top,dlgRc.right,sliderRect.bottom),
			FALSE);
	}

	if (m_screenDistSlider.GetSafeHwnd())
	{
		CRect dlgRc;
		GetClientRect(dlgRc);
		CRect sliderRect;
		m_screenDistSlider.GetWindowRect(sliderRect);
		ScreenToClient(sliderRect);
		m_screenDistSlider.MoveWindow(
			CRect(dlgRc.left,sliderRect.top,dlgRc.right,sliderRect.bottom),
			FALSE);
	}
}

void CPanelDisplayStereo::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	RecalcSliderSize();
}

void CPanelDisplayStereo::RegisterChangeCallback(const char* szVariableName,ConsoleVarFunc fnCallbackFunction)
{
	ICVar		*piVariable(NULL);

	piVariable=gEnv->pConsole->GetCVar(szVariableName);
	if (!piVariable)
	{
		return;
	}

	m_cVariableNameToConsoleFunction[(string)szVariableName]=piVariable->GetOnChangeCallback();
	piVariable->SetOnChangeCallback(fnCallbackFunction);
}

CPanelDisplayStereo*& CPanelDisplayStereo::GetCurrentDisplayStereo()
{
	static CPanelDisplayStereo* poCurrentDisplayStereo(NULL);
	return poCurrentDisplayStereo;
}

void CPanelDisplayStereo::OnDisplayOptionChanged(ICVar* piDisplayModeVariable)
{
	CPanelDisplayStereo* poDisplayMode=CPanelDisplayStereo::GetCurrentDisplayStereo();
	if (!poDisplayMode)
	{
		return;
	}
	poDisplayMode->OnDisplayOptionChanged();

	TDVariableNameToConsoleFunction::iterator		itIterator;
	itIterator=poDisplayMode->m_cVariableNameToConsoleFunction.find(piDisplayModeVariable->GetName());
	if (itIterator!=poDisplayMode->m_cVariableNameToConsoleFunction.end())
	{
		if (itIterator->second)
		{
			itIterator->second(piDisplayModeVariable);
		}
	}
}

void CPanelDisplayStereo::OnDisplayOptionChanged()
{
	SetControls();
}

void CPanelDisplayStereo::SetupCallbacks()
{
	RegisterChangeCallback("r_StereoMode",&CPanelDisplayStereo::OnDisplayOptionChanged);
	RegisterChangeCallback("r_StereoOutput",&CPanelDisplayStereo::OnDisplayOptionChanged);
	RegisterChangeCallback("r_StereoEyeDist",&CPanelDisplayStereo::OnDisplayOptionChanged);
	RegisterChangeCallback("r_StereoScreenDist",&CPanelDisplayStereo::OnDisplayOptionChanged);
	RegisterChangeCallback("r_StereoFlipEyes",&CPanelDisplayStereo::OnDisplayOptionChanged);
}

void CPanelDisplayStereo::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnDisplayRenderUpdate:
		SetControls();
		break;
	}
}

void CPanelDisplayStereo::OnEnKillfocusEyeDistEdit()
{
	ApplyEditControlToDistValue();
}

void CPanelDisplayStereo::OnEnKillfocusScreenDistEdit()
{
	ApplyEditControlToDistValue();
}

BOOL CPanelDisplayStereo::PreTranslateMessage( MSG* pMsg )
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->wParam == VK_RETURN)
	{
		bool eyeDistEditHasFocus = (pMsg->hwnd == m_eyeDistEdit.GetSafeHwnd());
		bool screenDistEditHasFocus = (pMsg->hwnd == m_screenDistEdit.GetSafeHwnd());
		if (eyeDistEditHasFocus || screenDistEditHasFocus)
		{
			ApplyEditControlToDistValue();
			return TRUE;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}
