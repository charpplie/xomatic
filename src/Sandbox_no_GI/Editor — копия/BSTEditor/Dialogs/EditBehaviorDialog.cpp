#include "stdafx.h"
#include "EditBehaviorDialog.h"

#include "DialogCommon.h"

#include "AI/AIManager.h"
#include "AI/AiBehaviorLibrary.h"

IMPLEMENT_DYNAMIC(CEditBehaviorDialog, CDialog)
CEditBehaviorDialog::CEditBehaviorDialog(CWnd* pParent /*=NULL*/)
: CDialog(CEditBehaviorDialog::IDD, pParent)
{
}

void CEditBehaviorDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDD_BST_BEHAVIOR_LIST, m_comboBehaviorList);
}

BEGIN_MESSAGE_MAP(CEditBehaviorDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CEditBehaviorDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDialogPositionHelper::SetToMouseCursor(*this, true);

	CDialog::SetWindowText(m_title.c_str());
	LoadBehaviors();

	for ( int i = 0; i < m_comboBehaviorList.GetCount(); ++i )
	{
		CString str;
		m_comboBehaviorList.GetLBText( i, str );
		if ( string(str.GetString()) == m_behaviorName )
		{
			m_comboBehaviorList.SetCurSel( i );
			break;
		}
	}

	return TRUE;
}

void CEditBehaviorDialog::OnBnClickedOk()
{
	CString tempString;
	m_comboBehaviorList.GetWindowText(tempString);
	m_behaviorName = tempString;
	OnOK();
}

void CEditBehaviorDialog::LoadBehaviors()
{
	std::vector<CAIBehaviorPtr> behaviors;
	GetIEditor()->GetAI()->GetBehaviorLibrary()->GetBehaviors(behaviors);
	bool ok = false;
	for (std::vector<CAIBehaviorPtr>::iterator it = behaviors.begin(); it != behaviors.end(); ++it)
	{
		m_comboBehaviorList.AddString( (*it)->GetName() );	
		if ( m_behaviorName == string((*it)->GetName().GetString()) )
			ok = true;
	}
	if (!ok)
		m_comboBehaviorList.AddString( m_behaviorName );	
}