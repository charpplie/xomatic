#include "stdafx.h"
#include "AddNewSignalDialog.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "DialogCommon.h"

IMPLEMENT_DYNAMIC(CAddNewSignalDialog, CDialog)
CAddNewSignalDialog::CAddNewSignalDialog(CWnd* pParent /*=NULL*/)
: CDialog(CAddNewSignalDialog::IDD, pParent)
{
}

void CAddNewSignalDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BST_VARIABLES_NAME_COMBO, m_comboBoxVariableName);
	DDX_Control(pDX, IDC_BST_VARIABLE_VALUE_COMBO, m_comboBoxVariableValue);
	DDX_Control(pDX, IDD_BST_ADD_NEW_SIGNAL, m_comboBoxSignals);
}

BEGIN_MESSAGE_MAP(CAddNewSignalDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CAddNewSignalDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDialogPositionHelper::SetToMouseCursor(*this);

	// Value
	m_comboBoxVariableValue.AddString("True");
	m_comboBoxVariableValue.AddString("False");
	SetSelection( m_comboBoxVariableValue, m_bVariableValue ? "True" : "False" );

	m_comboBoxVariableValue.SetCurSel( m_bVariableValue ? 1 : 0);

	// Vars
	std::list< string > varList;
	GetVariables( varList );
	for ( std::list< string >::iterator it = varList.begin(); it != varList.end(); ++it )
	{
		m_comboBoxVariableName.AddString( *it );
	}
	SetSelection( m_comboBoxVariableName, m_variableName );


	// signals
	std::list< string > sigList;
	GetSignals( sigList );
	bool inList = false;
	for ( std::list< string >::iterator it = sigList.begin(); it != sigList.end(); ++it )
	{
		m_comboBoxSignals.AddString( *it );
		if (m_signalName == *it) inList = true;
	}
	if (!inList && m_signalName.length() > 0)
		m_comboBoxSignals.AddString( m_signalName );
	SetSelection( m_comboBoxSignals, m_signalName );

	return TRUE;

}

void CAddNewSignalDialog::OnBnClickedOk()
{
	CString tempString;
	m_comboBoxSignals.GetWindowText(tempString);
	m_signalName = tempString;
	m_comboBoxVariableValue.GetWindowText(tempString);
	m_bVariableValue = (tempString == "True") ? true : false;
	m_comboBoxVariableName.GetWindowText(tempString);
	m_variableName = tempString;
	OnOK();
}


void CAddNewSignalDialog::SetSelection( CComboBox& comboBox, const string& selection )
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

void CAddNewSignalDialog::GetVariables( std::list< string >& strList )
{
	SSelectionTreeInfo currTreeInfo;
	if ( GetIEditor()->GetSelectionTreeManager()->GetCurrentTreeInfo(currTreeInfo) )
	{
		SSelectionTreeBlockInfo varInfo;
		bool ok = currTreeInfo.GetBlockById(varInfo, eSTTI_Variables );
		if ( ok )
		{
			LoadVarsFromXml( strList, varInfo.XmlData );
		}
	}
}

void CAddNewSignalDialog::LoadVarsFromXml( std::list< string >& strList, const XmlNodeRef& xmlNode )
{
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		const XmlNodeRef& child = xmlNode->getChild( i );
		const char* tag = child->getTag();
		if ( tag )
		{
			if ( strcmpi( tag, "Variable" ) == 0)
			{
				strList.push_back( child->getAttr( "name" ) );
			}
			else if ( strcmpi( tag, "Ref" ) == 0)
			{
				SSelectionTreeBlockInfo refInfo;
				bool found = GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Variables, child->getAttr( "name" ), refInfo );

				assert( found );
				if ( found )
				{
					LoadVarsFromXml( strList, refInfo.XmlData );
				}
			}
		}
	}
}


void CAddNewSignalDialog::GetSignals( std::list< string >& strList )
{
	XmlNodeRef signals = gEnv->pSystem->LoadXmlFromFile("Scripts/AI/SystemSignals.xml");
	if ( signals )
	{
		XmlNodeRef systemsignals = signals->findChild("SystemSignals");
		for ( int i = 0; i < systemsignals->getChildCount(); ++i )
		{
			XmlNodeRef syssig = systemsignals->getChild( i );
			if ( strcmpi(syssig->getTag(), "Signal") == 0 )
			{
				const char* name = syssig->getAttr("name");
				if ( name )
				{
					strList.push_back( name );
				}
			}
		}
	}
}
