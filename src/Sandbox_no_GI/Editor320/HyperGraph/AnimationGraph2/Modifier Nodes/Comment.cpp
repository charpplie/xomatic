#include "StdAfx.h"
#include "Comment.h"

static const char LINE_BREAK_CHAR = '|';

CAG2Modifier_Comment::CAG2Modifier_Comment() : CAG2ModifierBase()
{
}

CAG2Modifier_Comment::~CAG2Modifier_Comment()
{
}

CAG2ModifierBase* CAG2Modifier_Comment::Duplicate() const
{
	return new CAG2Modifier_Comment();
}


const void CAG2Modifier_Comment::Save( XmlNodeRef modifierNode ) const
{
	// convert the line breaks 
	// Windows Edit Box will put two line breaks characters in the text
	// so remove one and replace the other
	CString writeText = m_noteText;
	writeText.Replace(char(13), LINE_BREAK_CHAR);
	writeText.Remove('\n');

	modifierNode->setAttr("noteText", writeText);
}

const void CAG2Modifier_Comment::Load( XmlNodeRef modifierNode )
{
	// convert the line breaks 
	modifierNode->getAttr("noteText", m_noteText);
	int pos = 0;
	CString mfcLineBreakText;
	mfcLineBreakText.Format("%c\n", char(13));
	while (-1 != (pos = m_noteText.Find(LINE_BREAK_CHAR, pos)))
	{
		++pos;
		m_noteText.Insert(pos, mfcLineBreakText);
	}
	m_noteText.Remove(LINE_BREAK_CHAR);
}

BEGIN_MESSAGE_MAP(CAG2Modifier_Comment,CDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_COMMENT, OnUserInput)
END_MESSAGE_MAP()


void CAG2Modifier_Comment::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_EDIT_COMMENT, m_noteText_Ctrl );
}

void CAG2Modifier_Comment::InitParameterPanel()
{
	m_noteText_Ctrl.SetWindowText(m_noteText);
}

void CAG2Modifier_Comment::OnUserInput()
{
	m_noteText_Ctrl.GetWindowText(m_noteText);
}

const CString CAG2Modifier_Comment::GetCustomText() const
{
	CString retVal;
	retVal = m_noteText;
	return retVal;
}

