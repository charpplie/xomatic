#include "StdAfx.h"
#include "Event.h"


CAG2Modifier_Event::CAG2Modifier_Event() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_Event::~CAG2Modifier_Event()
{
}

CAG2ModifierBase* CAG2Modifier_Event::Duplicate() const
{
	return new CAG2Modifier_Event();
}


void CAG2Modifier_Event::Reset()
{
	m_useEventOnEnter = m_useEventOnExit = m_matchAnimation = false;
	m_eventOnEnter.Empty();
	m_eventOnExit.Empty();
}

const void CAG2Modifier_Event::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("useOnEnter", m_useEventOnEnter);
	modifierNode->setAttr("eventEnter", m_eventOnEnter);
	modifierNode->setAttr("useOnExit", m_useEventOnExit);
	modifierNode->setAttr("eventExit", m_eventOnExit);
	modifierNode->setAttr("matchAnim", m_matchAnimation);
}

const void CAG2Modifier_Event::Load( XmlNodeRef modifierNode )
{
	modifierNode->getAttr("useOnEnter", m_useEventOnEnter);
	modifierNode->getAttr("eventEnter", m_eventOnEnter);
	modifierNode->getAttr("useOnExit", m_useEventOnExit);
	modifierNode->getAttr("eventExit", m_eventOnExit);
	modifierNode->getAttr("matchAnim", m_matchAnimation);
}

const void CAG2Modifier_Event::Export( XmlNodeRef node ) const
{
	// <Event onEnter="XXX" onExit="YYY" matchAnimation="Z"/>

	// Only write out what is there
	if (!m_useEventOnEnter && !m_useEventOnExit)
		return;

	XmlNodeRef eventNode = node->createNode("Event");
	if (m_useEventOnEnter && !m_eventOnEnter.IsEmpty())
		eventNode->setAttr("onEnter", m_eventOnEnter);
	if (m_useEventOnExit && !m_eventOnExit.IsEmpty())
		eventNode->setAttr("onExit", m_eventOnExit);
	eventNode->setAttr("matchAnimation", m_matchAnimation);
	node->addChild(eventNode);
}



BEGIN_MESSAGE_MAP(CAG2Modifier_Event, CDialog)
	ON_BN_CLICKED(IDC_CHECK_ENTER_EVENT, OnUserInput)
	ON_BN_CLICKED(IDC_CHECK_LEAVE_EVENT, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_ENTER_EVENT, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_LEAVE_EVENT, OnUserInput)
	ON_BN_CLICKED(IDC_CHECK_MATCH_ANIMATION, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_Event::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_CHECK_ENTER_EVENT, m_useEventOnEnter_Ctrl );
	DDX_Control( pDX, IDC_CHECK_LEAVE_EVENT, m_useEventOnExit_Ctrl );
	DDX_Control( pDX, IDC_EDIT_ENTER_EVENT, m_eventOnEnter_Ctrl );
	DDX_Control( pDX, IDC_EDIT_LEAVE_EVENT, m_eventOnExit_Ctrl );
	DDX_Control( pDX, IDC_CHECK_MATCH_ANIMATION, m_matchAnimation_Ctrl );
}

void CAG2Modifier_Event::GrayOutInactiveElements()
{
	m_eventOnEnter_Ctrl.EnableWindow(m_useEventOnEnter);
	m_eventOnExit_Ctrl.EnableWindow(m_useEventOnExit);
}

void CAG2Modifier_Event::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	m_useEventOnEnter_Ctrl.SetCheck(m_useEventOnEnter);
	m_useEventOnExit_Ctrl.SetCheck(m_useEventOnExit);
	m_eventOnEnter_Ctrl.SetWindowText(m_eventOnEnter);
	m_eventOnExit_Ctrl.SetWindowText(m_eventOnExit);
	m_matchAnimation_Ctrl.SetCheck(m_matchAnimation);

	GrayOutInactiveElements();
}

void CAG2Modifier_Event::OnUserInput()
{
	// Get changed Data from panel
	m_useEventOnEnter = m_useEventOnEnter_Ctrl.GetCheck();
	m_useEventOnExit = m_useEventOnExit_Ctrl.GetCheck();
	m_eventOnEnter_Ctrl.GetWindowText(m_eventOnEnter);
	m_eventOnExit_Ctrl.GetWindowText(m_eventOnExit);
	m_matchAnimation = m_matchAnimation_Ctrl.GetCheck();

	GrayOutInactiveElements();
}

const CString CAG2Modifier_Event::GetCustomText() const
{
	CString retVal;

	if (m_useEventOnEnter && !m_eventOnEnter.IsEmpty())
		retVal = m_eventOnEnter;
	
	if (m_useEventOnExit && !m_eventOnExit.IsEmpty())
	{
		if (!retVal.IsEmpty())
			retVal.Append("\n");
		retVal.Append(m_eventOnExit);
	}

	if (retVal.IsEmpty())
		retVal = "<not set>";

	return retVal;
}

bool CAG2Modifier_Event::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "Event"))
		return false;

	if (!node->haveAttr("onEnter") && !node->haveAttr("onExit"))
	{
		if (node->getNumAttributes() > 0)
		{
			AfxMessageBox( "Warning, found a state node <Event> with attributes, but cannot read them.\nConversion might have failed and graph may not function correctly.", MB_OK|MB_ICONERROR );
		}

		return false;
	}

	return true;
}

void CAG2Modifier_Event::ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode )
{
	CRY_ASSERT_MESSAGE(CanConvertFromOldGraphVersion(node), "Trying to convert AG1.0 State Node entry into Event, but this node is not an event!\nDo not call this function unless you called CanConvertFromOldGraphVersion first!");

	// Make sure there isn't any old data left here in case someone called this
	// function multiple times (which you shouldn't do, but it also does no harm)
	Reset();

	if (node->haveAttr("onEnter"))
	{
		m_useEventOnEnter = true;
		node->getAttr("onEnter", m_eventOnEnter);
	}

	if (node->haveAttr("onExit"))
	{
		m_useEventOnExit = true;
		node->getAttr("onExit", m_eventOnExit);
	}

	if (node->haveAttr("matchAnimation"))
	{
		node->getAttr("matchAnimation", m_matchAnimation);
	}
}

