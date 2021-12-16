////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PythonScriptsDialog.h"
#include "Controls/FolderTreeCtrl.h"

IMPLEMENT_DYNCREATE(CPythonScriptsDialog, CXTResizeDialog)

//////////////////////////////////////////////////////////////////////////
namespace
{
	// File name extension for python files
	const CString s_kSequenceFileNameSpec = "*.py";

	// Tree root element name
	const CString s_kRootElementName = "Python Scripts";
}

//////////////////////////////////////////////////////////////////////////
class CPythonScriptsDialogClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {C61C9C4C-CFED-47C4-8FE1-79069D0284E1}
		static const GUID guid =  { 0xc61c9c4c, 0xcfed, 0x47c4, { 0x8f, 0xe1, 0x79, 0x6, 0x9d, 0x2, 0x84, 0xe1 } };

		return guid;
	}
	virtual const char* ClassName() { return "Python Scripts"; };
	virtual const char* Category() { return "Editor"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CPythonScriptsDialog); };
	virtual const char* GetPaneTitle() { return _T("Python Scripts"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(0,0,500,300); };
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CPythonScriptsDialogClass );
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CPythonScriptsDialog, CXTResizeDialog)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE, OnTreeDoubleClicked)
	ON_COMMAND(IDC_EXECUTE, OnExecute)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CPythonScriptsDialog::CPythonScriptsDialog() : CXTResizeDialog( CPythonScriptsDialog::IDD )
{
	std::vector<CString> scriptFolders;

	XmlNodeRef envNode = XmlHelpers::LoadXmlFromFile(gSettings.strEditorEnv);
	if ( envNode )
	{
		CString scriptPath;
		int childrenCount = envNode->getChildCount();
		for(int idx = 0; idx < childrenCount; ++idx )
		{
			XmlNodeRef child = envNode->getChild(idx);
			if ( child->haveAttr("scriptPath") )
			{
				scriptPath = child->getAttr("scriptPath");
				scriptFolders.push_back(scriptPath);
			}
		}
	}

	m_pTree = new CFolderTreeCtrl(scriptFolders, s_kSequenceFileNameSpec, s_kRootElementName, false, false );
	Create(IDD, nullptr);
}

//////////////////////////////////////////////////////////////////////////
BOOL CPythonScriptsDialog::OnInitDialog()
{
	__super::OnInitDialog();

	SetResize( IDC_TREE, SZ_RESIZE( 1.0f ) );
	SetResize( IDC_EXECUTE, SZ_REPOS( 1.0f ) );

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	DDX_Control(pDX, IDC_TREE, *m_pTree);		
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::OnTreeDoubleClicked( NMHDR* pNMHDR, LRESULT* pResult )
{
	OnExecute();
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::OnExecute()
{
	HTREEITEM selectedItem = m_pTree->GetSelectedItem();

	if ( selectedItem == NULL )
		return;

	if ( m_pTree->IsFile( selectedItem ) )
	{
		char workingDirectory[MAX_PATH];
		GetCurrentDirectory( MAX_PATH, workingDirectory );
		const CString scriptPath = CString(workingDirectory) + "/" + m_pTree->GetPath( selectedItem );
		GetIEditor()->ExecuteCommand("general.run_file '%s'", scriptPath.GetString() );
	}
}