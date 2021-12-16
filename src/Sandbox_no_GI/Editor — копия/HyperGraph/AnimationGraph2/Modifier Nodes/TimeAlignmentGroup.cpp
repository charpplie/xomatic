#include "StdAfx.h"
#include "TimeAlignmentGroup.h"

CAG2Modifier_TimeAlignGroup::CAG2Modifier_TimeAlignGroup() : CAG2ModifierBase()
{
}

CAG2Modifier_TimeAlignGroup::~CAG2Modifier_TimeAlignGroup()
{
}

void CAG2Modifier_TimeAlignGroup::Init()
{
	EnsureValidGroupName();
}

CAG2ModifierBase* CAG2Modifier_TimeAlignGroup::Duplicate() const
{
	return new CAG2Modifier_TimeAlignGroup();
}


const void CAG2Modifier_TimeAlignGroup::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("groupName", m_groupName);
}

const void CAG2Modifier_TimeAlignGroup::Load( XmlNodeRef modifierNode )
{
	CRY_ASSERT_MESSAGE(modifierNode->haveAttr("groupName"), "The Attribute groupName is missing from this node.\nDo not change this Attribute's name, as it is being used in other places as well!");
	modifierNode->getAttr("groupName", m_groupName);

	if (m_groupName.IsEmpty())
	{
		EnsureValidGroupName();
		AfxMessageBox("Error: Time Alignment Group Modifier with no Group Name found. Assigning generic name - this could lead to undesired behaviour.");
	}
}

BEGIN_MESSAGE_MAP(CAG2Modifier_TimeAlignGroup, CDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_GROUPNAME, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_TimeAlignGroup::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_GROUPNAME, m_groupName_Ctrl );
}

void CAG2Modifier_TimeAlignGroup::InitParameterPanel()
{
	m_groupName_Ctrl.SetWindowText(m_groupName);
}

void CAG2Modifier_TimeAlignGroup::OnUserInput()
{
	m_groupName_Ctrl.GetWindowText(m_groupName);
}

const CString CAG2Modifier_TimeAlignGroup::GetCustomText() const
{
	// DO not change the implementation of the method, it will have
	// sideeffects upon saving the graph
	CString retVal;
	retVal = m_groupName;
	
	return retVal;
}

void CAG2Modifier_TimeAlignGroup::EnsureValidGroupName()
{
	if (!m_groupName.IsEmpty())
		return;

	m_groupName.Format("Group_%i", GetId());
}
