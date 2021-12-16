
#include "stdafx.h"
#include "AnimationGraph2_DefaultsDialog.h"


CAnimationGraph2DefaultsDialog::CAnimationGraph2DefaultsDialog()
	:	CDialog(IDD_AG2_GRAPH_DEFAULTS_DIALOG)
{
	m_pAnimGraph = NULL;
	m_currSelectedTab = -1;
	m_animationNodeDefaults = new CAnimationNodeDefaultsPanel();
	m_graphNodeDefaults = new CGeneralNodeDefaultsPanel();
	m_movementNodeDefaults = new CMovementNodeDefaultsPanel();
}

void CAnimationGraph2DefaultsDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control( pDX, IDC_DEFAULTTYPES, m_dialogTabCtrl );
}


BEGIN_MESSAGE_MAP(CAnimationGraph2DefaultsDialog,CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_DEFAULTTYPES, &CAnimationGraph2DefaultsDialog::OnActiveTabChanged)
END_MESSAGE_MAP()


BOOL CAnimationGraph2DefaultsDialog::OnInitDialog()
{
	if (__super::OnInitDialog() == FALSE)
		return FALSE;
	
	m_dialogTabCtrl.InsertItem(0, _T("General Settings"));
	m_dialogTabCtrl.InsertItem(1, _T("Animation Defaults"));
	m_dialogTabCtrl.InsertItem(2, _T("Movement and Physics Defaults"));
	m_dialogTabCtrl.SetCurSel(0);

	m_dialogTabCtrl.GetClientRect(&m_ClientRect);
	m_dialogTabCtrl.AdjustRect(FALSE, &m_ClientRect);
	CRect WindowRect;
	m_dialogTabCtrl.GetWindowRect(&WindowRect);
	m_dialogTabCtrl.GetParent()->ScreenToClient(WindowRect);

	m_graphNodeDefaults->Create(IDD_AG2_GRAPH_DEFAULTS_GENERAL, &m_dialogTabCtrl);
	m_graphNodeDefaults->ReadFromGraphDefaults(m_pAnimGraph);
	m_graphNodeDefaults->MoveWindow(m_ClientRect);

	m_animationNodeDefaults->Create(IDD_AG2_GRAPH_DEFAULTS_ANIMATION, &m_dialogTabCtrl);
	m_animationNodeDefaults->ReadFromGraphDefaults(m_pAnimGraph);
	m_animationNodeDefaults->MoveWindow(m_ClientRect);

	m_movementNodeDefaults->Create(IDD_AG2_GRAPH_DEFAULTS_MCM_COLLIDER, &m_dialogTabCtrl);
	m_movementNodeDefaults->ReadFromGraphDefaults(m_pAnimGraph);
	m_movementNodeDefaults->MoveWindow(m_ClientRect);

	bool invalid = m_pAnimGraph == NULL;
	m_animationNodeDefaults->GrayOutAllElements(invalid);
	m_graphNodeDefaults->GrayOutAllElements(invalid);
	m_movementNodeDefaults->GrayOutAllElements(invalid);

	m_currSelectedTab = -1;
	OnSelectionChange();
	GetParent()->RedrawWindow();
	
	return TRUE;
}

void CAnimationGraph2DefaultsDialog::OnOK()
{
	if (!m_pAnimGraph)
	{
		AfxMessageBox( "Sorry, but you cannot save graph settings without a graph being loaded.", MB_OK|MB_ICONERROR );

		__super::OnOK();
		return;
	}

	// Save all the settings
	m_graphNodeDefaults->WriteToGraphDefaults(m_pAnimGraph);
	m_animationNodeDefaults->WriteToGraphDefaults(m_pAnimGraph);
	m_movementNodeDefaults->WriteToGraphDefaults(m_pAnimGraph);

	__super::OnOK();
}

void CAnimationGraph2DefaultsDialog::OnCancel()
{
	// Discard all the settings

	__super::OnCancel();
}

void CAnimationGraph2DefaultsDialog::OnSelectionChange()
{
	int newSelTab = m_dialogTabCtrl.GetCurSel();
	if (m_currSelectedTab == newSelTab)
		return;
	
	// disable all
	m_graphNodeDefaults->EnableWindow(false);
	m_graphNodeDefaults->ShowWindow(SW_HIDE);
	m_animationNodeDefaults->EnableWindow(false);
	m_animationNodeDefaults->ShowWindow(SW_HIDE);
	m_movementNodeDefaults->EnableWindow(false);
	m_movementNodeDefaults->ShowWindow(SW_HIDE);

	// enable the newly selected one
	switch (newSelTab)
	{
		case 0: 
			m_graphNodeDefaults->ShowWindow(SW_SHOW);
			m_graphNodeDefaults->EnableWindow(true);
			break;
		case 1: 
			m_animationNodeDefaults->ShowWindow(SW_SHOW);
			m_animationNodeDefaults->EnableWindow(true);
			break;
		case 2:
			m_movementNodeDefaults->ShowWindow(SW_SHOW);
			m_movementNodeDefaults->EnableWindow(true);
			break;
		default:
			CRY_ASSERT_MESSAGE(0, "This tab does not have a dialog attached to it... pleae add code here");
			break;
	}

	m_currSelectedTab = newSelTab;
}

void CAnimationGraph2DefaultsDialog::OnActiveTabChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnSelectionChange();
	*pResult = 0;
}

void CAnimationGraph2DefaultsDialog::SetAnimationGraph( CAnimationGraph2Ptr pAnimGraph )
{
	m_pAnimGraph = pAnimGraph;
}

/************************************************************************/
/*           the classes for the different tab panels                   */
/************************************************************************/

// TODO: Put everything below in a separate cpp class to keep the file small

//------------------------------------------------------------
//		Animation Node Tab
//------------------------------------------------------------



CAnimationNodeDefaultsPanel::CAnimationNodeDefaultsPanel()
:	CDialog()
{
	m_defaultLayer = -1;
}

BEGIN_MESSAGE_MAP(CAnimationNodeDefaultsPanel, CDialog)
	ON_EN_KILLFOCUS(IDC_AG_ANIM_LAYER, OnLayerChanged)
	ON_BN_CLICKED(IDC_RESET_BUTTON, ResetToDefaultValues)
END_MESSAGE_MAP()

void CAnimationNodeDefaultsPanel::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control( pDX, IDC_AG_ANIM_LAYER, m_layer );

	DDX_Control( pDX, IDC_ENSURESTACK, m_displayDetails.m_ensureInStack );
	DDX_Control( pDX, IDC_FORCELEAVE, m_displayDetails.m_forceLeaveWhenFinished );
	DDX_Control( pDX, IDC_STOPCURRANIM, m_displayDetails.m_stopCurrentAnimation );
	DDX_Control( pDX, IDC_INTERRUPTANIM, m_displayDetails.m_interruptCurrAnim );
	DDX_Control( pDX, IDC_MANUAL_UPDATE, m_displayDetails.m_ManualUpdate );
	DDX_Control( pDX, IDC_LOOP_ASSET, m_displayDetails.m_LoopAnimation );
	DDX_Control( pDX, IDC_REPEATLASTKEY, m_displayDetails.m_RepeatLastKey );
	DDX_Control( pDX, IDC_ALLOWRESTART, m_displayDetails.m_AllowAnimRestart );
	DDX_Control( pDX, IDC_TIMEWARP, m_displayDetails.m_VTimeWarping );
	DDX_Control( pDX, IDC_IDLE2MOVE, m_displayDetails.m_Idle2Move );
	DDX_Control( pDX, IDC_MOVE2IDLE, m_displayDetails.m_Move2Idle );
	DDX_Control( pDX, IDC_PARTIALBODY, m_displayDetails.m_PartialBodyUpdate );
	//DDX_Control( pDX, IDC_WAITFORANIM, m_displayDetails.m_WaitForAnimation );

	DDX_Control( pDX, IDC_STICKYOUTTIME, m_displayDetails.m_stickyOutTime );
	DDX_Control( pDX, IDC_SPEED_MUL, m_displayDetails.m_speedMultiplier );
	DDX_Control( pDX, IDC_MP_SPEED_MUL, m_displayDetails.m_MPSpeedMultiplier );
	DDX_Control( pDX, IDC_STAYUNTIL, m_displayDetails.m_stayInStateUntil );
	DDX_Control( pDX, IDC_DONT_INTERRUPT, m_displayDetails.m_dontInterrupt );
	//DDX_Control( pDX, IDC_FORCESTAYUNTIL, m_displayDetails.m_forceStayInStateUntil );
	DDX_Control( pDX, IDC_TRANSITIONTIME, m_displayDetails.m_transitionTime );
	DDX_Control( pDX, IDC_KEYTIME, m_displayDetails.m_keyTime );
}

BOOL CAnimationNodeDefaultsPanel::OnInitDialog()
{
	if (__super::OnInitDialog() == FALSE)
		return FALSE;

	return TRUE;
}

void CAnimationNodeDefaultsPanel::ReadFromGraphDefaults( CAnimationGraph2Ptr pAnimGraph )
{
	if (!pAnimGraph)
		return;

	m_pAnimGraph = pAnimGraph;

	// copy the general node details
	m_displayDetails.CopyFromStateDetails(*pAnimGraph->GetAnimationNodeDefaultDetails());

	// copy the animation layer 
	CString layerText = "";
	m_defaultLayer = pAnimGraph->GetAnimationNodeDefaultDetails()->m_animLayer;
	layerText.Format("%i", m_defaultLayer);
	m_layer.SetWindowText(layerText);
}

void CAnimationNodeDefaultsPanel::WriteToGraphDefaults( CAnimationGraph2Ptr pAnimGraph )
{
	if (!pAnimGraph)
		return;

	m_pAnimGraph = pAnimGraph;

	// copy the general node details
	m_displayDetails.CopyToStateDetails(*pAnimGraph->GetAnimationNodeDefaultDetails());

	// copy the animation layer 
	pAnimGraph->GetAnimationNodeDefaultDetails()->m_animLayer = m_defaultLayer;
}

void CAnimationNodeDefaultsPanel::OnLayerChanged()
{
	// get the new layer value
	CString tempString;
	m_layer.GetWindowText(tempString);
	int newLayer = atoi(tempString);

	// if it hasn't changed, nevermind and return
	if (newLayer == m_defaultLayer)
		return;

	// Message Dialog to ask whether the person really really wants to change the layer
	int retVal = AfxMessageBox( "Do you really want to change the default animation layer for all new states?|\nThis can have undesired blending effects if your graph has many states with different layers already set up.", MB_YESNO|MB_ICONWARNING );
	if (retVal == IDYES)
	{
		// ok, the user really really thinks he knows what he is doing
		if (newLayer <= 0 || newLayer >= 10)
		{
			AfxMessageBox("Layer must be between 1 and 9.", MB_OK);
			tempString.Format("%i", m_defaultLayer);
			m_layer.SetWindowText(tempString);
			return;
		}

		m_defaultLayer = newLayer;
	}
}

void CAnimationNodeDefaultsPanel::GrayOutAllElements( bool grayOut )
{
	GetDlgItem(IDC_STATIC5)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC6)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC7)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC8)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC9)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC10)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC12)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC13)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_RESET_BUTTON)->EnableWindow(!grayOut); 

	m_layer.EnableWindow(!grayOut);
	m_displayDetails.m_ensureInStack.EnableWindow(!grayOut);
	m_displayDetails.m_forceLeaveWhenFinished.EnableWindow(!grayOut);
	m_displayDetails.m_stopCurrentAnimation.EnableWindow(!grayOut);
	m_displayDetails.m_interruptCurrAnim.EnableWindow(!grayOut);
	m_displayDetails.m_ManualUpdate.EnableWindow(!grayOut);
	m_displayDetails.m_LoopAnimation.EnableWindow(!grayOut);
	m_displayDetails.m_RepeatLastKey.EnableWindow(!grayOut);
	m_displayDetails.m_AllowAnimRestart.EnableWindow(!grayOut);
	m_displayDetails.m_VTimeWarping.EnableWindow(!grayOut);
	m_displayDetails.m_Idle2Move.EnableWindow(!grayOut);
	m_displayDetails.m_Move2Idle.EnableWindow(!grayOut);
	m_displayDetails.m_PartialBodyUpdate.EnableWindow(!grayOut);
	//m_displayDetails.m_WaitForAnimation.EnableWindow(!grayOut);

	m_displayDetails.m_stickyOutTime.EnableWindow(!grayOut);
	m_displayDetails.m_speedMultiplier.EnableWindow(!grayOut);
	m_displayDetails.m_MPSpeedMultiplier.EnableWindow(!grayOut);
	m_displayDetails.m_stayInStateUntil.EnableWindow(!grayOut);
	m_displayDetails.m_dontInterrupt.EnableWindow(!grayOut);
	//m_displayDetails.m_forceStayInStateUntil.EnableWindow(!grayOut);
	m_displayDetails.m_transitionTime.EnableWindow(!grayOut);
	m_displayDetails.m_keyTime.EnableWindow(!grayOut);
}

void CAnimationNodeDefaultsPanel::ResetToDefaultValues()
{
	if (!m_pAnimGraph)
		return;

	// create the default node details
	SAnimNodeDetails defaultNodeDetails;

	// The Layer will not be reset to avoid accidentally
	// transforming an upper body graph into a FullBody one.
	defaultNodeDetails.m_animLayer = m_defaultLayer;

	m_displayDetails.CopyFromStateDetails(defaultNodeDetails);
}


void CAnimationNodeDefaultsPanel::SDisplayDetails::CopyFromStateDetails( const SAnimNodeDetails& pNodeDetails )
{
	m_ensureInStack.SetCheck(pNodeDetails.ensureInStack);
	m_forceLeaveWhenFinished.SetCheck(pNodeDetails.forceLeaveWhenFinished);
	m_stopCurrentAnimation.SetCheck(pNodeDetails.stopCurrentAnimation);
	m_interruptCurrAnim.SetCheck(pNodeDetails.interruptCurrAnim);
	m_ManualUpdate.SetCheck(pNodeDetails.ManualUpdate);
	m_LoopAnimation.SetCheck(pNodeDetails.LoopAnimation);
	m_RepeatLastKey.SetCheck(pNodeDetails.RepeatLastKey);
	m_AllowAnimRestart.SetCheck(pNodeDetails.AllowAnimRestart);
	m_VTimeWarping.SetCheck(pNodeDetails.VTimeWarping);
	m_Idle2Move.SetCheck(pNodeDetails.Idle2Move);
	m_Move2Idle.SetCheck(pNodeDetails.Move2Idle);
	m_PartialBodyUpdate.SetCheck(pNodeDetails.PartialBodyUpdate);
	m_dontInterrupt.SetCheck(pNodeDetails.dontInterrupt);
	//m_WaitForAnimation.SetCheck(pNodeDetails.WaitForAnimation);

	CString tempString = "";
	tempString.Format("%.2f", pNodeDetails.stickyOutTime);
	m_stickyOutTime.SetWindowText(tempString);
	tempString.Format("%.2f", pNodeDetails.speedMultiplier);
	m_speedMultiplier.SetWindowText(tempString);
	tempString.Format("%.2f", pNodeDetails.MPSpeedMultiplier);
	m_MPSpeedMultiplier.SetWindowText(tempString);
	tempString.Format("%.2f", pNodeDetails.stayInStateUntil);
	m_stayInStateUntil.SetWindowText(tempString);
	//	tempString.Format("%.2f", pNodeDetails.forceInStateUntil);
	//	m_forceStayInStateUntil.SetWindowText(tempString);
	tempString.Format("%.2f", pNodeDetails.transitionTime);
	m_transitionTime.SetWindowText(tempString);
	tempString.Format("%.2f", pNodeDetails.keyTime);
	m_keyTime.SetWindowText(tempString);
}

void CAnimationNodeDefaultsPanel::SDisplayDetails::CopyToStateDetails( SAnimNodeDetails& pNodeDetails ) const
{
	pNodeDetails.ensureInStack = m_ensureInStack.GetCheck();
	pNodeDetails.forceLeaveWhenFinished = m_forceLeaveWhenFinished.GetCheck();
	pNodeDetails.stopCurrentAnimation = m_stopCurrentAnimation.GetCheck();
	pNodeDetails.interruptCurrAnim = m_interruptCurrAnim.GetCheck();
	pNodeDetails.ManualUpdate = m_ManualUpdate.GetCheck();
	pNodeDetails.LoopAnimation = m_LoopAnimation.GetCheck();
	pNodeDetails.RepeatLastKey = m_RepeatLastKey.GetCheck();
	pNodeDetails.AllowAnimRestart = m_AllowAnimRestart.GetCheck();
	pNodeDetails.VTimeWarping = m_VTimeWarping.GetCheck();
	pNodeDetails.Idle2Move = m_Idle2Move.GetCheck();
	pNodeDetails.Move2Idle = m_Move2Idle.GetCheck();
	pNodeDetails.PartialBodyUpdate = m_PartialBodyUpdate.GetCheck();
	pNodeDetails.dontInterrupt = m_dontInterrupt.GetCheck();
	//pNodeDetails.WaitForAnimation = m_WaitForAnimation.GetCheck();

	CString tempStr = "";
	m_stickyOutTime.GetWindowText(tempStr);
	pNodeDetails.stickyOutTime = atof(tempStr);
	m_speedMultiplier.GetWindowText(tempStr);
	pNodeDetails.speedMultiplier = atof(tempStr);
	m_MPSpeedMultiplier.GetWindowText(tempStr);
	pNodeDetails.MPSpeedMultiplier = atof(tempStr);
	m_stayInStateUntil.GetWindowText(tempStr);
	pNodeDetails.stayInStateUntil = atof(tempStr);
	//	m_forceStayInStateUntil.GetWindowText(tempStr);
	//	pNodeDetails.forceInStateUntil = atof(tempStr);
	m_transitionTime.GetWindowText(tempStr);
	pNodeDetails.transitionTime = atof(tempStr);
	m_keyTime.GetWindowText(tempStr);
	pNodeDetails.keyTime = atof(tempStr);
}


//////////////////////////////////////////////////////////////////////////
//		General Tab
//////////////////////////////////////////////////////////////////////////



CGeneralNodeDefaultsPanel::CGeneralNodeDefaultsPanel()
:	CDialog()
{
}

BEGIN_MESSAGE_MAP(CGeneralNodeDefaultsPanel,CDialog)
	ON_BN_CLICKED(IDC_BROWSECHARACTER, OnCharacterBrowse)
	ON_BN_CLICKED(IDC_RESET_BUTTON, ResetToDefaultValues)
END_MESSAGE_MAP()

void CGeneralNodeDefaultsPanel::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control( pDX, IDC_ALLOWSELECTION, m_graphDisplayDetails.m_allowSelect );
	DDX_Control( pDX, IDC_INCLUDEINGAME, m_graphDisplayDetails.m_includeInGame );
	DDX_Control( pDX, IDC_CANMIX, m_graphDisplayDetails.m_canMix );
	DDX_Control( pDX, IDC_CANBEHURRIED, m_graphDisplayDetails.m_bHurryable );
	DDX_Control( pDX, IDC_SKIPFORPLAYERS, m_graphDisplayDetails.m_bSkipFP );
	
	DDX_Control( pDX, IDC_BASE_CHARACTER_FILE, m_characterFileName );
	DDX_Control( pDX, IDC_CHECK_USE_EXP_STATESELECTION, m_useExpensiveStateSelection_Ctrl);
}

BOOL CGeneralNodeDefaultsPanel::OnInitDialog()
{
	if (__super::OnInitDialog() == FALSE)
		return FALSE;

	return TRUE;
}

void CGeneralNodeDefaultsPanel::ReadFromGraphDefaults( CAnimationGraph2Ptr pAnimGraph )
{
	if (!pAnimGraph)
		return;

	m_pAnimGraph = pAnimGraph;

	// copy the general node details
	m_graphDisplayDetails.CopyFromStateDetails(*pAnimGraph->GetNodeDefaultDetails());

	// copy the filename from the base/preview model
	m_characterFileName.SetWindowText(pAnimGraph->GetCharacterName());

	// set whether or not the expensive state locomotion code path is to be used
	m_useExpensiveStateSelection_Ctrl.SetCheck(pAnimGraph->IsUsingExpensiveExpensiveLocoStateSelection());
}

void CGeneralNodeDefaultsPanel::WriteToGraphDefaults( CAnimationGraph2Ptr pAnimGraph )
{
	if (!pAnimGraph)
		return;

	m_pAnimGraph = pAnimGraph;

	// copy the general node details
	m_graphDisplayDetails.CopyToStateDetails(*pAnimGraph->GetNodeDefaultDetails());

	// copy the filename from the base/preview model
	CString fileName;
	m_characterFileName.GetWindowText(fileName);
	pAnimGraph->SetCharacterName(fileName);

	// get whether or not the expensive state locomotion code path is to be used
	pAnimGraph->SetUseExpensiveLocoStateSelection(m_useExpensiveStateSelection_Ctrl.GetCheck());
}

void CGeneralNodeDefaultsPanel::OnCharacterBrowse()
{
	CString filename;
	if (CFileUtil::SelectSingleFile( EFILE_TYPE_ANY, filename, "Character Files (*.cdf)|*.cdf" ))
	{
		// set the new filename
		m_characterFileName.SetWindowText(filename);
	}
}

void CGeneralNodeDefaultsPanel::GrayOutAllElements( bool grayOut )
{
	GetDlgItem(IDC_CHARACTER_TEXT)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_BROWSECHARACTER)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_RESET_BUTTON)->EnableWindow(!grayOut); 

	m_characterFileName.EnableWindow(!grayOut);
	m_useExpensiveStateSelection_Ctrl.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_allowSelect.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_includeInGame.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_bHurryable.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_bSkipFP.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_canMix.EnableWindow(!grayOut);
}

void CGeneralNodeDefaultsPanel::ResetToDefaultValues()
{
	if (!m_pAnimGraph)
		return;

	// create the general default node details
	SGraphNodeDetails defaultDetails;

	m_graphDisplayDetails.CopyFromStateDetails(defaultDetails);
}

void CGeneralNodeDefaultsPanel::SGraphDisplayDetails::CopyFromStateDetails( const SGraphNodeDetails& pNodeDetails )
{
	m_allowSelect.SetCheck(pNodeDetails.m_allowSelect);
	m_includeInGame.SetCheck(pNodeDetails.m_includeInGame);
	m_canMix.SetCheck(pNodeDetails.m_canMix);
	m_bHurryable.SetCheck(pNodeDetails.m_bHurryable);
	m_bSkipFP.SetCheck(pNodeDetails.m_bSkipFP);
}

void CGeneralNodeDefaultsPanel::SGraphDisplayDetails::CopyToStateDetails( SGraphNodeDetails& pNodeDetails ) const
{
	pNodeDetails.m_allowSelect = m_allowSelect.GetCheck();
	pNodeDetails.m_includeInGame = m_includeInGame.GetCheck();
	pNodeDetails.m_canMix = m_canMix.GetCheck();
	pNodeDetails.m_bHurryable = m_bHurryable.GetCheck();
	pNodeDetails.m_bSkipFP = m_bSkipFP.GetCheck();
}

//CFileUtil::SelectSingleFile( EFILE_TYPE_ANY,filename,CHARACTER_FILE_FILTER )


//////////////////////////////////////////////////////////////////////////
//		Movement Tab
//////////////////////////////////////////////////////////////////////////

CMovementNodeDefaultsPanel::CMovementNodeDefaultsPanel()
:	CDialog()
{
}

BOOL CMovementNodeDefaultsPanel::OnInitDialog()
{
	if (__super::OnInitDialog() == FALSE)
		return FALSE;

	return TRUE;
}

BEGIN_MESSAGE_MAP(CMovementNodeDefaultsPanel, CDialog)
	ON_BN_CLICKED(IDC_RESET_BUTTON, ResetToDefaultValues)
END_MESSAGE_MAP()

void CMovementNodeDefaultsPanel::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_DROPDOWN_COLLIDERMODE, m_panelItems.m_colliderMode );

	DDX_Control( pDX, IDC_DROPDOWN_HORIZ_MCM, m_panelItems.m_horizontalMCM );
	DDX_Control( pDX, IDC_DROPDOWN_VERT_MCM, m_panelItems.m_verticalMCM );
	DDX_Control( pDX, IDC_ANGLE_DEVIATION, m_panelItems.m_angle );
	DDX_Control( pDX, IDC_POS_DEVIATION, m_panelItems.m_distance );

}

void CMovementNodeDefaultsPanel::GrayOutAllElements( bool grayOut )
{
	m_panelItems.m_angle.EnableWindow(!grayOut);
	m_panelItems.m_distance.EnableWindow(!grayOut);
	m_panelItems.m_horizontalMCM.EnableWindow(!grayOut);
	m_panelItems.m_verticalMCM.EnableWindow(!grayOut);
	m_panelItems.m_colliderMode.EnableWindow(!grayOut);
	GetDlgItem(IDC_TEXT_ALLOWED_ANGLE)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_TEXT_ALLOWED_DISTANCE)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_TEXT_DEGREES)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_TEXT_METERS)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_RESET_BUTTON)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_TEXT_HORIZ_MCM)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_TEXT_VERT_MCM)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_TEXT_COLLIDER)->EnableWindow(!grayOut); 
}

void CMovementNodeDefaultsPanel::ReadFromGraphDefaults( CAnimationGraph2Ptr pAnimGraph )
{
	if (!pAnimGraph)
		return;

	m_pAnimGraph = pAnimGraph;

	// Add the Graph Default Entry to the lists (with the current graph default for MCMs and Coll)
	CString tempString;
	const SMovementParams* moveDefaultParams = pAnimGraph->GetDefaultMovementDetails();

	m_panelItems.m_verticalMCM.ResetContent();
	m_panelItems.m_horizontalMCM.ResetContent();
	m_panelItems.m_colliderMode.ResetContent();

	// Add all existing MCMs in here
	for (int i = 1; i < eMCM_COUNT; ++i)
	{
		if ((EMovementControlMethod)i < eMCM_AnimationHCollision)
			m_panelItems.m_verticalMCM.AddString(g_szMCMString[i]);
		m_panelItems.m_horizontalMCM.AddString(g_szMCMString[i]);
	}

	// Add all existing Collider Modes in here
	for (int i = 1; i < eColliderMode_COUNT; ++i)
	{
		m_panelItems.m_colliderMode.AddString(g_szColliderModeString[i]);
	}

	// Select the graph default on the dropdown lists
	m_panelItems.m_horizontalMCM.SetCurSel(moveDefaultParams->mcmHoriz - 1);
	m_panelItems.m_verticalMCM.SetCurSel(moveDefaultParams->mcmVert - 1);
	m_panelItems.m_colliderMode.SetCurSel(moveDefaultParams->colliderMode - 1);
	// Set all the values from the state to the panel items
	tempString.Format("%.2f", moveDefaultParams->angle);
	m_panelItems.m_angle.SetWindowText(tempString);
	tempString.Format("%.2f", moveDefaultParams->distance);
	m_panelItems.m_distance.SetWindowText(tempString);
}

void CMovementNodeDefaultsPanel::WriteToGraphDefaults( CAnimationGraph2Ptr pAnimGraph )
{
	if (!pAnimGraph)
		return;

	m_pAnimGraph = pAnimGraph;

	CString angleString, distanceString;
	SMovementParams* moveDefaultParams = m_pAnimGraph->GetDefaultMovementDetails();

	moveDefaultParams->mcmHoriz = (EMovementControlMethod)(m_panelItems.m_horizontalMCM.GetCurSel() + 1);
	moveDefaultParams->mcmVert = (EMovementControlMethod)(m_panelItems.m_verticalMCM.GetCurSel() + 1);
	moveDefaultParams->colliderMode = (EColliderMode)(m_panelItems.m_colliderMode.GetCurSel() + 1);
	m_panelItems.m_angle.GetWindowText(angleString);
	m_panelItems.m_distance.GetWindowText(distanceString);
	moveDefaultParams->angle = atof(angleString);
	moveDefaultParams->distance = atof(distanceString);
}

void CMovementNodeDefaultsPanel::ResetToDefaultValues()
{
	if (!m_pAnimGraph)
		return;

	SMovementParams* moveDefaultParams = m_pAnimGraph->GetDefaultMovementDetails();
	moveDefaultParams->Reset();
	ReadFromGraphDefaults(m_pAnimGraph);
}

