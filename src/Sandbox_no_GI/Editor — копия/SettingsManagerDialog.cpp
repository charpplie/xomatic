#include "stdafx.h"
#include "stdlib.h"
#include <stdio.h>
#include "SettingsManagerDialog.h"

IMPLEMENT_DYNCREATE(CSettingsManagerFrame,CBaseFrameWnd)


//////////////////////////////////////////////////////////////////////////
class CSettingsManagerViewClass : public IViewPaneClass
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {64E0B47F-FA9B-46a9-AEF4-BDAC021B5B2F}
		static const GUID guid = 
		{ 0x64e0b47f, 0xfa9b, 0x46a9, { 0xae, 0xf4, 0xbd, 0xac, 0x2, 0x1b, 0x5b, 0x2f } };

		return guid;
	}
	virtual const char* ClassName() { return _T(SETTINGS_MANAGER_WINDOW_NAME); };
	virtual const char* Category() { return _T(SETTINGS_MANAGER_WINDOW_NAME); };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CSettingsManagerFrame); };
	virtual const char* GetPaneTitle() { return _T(SETTINGS_MANAGER_WINDOW_NAME); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(5,100,210,505); };
	virtual bool SinglePane() { return false; };
	virtual bool WantIdleUpdate() { return true; };
};
REGISTER_CLASS_DESC(CSettingsManagerViewClass)

CSettingsManagerFrame::CSettingsManagerFrame()
{
	CRect rcClient(5,100,210,505);
	Create(WS_CHILD|WS_VISIBLE|WM_VSCROLL,rcClient, AfxGetMainWnd() );
}

BOOL CSettingsManagerFrame::OnInitDialog()
{
	CRect dlgRect(5,100,195,500);
	dlg.Create(IDD_SETTINGS_MANAGER_DIALOG,this);
	dlg.SetWindowPos(NULL,-5,-5,dlgRect.Width(),dlgRect.Height(),SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
	dlg.ShowWindow(1);

	return TRUE;  // return TRUE unless you set the focus to a control
}

CSettingsManagerDialog* CSettingsManagerFrame::GetDialog()
{
	return &dlg;
}

//////////////////////////////////////////////////////////////////////////
CSettingsManagerDialog::CSettingsManagerDialog(CWnd* pParent)
: CDialog(CSettingsManagerDialog::IDD, pParent)
{
}

CSettingsManagerDialog::~CSettingsManagerDialog()
{
}

BOOL CSettingsManagerDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_importThemeChk.SetCheck(TRUE);
	m_skipToolsChk.SetCheck(FALSE);
	m_importSettingsChk.SetCheck(FALSE);

	return TRUE;
}

void CSettingsManagerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_SETTINGS_MANAGER_LOADTHEME, m_importThemeChk );
	DDX_Control(pDX, IDD_SETTINGS_MANAGER_SKIPTOOLS, m_skipToolsChk);
	DDX_Control(pDX, IDC_SETTINGS_MANAGER_SETTINGS_CHK, m_importSettingsChk );
	DDX_Control(pDX, IDC_SETTINGS_MANAGER_LAYOUTS_LST, m_layoutListBox );
}

BEGIN_MESSAGE_MAP(CSettingsManagerDialog, CDialog)
	ON_BN_CLICKED(IDC_SETTINGS_MANAGER_EXPORT_BTN, OnExportBtnClick)
	ON_BN_CLICKED(IDC_SETTINGS_MANAGER_READ_BTN, OnReadBtnClick)
	ON_BN_CLICKED(IDC_SETTINGS_IMPORT_BTN, OnImportBtnClick)
	ON_BN_CLICKED(IDC_SETTINGS_MANAGER_CLOSETOOLS, OnCloseAllTools)
END_MESSAGE_MAP()

void CSettingsManagerDialog::OnReadBtnClick()
{
	char szFilters[] = "Editor Settings and Layout File (*.xml) |All files (*.*)|*.*||";
	CAutoDirectoryRestoreFileDialog importFileSelectionDialog(TRUE, "xml", "*.xml", OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, szFilters);

	if (importFileSelectionDialog.DoModal() == IDOK)
	{
		CString readFileStr = importFileSelectionDialog.GetPathName().GetBuffer(1);
		m_importFileStr = readFileStr;

		m_layoutListBox.ResetContent();

		TToolNamesMap toolNames;
		XmlNodeRef dummyNode = NULL; 

		GetIEditor()->GetSettingsManager()->GetMatchingLayoutNames(toolNames,dummyNode,readFileStr);

		if (!toolNames.empty())
		{
			TToolNamesMap::const_iterator tIt = toolNames.begin();

			for ( ; tIt!=toolNames.end()  ; ++tIt )
			{
				if (!tIt->second.IsEmpty())
				{
					m_layoutListBox.AddString(tIt->second);
					m_layoutListBox.SetSel(m_layoutListBox.FindString(0,tIt->second),TRUE);
				}
				else
					if (!tIt->first.IsEmpty())
					{
						m_layoutListBox.AddString(tIt->first);
						m_layoutListBox.SetSel(m_layoutListBox.FindString(0,tIt->first),TRUE);
					}
			}
		}

	}
}

void CSettingsManagerDialog::OnExportBtnClick()
{
	char szFilters[] = "Editor Settings and Layout File (*.xml) |All files (*.*)|*.*||";
	CAutoDirectoryRestoreFileDialog exportFileSelectionDialog(FALSE, "xml", "ExportedEditor.xml", OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, szFilters);

	if (exportFileSelectionDialog.DoModal() == IDOK)
	{
		GetIEditor()->GetSettingsManager()->SetExportFileName(exportFileSelectionDialog.GetPathName().GetBuffer(1));
		((CMainFrame*)AfxGetMainWnd())->ClosePane(SETTINGS_MANAGER_WINDOW_NAME);
		GetIEditor()->GetSettingsManager()->Export();
	}
}

void CSettingsManagerDialog::OnImportBtnClick()
{
	CString importFileStr = m_importFileStr;
	bool bImportTheme = m_importThemeChk.GetCheck();
	bool bImportSettings = m_importSettingsChk.GetCheck();

	CString ask( "This will close all opened Views. Make sure to save your projects and backup layout before continuing" );

	// Show warning window to the user in case he selects some layouts to import
	if (m_layoutListBox.GetSelCount()>0 )
	{

		if (AfxMessageBox( ask,MB_YESNO|MB_ICONQUESTION ) == IDYES)
		{
		}
		else
		{
			return;
		}
	}

	if (bImportTheme)
	{
		GetIEditor()->GetSettingsManager()->ReadThemeSettings(importFileStr);

		// Update Skins Manager
		//((CMainFrame*)AfxGetMainWnd())->UpdateSkinning((XTPDockingPanePaintTheme)gSettings.gui.paneTheme,(XTPPaintTheme)gSettings.gui.paintTheme);
	}

	// Import Settings
	if (bImportSettings)
		ImportSettings(importFileStr);

	((CMainFrame*)AfxGetMainWnd())->CloseAllOpenedTools();
	
	// Import layout
	if (m_layoutListBox.GetSelCount()>0 )
		ImportLayouts(importFileStr);
}

void CSettingsManagerDialog::ImportSettings(CString file)
{
	if (FILEEXISTS_S(file))
		GetIEditor()->GetSettingsManager()->ImportSettings(file);
}

void CSettingsManagerDialog::ImportLayouts(CString file)
{
	int count = m_layoutListBox.GetSelCount();
	bool mainLayoutImported=false;
	CString mainLayoutStr("");

	if (count > 0)
	{
		CXTPDockingPaneManager *man = ((CMainFrame*)AfxGetMainWnd())->GetDockingPaneManager();
		CXTPDockingPaneLayout layout(man);
		CXTPDockingPaneLayout mainFrmlayout(man);

		std::vector<int> selItems(count);

		m_layoutListBox.GetSelItems( count, &selItems[0] );

		for (int i=0; i<count; ++i)
		{
			TToolNamesMap toolNames;
			CString layoutStr("");
			m_layoutListBox.GetText(selItems[i],layoutStr);

			TToolNamesMap *allToolNames = GetIEditor()->GetSettingsManager()->GetToolNames();

			if (allToolNames->empty())
				return;

			TToolNamesMap::const_iterator toolIT = allToolNames->begin();
			for ( ; toolIT!=allToolNames->end(); ++toolIT )
			{
				if (toolIT->second==layoutStr)
				{
					layoutStr=toolIT->first;
					break;
				}
			}

			if (layoutStr.IsEmpty())
				continue;

			toolNames[layoutStr]=layoutStr;			

			// Remove previous tool node
			XmlNodeRef layoutNode = XmlHelpers::CreateXmlNode(EDITOR_LAYOUT_NODE);

			// Find in file and add selected tool node
			GetIEditor()->GetSettingsManager()->GetMatchingLayoutNames(toolNames,layoutNode,m_importFileStr );

			IXMLDOMDocumentPtr domDocument;		
			HRESULT hr = domDocument.CreateInstance(CLSID_DOMDocument);
			if (FAILED(hr))
				return;

			IXMLDOMNode *pDockingLayoutNode = NULL;

			if (layoutNode->getChildCount() == 0 )
				continue;

			CString xmlContent = layoutNode->getXML(0);
			BSTR tmpStr = xmlContent.AllocSysString();

			_bstr_t xmlString("");
			VARIANT_BOOL isSuccessful=false;
			CComBSTR dockingPanelNodeName(EDITOR_LAYOUT_NODE);


			if (SysStringLen(tmpStr) > 0 )
			{
				xmlString += tmpStr;
				domDocument->loadXML(xmlString,&isSuccessful);
			}

			SysFreeString(tmpStr);
			tmpStr=NULL;

			if (isSuccessful)
			{
				CComBSTR toolLayoutStr(layoutStr);
				domDocument->selectSingleNode(dockingPanelNodeName,&pDockingLayoutNode);

				if (!pDockingLayoutNode)
					continue;
			
				if ( layout.LoadFromNode(pDockingLayoutNode,layoutStr) )
				{
					layout.Save(layoutStr);

					if (layout.GetPaneList().GetCount() > 0)
					{
						if ( (layoutStr==MAINFRM_LAYOUT_NORMAL) || (layoutStr==MAINFRM_LAYOUT_PREVIEW) )
							man->SetLayout(&layout);
						else
						{
							if (GetIEditor()->FindView(layoutStr))
								man->SetLayout(&layout);
						}


					}
				}
			}
		}
	}
}

void CSettingsManagerDialog::OnCloseAllTools()
{
	((CMainFrame*)AfxGetMainWnd())->CloseAllOpenedTools();

	CXTPDockingPaneManager *dm = ((CMainFrame*)AfxGetMainWnd())->GetDockingPaneManager();

	if (!dm)
		return;

	CXTPDockingPaneLayout layout(dm);
	dm->GetLayout(&layout);
	
	if (!GetIEditor()->IsInPreviewMode())
		layout.Save(_T(MAINFRM_LAYOUT_NORMAL));
	else
		layout.Save(_T(MAINFRM_LAYOUT_PREVIEW));
}