#include "StdAfx.h"
#include "StringDlg.h"
#include "IViewPane.h"
#include "resource.h"
#include "AnimationGraph_2.h"
#include "ICryAnimation.h"
#include "PropertiesPanel.h"
//#include "IAnimationGraph"

#include "AnimationGraph2_AnimationNodeDetails.h"

#define CUSTOM_PRESET_NAME "<Custom>"

IMPLEMENT_DYNCREATE(CAnimationGraphAnimationNodeDetails_General,CDialog);

BEGIN_MESSAGE_MAP(CAnimationGraphAnimationNodeDetails_General,CDialog)
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_EN_KILLFOCUS(IDC_ANIMATION_NAME, OnAnimationNameChanged)
	ON_EN_KILLFOCUS(IDC_NODE_NAME, OnStateNameChanged)
	ON_EN_KILLFOCUS(IDC_AG_ANIM_LAYER, OnLayerChanged)

	ON_BN_CLICKED(IDC_ALLOWRESTART, OnUserChangedValues)
	ON_BN_CLICKED(IDC_ROOTPRIORITY, OnUserChangedValues)
	ON_BN_CLICKED(IDC_ENSURESTACK, OnUserChangedValues)
	ON_BN_CLICKED(IDC_IDLE2MOVE, OnUserChangedValues)
	ON_BN_CLICKED(IDC_FORCELEAVE, OnUserChangedValues)
	ON_BN_CLICKED(IDC_INTERRUPTANIM, OnUserChangedValues)
	ON_BN_CLICKED(IDC_LOOP_ASSET, OnUserChangedValues)
	ON_BN_CLICKED(IDC_MANUAL_UPDATE, OnUserChangedValues)
	ON_BN_CLICKED(IDC_MOVE2IDLE, OnUserChangedValues)
	ON_BN_CLICKED(IDC_PARTIALBODY, OnUserChangedValues)
	ON_BN_CLICKED(IDC_STOPCURRANIM, OnUserChangedValues)
	ON_BN_CLICKED(IDC_TIMEWARP, OnUserChangedValues)
	//ON_BN_CLICKED(IDC_WAITFORANIM, OnUserChangedValues)
	ON_BN_CLICKED(IDC_REPEATLASTKEY, OnUserChangedValues)
	ON_EN_KILLFOCUS(IDC_KEYTIME, OnUserChangedValues)
	ON_EN_KILLFOCUS(IDC_MP_SPEED_MUL, OnUserChangedValues)
	ON_EN_KILLFOCUS(IDC_SPEED_MUL, OnUserChangedValues)
	ON_EN_KILLFOCUS(IDC_TRANSITIONTIME, OnNodeParamsChanged)
	ON_EN_KILLFOCUS(IDC_STICKYOUTTIME, OnUserChangedValues)
	//ON_EN_KILLFOCUS(IDC_FORCESTAYUNTIL, OnUserChangedValues)
	ON_EN_KILLFOCUS(IDC_STAYUNTIL, OnUserChangedValues)
	ON_BN_CLICKED(IDC_DONT_INTERRUPT, OnUserChangedValues)
	ON_CONTROL(WMU_FS_CHANGED, IDC_STAYINSTATE_TIME_SLIDER, OnStayUntilSliderChanged)

	ON_BN_CLICKED(IDC_ALLOWSELECTION, OnUserChangedValues)
	ON_BN_CLICKED(IDC_INCLUDEINGAME, OnNodeParamsChanged)
	ON_BN_CLICKED(IDC_CANMIX, OnNodeParamsChanged)
	ON_BN_CLICKED(IDC_CANBEHURRIED, OnNodeParamsChanged)
	ON_BN_CLICKED(IDC_SKIPFORPLAYERS, OnNodeParamsChanged)

	ON_CBN_SELENDOK(IDC_PRESET_LIST, OnPresetSelected)
	ON_BN_CLICKED(IDC_UN_HIDE_PARAMS, OnUnHideParameters)
END_MESSAGE_MAP()


CAnimationGraphAnimationNodeDetails_General::CAnimationGraphAnimationNodeDetails_General() 
: m_bIgnoreVScroll(false), 
	m_pParamsDeclaration(NULL), 
	m_pAnimNode(NULL)
{
	//UpdateInformationDisplay();
}


void CAnimationGraphAnimationNodeDetails_General::Init()
{
	CRect rc;
	m_MiniPanelWidth = rc.Width();
	m_StateParamsDlgHeight = rc.Height();
	m_pAnimNode = NULL;

	m_animationNameEditBox.SetLimitText(255);

	// Set up the animation progress slider.
	//m_displayDetails.m_stayInStateTimer.Create(TBS_HORZ |TBS_NOTICKS, CRect(0, 0, 20, 16), this, IDC_STAYINSTATE_TIME_SLIDER);
	m_displayDetails.m_stayInStateTimer.SubclassDlgItem( IDC_STAYINSTATE_TIME_SLIDER, this );
	m_displayDetails.m_stayInStateTimer.SetRangeFloat( 0.0f, 1.0f );
	m_displayDetails.m_stayInStateTimer.SetValue(0.0f);
	m_displayDetails.m_stayInStateTimer.EnableWindow(TRUE);
}

void CAnimationGraphAnimationNodeDetails_General::SetParamsDeclaration( CParamsDeclaration2* pParamsDcl )
{
	if ( pParamsDcl == m_pParamsDeclaration || !m_hWnd )
		return;
	m_pParamsDeclaration = pParamsDcl;

	LockWindowUpdate();

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	GetScrollInfo( SB_VERT, &si );
	ScrollWindow( 0, -si.nPos );
	si.nPos = 0;
	si.fMask = SIF_DISABLENOSCROLL | SIF_POS;
	SetScrollInfo( SB_VERT, &si );

	UnlockWindowUpdate();
}


void CAnimationGraphAnimationNodeDetails_General::OnSize(UINT nType, int cx, int cy)
{
	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	int offset = si.nPos;

	__super::OnSize(nType, cx, cy);

	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS;
	GetScrollInfo( SB_VERT, &si );
	offset -= si.nPos;
	ScrollWindow( 0, offset );
}

void CAnimationGraphAnimationNodeDetails_General::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	__super::OnVScroll(nSBCode, nPos, pScrollBar);
	if ( m_bIgnoreVScroll )
		return;

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_ALL;
	GetScrollInfo( SB_VERT, &si );
	int offset = si.nPos;

	switch (nSBCode)
	{
	case SB_BOTTOM: // Scroll to bottom
		si.nPos = si.nMax;
		break;
	case SB_ENDSCROLL: // End scroll
		break;
	case SB_LINEDOWN: // Scroll one line down
		si.nPos += 8;
		break;
	case SB_LINEUP: // Scroll one line up
		si.nPos -= 8;
		break;
	case SB_PAGEDOWN: // Scroll one page down
		si.nPos += si.nPage;
		break;
	case SB_PAGEUP: // Scroll one page up
		si.nPos -= si.nPage;
		break;
	case SB_THUMBPOSITION: // Scroll to the absolute position. The current position is provided in nPos
	case SB_THUMBTRACK: // Drag scroll box to specified position. The current position is provided in nPos
		si.nPos = nPos;
		break;
	case SB_TOP: // Scroll to top
		si.nPos = 0;
		break;
	}

	si.fMask = SIF_POS;
	si.nPos = max(min(si.nPos, si.nMax-(int)si.nPage),0);
	SetScrollInfo( SB_VERT, &si );

	offset -= si.nPos;
	ScrollWindow( 0, offset );
}

void CAnimationGraphAnimationNodeDetails_General::SetActiveAnimNode( CAGState2Ptr animNode )
{
	m_pAnimNode = animNode;
	m_displayParamedNodeDetails.Reset();
	
	if (animNode)
	{
		UpdateInformationDisplay();
	}

	// Show params depending upon whether the current
	// preset is <Custom> or not
	HideParameters( m_presetList.GetCurSel() != 1 );
}

void CAnimationGraphAnimationNodeDetails_General::UpdateInformationDisplay()
{
	// set all the defaults for the most important data
	CString nodeName = "";
	CString animationName = "";
	int layer = -1;

	// overwrite the defaults in case the animNode is existing
	if (m_pAnimNode)
	{
		// general Data
		nodeName = m_pAnimNode->GetName();
		SAnimNodeDetails* pNodeDetails = m_pAnimNode->GetAnimNodeDetails();
		layer = pNodeDetails->m_animLayer;

		// update the pane display with the information from the state
		m_displayDetails.CopyFromStateDetails(*pNodeDetails);

		// update the general details (the same for every graph node type)
		SGraphNodeDetails* pGraphNodeDetails = m_pAnimNode->GetGraphNodeDetails();
		m_graphDisplayDetails.CopyFromStateDetails(*pGraphNodeDetails);

		// per Parameterization Data
		// Animation Name
		animationName = m_pAnimNode->GetAnimationName();

		// if the super state is not selected, gray OUT the stuff that cannot be changed per Parameter, 
		// which at the moment is everything but the animation NAME :)
		GrayOutNonParameterizedData(m_pAnimNode->GetActiveParameterization() != NULL);

		// Set up the presets lists
		m_presetList.ResetContent();
		m_presetList.AddString("Default");
		m_presetList.AddString(CUSTOM_PRESET_NAME);
		// Now add all other presets
		int presetCount = m_pAnimNode->GetGraph()->GetAnimNodePresetCount();
		for (int i = 0; i < presetCount; ++i)
		{
			const SAnimNodeParams* pPreset = m_pAnimNode->GetGraph()->GetAnimNodePreset(i);
			CRY_ASSERT_MESSAGE(pPreset, "Error retrieving animation node preset. Count of presets might not match the actual number of existing presets?");
			// not to cause an editor crash due to such a minor thing, another safety check
			if (pPreset) 
				m_presetList.AddString(pPreset->name);
		}
		// Set active Preset name
		CString presetName = m_pAnimNode->GetNodePresetName();
		int presetIndex = m_presetList.FindStringExact(-1, presetName);
		if (presetIndex < 0)
		{
			presetIndex = 1;  // 1 is always the "<Custom>" one
		}
		m_presetList.SetCurSel(presetIndex);
	}
	else
	{
		// this pane should not exist
		CRY_ASSERT_MESSAGE(m_pAnimNode, "No active state but the panel is active and being updated...");
	}

	// display the values
	CString layerText = "";
	layerText.Format("%i", layer);
	m_layer.SetWindowText(layerText);
	m_stateNameEditBox.SetWindowText(nodeName);
	m_displayParamedNodeDetails.SetAnimationName(animationName);
	m_animationNameEditBox.SetWindowText(animationName);

	UpdateParameterExclusions();
}

void CAnimationGraphAnimationNodeDetails_General::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_NODE_NAME, m_stateNameEditBox );
	DDX_Control( pDX, IDC_ANIMATION_NAME, m_animationNameEditBox );
	DDX_Control( pDX, IDC_AG_ANIM_LAYER, m_layer );

	DDX_Control( pDX, IDC_ROOTPRIORITY, m_displayDetails.m_fullRootPriority );
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
	//DDX_Control( pDX, IDC_FORCESTAYUNTIL, m_displayDetails.m_forceStayInStateUntil );
	DDX_Control( pDX, IDC_DONT_INTERRUPT, m_displayDetails.m_dontInterrupt );
	//DDX_Control( pDX, IDC_STAYINSTATE_TIME_SLIDER, m_displayDetails.m_stayInStateTimer );
	
	DDX_Control( pDX, IDC_TRANSITIONTIME, m_displayDetails.m_transitionTime );
	DDX_Control( pDX, IDC_KEYTIME, m_displayDetails.m_keyTime );

	DDX_Control( pDX, IDC_ALLOWSELECTION, m_graphDisplayDetails.m_allowSelect );
	DDX_Control( pDX, IDC_INCLUDEINGAME, m_graphDisplayDetails.m_includeInGame );
	DDX_Control( pDX, IDC_CANMIX, m_graphDisplayDetails.m_canMix );
	DDX_Control( pDX, IDC_CANBEHURRIED, m_graphDisplayDetails.m_bHurryable );
	DDX_Control( pDX, IDC_SKIPFORPLAYERS, m_graphDisplayDetails.m_bSkipFP );

	DDX_Control( pDX, IDC_PRESET_LIST, m_presetList );
	DDX_Control( pDX, IDC_UN_HIDE_PARAMS, m_UnHideParameters );
}

void CAnimationGraphAnimationNodeDetails_General::OnAnimationNameChanged()
{
	if (!m_pAnimNode)
		return;

	CString newAnimName = "";
	m_animationNameEditBox.GetWindowText(newAnimName);
	if (m_displayParamedNodeDetails.GetAnimationName().Compare(newAnimName) == 0)
	{
		// no change in the actual content
		return;
	}

	// set the animation name to the correct parameterization
	// TODO: Extract into SetAnimationName method which automatically
	//				updates
	if (!m_pAnimNode->IsParameterized() || !m_pAnimNode->GetActivePerParamDetails())
	{
		m_pAnimNode->GetUnparamedPerParamDetails()->SetAnimationName(newAnimName);
	}
	else
	{
		m_pAnimNode->GetActivePerParamDetails()->SetAnimationName(newAnimName);
	}

}

void CAnimationGraphAnimationNodeDetails_General::OnStateNameChanged()
{
	if (!m_pAnimNode)
		return;

	CString newStateName = "";
	CString oldStateName = m_pAnimNode->GetName();
	m_stateNameEditBox.GetWindowText(newStateName);

	if (newStateName.Compare(oldStateName) == 0)
	{
		// no change in the actual content
		return;
	}

	m_pAnimNode->SetName(newStateName);

	// update the panel, because setting the new name might actually fail...
	UpdateInformationDisplay();
}

void CAnimationGraphAnimationNodeDetails_General::OnDetailsChanged( bool switchToCustomPreset /*= true*/ )
{
	if (!m_pAnimNode)
		return;

	SAnimNodeDetails* pNodeDetails = m_pAnimNode->GetAnimNodeDetails();
	SGraphNodeDetails* pGraphNodeDetails = m_pAnimNode->GetGraphNodeDetails();

	// read out all the updated data and write it back to the state
	m_displayDetails.CopyToStateDetails(*pNodeDetails);
	m_graphDisplayDetails.CopyToStateDetails(*pGraphNodeDetails);
	
	// Update slider from edit box
	CString tempString;
	m_displayDetails.m_stayInStateUntil.GetWindowText(tempString);
	float timeVal = atof(tempString);
	m_displayDetails.m_stayInStateTimer.SetValue(timeVal);

	// if this function call came from a user made input (and not preset changes)
	// switch to the custom preset
	if (switchToCustomPreset)
	{
		m_presetList.SetCurSel(1);
	}

	// and last, but not least, gray out currently non-usable parameters
	UpdateParameterExclusions();
}

void CAnimationGraphAnimationNodeDetails_General::OnLayerChanged()
{
	// Message Dialog to ask whether the person really really wants to change the layer
	int retVal = AfxMessageBox( "Do you really want to change the layer?|\nThis can have undesired blending effects.", MB_YESNO|MB_ICONWARNING );
	if (retVal == IDYES)
	{
		// ok, the user really really thinks he knows what he is doing
		SAnimNodeDetails* pNodeDetails = m_pAnimNode->GetAnimNodeDetails();
		CString tempString;
		m_layer.GetWindowText(tempString);
		int newLayer = atoi(tempString);
		if (newLayer <= 0 || newLayer >= 10)
		{
			AfxMessageBox("Layer must be between 1 and 9.", MB_OK);
			tempString.Format("%i", pNodeDetails->m_animLayer);
			m_layer.SetWindowText(tempString);
			return;
		}

		pNodeDetails->m_animLayer = newLayer;
	}
	else
	{
		// reset the box to the previous layer
		CString tempString;
		tempString.Format("%i", m_pAnimNode->GetAnimNodeDetails()->m_animLayer);
		m_layer.SetWindowText(tempString);
	}
}

void CAnimationGraphAnimationNodeDetails_General::GrayOutNonParameterizedData( bool grayOut )
{

	m_stateNameEditBox.EnableWindow(!grayOut);
	m_layer.EnableWindow(!grayOut);

	GetDlgItem(IDC_NODE_NAME_TEXT)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC5)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC6)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC7)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC8)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC9)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC10)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_TRANSITION_TIME_TEXT)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_STATIC13)->EnableWindow(!grayOut); 
	GetDlgItem(IDC_PRESET_TEXT)->EnableWindow(!grayOut); 

	m_displayDetails.m_fullRootPriority.EnableWindow(!grayOut);
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
	m_displayDetails.m_stayInStateTimer.EnableWindow(!grayOut);
	//m_displayDetails.m_forceStayInStateUntil.EnableWindow(!grayOut);
	m_displayDetails.m_transitionTime.EnableWindow(!grayOut);
	m_displayDetails.m_keyTime.EnableWindow(!grayOut);

	m_graphDisplayDetails.m_allowSelect.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_includeInGame.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_bHurryable.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_bSkipFP.EnableWindow(!grayOut);
	m_graphDisplayDetails.m_canMix.EnableWindow(!grayOut);

	m_presetList.EnableWindow(!grayOut);
}

void CAnimationGraphAnimationNodeDetails_General::OnStayUntilSliderChanged()
{
	// Update edit box with new time
	float timeVal = m_displayDetails.m_stayInStateTimer.GetValue();
	CString tempString;
	tempString.Format("%.2f", timeVal);
	m_displayDetails.m_stayInStateUntil.SetWindowText(tempString);
	OnDetailsChanged();
}

void CAnimationGraphAnimationNodeDetails_General::OnPresetSelected()
{
	// get the current selection from the dropdown box
	// if it has been set to custom, don't do anything to the values
	// if it has been set to Default, copy from graph defaults
	// in all other cases find the preset with the correct name
	// and copy the values (not the animation layer obviously)

	int activeSel = m_presetList.GetCurSel();
	
	if (activeSel == 1) // "<Custom>" settings that don't change anything
	{
		// Set the name of the active preset to "<Custom>"
		m_pAnimNode->SetPresetName(CUSTOM_PRESET_NAME);

		// automatically unhide the parameters
		HideParameters( false );
		return;
	}

	// First retrieve the correct setting to copy from depending on selection
	// then in a second step copy them over
	const SGraphNodeDetails* generalParams;
	const SAnimNodeDetails*	animParams;
	if (activeSel == 0) // "Default" settings for new states
	{
		generalParams = m_pAnimNode->GetGraph()->GetNodeDefaultDetails();
		animParams		= m_pAnimNode->GetGraphDefaultAnimNodeDetails();
		m_pAnimNode->SetPresetName("Default");
	}
	else // an actual preset
	{
		activeSel -= 2; // remove "Default" and "<Custom>" entries

		const SAnimNodeParams* pPreset = m_pAnimNode->GetGraph()->GetAnimNodePreset(activeSel);
		if (!pPreset)
		{
			CRY_ASSERT_MESSAGE(pPreset, "Animation Graph 2.0 Node Preset could not be found!");
			AfxMessageBox("Could not find preset with this id", MB_OK | MB_ICONWARNING);
			m_presetList.SetCurSel(1); // for safety set to custom
			m_pAnimNode->SetPresetName(CUSTOM_PRESET_NAME);
			return;
		}

		generalParams = &pPreset->nodeDetails;
		animParams		= &pPreset->animDetails;
		m_pAnimNode->SetPresetName(pPreset->name);
	}
	m_graphDisplayDetails.CopyFromStateDetails(*generalParams);
	m_displayDetails.CopyFromStateDetails(*animParams);

	// and save this stuff, but do not switch to custom preset (hence the "false" parameter)
	OnDetailsChanged( false );
}

void CAnimationGraphAnimationNodeDetails_General::OnUserChangedValues()
{
	// update everything and switch to custom preset
	OnDetailsChanged( true );
}

void CAnimationGraphAnimationNodeDetails_General::OnNodeParamsChanged()
{
	// only non- animation/selection/flag/flowcontrol values changed
	// update everything and do NOT switch to custom preset
	OnDetailsChanged( false );
}

void CAnimationGraphAnimationNodeDetails_General::UpdateParameterExclusions()
{
	// Looping invalidates RepeatLastKey
	if (m_displayDetails.m_LoopAnimation.GetCheck())
	{
		m_displayDetails.m_RepeatLastKey.SetCheck(0);
		m_displayDetails.m_RepeatLastKey.EnableWindow(false);
	}
	else
	{
		m_displayDetails.m_RepeatLastKey.EnableWindow(true);
	}

	// Stop Current Animation invalidates Transition Time
	if (m_displayDetails.m_stopCurrentAnimation.GetCheck())
	{
		m_displayDetails.m_transitionTime.SetWindowText("0.00");
		m_displayDetails.m_transitionTime.EnableWindow(false);
		GetDlgItem(IDC_TRANSITION_TIME_TEXT)->EnableWindow(false); 
	}
	else
	{
		m_displayDetails.m_transitionTime.EnableWindow(true);
		GetDlgItem(IDC_TRANSITION_TIME_TEXT)->EnableWindow(true); 
	}
}

void CAnimationGraphAnimationNodeDetails_General::OnUnHideParameters()
{
	HideParameters(!m_ParametersAreHidden);
}

void CAnimationGraphAnimationNodeDetails_General::HideParameters( bool hide )
{
	// Unhide/Hide all of them
	m_layer.ShowWindow(!hide);

	//GetDlgItem(IDC_STATIC5)->ShowWindow(!hide); 
	GetDlgItem(IDC_STATIC6)->ShowWindow(!hide); 
	GetDlgItem(IDC_STATIC7)->ShowWindow(!hide); 
	GetDlgItem(IDC_STATIC8)->ShowWindow(!hide); 
	//GetDlgItem(IDC_STATIC10)->ShowWindow(!hide); 
	GetDlgItem(IDC_TRANSITION_TIME_TEXT)->ShowWindow(!hide); 
	GetDlgItem(IDC_STATIC13)->ShowWindow(!hide); 
	GetDlgItem(IDC_COMMON_BOX)->ShowWindow(!hide); 
	GetDlgItem(IDC_ADVANCED_BOX)->ShowWindow(!hide); 

	m_displayDetails.m_stopCurrentAnimation.ShowWindow(!hide);
	m_displayDetails.m_ManualUpdate.ShowWindow(!hide);
	m_displayDetails.m_LoopAnimation.ShowWindow(!hide);
	m_displayDetails.m_RepeatLastKey.ShowWindow(!hide);
	m_displayDetails.m_AllowAnimRestart.ShowWindow(!hide);
	m_displayDetails.m_VTimeWarping.ShowWindow(!hide);
	m_displayDetails.m_Idle2Move.ShowWindow(!hide);
	m_displayDetails.m_Move2Idle.ShowWindow(!hide);
	m_displayDetails.m_PartialBodyUpdate.ShowWindow(!hide);
	m_displayDetails.m_fullRootPriority.ShowWindow(!hide);
	m_displayDetails.m_ensureInStack.ShowWindow(!hide);
	m_displayDetails.m_forceLeaveWhenFinished.ShowWindow(!hide);

	m_displayDetails.m_speedMultiplier.ShowWindow(!hide);
	m_displayDetails.m_MPSpeedMultiplier.ShowWindow(!hide);
	m_displayDetails.m_stayInStateUntil.ShowWindow(!hide);
	m_displayDetails.m_dontInterrupt.ShowWindow(!hide);
	m_displayDetails.m_stayInStateTimer.ShowWindow(!hide);
	m_displayDetails.m_transitionTime.ShowWindow(!hide);

	// Set the button text
	if (hide)
	{
		m_UnHideParameters.SetWindowText("Unhide Parameters");
	}
	else
	{
		m_UnHideParameters.SetWindowText("Hide Parameters");
	}

	m_ParametersAreHidden = hide;

}

void CAnimationGraphAnimationNodeDetails_General::SDisplayDetails::CopyFromStateDetails( const SAnimNodeDetails& pNodeDetails )
{
	m_fullRootPriority.SetCheck(pNodeDetails.fullRootPriority);
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
	tempString.Format("%.2f", pNodeDetails.transitionTime);
	m_transitionTime.SetWindowText(tempString);
	tempString.Format("%.2f", pNodeDetails.keyTime);
	m_keyTime.SetWindowText(tempString);

//	tempString.Format("%.2f", pNodeDetails.forceInStateUntil);
//	m_forceStayInStateUntil.SetWindowText(tempString);
	tempString.Format("%.2f", pNodeDetails.stayInStateUntil);
	m_stayInStateUntil.SetWindowText(tempString);
	// Update slider from edit box
	m_stayInStateUntil.GetWindowText(tempString);
	float timeVal = atof(tempString);
	m_stayInStateTimer.SetValue(timeVal);

	if (pNodeDetails.stopCurrentAnimation)
	{
		// visualize that this means that there will be no transition time
		m_transitionTime.SetWindowText("0.00");
	}
}

void CAnimationGraphAnimationNodeDetails_General::SDisplayDetails::CopyToStateDetails( SAnimNodeDetails& pNodeDetails ) 
{
	pNodeDetails.fullRootPriority = m_fullRootPriority.GetCheck();
	pNodeDetails.ensureInStack = m_ensureInStack.GetCheck();
	pNodeDetails.forceLeaveWhenFinished = m_forceLeaveWhenFinished.GetCheck();
	pNodeDetails.stopCurrentAnimation = m_stopCurrentAnimation.GetCheck();
	if (pNodeDetails.stopCurrentAnimation)
	{
		// visualize that this means that there will be no transition time
		CString tempStr = "0.00";
		m_transitionTime.SetWindowText(tempStr);
	}
		
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

void CAnimationGraphAnimationNodeDetails_General::SGraphDisplayDetails::CopyFromStateDetails( const SGraphNodeDetails& pNodeDetails )
{
	m_allowSelect.SetCheck(pNodeDetails.m_allowSelect);
	m_includeInGame.SetCheck(pNodeDetails.m_includeInGame);
	m_canMix.SetCheck(pNodeDetails.m_canMix);
	m_bHurryable.SetCheck(pNodeDetails.m_bHurryable);
	m_bSkipFP.SetCheck(pNodeDetails.m_bSkipFP);
}

void CAnimationGraphAnimationNodeDetails_General::SGraphDisplayDetails::CopyToStateDetails( SGraphNodeDetails& pNodeDetails ) const
{
	pNodeDetails.m_allowSelect = m_allowSelect.GetCheck();
	pNodeDetails.m_includeInGame = m_includeInGame.GetCheck();
	pNodeDetails.m_canMix = m_canMix.GetCheck();
	pNodeDetails.m_bHurryable = m_bHurryable.GetCheck();
	pNodeDetails.m_bSkipFP = m_bSkipFP.GetCheck();
}

//////////////////////////////////////////////////////////////////////////
// The Tab Selector
//////////////////////////////////////////////////////////////////////////


CAnimationGraphAnimationNodeDetails::CAnimationGraphAnimationNodeDetails()
:	CDialog(IDD_AG2_GRAPH_DEFAULTS_DIALOG)
{
	m_pAnimGraph = NULL;
	m_currSelectedTab = -1;
	m_animationTab = new CAnimationGraphAnimationNodeDetails_General();
	m_movementTab = new CAnimationGraphAnimationNodeDetails_MCMColl();
}

void CAnimationGraphAnimationNodeDetails::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control( pDX, IDC_DETAILS_TAB, m_dialogTabCtrl ); 
}

IMPLEMENT_DYNCREATE(CAnimationGraphAnimationNodeDetails,CDialog);

BEGIN_MESSAGE_MAP(CAnimationGraphAnimationNodeDetails, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_DETAILS_TAB, &CAnimationGraphAnimationNodeDetails::OnActiveTabChanged)
END_MESSAGE_MAP()


void CAnimationGraphAnimationNodeDetails::Init()
{
	m_dialogTabCtrl.InsertItem(0, _T("Animation"));
	m_dialogTabCtrl.InsertItem(1, _T("Movement and Physics"));
	m_dialogTabCtrl.SetCurSel(0);

	m_dialogTabCtrl.GetClientRect(&m_ClientRect);
	m_dialogTabCtrl.AdjustRect(FALSE, &m_ClientRect);
	CRect WindowRect;
	m_dialogTabCtrl.GetWindowRect(&WindowRect);
	m_dialogTabCtrl.GetParent()->ScreenToClient(WindowRect);

	m_animationTab->Create(IDD_AG2_ANIM_NODE_DETAILS_GENERAL, &m_dialogTabCtrl);
	m_animationTab->Init();
	m_animationTab->SetActiveAnimNode(m_pAnimNode);
	//m_animationTab->MoveWindow(m_ClientRect);

	m_movementTab->Create(IDD_AG2_ANIM_NODE_DETAILS_MVMTCTRL, &m_dialogTabCtrl);
	m_movementTab->Init();
	m_movementTab->SetActiveAnimNode(m_pAnimNode);
	//m_movementTab->MoveWindow(m_ClientRect);

	m_currSelectedTab = -1;
	OnSelectionChange();

	DisableAllTabs();
// 	ShowWindow( SW_HIDE );
// 	EnableWindow(false);
 	m_dialogTabCtrl.ShowWindow(SW_HIDE);
	GetParent()->RedrawWindow();

}

void CAnimationGraphAnimationNodeDetails::OnSelectionChange()
{
	int newSelTab = m_dialogTabCtrl.GetCurSel();
	if (m_currSelectedTab == newSelTab)
		return;

	DisableAllTabs();

	// enable the newly selected one
	switch (newSelTab)
	{
	case 0: 
		m_animationTab->ShowWindow(SW_SHOW);
		m_animationTab->EnableWindow(true);
		break;
	case 1: 
		m_movementTab->ShowWindow(SW_SHOW);
		m_movementTab->EnableWindow(true);
		break;
	default:
		CRY_ASSERT_MESSAGE(0, "This tab does not have a dialog attached to it... pleae add code here");
		break;
	}

	m_currSelectedTab = newSelTab;
}

void CAnimationGraphAnimationNodeDetails::OnActiveTabChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnSelectionChange();
	*pResult = 0;
}

void CAnimationGraphAnimationNodeDetails::SetAnimationGraph( CAnimationGraph2Ptr pAnimGraph )
{
	m_pAnimGraph = pAnimGraph;
}

void CAnimationGraphAnimationNodeDetails::SetActiveAnimNode( CAGState2Ptr animNode )
{
	if (animNode)
	{
		ShowWindow( SW_SHOW );
		m_dialogTabCtrl.ShowWindow( SW_SHOW );
	}
	else
		ShowWindow( SW_HIDE );

	if (m_pAnimNode == animNode)
	{
		UpdateInformationDisplay();
	}
	else
	{
		m_pAnimNode = animNode;
		m_animationTab->SetActiveAnimNode(animNode);
		m_movementTab->SetActiveAnimNode(animNode);
		m_currSelectedTab = -1;
		m_dialogTabCtrl.SetCurSel(0);
		OnSelectionChange();
		GetParent()->RedrawWindow();	
	}
}

void CAnimationGraphAnimationNodeDetails::SetParamsDeclaration( CParamsDeclaration2* pParamsDcl )
{
	if ( pParamsDcl == m_pParamsDeclaration || !m_hWnd )
		return;
	m_pParamsDeclaration = pParamsDcl;

	m_animationTab->SetParamsDeclaration(pParamsDcl);
}

void CAnimationGraphAnimationNodeDetails::UpdateInformationDisplay()
{
	m_animationTab->UpdateInformationDisplay();
	m_movementTab->OnChangedParameterization();
}

void CAnimationGraphAnimationNodeDetails::DisableAllTabs()
{
	// disable all
	m_animationTab->EnableWindow(false);
	m_animationTab->ShowWindow(SW_HIDE);
	m_movementTab->EnableWindow(false);
	m_movementTab->ShowWindow(SW_HIDE);
}


//////////////////////////////////////////////////////////////////////////
//     Movement Control and Collider Mode Panel
//////////////////////////////////////////////////////////////////////////


CAnimationGraphAnimationNodeDetails_MCMColl::CAnimationGraphAnimationNodeDetails_MCMColl()
:	CDialog()
{
	m_pAnimNode = NULL;
}

BEGIN_MESSAGE_MAP(CAnimationGraphAnimationNodeDetails_MCMColl,CDialog)
	ON_CBN_SELENDOK(IDC_DROPDOWN_COLLIDERMODE, OnColliderModeSelected)
	ON_CBN_SELENDOK(IDC_DROPDOWN_HORIZ_MCM, OnMCMChanged)
	ON_CBN_SELENDOK(IDC_DROPDOWN_VERT_MCM, OnMCMChanged)
	ON_EN_KILLFOCUS(IDC_ANGLE_DEVIATION, OnAngleOrDistanceChanged)
	ON_EN_KILLFOCUS(IDC_POS_DEVIATION, OnAngleOrDistanceChanged)
END_MESSAGE_MAP()

void CAnimationGraphAnimationNodeDetails_MCMColl::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control( pDX, IDC_DROPDOWN_COLLIDERMODE, m_panelItems.m_colliderMode );

	DDX_Control( pDX, IDC_DROPDOWN_HORIZ_MCM, m_panelItems.m_horizontalMCM );
	DDX_Control( pDX, IDC_DROPDOWN_VERT_MCM, m_panelItems.m_verticalMCM );
	DDX_Control( pDX, IDC_ANGLE_DEVIATION, m_panelItems.m_angle );
	DDX_Control( pDX, IDC_POS_DEVIATION, m_panelItems.m_distance );

}

void CAnimationGraphAnimationNodeDetails_MCMColl::Init()
{
	if (!m_pAnimNode)
		return;

	// Add the Graph Default Entry to the lists (with the current graph default for MCMs and Coll)
	CString tempString;
	const SMovementParams* moveDefaultParams = m_pAnimNode->GetGraphDefaultMovementDetails();

	m_panelItems.m_verticalMCM.ResetContent();
	m_panelItems.m_horizontalMCM.ResetContent();
	m_panelItems.m_colliderMode.ResetContent();

	tempString.Format("Use Graph Default (%s)", g_szMCMString[moveDefaultParams->mcmVert]);
	m_panelItems.m_verticalMCM.AddString(tempString);
	tempString.Format("Use Graph Default (%s)", g_szMCMString[moveDefaultParams->mcmHoriz]);
	m_panelItems.m_horizontalMCM.AddString(tempString);

	// Add all existing MCMs in here
	for (int i = 1; i < eMCM_COUNT; ++i)
	{
		if ((EMovementControlMethod)i < eMCM_AnimationHCollision)
			m_panelItems.m_verticalMCM.AddString(g_szMCMString[i]);
		m_panelItems.m_horizontalMCM.AddString(g_szMCMString[i]);
	}

	tempString.Format("Use Graph Default (%s)", g_szColliderModeString[moveDefaultParams->colliderMode]);
	m_panelItems.m_colliderMode.AddString(tempString);

	// Add all existing Collider Modes in here
	for (int i = 1; i < eColliderMode_COUNT; ++i)
	{
		m_panelItems.m_colliderMode.AddString(g_szColliderModeString[i]);
	}

	// Select either the graph default or the ones this state has
	SMovementParams* nodeMoveParams = m_pAnimNode->GetMovementNodeDetails();

	// Set all the values from the state to the panel items
	m_panelItems.m_horizontalMCM.SetCurSel(nodeMoveParams->mcmHoriz);
	m_panelItems.m_verticalMCM.SetCurSel(nodeMoveParams->mcmVert);
	m_panelItems.m_colliderMode.SetCurSel(nodeMoveParams->colliderMode);
	tempString.Format("%.2f", nodeMoveParams->angle);
	m_panelItems.m_angle.SetWindowText(tempString);
	tempString.Format("%.2f", nodeMoveParams->distance);
	m_panelItems.m_distance.SetWindowText(tempString);

	// Override in case the graph defaults are to be used
	if (nodeMoveParams->useHorizGraphDefault)
	{
		m_panelItems.m_horizontalMCM.SetCurSel(0);
	}

	if (nodeMoveParams->useVertGraphDefault)
	{
		m_panelItems.m_verticalMCM.SetCurSel(0);
	}

	if (nodeMoveParams->useGraphDefaultCollider)
	{
		m_panelItems.m_colliderMode.SetCurSel(0);
	}

	UpdateAngleAndPositionOverride();

	//GrayOutIfNotDecoupled();

	OnChangedParameterization();
}

void CAnimationGraphAnimationNodeDetails_MCMColl::SetActiveAnimNode( CAGState2Ptr animNode )
{
	m_pAnimNode = animNode;
	Init();
}

void CAnimationGraphAnimationNodeDetails_MCMColl::GrayOutIfNotDecoupled()
{
	// Check if any decoupled method is selected (graph default does NOT count)
	bool isDecoupled = ((EMovementControlMethod)m_panelItems.m_horizontalMCM.GetCurSel() == eMCM_DecoupledCatchUp) 
		|| ((EMovementControlMethod)m_panelItems.m_verticalMCM.GetCurSel() == eMCM_DecoupledCatchUp);


	// if the graph default decoupled is selected, always gray out, because that overrides
	// the values anyway
	const SMovementParams* moveDefaultParams = m_pAnimNode->GetGraphDefaultMovementDetails();
	SMovementParams* nodeMoveParams = m_pAnimNode->GetMovementNodeDetails();

	// Update the angle/position deviation
	bool isDefaultDecoupled = false;
	if (nodeMoveParams->useHorizGraphDefault && (EMovementControlMethod)moveDefaultParams->mcmHoriz == eMCM_DecoupledCatchUp)
		isDefaultDecoupled = true;
	if (nodeMoveParams->useVertGraphDefault && (EMovementControlMethod)moveDefaultParams->mcmVert == eMCM_DecoupledCatchUp)
		isDefaultDecoupled = true;

	if (isDecoupled && !isDefaultDecoupled)
	{
		m_panelItems.m_distance.EnableWindow(true);
		m_panelItems.m_angle.EnableWindow(true);
		GetDlgItem(IDC_TEXT_ALLOWED_ANGLE)->EnableWindow(true); 
		GetDlgItem(IDC_TEXT_ALLOWED_DISTANCE)->EnableWindow(true); 
		GetDlgItem(IDC_TEXT_DEGREES)->EnableWindow(true); 
		GetDlgItem(IDC_TEXT_METERS)->EnableWindow(true); 
	}
	else
	{
		m_panelItems.m_distance.EnableWindow(false);
		m_panelItems.m_angle.EnableWindow(false);
		GetDlgItem(IDC_TEXT_ALLOWED_ANGLE)->EnableWindow(false); 
		GetDlgItem(IDC_TEXT_ALLOWED_DISTANCE)->EnableWindow(false); 
		GetDlgItem(IDC_TEXT_DEGREES)->EnableWindow(false); 
		GetDlgItem(IDC_TEXT_METERS)->EnableWindow(false); 
	}
}

void CAnimationGraphAnimationNodeDetails_MCMColl::OnMCMChanged()
{
	// Get the currently selected MCMs from the two dropdown lists
	int horizMCM = m_panelItems.m_horizontalMCM.GetCurSel();
	int vertMCM = m_panelItems.m_verticalMCM.GetCurSel();

	// write them back to the state
	SMovementParams* nodeMoveParams = m_pAnimNode->GetMovementNodeDetails();

	if (horizMCM == 0)
	{
		nodeMoveParams->useHorizGraphDefault = true;
	}
	else
	{
		nodeMoveParams->useHorizGraphDefault = false;
		nodeMoveParams->mcmHoriz = (EMovementControlMethod)horizMCM;
	}

	if (vertMCM == 0)
	{
		nodeMoveParams->useVertGraphDefault = true;
	}
	else
	{
		nodeMoveParams->useVertGraphDefault = false;
		nodeMoveParams->mcmVert = (EMovementControlMethod)vertMCM;
	}

	// if a default was chosen, and the default was decoupled
	// the angle and distance values must be overridden
	UpdateAngleAndPositionOverride();

	// Lastly check if things need to be grayed out or be re-enabled
	GrayOutIfNotDecoupled();
}

void CAnimationGraphAnimationNodeDetails_MCMColl::UpdateAngleAndPositionOverride()
{
	CString tempString;
	const SMovementParams* moveDefaultParams = m_pAnimNode->GetGraphDefaultMovementDetails();
	SMovementParams* nodeMoveParams = m_pAnimNode->GetMovementNodeDetails();

	// Update the angle/position deviation
	bool isDefaultDecoupled = false;
	if (nodeMoveParams->useHorizGraphDefault && (EMovementControlMethod)moveDefaultParams->mcmHoriz == eMCM_DecoupledCatchUp)
		isDefaultDecoupled = true;
	if (nodeMoveParams->useVertGraphDefault && (EMovementControlMethod)moveDefaultParams->mcmVert == eMCM_DecoupledCatchUp)
		isDefaultDecoupled = true;

	if (isDefaultDecoupled)
	{
		tempString.Format("%.2f", moveDefaultParams->angle);
		m_panelItems.m_angle.SetWindowText(tempString);
		tempString.Format("%.2f", moveDefaultParams->distance);
		m_panelItems.m_distance.SetWindowText(tempString);

		// and copy them into the state
		nodeMoveParams->angle = moveDefaultParams->angle;
		nodeMoveParams->distance = moveDefaultParams->distance;
	}

}

void CAnimationGraphAnimationNodeDetails_MCMColl::OnAngleOrDistanceChanged()
{
	// Get angle and distance
	CString tempString;
	m_panelItems.m_angle.GetWindowText(tempString);
	float angle = atof(tempString);
	m_panelItems.m_distance.GetWindowText(tempString);
	float distance = atof(tempString);

	// Write them back to the node
	SMovementParams* nodeMoveParams = m_pAnimNode->GetMovementNodeDetails();
	nodeMoveParams->angle = angle;
	nodeMoveParams->distance = distance;
}

void CAnimationGraphAnimationNodeDetails_MCMColl::OnChangedParameterization()
{
	bool isNotParameterized = m_pAnimNode->GetActiveParameterization() == NULL;
	
	m_panelItems.m_angle.EnableWindow(isNotParameterized);
	m_panelItems.m_distance.EnableWindow(isNotParameterized);
	m_panelItems.m_horizontalMCM.EnableWindow(isNotParameterized);
	m_panelItems.m_verticalMCM.EnableWindow(isNotParameterized);
	m_panelItems.m_colliderMode.EnableWindow(isNotParameterized);
	GetDlgItem(IDC_TEXT_ALLOWED_ANGLE)->EnableWindow(isNotParameterized); 
	GetDlgItem(IDC_TEXT_ALLOWED_DISTANCE)->EnableWindow(isNotParameterized); 
	GetDlgItem(IDC_TEXT_DEGREES)->EnableWindow(isNotParameterized); 
	GetDlgItem(IDC_TEXT_METERS)->EnableWindow(isNotParameterized); 

	this->EnableWindow(isNotParameterized);

	// Some stuff should be disabled even if the rest is on...
	if (isNotParameterized)
	{
		GrayOutIfNotDecoupled(); 
	}
}

void CAnimationGraphAnimationNodeDetails_MCMColl::OnColliderModeSelected()
{
	// Get Selected Collider Mode
	int collMode = m_panelItems.m_colliderMode.GetCurSel();

	// Write back to state
	SMovementParams* nodeMoveParams = m_pAnimNode->GetMovementNodeDetails();

	if (collMode == 0)
	{
		nodeMoveParams->useGraphDefaultCollider = true;
	}
	else
	{
		nodeMoveParams->useGraphDefaultCollider = false;
		nodeMoveParams->colliderMode = (EColliderMode)collMode;
	}
}

/*

void CAnimationGraphAnimationNodeDetails_MCMColl::SGraphDisplayDetails::CopyFromStateDetails( const SGraphNodeDetails& pNodeDetails )
{
	m_allowSelect.SetCheck(pNodeDetails.m_allowSelect);
	m_includeInGame.SetCheck(pNodeDetails.m_includeInGame);
	m_canMix.SetCheck(pNodeDetails.m_canMix);
	m_bHurryable.SetCheck(pNodeDetails.m_bHurryable);
	m_bSkipFP.SetCheck(pNodeDetails.m_bSkipFP);
}

void CAnimationGraphAnimationNodeDetails_MCMColl::SGraphDisplayDetails::CopyToStateDetails( SGraphNodeDetails& pNodeDetails ) const
{
	pNodeDetails.m_allowSelect = m_allowSelect.GetCheck();
	pNodeDetails.m_includeInGame = m_includeInGame.GetCheck();
	pNodeDetails.m_canMix = m_canMix.GetCheck();
	pNodeDetails.m_bHurryable = m_bHurryable.GetCheck();
	pNodeDetails.m_bSkipFP = m_bSkipFP.GetCheck();
}*/

