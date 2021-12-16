#include "StdAfx.h"
#include "AttachmentEffect.h"


CAG2Modifier_AttachmentEffect::CAG2Modifier_AttachmentEffect() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_AttachmentEffect::~CAG2Modifier_AttachmentEffect()
{
}

CAG2ModifierBase* CAG2Modifier_AttachmentEffect::Duplicate() const
{
	return new CAG2Modifier_AttachmentEffect();
}


void CAG2Modifier_AttachmentEffect::Reset()
{
	m_countOffset = m_sizeOffset = 0.0f;
	m_countMult = m_sizeMult = 1.0f;
	m_attachmentName.Empty();
	m_particleName.Empty();
	m_countInput.Empty();
	m_sizeInput.Empty();
	m_pos.zero();
}

const void CAG2Modifier_AttachmentEffect::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("m_countOffset", m_countOffset);
	modifierNode->setAttr("m_countMult", m_countMult);
	modifierNode->setAttr("m_sizeOffset", m_sizeOffset);
	modifierNode->setAttr("m_sizeMult", m_sizeMult);
	modifierNode->setAttr("m_attachmentName", m_attachmentName);
	modifierNode->setAttr("m_particleName", m_particleName);
	modifierNode->setAttr("m_pos", m_pos);
	modifierNode->setAttr("m_countInput", m_countInput);
	modifierNode->setAttr("m_sizeInput", m_sizeInput);
}

const void CAG2Modifier_AttachmentEffect::Load( XmlNodeRef modifierNode )
{
	modifierNode->getAttr("m_countOffset", m_countOffset);
	modifierNode->getAttr("m_countMult", m_countMult);
	modifierNode->getAttr("m_sizeOffset", m_sizeOffset);
	modifierNode->getAttr("m_sizeMult", m_sizeMult);
	modifierNode->getAttr("m_attachmentName", m_attachmentName);
	modifierNode->getAttr("m_particleName", m_particleName);
	modifierNode->getAttr("m_pos", m_pos);
	modifierNode->getAttr("m_countInput", m_countInput);
	modifierNode->getAttr("m_sizeInput", m_sizeInput);
}

const void CAG2Modifier_AttachmentEffect::Export( XmlNodeRef node ) const
{
	// <AttachmentEffect attachment="XXX" effect="YYY" pos="X,Y,Z"  sizeScaleInput="XXX" countScaleInput="XXX" sizeScaleOffset="XXX" sizeScaleMult="XXX" countScaleOffset="XXX" countScaleMult="XXX"/>

	// Only write out if it is valid
	if (m_attachmentName.IsEmpty() || m_particleName.IsEmpty())
		return;

	XmlNodeRef attachmentNode = node->createNode("AttachmentEffect");
	attachmentNode->setAttr("attachment", m_attachmentName);
	attachmentNode->setAttr("effect", m_particleName);
	attachmentNode->setAttr("pos", m_pos);
	attachmentNode->setAttr("sizeScaleInput", m_sizeInput);
	attachmentNode->setAttr("countScaleInput", m_countInput);
	attachmentNode->setAttr("sizeScaleOffset", m_sizeOffset);
	attachmentNode->setAttr("sizeScaleMult", m_sizeMult);
	attachmentNode->setAttr("countScaleOffset", m_countOffset);
	attachmentNode->setAttr("countScaleMult", m_countMult);
	node->addChild(attachmentNode);
}

bool CAG2Modifier_AttachmentEffect::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "AttachmentEffect"))
		return false;

	return true;
}

void CAG2Modifier_AttachmentEffect::ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode )
{
	CRY_ASSERT_MESSAGE(CanConvertFromOldGraphVersion(node), "Trying to convert AG1.0 State Node entry into AttachmentEffect, but this node is not an AttachmentEffect!\nDo not call this function unless you called CanConvertFromOldGraphVersion first!");

	// Make sure there isn't any old data left here in case someone called this
	// function multiple times (which you shouldn't do, but it also does no harm)
	Reset();

	node->getAttr("countScaleOffset", m_countOffset);
	node->getAttr("countScaleMult", m_countMult);
	node->getAttr("sizeScaleOffset", m_sizeOffset);
	node->getAttr("sizeScaleMult", m_sizeMult);
	node->getAttr("attachment", m_attachmentName);
	node->getAttr("effect", m_particleName);
	node->getAttr("pos", m_pos);
	node->getAttr("countScaleInput", m_countInput);
	node->getAttr("sizeScaleInput", m_sizeInput);
}


BEGIN_MESSAGE_MAP(CAG2Modifier_AttachmentEffect, CDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_ATTACHMENT_NAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_PARTICLE_NAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_OFFSET_X, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_OFFSET_Y, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_OFFSET_Z, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SIZE_INPUT, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_COUNT_INPUT, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SIZE_OFFSET, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_COUNT_OFFSET, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SIZE_MULT, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_COUNT_MULT, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_AttachmentEffect::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_ATTACHMENT_NAME, m_attachmentName_Ctrl );
	DDX_Control( pDX, IDC_EDIT_PARTICLE_NAME, m_particleName_Ctrl );
	DDX_Control( pDX, IDC_EDIT_OFFSET_X, m_posX_Ctrl );
	DDX_Control( pDX, IDC_EDIT_OFFSET_Y, m_posY_Ctrl );
	DDX_Control( pDX, IDC_EDIT_OFFSET_Z, m_posZ_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SIZE_INPUT, m_sizeInput_Ctrl );
	DDX_Control( pDX, IDC_EDIT_COUNT_INPUT, m_countInput_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SIZE_OFFSET, m_sizeOffset_Ctrl );
	DDX_Control( pDX, IDC_EDIT_COUNT_OFFSET, m_countOffset_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SIZE_MULT, m_sizeMult_Ctrl );
	DDX_Control( pDX, IDC_EDIT_COUNT_MULT, m_countMult_Ctrl );
}

void CAG2Modifier_AttachmentEffect::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	CString tempText;

	m_attachmentName_Ctrl.SetWindowText(m_attachmentName);
	m_particleName_Ctrl.SetWindowText(m_particleName);
	m_sizeInput_Ctrl.SetWindowText(m_sizeInput);
	m_countInput_Ctrl.SetWindowText(m_countInput);
	
	tempText.Format("%.2f", m_pos.x);
	m_posX_Ctrl.SetWindowText(tempText);
	tempText.Format("%.2f", m_pos.y);
	m_posY_Ctrl.SetWindowText(tempText);
	tempText.Format("%.2f", m_pos.z);
	m_posZ_Ctrl.SetWindowText(tempText);

	tempText.Format("%.2f", m_sizeOffset);
	m_sizeOffset_Ctrl.SetWindowText(tempText);
	tempText.Format("%.2f", m_countOffset);
	m_countOffset_Ctrl.SetWindowText(tempText);
	tempText.Format("%.2f", m_sizeMult);
	m_sizeMult_Ctrl.SetWindowText(tempText);
	tempText.Format("%.2f", m_countMult);
	m_countMult_Ctrl.SetWindowText(tempText);
}

void CAG2Modifier_AttachmentEffect::OnUserInput()
{
	// Get changed Data from panel
	CString tempText;

	m_attachmentName_Ctrl.GetWindowText(m_attachmentName);
	m_particleName_Ctrl.GetWindowText(m_particleName);
	m_sizeInput_Ctrl.GetWindowText(m_sizeInput);
	m_countInput_Ctrl.GetWindowText(m_countInput);

	m_posX_Ctrl.GetWindowText(tempText);
	m_pos.x = atof(tempText);
	m_posY_Ctrl.GetWindowText(tempText);
	m_pos.y = atof(tempText);
	m_posZ_Ctrl.GetWindowText(tempText);
	m_pos.z = atof(tempText);

	m_sizeOffset_Ctrl.GetWindowText(tempText);
	m_sizeOffset = atof(tempText);
	m_countOffset_Ctrl.GetWindowText(tempText);
	m_countOffset = atof(tempText);
	m_sizeMult_Ctrl.GetWindowText(tempText);
	m_sizeMult = atof(tempText);
	m_countMult_Ctrl.GetWindowText(tempText);
	m_countMult = atof(tempText);
}

const CString CAG2Modifier_AttachmentEffect::GetCustomText() const
{
	CString retVal;

	if (!m_attachmentName.IsEmpty() && !m_particleName.IsEmpty())
		retVal.Format("%s - %s", m_attachmentName, m_particleName);

	if (retVal.IsEmpty())
		retVal = "<not set>";

	return retVal;
}

