#include "stdafx.h"
#include "MainFrm.h"
#include "Controls/DynamicRibbonBar.h"

//-----------------------------------------------------------------------------------------------------------
void CMainFrame::ReloadRibbonBarCmd( IConsoleCmdArgs* pArgs )
{
	// Disable duplicate command registration warning.
	GetIEditor()->GetCommandManager()->EnableWarnings(false);
	((CMainFrame*)AfxGetMainWnd())->ReloadRibbonBar();
	GetIEditor()->GetCommandManager()->EnableWarnings(true);
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::ReloadRibbonBar()
{
	if (!m_pRibbonBar)
		return;

	m_pRibbonBar->RemoveAllTabs();

	CDynamicRibbonBar bar;
	bar.Init( m_pRibbonBar );



	bar.LoadFromFile( "Editor/UI/Ribbon.xml" );

	bar.MakeCreateTab();
	bar.MakeViewTab();
	bar.LoadFromFile( "Editor/UI/CustomRibbon.xml" );

	{
		CXTPControlComboBox *pComboBox = 0;
		pComboBox = (CXTPControlComboBox*)m_pRibbonBar->GetControls()->FindControl( xtpControlComboBox,ID_REF_COORDS_SYS,TRUE,TRUE );
		if (pComboBox)
		{
			InitRefCoordControl(pComboBox);
		}
		pComboBox = (CXTPControlComboBox*)m_pRibbonBar->GetControls()->FindControl( xtpControlComboBox,IDC_SELECTION_MASK,TRUE,TRUE );
		if (pComboBox)
		{
			InitSelectionMaskControl(pComboBox);
		}
	}

}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::CreateRibbonBar()
{
	gEnv->pConsole->AddCommand("ed_reloadRibbon", (ConsoleCommandFunc)ReloadRibbonBarCmd, VF_CHEAT, "Reload Editor RibbonBar");

	AfxEnableControlContainer();
	CXTPWinDwmWrapper().SetProcessDPIAware();

	CXTPCommandBars* pCommandBars = GetCommandBars();

	{
		// Tooltips
		CXTPToolTipContext* pToolTipContext = GetCommandBars()->GetToolTipContext();
		pToolTipContext->SetStyle(xtpToolTipOffice2007);
		pToolTipContext->ShowTitleAndDescription();
		pToolTipContext->SetMargin(CRect(2, 2, 2, 2));
		pToolTipContext->SetMaxTipWidth(180);
		pToolTipContext->SetFont(pCommandBars->GetPaintManager()->GetIconFont());

		pCommandBars->GetCommandBarsOptions()->ShowKeyboardCues(xtpKeyboardCuesShowWindowsDefault);
		pCommandBars->GetCommandBarsOptions()->bToolBarAccelTips = TRUE;
	}

	//XTPOffice2007Images()->SetHandle( Path::Make("Editor\\Styles","Office2007Black.dll") );

	GetCommandBars()->GetPaintManager()->m_bEnableAnimation = TRUE;

	XTPPaintManager()->SetTheme(xtpThemeRibbon);

	CMenu menu;
	menu.LoadMenu(IDR_MAINFRAME);
	SetMenu(NULL);

	CXTPRibbonBar* pRibbonBar = (CXTPRibbonBar*)pCommandBars->Add(_T("The Ribbon"), xtpBarTop, RUNTIME_CLASS(CXTPRibbonBar));
	pRibbonBar->SetCommandBars( pCommandBars );
	m_pRibbonBar = pRibbonBar;

	pRibbonBar->EnableDocking(0);

	CXTPControlPopup* pControlFile = (CXTPControlPopup*)pRibbonBar->AddSystemButton(0);
	pControlFile->SetCommandBar(menu.GetSubMenu(0));
	pControlFile->SetIconId(IDB_RIBBON_SYSTEM_BUTTON);
	{
		UINT uCommand = {IDB_RIBBON_SYSTEM_BUTTON};
		pCommandBars->GetImageManager()->SetIcons(IDB_RIBBON_SYSTEM_BUTTON, &uCommand, 1, CSize(0, 0), xtpImageNormal);
	}

	ReloadRibbonBar();

	CXTPControlPopup* pControlOptions = (CXTPControlPopup*)pRibbonBar->GetControls()->Add(xtpControlPopup, -1);
	pControlOptions->SetFlags(xtpFlagRightAlign);
	CMenu mnuOptions;
	mnuOptions.LoadMenu(IDR_MAINFRAME);
	pControlOptions->SetCommandBar(mnuOptions.GetSubMenu(15));
	pControlOptions->SetCaption(_T("View"));

	{
		CXTPControlPopup* pControlTools = (CXTPControlPopup*)pRibbonBar->GetControls()->Add(xtpControlPopup, -1);
		pControlTools->SetFlags(xtpFlagRightAlign);
		CMenu mnuTools;
		mnuTools.LoadMenu(IDR_MAINFRAME);
		pControlTools->SetCommandBar(mnuTools.GetSubMenu(14));
		pControlTools->SetCaption(_T("Tools"));
	}

	CXTPControl* pControlAbout = pRibbonBar->GetControls()->Add(xtpControlButton, ID_APP_ABOUT);
	pControlAbout->SetFlags(xtpFlagRightAlign);

	pRibbonBar->GetQuickAccessControls()->Add(new CCustomControlSplitButtonPopup, ID_UNDO);
	pRibbonBar->GetQuickAccessControls()->Add(new CCustomControlSplitButtonPopup, ID_REDO);
	pRibbonBar->GetQuickAccessControls()->CreateOriginalControls();

	pRibbonBar->EnableFrameTheme();
}
