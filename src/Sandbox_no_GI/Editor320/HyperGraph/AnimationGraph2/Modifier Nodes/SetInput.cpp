#include "StdAfx.h"
#include "SetInput.h"


CAG2Modifier_SetInput::CAG2Modifier_SetInput() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_SetInput::~CAG2Modifier_SetInput()
{
}

CAG2ModifierBase* CAG2Modifier_SetInput::Duplicate() const
{
	return new CAG2Modifier_SetInput();
}


void CAG2Modifier_SetInput::Reset()
{
	m_restoreOnLeave = false;
	m_timer = 0.0f;
	m_newValue.Empty();
	m_inputName.Empty();
}

const void CAG2Modifier_SetInput::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("restoreOnLeave", m_restoreOnLeave);
	modifierNode->setAttr("timer", m_timer);
	modifierNode->setAttr("newValue", m_newValue);
	modifierNode->setAttr("inputName", m_inputName);
}

const void CAG2Modifier_SetInput::Load( XmlNodeRef modifierNode )
{
	modifierNode->getAttr("restoreOnLeave", m_restoreOnLeave);
	modifierNode->getAttr("timer", m_timer);
	modifierNode->getAttr("newValue", m_newValue);
	modifierNode->getAttr("inputName", m_inputName);
}

const void CAG2Modifier_SetInput::Export( XmlNodeRef node ) const
{
	// <SetInput time="AA" input="BBB" value="CCC" restoreOnLeave="DD"/>

	// Only write out what is there
	if (m_inputName.IsEmpty() || m_newValue.IsEmpty())
	{
		AfxMessageBox("SetInput Modifier Node %s-%s not complete - will not be exported. Inactive.", MB_OK|MB_ICONERROR);
		return;
	}

	XmlNodeRef setInputNode = node->createNode("SetInput");
	setInputNode->setAttr("time", m_timer);
	setInputNode->setAttr("input", m_inputName);
	setInputNode->setAttr("value", m_newValue);
	setInputNode->setAttr("restoreOnLeave", m_restoreOnLeave);
	node->addChild(setInputNode);
}



BEGIN_MESSAGE_MAP(CAG2Modifier_SetInput, CDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_INPUT_NAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_NEW_VALUE, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_TIMER, OnUserInput)
	ON_BN_CLICKED(IDC_CHECK_RESTORE_ON_LEAVE, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_SetInput::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_INPUT_NAME, m_inputName_Ctrl );
	DDX_Control( pDX, IDC_EDIT_NEW_VALUE, m_newValue_Ctrl );
	DDX_Control( pDX, IDC_EDIT_TIMER, m_timer_Ctrl );
	DDX_Control( pDX, IDC_CHECK_RESTORE_ON_LEAVE, m_restoreOnLeave_Ctrl );
}

void CAG2Modifier_SetInput::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	m_inputName_Ctrl.SetWindowText(m_inputName);
	m_newValue_Ctrl.SetWindowText(m_newValue);
	m_restoreOnLeave_Ctrl.SetCheck(m_restoreOnLeave);

	CString tempString;
	tempString.Format("%.5f", m_timer);
	m_timer_Ctrl.SetWindowText(tempString);
}

void CAG2Modifier_SetInput::OnUserInput()
{
	// Get changed Data from panel
	m_inputName_Ctrl.GetWindowText(m_inputName);
	m_newValue_Ctrl.GetWindowText(m_newValue);
	m_restoreOnLeave = m_restoreOnLeave_Ctrl.GetCheck();

	CString tempString;
	m_timer_Ctrl.GetWindowText(tempString);
	m_timer = atof(tempString);
	tempString.Format("%.5f", m_timer);
	m_timer_Ctrl.SetWindowText(tempString);
}

const CString CAG2Modifier_SetInput::GetCustomText() const
{
	CString retVal;
	retVal = "<Invalid>";

	// Action: idleBreak
	// 5sec

	if (!m_inputName.IsEmpty() && !m_newValue.IsEmpty())
	{
		retVal.Format("%s: %s", m_inputName, m_newValue);
	
		if (m_timer > 0.0f)
		{
			retVal.AppendFormat("\n%.2fsec", m_timer);
		}
	}

	return retVal;
}

bool CAG2Modifier_SetInput::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "SetInput"))
		return false;

	if (!node->haveAttr("input") && !node->haveAttr("value"))
	{
		if (node->getNumAttributes() > 0)
		{
			AfxMessageBox( "Warning, found a state node <SetInput> with attributes, but cannot read them.\nConversion might have failed and graph may not function correctly.", MB_OK|MB_ICONERROR );
		}

		return false;
	}

	return true;
}

void CAG2Modifier_SetInput::ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode )
{
	CRY_ASSERT_MESSAGE(CanConvertFromOldGraphVersion(node), "Trying to convert AG1.0 State Node entry into Event, but this node is not an event!\nDo not call this function unless you called CanConvertFromOldGraphVersion first!");

	// Make sure there isn't any old data left here in case someone called this
	// function multiple times (which you shouldn't do, but it also does no harm)
	Reset();

	node->getAttr("input", m_inputName);
	node->getAttr("value", m_newValue);

	if (node->haveAttr("time"))
	{
		node->getAttr("time", m_timer);
	}

	if (node->haveAttr("restoreOnLeave"))
	{
		node->getAttr("restoreOnLeave", m_restoreOnLeave);
	}
}

