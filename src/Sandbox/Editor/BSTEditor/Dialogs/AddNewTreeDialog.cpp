#include "stdafx.h"
#include "AddNewTreeDialog.h"

#include "DialogCommon.h"

#include "BSTEditor/SelectionTreeManager.h"


IMPLEMENT_DYNAMIC(CAddNewTreeDialog, CDialog)
CAddNewTreeDialog::CAddNewTreeDialog(CWnd* pParent /*=NULL*/)
: CDialog(CAddNewTreeDialog::IDD, pParent)
{
}

void CAddNewTreeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
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

	return TRUE;

}

void CAddNewTreeDialog::OnBnClickedOk()
{
	CString tempString;
	m_editName.GetWindowText(tempString);
	m_name = tempString;
	OnOK();
}

