#include "stdafx.h"
#include "AddNewTreeDialog.h"

#include "DialogCommon.h"

#include "BSTEditor/SelectionTreeManager.h"

string g_BSTTypeStr[] = { 
	"BehaviorSelectionTree",
	"LIST_END"
};


IMPLEMENT_DYNAMIC(CAddNewTreeDialog, CDialog)
CAddNewTreeDialog::CAddNewTreeDialog(CWnd* pParent /*=NULL*/)
: CDialog(CAddNewTreeDialog::IDD, pParent)
{
}

void CAddNewTreeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDD_BST_TREE_TYPE, m_comboBoxType);
	DDX_Control(pDX, IDC_BST_TREE_NAME, m_editName);
}

BEGIN_MESSAGE_MAP(CAddNewTreeDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CAddNewTreeDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDialogPositionHelper::SetToMouseCursor(*this);

	SetWindowText(m_title.c_str());

	m_editName.SetWindowText( m_name.c_str() );

	if (m_type.length() == 0)
		m_type = g_BSTTypeStr[0];

	int i = 0;
	while ( g_BSTTypeStr[i] != "LIST_END" )
	{
		m_comboBoxType.AddString( g_BSTTypeStr[i++].c_str() );
	}
	SetSelection( m_comboBoxType, m_type );

	return TRUE;

}

void CAddNewTreeDialog::OnBnClickedOk()
{
	CString tempString;
	m_editName.GetWindowText(tempString);
	m_name = tempString;
	m_comboBoxType.GetWindowText(tempString);
	m_type = tempString;
	OnOK();
}


void CAddNewTreeDialog::SetSelection( CComboBox& comboBox, const string& selection )
{
	for ( int i = 0; i < comboBox.GetCount(); ++i )
	{
		CString str;
		comboBox.GetLBText( i, str );
		if ( string(str.GetString()) == selection )
		{
			comboBox.SetCurSel( i );
			break;
		}
	}
}
