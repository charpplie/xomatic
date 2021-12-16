#include "StdAfx.h"
#include "PlayFacialSequence.h"


CAG2Modifier_FacialSequence::CAG2Modifier_FacialSequence() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_FacialSequence::~CAG2Modifier_FacialSequence()
{
}

CAG2ModifierBase* CAG2Modifier_FacialSequence::Duplicate() const
{
	return new CAG2Modifier_FacialSequence();
}


void CAG2Modifier_FacialSequence::Reset()
{
	m_useOneForAll = false;
	m_oneForAllSequence.Empty();
	m_idleSequence.Empty();
	m_alertedSequence.Empty();
	m_combatSequence.Empty();
}

const void CAG2Modifier_FacialSequence::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("useOneForAll", m_useOneForAll);
	modifierNode->setAttr("oneForAllSequence", m_oneForAllSequence);
	modifierNode->setAttr("idleSequence", m_idleSequence);
	modifierNode->setAttr("alertedSequence", m_alertedSequence);
	modifierNode->setAttr("combatSequence", m_combatSequence);
}

const void CAG2Modifier_FacialSequence::Load( XmlNodeRef modifierNode )
{
	modifierNode->getAttr("useOneForAll", m_useOneForAll);
	modifierNode->getAttr("oneForAllSequence", m_oneForAllSequence);
	modifierNode->getAttr("idleSequence", m_idleSequence);
	modifierNode->getAttr("alertedSequence", m_alertedSequence);
	modifierNode->getAttr("combatSequence", m_combatSequence);
}

const void CAG2Modifier_FacialSequence::Export( XmlNodeRef node ) const
{
	// <FacialEffector idle="AAA" alerted="BBB" combat="CCC" all="DDD"/>

	// Only write out if there is some sequence defined
	if ( m_useOneForAll && m_oneForAllSequence.IsEmpty())
		return;
	if (!m_useOneForAll && m_idleSequence.IsEmpty() && m_alertedSequence.IsEmpty() && m_combatSequence.IsEmpty())
		return;

	XmlNodeRef facialNode = node->createNode("FacialEffector");
	facialNode->setAttr("idle", m_idleSequence);
	facialNode->setAttr("alerted", m_alertedSequence);
	facialNode->setAttr("combat", m_combatSequence);

	if (m_useOneForAll)
		facialNode->setAttr("all", m_oneForAllSequence);

	node->addChild(facialNode);
}



BEGIN_MESSAGE_MAP(CAG2Modifier_FacialSequence, CDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_SEQUENCE_IDLE, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SEQUENCE_ALERTED, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SEQUENCE_COMBAT, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SEQUENCE_ONEFORALL, OnUserInput)
	ON_BN_CLICKED(IDC_CHECK_ONEFORALL, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_FacialSequence::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_SEQUENCE_IDLE, m_idleSequence_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SEQUENCE_ALERTED, m_alertedSequence_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SEQUENCE_COMBAT, m_combatSequence_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SEQUENCE_ONEFORALL, m_oneForAllSequence_Ctrl );
	DDX_Control( pDX, IDC_CHECK_ONEFORALL, m_useOneForAll_Ctrl );
}

void CAG2Modifier_FacialSequence::GrayOutInactiveElements()
{
	m_idleSequence_Ctrl.EnableWindow(!m_useOneForAll);
	m_alertedSequence_Ctrl.EnableWindow(!m_useOneForAll);
	m_combatSequence_Ctrl.EnableWindow(!m_useOneForAll);
	m_oneForAllSequence_Ctrl.EnableWindow(m_useOneForAll);
}

void CAG2Modifier_FacialSequence::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	m_idleSequence_Ctrl.SetWindowText(m_idleSequence);
	m_alertedSequence_Ctrl.SetWindowText(m_alertedSequence);
	m_combatSequence_Ctrl.SetWindowText(m_combatSequence);
	m_oneForAllSequence_Ctrl.SetWindowText(m_oneForAllSequence);
	m_useOneForAll_Ctrl.SetCheck(m_useOneForAll);

	GrayOutInactiveElements();
}

void CAG2Modifier_FacialSequence::OnUserInput()
{
	// Get changed Data from panel
	m_idleSequence_Ctrl.GetWindowText(m_idleSequence);
	m_alertedSequence_Ctrl.GetWindowText(m_alertedSequence);
	m_combatSequence_Ctrl.GetWindowText(m_combatSequence);
	m_oneForAllSequence_Ctrl.GetWindowText(m_oneForAllSequence);
	m_useOneForAll = m_useOneForAll_Ctrl.GetCheck();

	GrayOutInactiveElements();
}

const CString CAG2Modifier_FacialSequence::GetCustomText() const
{
	CString retVal;

	if (m_useOneForAll)
	{
		retVal = m_oneForAllSequence;
	}
	else
	{
		if (!m_idleSequence.IsEmpty())
			retVal.Append(m_idleSequence);
		else if (!m_alertedSequence.IsEmpty())
			retVal.Append(m_alertedSequence);
		else if (!m_combatSequence.IsEmpty())
			retVal.Append(m_combatSequence);
	}

	if (retVal.IsEmpty())
		retVal = "<not set>";

	return retVal;
}

bool CAG2Modifier_FacialSequence::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "FacialEffector"))
		return false;

	return true;
}

void CAG2Modifier_FacialSequence::ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode )
{
	CRY_ASSERT_MESSAGE(CanConvertFromOldGraphVersion(node), "Trying to convert AG1.0 State Node entry into Facial Sequence, but this node is not an event!\nDo not call this function unless you called CanConvertFromOldGraphVersion first!");

	// Make sure there isn't any old data left here in case someone called this
	// function multiple times (which you shouldn't do, but it also does no harm)
	Reset();

	m_idleSequence = node->getAttr("idle");
	m_alertedSequence = node->getAttr("alerted");
	m_combatSequence = node->getAttr("combat");

	if (node->haveAttr("all"))
	{
		m_useOneForAll = true;
		m_oneForAllSequence = node->getAttr("all");
	}
}

