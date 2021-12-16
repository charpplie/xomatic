// PanelDisplatHide.cpp : implementation file
//

#include "StdAfx.h"
#include "PanelDisplayStereo.h"
#include "DisplaySettings.h"
#include "Console/ConsoleSync.h"

const float MinEyeDistValue			= 0.0f;
const float MaxEyeDistValue			= 1.0f;
const float MinScreenDistValue		= 0.1f;
const float MaxScreenDistValue		= 1000.0f;
const float ScreenDistStepValue		= 0.01f;
const float EyeDistStepValue		= 0.0001f;
const float kEyeDistMultiplier		= 0.0001f;
const float kScreenDistMultiplier	= 0.01f;

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
	DDX_Control(pDX, IDC_STEREO_FLIP_COMBO, m_flipCombo);
	DDX_Control(pDX, IDC_CHECK_PC, m_enablePC);
	DDX_Control(pDX, IDC_CHECK_CONSOLE, m_enableConsole);
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
	ON_CBN_SELCHANGE(IDC_STEREO_FLIP_COMBO, &CPanelDisplayStereo::OnCbnSelChangedFlipCheck)
	ON_BN_CLICKED(IDC_CHECK_PC, &CPanelDisplayStereo::OnBnClickedPCCheck)
	ON_BN_CLICKED(IDC_CHECK_CONSOLE, &CPanelDisplayStereo::OnBnClickedConsoleCheck)
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_STEREO_SCREENDIST_EDIT, OnNumber_ScreenDist)
	ON_EN_CHANGE(IDC_STEREO_EYEDIST_EDIT, OnNumber_EyeDist)
	ON_EN_UPDATE(IDC_STEREO_SCREENDIST_EDIT, OnNumber_ScreenDist)
	ON_EN_UPDATE(IDC_STEREO_EYEDIST_EDIT, OnNumber_EyeDist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayStereo message handlers

BOOL CPanelDisplayStereo::OnInitDialog() 
{
	CDialog::OnInitDialog();

	ICVar		*piVariable = gEnv->pConsole->GetCVar("r_StereoMode");
	if (piVariable)
	{
		m_nOldPCStereoMode = piVariable->GetIVal() != 0 ? piVariable->GetIVal() : 2;
		m_enablePC.SetCheck(piVariable->GetIVal() != 0);
	}

	m_mask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
	
	m_eyeDistEdit.Create(this,IDC_STEREO_EYEDIST_EDIT,0);
	m_screenDistEdit.Create(this,IDC_STEREO_SCREENDIST_EDIT,0);

	m_screenDistEdit.SetRange(MinScreenDistValue,MaxScreenDistValue);
	m_screenDistEdit.SetStep(kScreenDistMultiplier);
	m_eyeDistEdit.SetRange(MinEyeDistValue,MaxEyeDistValue);
	m_eyeDistEdit.SetStep(kEyeDistMultiplier);


	SetCheckButtons();
	
	SetControls();
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
	int curSel = m_stereoModeCombo.GetCurSel() + 1;

	ICVar		*piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoMode");
	if (piVariable)
	{
		piVariable->Set(curSel);
		m_nOldPCStereoMode = curSel;
		int stereoConsoleEnabled = m_enableConsole.GetCheck();
		SendCvarToConsole("r_stereoMode",stereoConsoleEnabled == 0 ? 0 : m_nOldPCStereoMode);
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

void CPanelDisplayStereo::OnCbnSelChangedFlipCheck()
{
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoFlipEyes");
	if (piVariable)
	{
		int curSel = m_flipCombo.GetCurSel();
		piVariable->Set(curSel);
	}
}

void CPanelDisplayStereo::OnBnClickedPCCheck()
{
	ICVar *pStereoMode = gEnv->pSystem->GetIConsole()->GetCVar("r_StereoMode");
	if(pStereoMode)
	{
		m_nOldPCStereoMode = m_nOldPCStereoMode ? m_nOldPCStereoMode : 1;
		pStereoMode->Set(pStereoMode->GetIVal() == 0 ? m_nOldPCStereoMode : 0);
		int stereoConsoleEnabled = m_enableConsole.GetCheck();
		SendCvarToConsole("r_stereoMode",stereoConsoleEnabled == 0 ? 0 : m_nOldPCStereoMode);
	}
}

void CPanelDisplayStereo::OnBnClickedConsoleCheck()
{
	int stereoEnabled = m_enableConsole.GetCheck();
	SendCvarToConsole("r_stereoMode",stereoEnabled == 0 ? 0 : m_nOldPCStereoMode);
}

void CPanelDisplayStereo::SendCvarToConsole(CString sVarName, float sVarValue)
{
	CConsoleSynchronization::SSyncMessage poSyncMessage;
	poSyncMessage.node=XmlHelpers::CreateXmlNode("SyncMessage");
	poSyncMessage.node->setAttr("Type","ConsoleVariables");
	poSyncMessage.node->setAttr(sVarName,sVarValue);

	GetIEditor()->GetConsoleSync()->SendMessage( poSyncMessage );
}

void CPanelDisplayStereo::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPanelDisplayStereo::SetControls()
{
	ICVar		*piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoMode");
	if (piVariable)
	{
		m_enablePC.SetCheck(piVariable->GetIVal() != 0);
		m_stereoModeCombo.SetCurSel(piVariable->GetIVal() == 0 ? m_nOldPCStereoMode-1 : piVariable->GetIVal()-1);
	}
	piVariable=gEnv->pConsole->GetCVar("r_StereoOutput");
	if (piVariable)
	{
		m_stereoOutputCombo.SetCurSel(piVariable->GetIVal());
	}

	ApplyDistValueToEdit();
	piVariable=gEnv->pConsole->GetCVar("r_StereoFlipEyes");
	if (piVariable)
	{
		if (0 == piVariable->GetIVal())
		{
			m_flipCombo.SetCurSel(0);
		}
		else
		{
			m_flipCombo.SetCurSel(1);
		}
	}
}

void CPanelDisplayStereo::ApplyDistValueToEdit()
{
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoEyeDist");

	if (piVariable)
	{
		m_eyeDistEdit.SetValue(piVariable->GetFVal());
	}
	piVariable=gEnv->pConsole->GetCVar("r_StereoScreenDist");
	if (piVariable)
	{
		m_screenDistEdit.SetValue(piVariable->GetFVal());
	}
}

void CPanelDisplayStereo::ApplyEditControlToDistValue()
{
	CString editText;
	float fValue = m_eyeDistEdit.GetValue();
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoEyeDist");
	if (piVariable)
	{
		piVariable->Set(fValue);
	}

	fValue = m_screenDistEdit.GetValue();
	piVariable=gEnv->pConsole->GetCVar("r_StereoScreenDist");
	if (piVariable)
	{ 
		piVariable->Set(fValue);
	}
}

void CPanelDisplayStereo::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
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

void CPanelDisplayStereo::OnNumber_EyeDist()
{
	float fValue = m_eyeDistEdit.GetValue();
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoEyeDist");
	if (piVariable)
	{
		piVariable->Set(fValue);
	}
}

void CPanelDisplayStereo::OnNumber_ScreenDist()
{
	float fValue = m_screenDistEdit.GetValue();
	ICVar *piVariable(NULL);
	piVariable=gEnv->pConsole->GetCVar("r_StereoScreenDist");
	if (piVariable)
	{
		piVariable->Set(fValue);
	}
}
