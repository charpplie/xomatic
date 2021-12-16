#include "StdAfx.h"
#include "AttachmentControl.h"


CAG2Modifier_AttachmentControl::CAG2Modifier_AttachmentControl() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_AttachmentControl::~CAG2Modifier_AttachmentControl()
{
}

CAG2ModifierBase* CAG2Modifier_AttachmentControl::Duplicate() const
{
	return new CAG2Modifier_AttachmentControl();
}


void CAG2Modifier_AttachmentControl::Reset()
{
	m_attachmentName = "";
	m_animationName = "";
	m_animationLoop = false;
	m_animationLayer = 0;
	m_controlType = ePlayAnimation;
	m_animStopLayer = 0;
	m_animStopTime = 0.4f;
}

const void CAG2Modifier_AttachmentControl::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("m_attachmentName", m_attachmentName);
	modifierNode->setAttr("m_controlType", m_controlType);
	
	modifierNode->setAttr("m_animationName", m_animationName);
	modifierNode->setAttr("m_animationLoop", m_animationLoop);
	modifierNode->setAttr("m_animationLayer", m_animationLayer);
	
	modifierNode->setAttr("m_animStopLayer", m_animStopLayer);
	modifierNode->setAttr("m_animStopTime", m_animStopTime);
}

const void CAG2Modifier_AttachmentControl::Load( XmlNodeRef modifierNode )
{
	modifierNode->getAttr("m_attachmentName", m_attachmentName);
	modifierNode->getAttr("m_controlType", m_controlType);

	modifierNode->getAttr("m_animationName", m_animationName);
	modifierNode->getAttr("m_animationLoop", m_animationLoop);
	modifierNode->getAttr("m_animationLayer", m_animationLayer);

	modifierNode->getAttr("m_animStopLayer", m_animStopLayer);
	modifierNode->getAttr("m_animStopTime", m_animStopTime);
}

const void CAG2Modifier_AttachmentControl::Export( XmlNodeRef node ) const
{
	// <AttachmentControl attachment="XXX" control="YYY">
	// potentially one of these children:
	//			<PlayAnimation animName="XXX" looping="XXX" layer="XXX" />
	//			<StopAnimation layer="XXX" blendTime="XXX" />
	// </AttachmentControl/>

	// Only write out if it is valid
	if (m_attachmentName.IsEmpty())
		return;

	XmlNodeRef attachmentNode = node->createNode("AttachmentControl");
	attachmentNode->setAttr("attachment", m_attachmentName);
	attachmentNode->setAttr("control", m_controlType);

	if (m_controlType == ePlayAnimation)
	{
		if (!m_animationName[0] == 0)
		{
			XmlNodeRef playAnimNode = attachmentNode->createNode("PlayAnimation");
			playAnimNode->setAttr("animName", m_animationName);
			playAnimNode->setAttr("looping", m_animationLoop);
			playAnimNode->setAttr("layer", m_animationLayer);
			attachmentNode->addChild(playAnimNode);
		}
	}
	if (m_controlType == eStopAnimation)
	{
		XmlNodeRef stopAnimNode = attachmentNode->createNode("StopAnimation");
		stopAnimNode->setAttr("layer", m_animStopLayer);
		stopAnimNode->setAttr("blendTime", m_animStopTime);
		attachmentNode->addChild(stopAnimNode);
	}
	// Note: Hide and Unhide attachment have no additional parameters and therefore
	//				need not create any additional xml nodes.

	node->addChild(attachmentNode);
}


BEGIN_MESSAGE_MAP(CAG2Modifier_AttachmentControl, CDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_ATTACHMENT_NAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ATTACH_ANIM_NAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ATTACH_ANIM_LAYER, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ATTACH_STOPANIM_LAYER, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ATTACH_STOPANIM_BLENDTIME, OnUserInput)
	ON_BN_CLICKED(IDC_CHECK_ATTACH_ANIM_LOOP, OnUserInput)
	ON_BN_CLICKED(IDC_RADIO_PLAY_ATTACHANIM, GrayOutInactiveItems)
	ON_BN_CLICKED(IDC_RADIO_STOP_ATTACHANIM, GrayOutInactiveItems)
	ON_BN_CLICKED(IDC_RADIO_STOP_ATTACH_UNHIDE, GrayOutInactiveItems)
	ON_BN_CLICKED(IDC_RADIO_STOP_ATTACH_HIDE, GrayOutInactiveItems)
END_MESSAGE_MAP()


void CAG2Modifier_AttachmentControl::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_ATTACHMENT_NAME, m_attachmentName_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ATTACH_ANIM_NAME, m_animationName_Ctrl );
	DDX_Control( pDX, IDC_CHECK_ATTACH_ANIM_LOOP, m_animationLoop_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ATTACH_ANIM_LAYER, m_animationLayer_Ctrl );
	DDX_Control( pDX, IDC_RADIO_PLAY_ATTACHANIM, m_animation_Ctrl );
	DDX_Control( pDX, IDC_RADIO_STOP_ATTACHANIM, m_stopAnimation_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ATTACH_STOPANIM_LAYER, m_animationStopLayer_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ATTACH_STOPANIM_BLENDTIME, m_animationStopTime_Ctrl );
	DDX_Control( pDX, IDC_RADIO_STOP_ATTACH_UNHIDE, m_show_Ctrl );
	DDX_Control( pDX, IDC_RADIO_STOP_ATTACH_HIDE, m_hide_Ctrl );
}

void CAG2Modifier_AttachmentControl::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	CString tempText;

	m_attachmentName_Ctrl.SetWindowText(m_attachmentName);

	m_animationName_Ctrl.SetWindowText(m_animationName);
	m_animationLoop_Ctrl.SetCheck(m_animationLoop);
	tempText.Format("%i", m_animationLayer);
	m_animationLayer_Ctrl.SetWindowText(tempText);

	tempText.Format("%i", m_animStopLayer);
	m_animationStopLayer_Ctrl.SetWindowText(tempText);
	tempText.Format("%.3f", m_animStopTime);
	m_animationStopTime_Ctrl.SetWindowText(tempText);

	m_animation_Ctrl.SetCheck(m_controlType == ePlayAnimation);
	m_stopAnimation_Ctrl.SetCheck(m_controlType == eStopAnimation);
	m_show_Ctrl.SetCheck(m_controlType == eShowAttachment);
	m_hide_Ctrl.SetCheck(m_controlType == eHideAttachment);

	GrayOutInactiveItems();
}

void CAG2Modifier_AttachmentControl::OnUserInput()
{
	// Get changed Data from panel
	CString tempText;

	m_attachmentName_Ctrl.GetWindowText(m_attachmentName);

	m_animationName_Ctrl.GetWindowText(m_animationName);
	m_animationLoop = m_animationLoop_Ctrl.GetCheck();
	m_animationLayer_Ctrl.GetWindowText(tempText);
	m_animationLayer = atoi(tempText);

	m_animationStopLayer_Ctrl.GetWindowText(tempText);
	m_animStopLayer = atoi(tempText);
	m_animationStopTime_Ctrl.GetWindowText(tempText);
	m_animStopTime = atof(tempText);
}

//////////////////////////////////////////////////////////////////////////

void CAG2Modifier_AttachmentControl::GrayOutInactiveItems()
{
	// Get active radio button and set control type
	if (m_animation_Ctrl.GetCheck())
		m_controlType = ePlayAnimation;
	if (m_stopAnimation_Ctrl.GetCheck())
		m_controlType = eStopAnimation;
	if (m_show_Ctrl.GetCheck())
		m_controlType = eShowAttachment;
	if (m_hide_Ctrl.GetCheck())
		m_controlType = eHideAttachment;

	// Gray out the inactive control elements

	// Play Animation Control
	m_animationName_Ctrl.EnableWindow(m_controlType == ePlayAnimation);
	m_animationLoop_Ctrl.EnableWindow(m_controlType == ePlayAnimation);
	m_animationLayer_Ctrl.EnableWindow(m_controlType == ePlayAnimation);

	// Stop Animation Control
	m_animationStopTime_Ctrl.EnableWindow(m_controlType == eStopAnimation);
	m_animationStopLayer_Ctrl.EnableWindow(m_controlType == eStopAnimation);
}

//////////////////////////////////////////////////////////////////////////

const CString CAG2Modifier_AttachmentControl::GetCustomText() const
{
	CString retVal;

	if (m_controlType == ePlayAnimation)
	{
		if (!m_attachmentName.IsEmpty() && !m_animationName.IsEmpty())
			retVal.Format("%s - %s", m_attachmentName, m_animationName);
	}
	if (m_controlType == eStopAnimation)
	{
		if (!m_attachmentName.IsEmpty())
			retVal.Format("%s - Stop Animation", m_attachmentName);
	}
	if (m_controlType == eShowAttachment)
	{
		if (!m_attachmentName.IsEmpty())
			retVal.Format("%s - Unhide", m_attachmentName);
	}
	if (m_controlType == eHideAttachment)
	{
		if (!m_attachmentName.IsEmpty())
			retVal.Format("%s - Hide", m_attachmentName);
	}

	if (retVal.IsEmpty())
		retVal = "<no attachment chosen>";

	return retVal;
}

