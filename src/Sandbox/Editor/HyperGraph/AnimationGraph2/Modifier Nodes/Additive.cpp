#include "StdAfx.h"
#include "Additive.h"

static const int DEFAULT_LAYER_FOR_ADDITIVES = 4;

// try having this in synch with the one in SkeletonAnim (which cannot be included here)
static const int numVIRTUALLAYERS = (0x10);


CAG2Modifier_Additive::CAG2Modifier_Additive() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_Additive::~CAG2Modifier_Additive()
{
}

CAG2ModifierBase* CAG2Modifier_Additive::Duplicate() const
{
	return new CAG2Modifier_Additive();
}


void CAG2Modifier_Additive::Reset()
{
	m_playMode = PLAY_ONCE;
	m_fadeOutOneLeaving = true;
	m_layer = DEFAULT_LAYER_FOR_ADDITIVES;
	m_fadeOutTime = 1.5f;
	m_blendInTime = 1.2f;
	m_assetWeight = 1.0f;
	m_assetSpeed = 1.0f;
	m_assetName.Empty();
}

const void CAG2Modifier_Additive::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("playMode", (int)m_playMode);
	modifierNode->setAttr("fadeOutOneLeaving", m_fadeOutOneLeaving);
	modifierNode->setAttr("layer", m_layer);
	modifierNode->setAttr("fadeOutTime", m_fadeOutTime);
	modifierNode->setAttr("blendInTime", m_blendInTime);
	modifierNode->setAttr("assetWeight", m_assetWeight);
	modifierNode->setAttr("assetSpeed", m_assetSpeed);
	modifierNode->setAttr("assetName", m_assetName);
}

const void CAG2Modifier_Additive::Load( XmlNodeRef modifierNode )
{
	int playMode = m_playMode;
	modifierNode->getAttr("playMode", playMode);
	m_playMode = (EPlayModes)playMode;
	modifierNode->getAttr("fadeOutOneLeaving", m_fadeOutOneLeaving);
	modifierNode->getAttr("layer", m_layer);
	modifierNode->getAttr("fadeOutTime", m_fadeOutTime);
	modifierNode->getAttr("blendInTime", m_blendInTime);
	modifierNode->getAttr("assetWeight", m_assetWeight);
	modifierNode->getAttr("assetSpeed", m_assetSpeed);
	modifierNode->getAttr("assetName", m_assetName);
}

const void CAG2Modifier_Additive::Export( XmlNodeRef node ) const
{
	// <Additive layer="Z" playMode="AAA" assetName="XXX" assetWeight="YYY" blendInTime="AA" fadeOutOneLeaving="BB" fadeOutTime="CC"/>

	// Only export if something is there
	if (m_assetName.IsEmpty() && m_playMode != STOP_LAYER)
		return;

	XmlNodeRef additiveNode = node->createNode("Additive");
	additiveNode->setAttr("layer", m_layer);
	additiveNode->setAttr("playMode", m_playMode);
	if (m_playMode != STOP_LAYER)
	{
		additiveNode->setAttr("assetName", m_assetName);
		additiveNode->setAttr("assetWeight", m_assetWeight);
		additiveNode->setAttr("assetSpeed", m_assetSpeed);
		additiveNode->setAttr("blendInTime", m_blendInTime);
		additiveNode->setAttr("fadeOutOneLeaving", m_fadeOutOneLeaving);
	}
	if (m_fadeOutOneLeaving || m_playMode == STOP_LAYER)
		additiveNode->setAttr("fadeOutTime", m_fadeOutTime);
	node->addChild(additiveNode);
}



BEGIN_MESSAGE_MAP(CAG2Modifier_Additive, CDialog)
	ON_BN_CLICKED(IDC_RADIO_PLAYONCE, OnUserInput)
	ON_BN_CLICKED(IDC_RADIO_LOOP, OnUserInput)
	ON_BN_CLICKED(IDC_RADIO_STOPLAYER, OnUserInput)
	ON_BN_CLICKED(IDC_CHECK_FADEOUTONLEAVE, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ASSETNAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ASSETWEIGHT, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_LAYER, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_FADEOUTTIME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_BLENDINTIME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ASSETSPEED, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_Additive::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_RADIO_PLAYONCE, m_playMode_PlayOnce_Ctrl );
	DDX_Control( pDX, IDC_RADIO_LOOP, m_playMode_Loop_Ctrl );
	DDX_Control( pDX, IDC_RADIO_STOPLAYER, m_playMode_StopLayer_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ASSETNAME, m_assetName_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ASSETWEIGHT, m_assetWeight_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ASSETSPEED, m_assetSpeed_Ctrl );
	DDX_Control( pDX, IDC_EDIT_LAYER, m_layer_Ctrl );
	DDX_Control( pDX, IDC_CHECK_FADEOUTONLEAVE, m_fadeOutOneLeaving_Ctrl );
	DDX_Control( pDX, IDC_EDIT_FADEOUTTIME, m_fadeOutTime_Ctrl );
	DDX_Control( pDX, IDC_EDIT_BLENDINTIME, m_blendInTime_Ctrl );
}

void CAG2Modifier_Additive::GrayOutInactiveElements()
{
	// Stop Assets -> Gray out asset name + weight + blendInTime + fadeout + fadeoutTime
	bool stopAssets = (m_playMode == STOP_LAYER);
	{
		m_assetName_Ctrl.EnableWindow(!stopAssets);
		m_assetWeight_Ctrl.EnableWindow(!stopAssets);
		m_assetSpeed_Ctrl.EnableWindow(!stopAssets);
		m_fadeOutOneLeaving_Ctrl.EnableWindow(!stopAssets);
		m_fadeOutTime_Ctrl.EnableWindow(!stopAssets);
		m_blendInTime_Ctrl.EnableWindow(!stopAssets);
	}

	// FadeOutOnLeaving=false -> Gray out fadeout time
	{
		m_fadeOutTime_Ctrl.EnableWindow(m_fadeOutOneLeaving || stopAssets);
	}
}

void CAG2Modifier_Additive::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	m_playMode_PlayOnce_Ctrl.SetCheck(m_playMode == PLAY_ONCE);
	m_playMode_Loop_Ctrl.SetCheck(m_playMode == LOOP_ASSET);
	m_playMode_StopLayer_Ctrl.SetCheck(m_playMode == STOP_LAYER);

	m_assetName_Ctrl.SetWindowText(m_assetName);
	m_fadeOutOneLeaving_Ctrl.SetCheck(m_fadeOutOneLeaving);

	CString tempText;
	tempText.Format("%.3f", m_assetWeight);
	m_assetWeight_Ctrl.SetWindowText(tempText);
	tempText.Format("%.3f", m_assetSpeed);
	m_assetSpeed_Ctrl.SetWindowText(tempText);
	tempText.Format("%.i", m_layer);
	m_layer_Ctrl.SetWindowText(tempText);
	tempText.Format("%.3f", m_fadeOutTime);
	m_fadeOutTime_Ctrl.SetWindowText(tempText);
	tempText.Format("%.3f", m_blendInTime);
	m_blendInTime_Ctrl.SetWindowText(tempText);

	GrayOutInactiveElements();
}

void CAG2Modifier_Additive::OnUserInput()
{
	int prevLayer = m_layer;

	// Get changed Data from panel
	if (m_playMode_PlayOnce_Ctrl.GetCheck())
		m_playMode = PLAY_ONCE;
	else if (m_playMode_Loop_Ctrl.GetCheck())
		m_playMode = LOOP_ASSET;
	else if (m_playMode_StopLayer_Ctrl.GetCheck())
		m_playMode = STOP_LAYER;

	m_assetName_Ctrl.GetWindowText(m_assetName);
	m_fadeOutOneLeaving = m_fadeOutOneLeaving_Ctrl.GetCheck();

	CString tempText;
	m_assetWeight_Ctrl.GetWindowText(tempText);
	m_assetWeight = atof(tempText);
	m_assetSpeed_Ctrl.GetWindowText(tempText);
	m_assetSpeed = atof(tempText);
	m_layer_Ctrl.GetWindowText(tempText);
	m_layer = atoi(tempText);
	m_fadeOutTime_Ctrl.GetWindowText(tempText);
	m_fadeOutTime = atof(tempText);
	m_blendInTime_Ctrl.GetWindowText(tempText);
	m_blendInTime = atof(tempText);

	// Check that the inputs make sense and modify if needed
	// and send back to panel

	// Check: Layer should be positive and greater than 1
	if (m_layer < 0)
	{
		AfxMessageBox("The layer entry must be positive. Resetting to previous layer: " + prevLayer, MB_OK);
		m_layer = prevLayer;
	}
	if (m_layer < 2)
	{
		AfxMessageBox("Additives should not be played in the bottom two layers. Resetting to previous layer: " + prevLayer, MB_OK);
		m_layer = prevLayer;
	}
	if (m_layer >= numVIRTUALLAYERS)
	{
		AfxMessageBox("Invalid Layer Number. Setting highest possible layer: " + (numVIRTUALLAYERS - 1), MB_OK);
		m_layer = numVIRTUALLAYERS - 1;
	}

	// Check: FadeOutTime and BlendInTime must be positive
	if (m_fadeOutTime < 0.0f)
		m_fadeOutTime = 0.0f;
	if (m_blendInTime < 0.0f)
		m_blendInTime = 0.0f;
	
	InitParameterPanel();
	GrayOutInactiveElements();
}

const CString CAG2Modifier_Additive::GetCustomText() const
{
	CString retVal;

	if (m_playMode != STOP_LAYER && !m_assetName.IsEmpty())
		retVal.Format("Layer %i: %s", m_layer, m_assetName);
	
	if (m_playMode == STOP_LAYER)
	{
		retVal.Format("Stop Layer %i", m_layer);
	}

	if (retVal.IsEmpty())
		retVal = "<invalid>";

	return retVal;
}

