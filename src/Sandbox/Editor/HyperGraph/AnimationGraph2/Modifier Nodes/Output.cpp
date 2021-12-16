#include "StdAfx.h"
#include "Output.h"


CAG2Modifier_Output::CAG2Modifier_Output() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_Output::~CAG2Modifier_Output()
{
}

CAG2ModifierBase* CAG2Modifier_Output::Duplicate() const
{
	return new CAG2Modifier_Output();
}


void CAG2Modifier_Output::Reset()
{
	m_outputName.Empty();
	m_outputValue.Empty();
}

const void CAG2Modifier_Output::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("m_outputName", m_outputName);
	modifierNode->setAttr("m_outputValue", m_outputValue);
}

const void CAG2Modifier_Output::Load( XmlNodeRef modifierNode )
{
	modifierNode->getAttr("m_outputName", m_outputName);
	modifierNode->getAttr("m_outputValue", m_outputValue);
}

const void CAG2Modifier_Output::Export( XmlNodeRef node ) const
{
	// <Output name="XXX" value="YYY"/>

	XmlNodeRef outputNode = node->createNode("Output");
	outputNode->setAttr("name", m_outputName);
	outputNode->setAttr("value", m_outputValue);
	node->addChild(outputNode);
}

BEGIN_MESSAGE_MAP(CAG2Modifier_Output, CDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_OUTPUT_NAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_OUTPUT_VALUE, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_Output::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_OUTPUT_NAME, m_outputName_Ctrl );
	DDX_Control( pDX, IDC_EDIT_OUTPUT_VALUE, m_outputValue_Ctrl );
}

void CAG2Modifier_Output::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	m_outputValue_Ctrl.SetWindowText(m_outputValue);
	m_outputName_Ctrl.SetWindowText(m_outputName);
}

void CAG2Modifier_Output::OnUserInput()
{
	// Get changed Data from panel
	m_outputValue_Ctrl.GetWindowText(m_outputValue);
	m_outputName_Ctrl.GetWindowText(m_outputName);
}

const CString CAG2Modifier_Output::GetCustomText() const
{
	CString retVal;

	if (!m_outputName.IsEmpty())
		retVal.Format("%s - \"%s\"", m_outputName, m_outputValue);

	if (retVal.IsEmpty())
		retVal = "<not set>";

	return retVal;
}

bool CAG2Modifier_Output::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "Output"))
		return false;

	return true;
}

void CAG2Modifier_Output::ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode )
{
	CRY_ASSERT_MESSAGE(CanConvertFromOldGraphVersion(node), "Trying to convert AG1.0 State Node entry into Output, but this node is not an output!\nDo not call this function unless you called CanConvertFromOldGraphVersion first!");

	// Make sure there isn't any old data left here in case someone called this
	// function multiple times (which you shouldn't do, but it also does no harm)
	Reset();

	node->getAttr("name", m_outputName);
	node->getAttr("value", m_outputValue);

	// It is possible that the names contain references to parameter values. If so, these must be replaced.
	if (m_outputValue.Find('$') == 0)
	{
		m_outputValue.Delete(0, 1);
		m_outputValue = pAnimNode->GetTemplateParameter(m_outputValue);
	}
}

