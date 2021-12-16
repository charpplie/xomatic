#include "StdAfx.h"
#include "PlaySound.h"


CAG2Modifier_PlaySound::CAG2Modifier_PlaySound() : CAG2ModifierBase()
{
	Reset();
}

CAG2Modifier_PlaySound::~CAG2Modifier_PlaySound()
{
}

CAG2ModifierBase* CAG2Modifier_PlaySound::Duplicate() const
{
	return new CAG2Modifier_PlaySound();
}


void CAG2Modifier_PlaySound::Reset()
{
	m_voice = false;
	m_flags = -1;
	m_delay = 0.0f;
	m_soundName.Empty();
}

const void CAG2Modifier_PlaySound::Save( XmlNodeRef modifierNode ) const
{
	modifierNode->setAttr("voice", m_voice);
	modifierNode->setAttr("flags", m_flags);
	modifierNode->setAttr("delay", m_delay);
	modifierNode->setAttr("soundName", m_soundName);
}

const void CAG2Modifier_PlaySound::Load( XmlNodeRef modifierNode )
{
	modifierNode->getAttr("voice", m_voice);
	modifierNode->getAttr("flags", m_flags);
	modifierNode->getAttr("delay", m_delay);
	modifierNode->getAttr("soundName", m_soundName);
}

const void CAG2Modifier_PlaySound::Export( XmlNodeRef node ) const
{
	// <Sound sound="Languages/dialog/ai_korean01/choking_01.wav" delay="0" voiceSound="1" soundType="4"/>

	// Only write out what is there
	if (m_soundName.IsEmpty())
		return;

	XmlNodeRef soundNode = node->createNode("Sound");
	soundNode->setAttr("sound", m_soundName);
	soundNode->setAttr("delay", m_delay);
	if (m_flags != -1)
		soundNode->setAttr("soundType", m_flags);
	if (m_voice)
		soundNode->setAttr("voiceSound", m_voice);
	node->addChild(soundNode);
}



BEGIN_MESSAGE_MAP(CAG2Modifier_PlaySound, CDialog)
	ON_BN_CLICKED(IDC_CHECK_ISVOICE, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SOUNDNAME, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_DELAY, OnUserInput)
	ON_EN_KILLFOCUS(IDC_EDIT_SOUNDFLAGS, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_PlaySound::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_CHECK_ISVOICE, m_voice_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SOUNDNAME, m_soundName_Ctrl );
	DDX_Control( pDX, IDC_EDIT_DELAY, m_delay_Ctrl );
	DDX_Control( pDX, IDC_EDIT_SOUNDFLAGS, m_flags_Ctrl );
}

void CAG2Modifier_PlaySound::InitParameterPanel()
{
	// Write the modifier info into the panel controls
	CString tempStr;
	m_voice_Ctrl.SetCheck(m_voice);
	m_soundName_Ctrl.SetWindowText(m_soundName);
	tempStr.Format("%.3f", m_delay);
	m_delay_Ctrl.SetWindowText(tempStr);
	tempStr.Format("%i", m_flags);
	m_flags_Ctrl.SetWindowText(tempStr);
}

void CAG2Modifier_PlaySound::OnUserInput()
{
	// Get changed Data from panel
	CString tempStr;
	m_voice = m_voice_Ctrl.GetCheck();
	m_soundName_Ctrl.GetWindowText(m_soundName);
	m_delay_Ctrl.GetWindowText(tempStr);
	m_delay = atof(tempStr);
	m_flags_Ctrl.GetWindowText(tempStr);
	m_flags = atoi(tempStr);

	// Safety check values and modify to sensible values if needed
  if (m_flags < 0)
		m_flags = -1;
	if (m_delay < 0.0f)
		m_delay = 0.0f;
	InitParameterPanel();
}

const CString CAG2Modifier_PlaySound::GetCustomText() const
{
	CString retVal;

	if (!m_soundName.IsEmpty())
		retVal = m_soundName;
	
	if (retVal.IsEmpty())
		retVal = "<not set>";

	return retVal;
}

bool CAG2Modifier_PlaySound::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "Sound"))
		return false;

	if (!node->haveAttr("sound") && !node->haveAttr("delay"))
	{
		if (node->getNumAttributes() > 0)
		{
			AfxMessageBox( "Warning, found a state node <Sound> with attributes, but none known to this AG version.\nSkipping loading. The sound will not play.", MB_OK|MB_ICONERROR );
		}

		return false;
	}

	return true;
}

void CAG2Modifier_PlaySound::ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode )
{
	CRY_ASSERT_MESSAGE(CanConvertFromOldGraphVersion(node), "Trying to convert AG1.0 State Node entry into Sound, but this node is not a proper Sound node!\nDo not call this function unless you called CanConvertFromOldGraphVersion first to confirm the node type!");

	// Make sure there isn't any old data left here in case someone called this
	// function multiple times (which you shouldn't do, but it also does no harm)
	Reset();

	node->getAttr("sound", m_soundName);
	node->getAttr("delay", m_delay);

	if (node->haveAttr("voiceSound"))
	{
		node->getAttr("voiceSound", m_voice);
	}

	if (node->haveAttr("soundType"))
	{
		node->getAttr("soundType", m_flags);
	}
}

