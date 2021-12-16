// CharacterEditor.cpp : implementation file
//

#include "stdafx.h"

#include <I3DEngine.h>
#include "ICryAnimation.h"
#include <IRenderer.h>
#include "IRenderAuxGeom.h"
#include <Cderr.h>
//#include "IRendererD3D9.h"

#include "..\CryEdit.h"
#include "CharacterEditor.h"
#include "..\PropertiesPanel.h"
#include "..\ThumbnailGenerator.h"
#include "..\FileTypeUtils.h"

#include "IViewPane.h"
#include "..\ModelViewport.h"
#include "ModelViewportCE.h"
#include "StringUtils.h"
#include "CharPanel_Preset.h"


#define TAB_ATTACHMENTS 0
#define IDW_ROLLUP_PANE      AFX_IDW_CONTROLBAR_FIRST+10
#define IDW_ANIMATIONS_PANE  AFX_IDW_CONTROLBAR_FIRST+12
#define IDW_PARTS_PANE       AFX_IDW_CONTROLBAR_FIRST+13
#define IDW_MOTION_ADAPTOR_PANE   AFX_IDW_CONTROLBAR_FIRST+14
#define IDW_SET_MATERIAL_PANE AFX_IDW_CONTROLBAR_FIRST+15
#define IDW_SHAPE_DEFORMATION_PANE AFX_IDW_CONTROLBAR_FIRST+16
#define IDW_ANIMATION_CONTROL_PANE AFX_IDW_CONTROLBAR_FIRST+17
#define IDW_ATTACHMENTS_PANE AFX_IDW_CONTROLBAR_FIRST+18

#define IDW_FIRST_EXTENSION_PANE       AFX_IDW_CONTROLBAR_FIRST+20
#define IDW_LAST_EXTENSION_PANE       AFX_IDW_CONTROLBAR_FIRST+1000
namespace
{
	const int CharacterEditorLayoutVersion = 0x0001; // bump this up on every substantial pane layout change
}

//////////////////////////////////////////////////////////////////////////
class CCharacterEditorViewClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {D21C9FE5-22D3-41e3-B84B-A377AFA0A05C}
		static const GUID guid =	{ 0xd21c9fe5, 0x22d3, 0x41e3, { 0xb8, 0x4b, 0xa3, 0x77, 0xaf, 0xa0, 0xa0, 0x5c } };
		return guid;
	}
	virtual const char* ClassName() { return "Character Editor"; };
	virtual const char* Category() { return "Character Editor"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CCharacterEditor); };
	virtual const char* GetPaneTitle() { return _T("Character Editor"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(200,200,1300,1300); };
	virtual bool SinglePane() { return false; };
	virtual bool WantIdleUpdate() { return true; };
};

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CCharacterEditorViewClass );
}

IMPLEMENT_DYNCREATE(CCharacterEditor, CBaseFrameWnd)


//////////////////////////////////////////////////////////////////////////
CCharacterEditor::CCharacterEditor()
{
	m_pModelViewportCE = 0;
	m_pResizePane = NULL;
	m_strCharDefPathName="NoPathDefined";
	m_pCharPanel_Preset = 0;
	m_pWndMenuBar = 0;
	m_pMotionAdaptorDlg = NULL;
	m_strCharDefPathName = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_CharEditModel")->GetString( );
	m_currExtension ="NoExtension";

	GetIEditor()->RegisterNotifyListener(this);

	Create( WS_CHILD|WS_VISIBLE,CRect(0,0,0,0),AfxGetMainWnd() );
}

//////////////////////////////////////////////////////////////////////////
CCharacterEditor::~CCharacterEditor()
{
	GetIEditor()->UnregisterNotifyListener(this);

	delete m_pResizePane;
	//if(m_pCharPanel_Preset)
		//delete m_pCharPanel_Preset;
	m_pCharPanel_Preset = 0;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CCharacterEditor::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;    
		if (!pwndDockWindow->IsValid())
		{
			int id = pwndDockWindow->GetID();
			if (id >= IDW_FIRST_EXTENSION_PANE && id <= IDW_LAST_EXTENSION_PANE)
			{
				for (int i = 0; i < m_frameExtensions.size(); i++)
				{
					m_frameExtensions[i].pIFrameExtension->OnAttachToDockingPane( pwndDockWindow,m_frameExtensions[i].pPaneWnd );
				}
				return TRUE;
			}
			switch (id)
			{
			case IDW_ROLLUP_PANE:
				pwndDockWindow->Attach(&m_rollupCtrl);
				break;
			case IDW_ANIMATIONS_PANE:
				pwndDockWindow->Attach(&m_animationBrowser);
				break;
			case IDW_MOTION_ADAPTOR_PANE:
				if (m_pMotionAdaptorDlg){
					pwndDockWindow->Attach(m_pMotionAdaptorDlg);
					m_pMotionAdaptorDlg->m_pParentPane = pwndDockWindow;
				}
				break;
			case IDW_SET_MATERIAL_PANE:
				if(m_CharacterPropertiesDlg)
					pwndDockWindow->Attach(&m_CharacterPropertiesDlg);
				break;
			case IDW_SHAPE_DEFORMATION_PANE:
				if(m_CharacterPropertiesDlg)
					pwndDockWindow->Attach(&m_MorphingDlg);
				break;
			case IDW_ANIMATION_CONTROL_PANE:
				if(m_CharacterPropertiesDlg)
					pwndDockWindow->Attach(&m_AnimationControlDlg);
				break;
			case IDW_ATTACHMENTS_PANE:
				if(m_CharacterPropertiesDlg)
					pwndDockWindow->Attach(&m_AttachmentsDlg);
				break;

			default:
				return FALSE;
			}
		}
		return TRUE;
	}
	else if (wParam == XTP_DPN_CLOSEPANE)
	{
		// get a pointer to the docking pane being closed.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (pwndDockWindow->IsValid())
		{
		}
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX,IDC_PANE1,m_pane1);
	DDX_Control(pDX,IDC_PANE2,m_pane2);
}


BEGIN_MESSAGE_MAP(CCharacterEditor, CBaseFrameWnd)
	ON_COMMAND(ID_FILE_NEW,  OnFileNew)
	ON_COMMAND(ID_FILE_LOAD, OnFileLoad)
	ON_COMMAND(ID_FILE_SMARTOPEN, OnFileSmartOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateUIFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateUIFileSaveAs)

	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedExit)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedBrowseFile)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_COMMAND(ID_CHAREDIT_MOVE, OnMoveMode)
	ON_UPDATE_COMMAND_UI(ID_CHAREDIT_MOVE, OnMoveModeUpdateUI)

	ON_COMMAND(ID_CHAREDIT_ROTATE, OnRotateMode)
	ON_UPDATE_COMMAND_UI(ID_CHAREDIT_ROTATE, OnRotateModeUpdateUI)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMATIONS, &CCharacterEditor::OnUpdateViewAnimations)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CHARACTERPARTS, &CCharacterEditor::OnUpdateViewCharacterparts)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MOTIONADAPTOR, &CCharacterEditor::OnUpdateViewMotionadaptor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SETMATERIAL, &CCharacterEditor::OnUpdateViewSetMaterial)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHAPEDEFORMATION, &CCharacterEditor::OnUpdateViewShapeDeformation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMATIONCONTROL, &CCharacterEditor::OnUpdateViewAnimationControl)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ATTACHMENTS, &CCharacterEditor::OnUpdateViewAttachments)

	ON_COMMAND(ID_VIEW_ANIMATIONS, &CCharacterEditor::OnViewAnimations)
	ON_COMMAND(ID_VIEW_CHARACTERPARTS, &CCharacterEditor::OnViewCharacterparts)
	ON_COMMAND(ID_VIEW_ROLLUP, &CCharacterEditor::OnViewRollup)
	ON_COMMAND(ID_VIEW_MOTIONADAPTOR, &CCharacterEditor::OnViewMotionadaptor)
	ON_COMMAND(ID_VIEW_SETMATERIAL, &CCharacterEditor::OnViewSetMaterial)
	ON_COMMAND(ID_VIEW_SHAPEDEFORMATION, &CCharacterEditor::OnViewShapeDeformation)
	ON_COMMAND(ID_VIEW_ANIMATIONCONTROL, &CCharacterEditor::OnViewAnimationControl)
	ON_COMMAND(ID_VIEW_ATTACHMENTS, &CCharacterEditor::OnViewAttachments)

END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////



BOOL CCharacterEditor::OnInitDialog()
{
	__super::OnInitDialog();
	
	// Create and setup the heightmap edit viewport and the toolbars
	GetCommandBars()->GetCommandBarsOptions()->bShowExpandButtonAlways = FALSE;
	GetCommandBars()->EnableCustomization(FALSE);

	CRect rc;
	GetClientRect(rc);

	// Create Menu
	CXTPCommandBar* pMenuBar = GetCommandBars()->SetMenu( _T("Menu Bar"),IDD_CHARACTER_EDITOR );	
	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(FALSE);

	// Create toolbar.
	CXTPToolBar *pToolBar1 = GetCommandBars()->Add( _T("ToolBar1"),xtpBarTop );
	pToolBar1->EnableCustomization(FALSE);
	VERIFY(pToolBar1->LoadToolBar( IDR_CHARACTER_EDITOR ));

//Diesel cut	
	//CXTPDockingPane* pDockPane_Attachments = GetDockingPaneManager()->CreatePane(IDW_ATTACHMENTS_PANE, CRect(0, 0, 190, 500), dockBottomOf);
//Diesel new	
	CXTPDockingPane* pDockPane_Attachments = GetDockingPaneManager()->CreatePane(IDW_ATTACHMENTS_PANE, CRect(0, 0, 190, 500), xtpPaneDockBottom);
	
	pDockPane_Attachments->SetTitle("Attachments");

//Diesel cut	
	//CXTPDockingPane* pDockPane_SetMaterial = GetDockingPaneManager()->CreatePane(IDW_SET_MATERIAL_PANE, CRect(0, 0, 190, 500), dockBottomOf, pDockPane_Attachments);
//Diesel new	
	CXTPDockingPane* pDockPane_SetMaterial = GetDockingPaneManager()->CreatePane(IDW_SET_MATERIAL_PANE, CRect(0, 0, 190, 500), xtpPaneDockBottom, pDockPane_Attachments);
	
	
	pDockPane_SetMaterial->SetTitle("Set Material");

//Diesel cut
	//CXTPDockingPane* pDockPane_ShapeDeformation = GetDockingPaneManager()->CreatePane(IDW_SHAPE_DEFORMATION_PANE, CRect(0, 0, 190, 500), dockBottomOf, pDockPane_Attachments);
//Diesel new	
	CXTPDockingPane* pDockPane_ShapeDeformation = GetDockingPaneManager()->CreatePane(IDW_SHAPE_DEFORMATION_PANE, CRect(0, 0, 190, 500), xtpPaneDockBottom, pDockPane_Attachments);
	
	
	pDockPane_ShapeDeformation->SetTitle("Shape Deformation");

//Diesel cut
	//CXTPDockingPane* pDockPane_AnimationControl = GetDockingPaneManager()->CreatePane(IDW_ANIMATION_CONTROL_PANE, CRect(0, 0, 190, 500), dockBottomOf, pDockPane_Attachments);
//Diesel new	
	CXTPDockingPane* pDockPane_AnimationControl = GetDockingPaneManager()->CreatePane(IDW_ANIMATION_CONTROL_PANE, CRect(0, 0, 190, 500), xtpPaneDockBottom, pDockPane_Attachments);
	
	
	pDockPane_AnimationControl->SetTitle("Animation Control");

	GetDockingPaneManager()->AttachPane(pDockPane_SetMaterial, pDockPane_Attachments);
	GetDockingPaneManager()->AttachPane(pDockPane_AnimationControl, pDockPane_Attachments);
	GetDockingPaneManager()->AttachPane(pDockPane_ShapeDeformation, pDockPane_Attachments);

	pDockPane_Attachments->Select();

//Diesel cut
	//CXTPDockingPane* pDockPane_Rollup = GetDockingPaneManager()->CreatePane( IDW_ROLLUP_PANE, CRect(0,0,270,100), dockRightOf );
//Diesel new	
	CXTPDockingPane* pDockPane_Rollup = GetDockingPaneManager()->CreatePane( IDW_ROLLUP_PANE, CRect(0,0,270,100), xtpPaneDockRight );
	
	
	pDockPane_Rollup->SetTitle( "Rollup" );
	//pDockPane_Rollup->SetOptions(xtpPaneNoCloseable|xtpPaneNoFloatable);

	// Animation browser
	
//Diesel cut	
	//CXTPDockingPane* pDockPane_Animations = GetDockingPaneManager()->CreatePane( IDW_ANIMATIONS_PANE, CRect(0,0,270,400), dockLeftOf);
//Diesel new	
	CXTPDockingPane* pDockPane_Animations = GetDockingPaneManager()->CreatePane( IDW_ANIMATIONS_PANE, CRect(0,0,270,400), xtpPaneDockLeft);
	
	
	pDockPane_Animations->SetTitle( "Animations" );

//Diesel cut
	//CXTPDockingPane* pDockPane_MotionAdaptor = GetDockingPaneManager()->CreatePane(IDW_MOTION_ADAPTOR_PANE, CRect(0, 0, 280, 500), dockLeftOf);
//Diesel new	
	CXTPDockingPane* pDockPane_MotionAdaptor = GetDockingPaneManager()->CreatePane(IDW_MOTION_ADAPTOR_PANE, CRect(0, 0, 280, 500), xtpPaneDockLeft);
	

	pDockPane_MotionAdaptor->SetTitle("Motion Adaptor");

	
	
	//Close motion adaptor by default
	pDockPane_MotionAdaptor->Close();

	InitializeCaptionBar();

	// Create ModelViewport
	m_pModelViewportCE = new CModelViewportCE;
	m_pModelViewportCE->SetDlgCtrlID( m_wndSplitterCaption.IdFromRowCol(1,0) );
	m_pModelViewportCE->MoveWindow( CRect(20,50,500,500) );
	m_pModelViewportCE->ModifyStyle( WS_POPUP,WS_CHILD,0 );
	m_pModelViewportCE->SetParent( &m_wndSplitterCaption );
	m_pModelViewportCE->SetOwner( this );

	m_pModelViewportCE->SetType( ET_ViewportModel );
	//m_pModelViewportCE->Create( &m_pane1,ET_ViewportModel,"Model Preview" );
	m_pModelViewportCE->MoveWindow(rc);
	m_pModelViewportCE->ShowWindow(SW_SHOW);
	m_pModelViewportCE->SetCharacterChangeListener(this);

	// Create animations panel.
	m_animationBrowser.Create(CAnimationBrowser::IDD,this);
	m_animationBrowser.ShowWindow(SW_SHOW);
	m_animationBrowser.SetCharacterEditor(this);
	m_animationBrowser.SetModelViewportCE(m_pModelViewportCE);
	m_pModelViewportCE->SetAnimBrowserPanel(&m_animationBrowser);

	m_rollupCtrl.Create( WS_CHILD|WS_VISIBLE,CRect(4, 4, 187, 362),this, NULL);

	int idx;

	//add page for "Character Animation"
	//Pass this pointer as parent pointer so we can access pAnimationPanel
	CharPanel_Animation* pAnimationPanel = new CharPanel_Animation(m_pModelViewportCE,this);
	m_pModelViewportCE->SetModelPanelA(pAnimationPanel);
	idx=m_rollupCtrl.InsertPage( "Advanced Animation Settings",pAnimationPanel );
	m_rollupCtrl.ExpandPage(idx,1);



	//add page for Preset
	m_pCharPanel_Preset = new CharPanel_Preset(m_pModelViewportCE,NULL);
	m_pModelViewportCE->SetCharPanelPreset(m_pCharPanel_Preset);
	idx=m_rollupCtrl.InsertPage( "Preset", m_pCharPanel_Preset);
	m_rollupCtrl.ExpandPage(idx,0);


	//add page for "Bone Attachment"
	/*	m_pModelViewportCE->SetModelPanelW( new CharPanel_BAttach(m_pModelViewportCE,NULL) );
	idx=m_rollupCtrl.InsertPage( "Bone Attachment",m_pModelViewportCE->GetModelPanelW()  );
	m_rollupCtrl.ExpandPage(idx,0);

	//add page for "Face Attachment"
	m_pModelViewportCE->SetModelPanelF( new CharPanel_FAttach(m_pModelViewportCE,NULL) );
	idx=m_rollupCtrl.InsertPage( "Face Attachment",m_pModelViewportCE->GetModelPanelF()  );
	m_rollupCtrl.ExpandPage(idx,0);*/

	//add page for "Debug Options"
	s_varsPanel = new CPropertiesPanel(this);
	s_varsPanel->AddVars( m_pModelViewportCE->GetVarObject()->GetVarBlock() );
	idx=m_rollupCtrl.InsertPage( "Debug Options",s_varsPanel);
	m_rollupCtrl.ExpandPage(idx,0);

	//------------------------------------------------------------------------------
	// Read user's options if possible
	ReadUserOptions();
	
	//scroll to the top
	m_rollupCtrl.ScrollToPage(0);

	// Make tab panels.
	m_AttachmentsDlg.Create(CAttachmentsDlg::IDD,this);
	m_AttachmentsDlg.m_ButtonCLEAR.EnableWindow(FALSE);
	m_AttachmentsDlg.m_ButtonRENAME.EnableWindow(FALSE);
	m_AttachmentsDlg.m_ButtonREMOVE.EnableWindow(FALSE);
	m_AttachmentsDlg.m_ButtonEXPORT.EnableWindow(FALSE);
	m_AttachmentsDlg.m_pModelViewportCE = m_pModelViewportCE;

	m_AnimationControlDlg.Create(CAnimationControlDlg::IDD, this);
	m_AnimationControlDlg.m_pModelViewportCE = m_pModelViewportCE;

	m_CharacterPropertiesDlg.Create(CCharacterPropertiesDlg::IDD, this);
	m_CharacterPropertiesDlg.SetCharacterChangeListener(this);
	m_CharacterPropertiesDlg.m_pModelViewportCE = m_pModelViewportCE;

	

	m_MorphingDlg.Create(CMorphingDlg::IDD,this);
	m_MorphingDlg.m_button[0].EnableWindow(FALSE);
	m_MorphingDlg.m_button[1].EnableWindow(FALSE);
	m_MorphingDlg.m_button[2].EnableWindow(FALSE);
	m_MorphingDlg.m_button[3].EnableWindow(FALSE);
	m_MorphingDlg.m_button[4].EnableWindow(FALSE);
	m_MorphingDlg.m_button[5].EnableWindow(FALSE);
	m_MorphingDlg.m_button[6].EnableWindow(FALSE);
	m_MorphingDlg.m_button[7].EnableWindow(FALSE);

	m_MorphingDlg.m_button[0].SetCheck(FALSE);
	m_MorphingDlg.m_button[1].SetCheck(FALSE);
	m_MorphingDlg.m_button[2].SetCheck(FALSE);
	m_MorphingDlg.m_button[3].SetCheck(FALSE);
	m_MorphingDlg.m_button[4].SetCheck(FALSE);
	m_MorphingDlg.m_button[5].SetCheck(FALSE);
	m_MorphingDlg.m_button[6].SetCheck(FALSE);
	m_MorphingDlg.m_button[7].SetCheck(FALSE);

	m_MorphingDlg.m_button[0].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[0].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[1].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[1].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[2].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[2].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[3].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[3].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[4].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[4].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[5].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[5].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[6].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[6].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[7].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[7].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));


	m_MorphingDlg.m_pModelViewportCE=m_pModelViewportCE;

	
	m_pModelViewportCE->m_pCharacterEditor = this;
	m_pModelViewportCE->m_pAttachmentsDlg = &m_AttachmentsDlg;
	m_pModelViewportCE->m_pMorphingDlg = &m_MorphingDlg;
	m_pModelViewportCE->m_pAnimationControlDlg = &m_AnimationControlDlg;
	m_pModelViewportCE->m_pCharacterPropertiesDlg = &m_CharacterPropertiesDlg;
	


//	m_pModelViewportCE->m_FixedCamInViewport=0;
//	pCvar = gEnv->pConsole->GetCVar("ca_FixedCamInViewport");
//	if(pCvar && pCvar->GetIVal()==1)
//		m_pModelViewportCE->m_FixedCamInViewport=1;
//	GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_DrawBodyMoveDir")->Set( 1 );

	const char* pLoadName=GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_CharEditModel")->GetString( );
	if( pLoadName[0] )
	{
		LoadCharacter(pLoadName);
		m_currExtension = PathUtil::GetExt(pLoadName);
		m_strCharDefPathName = string(pLoadName);
	}

	m_AnimationControlDlg.ModelChanged();

	InitFrameExtensions();

	// Create Motion Adaptor dialog.
	m_pMotionAdaptorDlg = new CMotionAdaptorDlg(this);
	m_pMotionAdaptorDlg->m_pModelViewportCE = m_pModelViewportCE;
	m_pMotionAdaptorDlg->m_pCharacterEditor = this;
	m_pMotionAdaptorDlg->m_dlgMain.SetModelViewportCE(m_pModelViewportCE);
	m_pMotionAdaptorDlg->m_dlgMain.SetCharacterEditor(this);
	m_pModelViewportCE->m_pMotionAdaptorDlg = m_pMotionAdaptorDlg;

	ReadLayouts();
	
	return TRUE;
}

void CCharacterEditor::SaveLayouts()
{
	CXTPDockingPaneLayout layout(GetDockingPaneManager());
	GetDockingPaneManager()->GetLayout( &layout );
	layout.Save(_T("CharacterEditorLayout"));
}
void CCharacterEditor::ReadLayouts()
{
	CXTRegistryManager regMgr;
	int paneLayoutVersion = regMgr.GetProfileInt(_T("DockingPaneLayouts\\CharacterEditor"), _T("CharacterEditorLayoutVersion"), 0);
	if (paneLayoutVersion == CharacterEditorLayoutVersion)
	{
		CXTPDockingPaneLayout layout(GetDockingPaneManager());
		if (layout.Load(_T("CharacterEditorLayout"))) 
		{
			if (layout.GetPaneList().GetCount() > 0)
			{
				GetDockingPaneManager()->SetLayout(&layout);	
			}
		}
	}
	else
	{
		regMgr.WriteProfileInt(_T("DockingPaneLayouts\\CharacterEditor"), _T("CharacterEditorLayoutVersion"), CharacterEditorLayoutVersion);
	}

}
//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::InitializeCaptionBar()
{
	m_wndSplitterCaption.CreateStatic(this, 2, 1,WS_CHILD|WS_VISIBLE,AFX_IDW_PANE_FIRST);
	m_wndSplitterCaption.ModifyStyle(0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
	m_wndSplitterCaption.SetSplitterStyle(XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOSIZE | XT_SPLIT_NOBORDER);

	m_wndCaption.Create(&m_wndSplitterCaption, _T("No Character Loaded"), NULL,WS_VISIBLE|SS_CENTER|SS_CENTERIMAGE, CRect(0,0,0,0),m_wndSplitterCaption.IdFromRowCol(0, 0));

	//HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// set the caption colors.
	m_wndCaption.SetCaptionColors (GetXtremeColor(COLOR_3DFACE),GetXtremeColor(COLOR_3DSHADOW), GetXtremeColor(COLOR_WINDOW) );
	m_wndCaption.SetOffice2003Colors(true);

	LOGFONT lf;
	XTAuxData().fontBold.GetLogFont(&lf);
	lf.lfHeight = 18;
	m_fontCaption.CreateFontIndirect(&lf);

	// set the font and initial icon.
	m_wndCaption.ModifyCaptionStyle(0,&m_fontCaption, NULL, NULL);

	// Sizing for splitter
	m_wndSplitterCaption.SetRowInfo (0, 22, 0);
}

//////////////////////////////////////////////////////////////////////////
BOOL CCharacterEditor::PreTranslateMessage(MSG* pMsg)
{
	//if( GetFocus() == GetDlgItem( IDC_PANE1 ) )
	{
		if( WM_KEYDOWN == pMsg->message || WM_KEYUP == pMsg->message )
		{
			int mes = pMsg->message;
			int key = pMsg->wParam;
			switch( pMsg->message )
			{
			case WM_KEYDOWN:
				{
					m_pModelViewportCE->m_bKey[pMsg->wParam]=1;
					break;
				}
			case WM_KEYUP:
				{
					m_pModelViewportCE->m_bKey[pMsg->wParam]=0;
					break;
				}
			default:
				{
					break;
				}
			}
			return TRUE;
		}
	}

	return __super::PreTranslateMessage( pMsg );
}



void CCharacterEditor::OnEnChangeEdit1()
{

}




void CCharacterEditor::OnBnClickedOk() { /*	OnOK();*/ };


void CCharacterEditor::OnFileNew() {
	m_pModelViewportCE->ReleaseObject();
	m_strCharDefPathName="NoPathDefined";

	m_AttachmentsDlg.CharacterChanged=0;
	m_AttachmentsDlg.ReloadAttachment();
	m_AttachmentsDlg.ClearBones();

	m_MorphingDlg.m_DeformationSlider.EnableWindow(FALSE);
	m_MorphingDlg.m_DeformationNumber.EnableWindow(FALSE);

	m_MorphingDlg.m_UniformScalingSlider.EnableWindow(TRUE);
	m_MorphingDlg.m_UniformScalingNumber.EnableWindow(TRUE);

	m_MorphingDlg.m_button[0].EnableWindow(FALSE);
	m_MorphingDlg.m_button[1].EnableWindow(FALSE);
	m_MorphingDlg.m_button[2].EnableWindow(FALSE);
	m_MorphingDlg.m_button[3].EnableWindow(FALSE);
	m_MorphingDlg.m_button[4].EnableWindow(FALSE);
	m_MorphingDlg.m_button[5].EnableWindow(FALSE);
	m_MorphingDlg.m_button[6].EnableWindow(FALSE);
	m_MorphingDlg.m_button[7].EnableWindow(FALSE);

	m_MorphingDlg.m_button[0].SetCheck(FALSE);
	m_MorphingDlg.m_button[1].SetCheck(FALSE);
	m_MorphingDlg.m_button[2].SetCheck(FALSE);
	m_MorphingDlg.m_button[3].SetCheck(FALSE);
	m_MorphingDlg.m_button[4].SetCheck(FALSE);
	m_MorphingDlg.m_button[5].SetCheck(FALSE);
	m_MorphingDlg.m_button[6].SetCheck(FALSE);
	m_MorphingDlg.m_button[7].SetCheck(FALSE);

	m_MorphingDlg.m_button[0].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[0].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[1].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[1].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[2].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[2].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[3].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[3].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[4].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[4].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[5].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[5].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[6].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[6].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[7].SetBkColor(RGB(0xcf,0xcf,0xcf));
	m_MorphingDlg.m_button[7].SetPushedBkColor(RGB(0xcf,0xcf,0xcf));

	m_MorphingDlg.m_nColor=9;

	m_wndCaption.UpdateCaption("No object loaded",0 );

}

void CCharacterEditor::OpenFile(bool bSmart)
{
	CString file;
	if (bSmart ? 
		CFileUtil::SmartSelectSingleFile( EFILE_TYPE_GEOMETRY,file,"characters cdf" ) :
		CFileUtil::SelectSingleFile( EFILE_TYPE_GEOMETRY,file ))
	{
		// Select the default save location.
		m_currExtension = PathUtil::GetExt(file.GetString());
		m_currExtension = CryStringUtils::toLower(m_currExtension);

		if(m_currExtension == "cdf" || m_currExtension == "chr")
			m_strCharDefPathName = file;
		else
			m_strCharDefPathName = "NoPathDefined";

		LoadCharacter( file );

		if (m_currExtension == "cpf")
		{
			m_strCharPartsPathName = file;
		}
		for (int i = 0; i < m_frameExtensions.size(); i++)
		{
			m_frameExtensions[i].pIFrameExtension->OnFileLoad( file );
		}
	}

	//-------------------------------------------------------------------------------
	m_AttachmentsDlg.CharacterChanged=0;
	m_AttachmentsDlg.UpdateList();

	//------------------------------------------------------------------------------
	// Clear animation in Motion Adaptor
	m_pMotionAdaptorDlg->m_dlgMain.ClearAnimation();
}

void CCharacterEditor::OnFileSmartOpen()
{
	OpenFile(true);
}

void CCharacterEditor::OnFileLoad()
{
	OpenFile(false);
}

void CCharacterEditor::OnFileSave()
{
	if (m_strCharDefPathName != "NoPathDefined")
	{
		CString path =  Path::GamePathToFullPath( (const char*)m_strCharDefPathName );
		if (path.GetLength() == 0 || CryGetFileAttributes( path )== INVALID_FILE_ATTRIBUTES)
		{
			// Since there is no filename associated with the character, allow the user to choose one.
			OnFileSaveAs();
		}
		else
		{
			string sExtension = PathUtil::GetExt(path);
			sExtension = CryStringUtils::toLower(sExtension);

      if (sExtension == "cpf")
      {
				CLogFile::WriteLine("Exporting Character Definitions...");
				if (m_pModelViewportCE->GetAnimationSystem()->SaveCharacterDefinition( m_pModelViewportCE->GetCharacterBase(), m_strCharDefPathName ))
					m_AttachmentsDlg.CharacterChanged=0;
			}
			for (int i = 0; i < m_frameExtensions.size(); i++)
			{
				m_frameExtensions[i].pIFrameExtension->SaveFile( path );
			}
		}
	}
  else
  {
  	CString path =  Path::GamePathToFullPath( (const char*)m_strCharPartsPathName );
  	if (path.GetLength() == 0 || CryGetFileAttributes( path )== INVALID_FILE_ATTRIBUTES)
  	{
  		// Since there is no filename associated with the character, allow the user to choose one.
  		OnFileSaveAs();
  	}
  	else
  	{
			for (int i = 0; i < m_frameExtensions.size(); i++)
			{
				m_frameExtensions[i].pIFrameExtension->SaveFile( path );
			}
  	}
  }
}
void CCharacterEditor::OnUpdateUIFileSave( CCmdUI* pCmdUI )
{
	bool bAnyModified = false;
	for (int i = 0; i < m_frameExtensions.size(); i++)
	{
		if (m_frameExtensions[i].pIFrameExtension->IsModified())
		{
			bAnyModified = true;
			break;
		}
	}
	pCmdUI->Enable( m_AttachmentsDlg.CharacterChanged || bAnyModified );
}


void CCharacterEditor::OnFileSaveAs()
{
	if(m_currExtension != "chr" && m_currExtension != "cdf" && m_currExtension != "cpf" )
	{
		MessageBox( _T("Cannot save current file: it is not a *.cdf or *.cpf or *.chr file"), _T("Character Editor"), MB_OK|MB_ICONINFORMATION);
		return;
	}

	char szFilters[] = "Character Definition Files (*.cdf)|*.cdf | Character Parts Files (*.cpf)|*.cpf|";

	CString path = Path::GetPath( Path::GamePathToFullPath( (const char*)m_strCharDefPathName ) );
	CString file = PathUtil::GetFileName(m_strCharDefPathName);

	CCryFile cryfile;
	if(!cryfile.Open(m_strCharDefPathName, "rb")) // file doesn't exist in pak or local disk
		return;

	if(cryfile.IsInPak()) // Let OnFileSaveAs deal with packed file's save
	{
		int res = MessageBox( _T("Character definition file is loaded from packed file. Save a local copy?"), _T("Character Editor"), MB_YESNO | MB_ICONINFORMATION);
		if(res == IDYES)
		{
			path = Path::GetGameFolder()+ CString("\\");
		}
		else if(res == IDNO)
			return;
	}
	cryfile.Close();

	CString finalPath = file;
	uint32 uSave = CFileUtil::SelectSaveFile( szFilters,"cdf", path, finalPath );

	if (uSave) 
	{
 		string sExtension = PathUtil::GetExt(finalPath.GetString());
    sExtension = CryStringUtils::toLower(sExtension);

    if (sExtension == "cdf")
    {		
	  	CLogFile::WriteLine("Exporting Character Definitions...");

	  	m_pModelViewportCE->GetAnimationSystem()->SaveCharacterDefinition( m_pModelViewportCE->GetCharacterBase(), finalPath);
	  	m_AttachmentsDlg.CharacterChanged=0;
    }
    else if (sExtension == "cpf")
    {
      m_strCharPartsPathName = file;
    }
		for (int i = 0; i < m_frameExtensions.size(); i++)
		{
			m_frameExtensions[i].pIFrameExtension->SaveFile( file );
		}
	}

}

void CCharacterEditor::OnUpdateUIFileSaveAs( CCmdUI* pCmdUI )
{
	// [MichaelS 21/12/2005] We should be able to save to a different file, even if the file has not been changed.
	//pCmdUI->Enable( m_AttachmentsDlg.CharacterChanged );
	pCmdUI->Enable( TRUE );
}

void CCharacterEditor::OnBnClickedExit()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

void CCharacterEditor::OnBnClickedBrowseFile()
{
	// TODO: Add your control notification handler code here
	CString file;
	bool t=CFileUtil::SelectSingleFile( EFILE_TYPE_GEOMETRY,file );
	if (t)
	{
		LoadCharacter( file );
		m_fileEdit.SetWindowText( file );

		m_AnimationControlDlg.ModelChanged();
	}
}



void CCharacterEditor::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	CString filename;
	m_fileEdit.GetWindowText( filename );
	LoadCharacter( filename );
}

void CCharacterEditor::SaveUserOptions()
{
	CharPanel_Animation* animPanel = m_pModelViewportCE->GetModelPanelA();
	if(!animPanel)
		return;
	
	CXTRegistryManager regMgr;
	CString str;
	const CString strSection = _T("Settings\\CharacterEditorUserOptions");

	f32 floatValue = .0f;
	UINT byteNum = sizeof(f32);
	
	//------------------------------------------------------------------------------
	// Flag to determine whether this section exists in the registry
	regMgr.WriteProfileInt(strSection, _T("bCharacterEditorOptionsExist"), TRUE);

	//------------------------------------------------------------------------------
	// Current character path
	regMgr.WriteProfileString(strSection, _T("strCurrentCharacterPath"), animPanel->GetCurrentCharacterPath());

	//------------------------------------------------------------------------------
	// Speed turn lock
	regMgr.WriteProfileInt(strSection, _T("bDesiredLocomotionSpeed"), animPanel->GetDesiredLocomotionSpeed());
	regMgr.WriteProfileInt(strSection, _T("bDesiredTurnSpeed"), animPanel->GetDesiredTurnSpeed());
	regMgr.WriteProfileInt(strSection, _T("bLockMoveBody"), animPanel->GetLockMoveBody());

	//------------------------------------------------------------------------------
	// Blend-space Control

	floatValue = animPanel->GetBlendSpaceSliderX();
	regMgr.WriteProfileBinary( strSection, _T("fBlendSpaceSliderX"), (LPBYTE)(&floatValue), byteNum);
	floatValue = animPanel->GetBlendSpaceSliderY();
	regMgr.WriteProfileBinary( strSection, _T("fBlendSpaceSliderY"), (LPBYTE)(&floatValue), byteNum);
	floatValue = animPanel->GetBlendSpaceSliderZ();
	regMgr.WriteProfileBinary( strSection, _T("fBlendSpaceSliderZ"), (LPBYTE)(&floatValue), byteNum);

	//------------------------------------------------------------------------------
	// Animation driven and allow vertical movement
	regMgr.WriteProfileInt(strSection, _T("bAnimationDrivenMotion"), animPanel->GetAnimationDrivenMotion());
	regMgr.WriteProfileInt(strSection, _T("bVerticalMovement"), animPanel->GetVerticalMovement());

	//------------------------------------------------------------------------------
	// Animation flags
	regMgr.WriteProfileInt(strSection, _T("bMirrorAnimation"), animPanel->GetMirrorAnimation());
	regMgr.WriteProfileInt(strSection, _T("bLoopAnimation"), animPanel->GetLoopAnimation());
	regMgr.WriteProfileInt(strSection, _T("bRepeatLastKey"), animPanel->GetRepeatLastKey());
	regMgr.WriteProfileInt(strSection, _T("bVTimeWarping"), animPanel->GetVTimeWarping());
	regMgr.WriteProfileInt(strSection, _T("bTransitionTimeCheck"), animPanel->GetTransitionTimeCheck());
	floatValue = animPanel->GetTransitionTimeValue();
	regMgr.WriteProfileBinary( strSection, _T("fTransitionTimeValue"), (LPBYTE)(&floatValue), byteNum);
	regMgr.WriteProfileInt(strSection, _T("bDisableMultilayer"), animPanel->GetDisableMultilayer());
	regMgr.WriteProfileInt(strSection, _T("bAllowAnimationRestart"), animPanel->GetAllowAnimationRestart());
	regMgr.WriteProfileInt(strSection, _T("iAimIKLayerValue"), animPanel->GetAimIKLayerValue());
	floatValue = animPanel->GetAdditiveWeight(animPanel->GetAimIKLayerValue());
	regMgr.WriteProfileBinary( strSection, _T("fAdditiveWeight"), (LPBYTE)(&floatValue), byteNum);

	//------------------------------------------------------------------------------
	// Post processing
	regMgr.WriteProfileInt(strSection, _T("bLookIK"), animPanel->GetLookIK());
	regMgr.WriteProfileInt(strSection, _T("bAimIK"), animPanel->GetAimIK());
	regMgr.WriteProfileInt(strSection, _T("bLFootIK"), animPanel->GetLFootIK());
	regMgr.WriteProfileInt(strSection, _T("bRFootIK"), animPanel->GetRFootIK());
	regMgr.WriteProfileInt(strSection, _T("bLArmIK"), animPanel->GetLArmIK());
	regMgr.WriteProfileInt(strSection, _T("bRArmIK"), animPanel->GetRArmIK());
	regMgr.WriteProfileInt(strSection, _T("bFootAnchoring"), animPanel->GetFootAnchoring());
	regMgr.WriteProfileInt(strSection, _T("bGroundAlign"), animPanel->GetGroundAlign());

	//------------------------------------------------------------------------------
	// Animation speed control
	floatValue = animPanel->GetAnimationProgress();
	regMgr.WriteProfileBinary( strSection, _T("fAnimationProgress"), (LPBYTE)(&floatValue), byteNum);

	//------------------------------------------------------------------------------
	// Unit Test for Humans
	regMgr.WriteProfileInt(strSection, _T("bPlayerControl"), animPanel->GetPlayerControl());
	regMgr.WriteProfileInt(strSection, _T("bFixedCamera"), animPanel->GetFixedCamera());
	regMgr.WriteProfileInt(strSection, _T("bPathFollowing"), animPanel->GetPathFollowing());
	regMgr.WriteProfileInt(strSection, _T("bAttachedCamera"), animPanel->GetAttachedCamera());
	regMgr.WriteProfileInt(strSection, _T("bIdle2Move"), animPanel->GetIdle2Move());
	regMgr.WriteProfileInt(strSection, _T("bIdleStep"), animPanel->GetIdleStep());

	//------------------------------------------------------------------------------
	// Gamepad and morph
	regMgr.WriteProfileInt(strSection, _T("bGamepad"), animPanel->GetGamepad());
	regMgr.WriteProfileInt(strSection, _T("bLinearMorphSequence"), animPanel->GetLinearMorphSequence());
	regMgr.WriteProfileInt(strSection, _T("bUseMorphTargets"), animPanel->GetUseMorphTargets());

	//------------------------------------------------------------------------------
	// Linear morph slider
	floatValue = animPanel->GetLinearMorphSlider();
	regMgr.WriteProfileBinary( strSection, _T("fLinearMorphSlider"), (LPBYTE)(&floatValue), byteNum);

	//------------------------------------------------------------------------------
	// Debug options
	CVarBlock *vb = m_pModelViewportCE->GetVarObject()->GetVarBlock();
	int32 vbCount = vb->GetVarsCount();

	regMgr.WriteProfileInt(strSection, _T("iDebugOptionCount"), vbCount);

	char keyType[64], keyValue[64];
	for(int32 i=0; i<vbCount; ++i)
	{
		IVariable* var = vb->GetVariable(i);
		IVariable::EType vType = var->GetType();
		sprintf(keyType, "DebugOption_%s_type", var->GetName());
		sprintf(keyValue, "DebugOption_%s_value", var->GetName());
		switch(vType)
		{
		case IVariable::UNKNOWN:
			{
				break;
			}
		case IVariable::INT:
			{
				int32 value = 0;
				var->Get(value);
				regMgr.WriteProfileInt(strSection, _T(keyType), IVariable::INT);
				regMgr.WriteProfileInt(strSection, _T(keyValue), value);
				
				break;
			}
		case IVariable::BOOL:
			{
				BOOL value = 0;
				var->Get(value);
				regMgr.WriteProfileInt(strSection, _T(keyType), IVariable::BOOL);
				regMgr.WriteProfileInt(strSection, _T(keyValue), value);
				break;
			}
		case IVariable::FLOAT:
			{
				f32 value = 0;
				var->Get(value);
				regMgr.WriteProfileInt(strSection, _T(keyType), IVariable::FLOAT);
				regMgr.WriteProfileBinary(strSection, _T(keyValue), (LPBYTE)(&value), sizeof(f32));
				break;
			}
			case IVariable::VECTOR:
				{
					Vec3 value;
					var->Get(value);
					f32 valueArray[3];
					valueArray[0] = value.x; 
					valueArray[1] = value.y;
					valueArray[2] = value.z;
					regMgr.WriteProfileInt(strSection, _T(keyType), IVariable::VECTOR);
					regMgr.WriteProfileBinary(strSection, _T(keyValue), (LPBYTE)(&value), 3*sizeof(f32));

					break;
				}
			case IVariable::QUAT:
				{
					Quat value;
					var->Get(value);
					f32 valueArray[4];
					valueArray[0] = value.w; 
					valueArray[1] = value.v.x;
					valueArray[2] = value.v.y;
					valueArray[3] = value.v.z;
					
					regMgr.WriteProfileInt(strSection, _T(keyType), IVariable::QUAT);
					regMgr.WriteProfileBinary(strSection, _T(keyValue), (LPBYTE)(&value), 4*sizeof(f32));

					break;
				}
			case IVariable::STRING:
				{
					CString value;
					var->Get(value);
					regMgr.WriteProfileInt(strSection, _T(keyType), IVariable::QUAT);
					regMgr.WriteProfileString(strSection, _T(keyValue), value);

					break;
				}
			case IVariable::ARRAY:
				{
					break;
				}				
		default:
			break;
		}		
	}
}

void CCharacterEditor::ReadUserOptions()
{
	CharPanel_Animation* animPanel = m_pModelViewportCE->GetModelPanelA();
	if(!animPanel)
		return;

	CXTRegistryManager regMgr;
	CString str;
	const CString strSection = _T("Settings\\CharacterEditorUserOptions");

	f32 floatValue = .0f;
	UINT byteNum = sizeof(f32);
	CString strRead = "";
	int32 iRead = 0;
	BOOL bRead = FALSE;
	f32 fRead = .0f;
	LPBYTE pbtData = NULL;
	UINT bytes = 0;

	bRead = regMgr.GetProfileInt(strSection, _T("bCharacterEditorOptionsExist"), FALSE);
	if(!bRead)
		return;

	//------------------------------------------------------------------------------
	// Current character path
	strRead = regMgr.GetProfileString(strSection, _T("strCurrentCharacterPath"), "");
	animPanel->SetCurrentCharacterPath(strRead);

	//------------------------------------------------------------------------------
	// Speed turn lock
	bRead = regMgr.GetProfileInt(strSection, _T("bDesiredLocomotionSpeed"), FALSE);
	animPanel->SetDesiredLocomotionSpeed(bRead);

	bRead = regMgr.WriteProfileInt(strSection, _T("bDesiredTurnSpeed"), FALSE);
	animPanel->SetDesiredTurnSpeed(bRead);

	bRead = regMgr.WriteProfileInt(strSection, _T("bLockMoveBody"), FALSE);
	animPanel->SetLockMoveBody(bRead);

	//------------------------------------------------------------------------------
	// Blend-space Control

	regMgr.GetProfileBinary( strSection, _T("fBlendSpaceSliderX"), &pbtData, &bytes);
	if(bytes == 0)
		return;

	fRead = *(f32*)(pbtData);
	animPanel->SetBlendSpaceSliderX(fRead);

	regMgr.GetProfileBinary( strSection, _T("fBlendSpaceSliderY"), &pbtData, &bytes);
	fRead = *(f32*)(pbtData);
	animPanel->SetBlendSpaceSliderY(fRead);

	regMgr.GetProfileBinary( strSection, _T("fBlendSpaceSliderZ"), &pbtData, &bytes);
	fRead = *(f32*)(pbtData);
	animPanel->SetBlendSpaceSliderZ(fRead);

	
	//------------------------------------------------------------------------------
	// Animation driven and allow vertical movement
	bRead = regMgr.GetProfileInt(strSection, _T("bAnimationDrivenMotion"), FALSE);
	animPanel->SetAnimationDrivenMotion(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bVerticalMovement"), FALSE);
	animPanel->SetVerticalMovement(bRead);


	//------------------------------------------------------------------------------
	// Animation flags
	bRead = regMgr.GetProfileInt(strSection, _T("bMirrorAnimation"), FALSE);
	animPanel->SetMirrorAnimation(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bLoopAnimation"), FALSE);
	animPanel->SetLoopAnimation(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bRepeatLastKey"), FALSE);
	animPanel->SetRepeatLastKey(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bVTimeWarping"), FALSE);
	animPanel->SetVTimeWarping(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bTransitionTimeCheck"), FALSE);
	animPanel->SetTransitionTimeCheck(bRead);

	regMgr.GetProfileBinary( strSection, _T("fTransitionTimeValue"), &pbtData, &bytes);
	fRead = *(f32*)(pbtData);
	animPanel->SetTransitionTimeValue(fRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bDisableMultilayer"), FALSE);
	animPanel->SetDisableMultilayer(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bAllowAnimationRestart"), FALSE);
	animPanel->SetAllowAnimationRestart(bRead);

	iRead = regMgr.GetProfileInt(strSection, _T("iAimIKLayerValue"), 0);
	animPanel->SetAimIKLayerValue(iRead);

	regMgr.GetProfileBinary( strSection, _T("fAdditiveWeight"), &pbtData, &bytes);
	fRead = *(f32*)(pbtData);
	animPanel->SetAdditiveWeight(fRead);
	
	//------------------------------------------------------------------------------
	// Post processing
	bRead = regMgr.GetProfileInt(strSection, _T("bLookIK"), FALSE);
	animPanel->SetLookIK(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bAimIK"), FALSE);
	animPanel->SetAimIK(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bLFootIK"), FALSE);
	animPanel->SetLFootIK(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bRFootIK"), FALSE);
	animPanel->SetRFootIK(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bLArmIK"), FALSE);
	animPanel->SetLArmIK(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bRArmIK"), FALSE);
	animPanel->SetRArmIK(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bFootAnchoring"), FALSE);
	animPanel->SetFootAnchoring(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bGroundAlign"), FALSE);
	animPanel->SetGroundAlign(bRead);

	//------------------------------------------------------------------------------
	// Animation speed control
	regMgr.GetProfileBinary( strSection, _T("fAnimationProgress"), &pbtData, &bytes);
	fRead = *(f32*)(pbtData);
	animPanel->SetAnimationProgress(fRead);

	//------------------------------------------------------------------------------
	// Unit Test for Humans
	bRead = regMgr.GetProfileInt(strSection, _T("bPlayerControl"), FALSE);
	animPanel->SetPlayerControl(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bFixedCamera"), FALSE);
	animPanel->SetFixedCamera(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bPathFollowing"), FALSE);
	animPanel->SetPathFollowing(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bAttachedCamera"), FALSE);
	animPanel->SetAttachedCamera(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bIdle2Move"), FALSE);
	animPanel->SetIdle2Move(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bIdleStep"), FALSE);
	animPanel->SetIdleStep(bRead);

	//------------------------------------------------------------------------------
	// Gamepad and morph
	bRead = regMgr.GetProfileInt(strSection, _T("bGamepad"), FALSE);
	animPanel->SetGamepad(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bLinearMorphSequence"), FALSE);
	animPanel->SetLinearMorphSequence(bRead);

	bRead = regMgr.GetProfileInt(strSection, _T("bUseMorphTargets"), FALSE);
	animPanel->SetUseMorphTargets(bRead);

	//------------------------------------------------------------------------------
	// Linear morph slider
	regMgr.GetProfileBinary( strSection, _T("fLinearMorphSlider"), &pbtData, &bytes);
	fRead = *(f32*)(pbtData);
	animPanel->SetLinearMorphSlider(fRead);

	//------------------------------------------------------------------------------
	// Debug options: The ModemViewport class is responsible to restore the debug options
	
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::OnDestroy()
{
	this->m_AnimationControlDlg.Close();

	SaveLayouts();

	CVarBlock *vb = m_pModelViewportCE->GetVarObject()->GetVarBlock();
	SaveUserOptions();

	__super::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::OnSize(UINT nType, int cx, int cy)
{
	if (m_pane1.m_hWnd && m_pWndMenuBar)
	{
		CRect rc,rcorg;
		GetClientRect(rc);
		rcorg = rc;

		int p2w = 260;

		CRect menuRc = rc;
		menuRc.bottom = 24;
		m_pWndMenuBar->MoveWindow(menuRc);

		rc.top += menuRc.bottom;
		rc.DeflateRect(4,0,4,4);

		CRect toolRc = rc;
		toolRc.bottom = toolRc.top + 32;
		toolRc.right -= p2w;
		m_toolbar.MoveWindow(toolRc);

		CRect pane2rc = rc;
		pane2rc.left = pane2rc.right - p2w;
		m_pane2.MoveWindow(pane2rc);

		CRect pane1rc = rc;
		pane1rc.top = toolRc.bottom + 4;
		pane1rc.right = pane2rc.left-4;
		pane1rc.bottom = rc.bottom - 210;
		m_pane1.MoveWindow(pane1rc);

		CRect tabRc = rc;
		tabRc.top = pane1rc.bottom + 4;
		tabRc.right = pane2rc.left - 4;
		
		m_pane1.GetClientRect(rc);
		m_pModelViewportCE->MoveWindow(rc);

		m_pane2.GetClientRect(rc);
		m_rollupCtrl.MoveWindow(rc);
	}
	__super::OnSize(nType,cx,cy);
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnIdleUpdate:
		//DoIdleUpdate();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::DoIdleUpdate()
{
	m_pModelViewportCE->Update();
}


//-------------------------------------------------------------------


void CCharacterEditor::OnMoveMode( ) {
	m_pModelViewportCE->m_Button_MOVE+=1;
	m_pModelViewportCE->m_Button_MOVE&=1;
	if (m_pModelViewportCE->m_Button_MOVE) m_pModelViewportCE->m_Button_ROTATE=0;

	ICharacterInstance* pCharacter=m_pModelViewportCE->GetCharacterBase();
	if (pCharacter) {
		if (m_pModelViewportCE->m_Button_MOVE) 
			ResetCharEditor(pCharacter);

		pCharacter->SetResetMode(m_pModelViewportCE->m_Button_MOVE);
		m_pModelViewportCE->SetPaused(true);
	}

	if(m_pMotionAdaptorDlg!= NULL){
		if(m_pModelViewportCE->m_Button_MOVE)
			m_pMotionAdaptorDlg->m_dlgMain.ResetAnimationProgress();
		else
			m_pMotionAdaptorDlg->m_dlgMain.ResumeAnimationProgress();
	}


}
void CCharacterEditor::OnMoveModeUpdateUI( CCmdUI *pCmdUI ) 
{
	pCmdUI->SetCheck(m_pModelViewportCE->m_Button_MOVE);
}

//-------------------------------------------------------------------

void CCharacterEditor::OnRotateMode() 
{
	m_pModelViewportCE->m_Button_ROTATE+=1;
	m_pModelViewportCE->m_Button_ROTATE&=1;
	if (m_pModelViewportCE->m_Button_ROTATE) m_pModelViewportCE->m_Button_MOVE=0;

	ICharacterInstance* pCharacter=m_pModelViewportCE->GetCharacterBase();
	if (pCharacter) 
	{
		if (m_pModelViewportCE->m_Button_ROTATE) 
			ResetCharEditor(pCharacter);
		pCharacter->SetResetMode(m_pModelViewportCE->m_Button_ROTATE);
	}
}

void CCharacterEditor::OnRotateModeUpdateUI( CCmdUI *pCmdUI ) 
{
	pCmdUI->SetCheck(m_pModelViewportCE->m_Button_ROTATE);
}


void CCharacterEditor::ResetCharEditor(ICharacterInstance* pCharacter) 
{
	CharPanel_Animation* pCharPanel_Animation=m_pModelViewportCE->m_pCharPanel_Animation;
	pCharPanel_Animation->SetBlendSpaceSliderX(0.0f);
	pCharPanel_Animation->SetBlendSpaceSliderY(0.0f);
	pCharPanel_Animation->SetBlendSpaceSliderZ(0.0f);

//	if ( m_pModelViewportCE->GetPlayerControl() )
	{
		m_pModelViewportCE->SetPlayerPos(Vec3(ZERO));
		m_pModelViewportCE->m_GridOrigin=Vec3(ZERO);
		m_pModelViewportCE->m_pCharPanel_Animation->m_PlayControl.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_FixedCamera.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_PathFollowing.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_AttachedCamera.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_Idle2Move.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_IdleStep.SetCheck(0);
	}

	if ( m_pModelViewportCE->GetPlayerControl() )
	{
		m_pModelViewportCE->SetPlayerPos(Vec3(ZERO));
		m_pModelViewportCE->m_pCharPanel_Animation->m_PlayControl.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_FixedCamera.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_PathFollowing.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_AttachedCamera.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_Idle2Move.SetCheck(0);
		m_pModelViewportCE->m_pCharPanel_Animation->m_IdleStep.SetCheck(0);
		m_pModelViewportCE->SetPlayerControl(0);
		Vec3 VPos=Vec3(0,-3,2);
		Vec3 VTar=Vec3(0,0,1);
		Matrix33 orient; orient.SetRotationVDir( (VTar-VPos).GetNormalized() );
		m_pModelViewportCE->m_Camera.SetMatrix( Matrix34(orient,Vec3(0,-3,2))  );
	}


	//pCharacter->StopAllAnimations();
	pCharacter->GetISkeletonPose()->SetDefaultPose();
	IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
	uint32 numAttachmnets = pIAttachmentManager->GetAttachmentCount();
	for (uint32 i=0; i<numAttachmnets; i++) 
	{
		IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(i);
		IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
		if (pIAttachmentObject) 
		{
			ICharacterInstance* pICharInstance = pIAttachmentObject->GetICharacterInstance();
			if (pICharInstance)
			{
				pCharacter->GetISkeletonPose()->SetDefaultPose();
			}
		}
	}

}

void CCharacterEditor::OnCharacterChanged()
{
	ICharacterInstance* pCharacter = m_pModelViewportCE->GetCharacterBase();
	if (pCharacter)
	{
		m_strCharDefPathName = pCharacter->GetFilePath();
		m_currExtension = PathUtil::GetExt( m_strCharDefPathName );
		m_currExtension = CryStringUtils::toLower( m_currExtension );

		m_animationBrowser.UpdateAnimations(pCharacter);
		m_wndCaption.UpdateCaption( m_strCharDefPathName,0 );
	}
	else
	{
		m_currExtension = "NoExtension";
		m_wndCaption.UpdateCaption( "No object loaded",0 );
	}
	this->m_AttachmentsDlg.CharacterChanged = 1;
}

void CCharacterEditor::LoadCharacter(const CString& file, float scale)
{
	if (m_pModelViewportCE)
		m_pModelViewportCE->LoadObject(file, scale);

 	m_AnimationControlDlg.ModelChanged();
}

void CCharacterEditor::SceneUpdate()
{
	for (int i = 0; i < m_frameExtensions.size(); i++)
	{
		m_frameExtensions[i].pIFrameExtension->OnViewUpdate();
	}
}
CAnimationControlDlg* CCharacterEditor::GetAnimationControlDlg()
{
  return &m_AnimationControlDlg;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::PlayAnimations( std::vector<CString> &anims )
{
	if (m_pModelViewportCE && anims.size() > 0)
	{
		m_pModelViewportCE->PlayAnimation( anims[0] );
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::OnAnimForcePlay()
{
	if (m_pModelViewportCE)
	{
		m_pModelViewportCE->OnAnimForcePlay();
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterEditor::InitFrameExtensions()
{
	int nPaneId = 0;
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID( ESYSTEM_CLASS_FRAMEWND_EXTENSION_PANE,classes );
	for (int i = 0; i < classes.size(); i++)
	{
		IClassDesc *pClass = classes[i];
		IFrameWindowExtensionPane *pExtension = NULL;
		HRESULT hRes = pClass->QueryInterface( __uuidof(IFrameWindowExtensionPane),(void**)&pExtension );
		
		_smart_ptr<IFrameWindowExtensionPane> pHolder(pExtension);

		if (FAILED(hRes))
		{
			continue;
		}
		if (strcmp(pExtension->SupportedFrameWindow(),"CharacterEditor") != 0)
		{
			continue;
		}
		SFrameExtension ext;
		ext.pIFrameExtension = pHolder;
		ext.pPaneWnd = pExtension->CreatePaneWindow(this);
		if (!ext.pPaneWnd)
			continue;
		m_frameExtensions.push_back(ext);

//Diesel cut
		//CXTPDockingPane* pDockPane = GetDockingPaneManager()->CreatePane( IDW_FIRST_EXTENSION_PANE+nPaneId, CRect(0,0,270,500), dockRightOf );
//Diesel new		
		CXTPDockingPane* pDockPane = GetDockingPaneManager()->CreatePane( IDW_FIRST_EXTENSION_PANE+nPaneId, CRect(0,0,270,500), xtpPaneDockRight );
		
		
		nPaneId++;
		pDockPane->SetTitle( pExtension->SupportedFrameWindow() );
	}
}

//////////////////////////////////////////////////////////////////////////
CModelViewportCE* CCharacterEditor::GetModelViewport() const
{
	return m_pModelViewportCE;
}
//////////////////////////////////////////////////////////////////////////

void CCharacterEditor::OnUpdateViewAnimations(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_ANIMATIONS){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_ANIMATIONS_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}

}

void CCharacterEditor::OnUpdateViewCharacterparts(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_CHARACTERPARTS){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_PARTS_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}
}

void CCharacterEditor::OnUpdateViewRollup(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_ROLLUP){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_ROLLUP_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}
}

void CCharacterEditor::OnViewAnimations()
{
	if(GetDockingPaneManager()->IsPaneClosed(IDW_ANIMATIONS_PANE))
		GetDockingPaneManager()->ShowPane(IDW_ANIMATIONS_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_ANIMATIONS_PANE);
}


void CCharacterEditor::OnViewCharacterparts()
{
	if(!gEnv->pGame)
	{
		MessageBox("Game.dll not loaded. Please load it first to use CharacterParts panel.", "Character Editor", MB_OK|MB_ICONINFORMATION);
		return;
	}

	if(GetDockingPaneManager()->IsPaneClosed(IDW_PARTS_PANE))
		GetDockingPaneManager()->ShowPane(IDW_PARTS_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_PARTS_PANE);
}

void CCharacterEditor::OnViewRollup()
{
	if(GetDockingPaneManager()->IsPaneClosed(IDW_ROLLUP_PANE))
		GetDockingPaneManager()->ShowPane(IDW_ROLLUP_PANE);
	else

		GetDockingPaneManager()->ClosePane(IDW_ROLLUP_PANE);
}

void CCharacterEditor::OnViewMotionadaptor()
{
	if(GetDockingPaneManager()->IsPaneClosed(IDW_MOTION_ADAPTOR_PANE))
		GetDockingPaneManager()->ShowPane(IDW_MOTION_ADAPTOR_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_MOTION_ADAPTOR_PANE);
}

void CCharacterEditor::OnViewSetMaterial()
{
	if(GetDockingPaneManager()->IsPaneClosed(IDW_SET_MATERIAL_PANE))
		GetDockingPaneManager()->ShowPane(IDW_SET_MATERIAL_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_SET_MATERIAL_PANE);
}

void CCharacterEditor::OnViewShapeDeformation()
{
	if(GetDockingPaneManager()->IsPaneClosed(IDW_SHAPE_DEFORMATION_PANE))
		GetDockingPaneManager()->ShowPane(IDW_SHAPE_DEFORMATION_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_SHAPE_DEFORMATION_PANE);
}

void CCharacterEditor::OnViewAnimationControl()
{
	if(GetDockingPaneManager()->IsPaneClosed(IDW_ANIMATION_CONTROL_PANE))
		GetDockingPaneManager()->ShowPane(IDW_ANIMATION_CONTROL_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_ANIMATION_CONTROL_PANE);
}

void CCharacterEditor::OnViewAttachments()
{
	if(GetDockingPaneManager()->IsPaneClosed(IDW_ATTACHMENTS_PANE))
		GetDockingPaneManager()->ShowPane(IDW_ATTACHMENTS_PANE);
	else
		GetDockingPaneManager()->ClosePane(IDW_ATTACHMENTS_PANE);
}

void CCharacterEditor::OnUpdateViewMotionadaptor(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_MOTIONADAPTOR){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_MOTION_ADAPTOR_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}
}

void CCharacterEditor::OnUpdateViewSetMaterial(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_SETMATERIAL){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_SET_MATERIAL_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}
}

void CCharacterEditor::OnUpdateViewShapeDeformation(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_SHAPEDEFORMATION){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_SHAPE_DEFORMATION_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}
}

void CCharacterEditor::OnUpdateViewAnimationControl(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_ANIMATIONCONTROL){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_ANIMATION_CONTROL_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}
}

void CCharacterEditor::OnUpdateViewAttachments(CCmdUI *pCmdUI)
{
	if(pCmdUI->m_nID == ID_VIEW_ATTACHMENTS){
		if(GetDockingPaneManager()->IsPaneClosed(IDW_ATTACHMENTS_PANE))
			pCmdUI->SetCheck(BST_UNCHECKED);		
		else
			pCmdUI->SetCheck(BST_CHECKED);		
	}
}
