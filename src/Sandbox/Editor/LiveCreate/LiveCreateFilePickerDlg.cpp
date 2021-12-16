#include "StdAfx.h"
#include "ILiveCreateCommon.h"
#include "ILiveCreatePlatform.h"
#include "LiveCreateFilePickerDlg.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "EditorLiveCreateTasks.h"

#ifndef NO_LIVECREATE

IMPLEMENT_DYNAMIC(CLiveCreateFilePickerDlg, CDialog)

//-----------------------------------------------------------------------------

CLiveCreateFilePickerDlg::STreeItem::STreeItem(const STreeItem* parent, const CString& name, LiveCreate::IPlatformHandler* pPlatform)
	: m_name(name)
	, m_pParent(parent)
	, m_hItem(NULL)
	, m_pScanTask(NULL)
	, m_bWasScanned(false)
	, m_bIsConfirmed(false)
	, m_bIsExpaned(false)
	, m_bIsExecutable(false)
	, m_pPlatform(pPlatform)
{
}

CLiveCreateFilePickerDlg::STreeItem::~STreeItem()
{
	// delete sub objects
	for (int i=0; i<m_pSubDirs.size(); ++i)
	{
		delete m_pSubDirs[i];
	}
	m_pSubDirs.clear();

	if (NULL != m_pScanTask)
	{
		m_pScanTask->Cancel();
		m_pScanTask->Release();
		m_pScanTask = NULL;
	}
}

CLiveCreateFilePickerDlg::STreeItem* CLiveCreateFilePickerDlg::STreeItem::CreateSubDirectory(const CString& name, bool bFromScan)
{
	// confirm existing entries
	for (int i=0; i<m_pSubDirs.size(); ++i)
	{
		if (m_pSubDirs[i]->m_name == name && !m_pSubDirs[i]->m_bIsExecutable)
		{
			m_pSubDirs[i]->m_bIsConfirmed = true;
			return m_pSubDirs[i];
		}
	}

	// create new entry
	STreeItem* pDir = new STreeItem(this, name,m_pPlatform);
	pDir->m_bIsExecutable = false;
	pDir->m_bIsConfirmed = bFromScan; // was created by directory scan?
	m_pSubDirs.push_back(pDir);
	return pDir;
}

CLiveCreateFilePickerDlg::STreeItem* CLiveCreateFilePickerDlg::STreeItem::CreateExecutableFile(const CString& name, bool bFromScan)
{
	// confirm existing entries
	for (int i=0; i<m_pSubDirs.size(); ++i)
	{
		if (m_pSubDirs[i]->m_name == name && m_pSubDirs[i]->m_bIsExecutable)
		{
			m_pSubDirs[i]->m_bIsConfirmed = true;
			return m_pSubDirs[i];
		}
	}

	// create new entry
	STreeItem* pDir = new STreeItem(this, name, m_pPlatform);
	pDir->m_bIsExecutable = true;
	pDir->m_bIsConfirmed = bFromScan; // was created by scan?
	m_pSubDirs.push_back(pDir);
	return pDir;
}

CString CLiveCreateFilePickerDlg::STreeItem::FormatFullPath() const
{
	CString fullDir;
	if (NULL != m_pParent)
	{
		// start with dir name
		if (m_bIsExecutable)
		{
			fullDir = m_name;
		}
		else
		{
			fullDir = m_name + CString("\\");
		}

		// add parent directories
		const STreeItem* pDir = m_pParent;
		while (NULL != pDir->m_pParent)
		{
			const CString dirName = pDir->m_name + CString("\\");
			fullDir = dirName + fullDir;
			pDir = pDir->m_pParent;
		}
	}

	// add platform root path
	const CString platformRoot = m_pPlatform->GetRootPath();
	fullDir = platformRoot + fullDir;

	return fullDir;
}

void CLiveCreateFilePickerDlg::STreeItem::Scan()
{
	if (!m_bWasScanned && NULL==m_pScanTask && !m_bIsExecutable)
	{
		// assemble full directory path ended with "\"
		const CString fullDir = FormatFullPath();

		// create the directory scanning task
		m_pScanTask = new LiveCreate::CBGTask_ScanDirectory(m_pPlatform, fullDir);
		m_pScanTask->AddRef();
		GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pScanTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
	}
}

bool CLiveCreateFilePickerDlg::STreeItem::Update(CTreeCtrl& treeCtrl)
{
	bool bRequiresRedraw = false;

	// directory update
	if (NULL != m_pScanTask)
	{
		if (m_pScanTask->HasFinished())
		{
			// create sub-directory items
			const uint numDirs = m_pScanTask->m_directories.size();
			for (uint i=0; i<numDirs; ++i)
			{
				const CString& dirName = m_pScanTask->m_directories[i];
				CreateSubDirectory(dirName, true);
			}

			// create the executable items
			const uint numExecutables = m_pScanTask->m_executables.size();
			for (uint i=0; i<numExecutables; ++i)
			{
				const CString& fileName = m_pScanTask->m_executables[i];
				CreateExecutableFile(fileName, true);
			}

			// cleanup
			m_pScanTask->Release();
			m_pScanTask = NULL;

			// mark as scanned
			m_bWasScanned = true;

			// recreate missing tree items
			if (CreateItems(treeCtrl))
			{
				bRequiresRedraw = true;
			}
		}
	}

	// sub item update
	for (uint i=0; i<m_pSubDirs.size(); ++i)
	{
		if (m_pSubDirs[i]->Update(treeCtrl))
		{
			bRequiresRedraw = true;
		}
	}

	return bRequiresRedraw;
}

bool CLiveCreateFilePickerDlg::STreeItem::CreateItemsRecursive(CTreeCtrl& treeCtrl)
{
	bool bItemsCreated = CreateItems(treeCtrl);

	for (uint i=0; i<m_pSubDirs.size(); ++i)
	{
		if (m_pSubDirs[i]->CreateItemsRecursive(treeCtrl))
		{
			bItemsCreated = true;
		}
	}

	return bItemsCreated;
}

CLiveCreateFilePickerDlg::STreeItem* CLiveCreateFilePickerDlg::STreeItem::FindItem(HTREEITEM hItem)
{
	if (m_hItem == hItem)
	{
		return this;
	}

	for (uint i=0; i<m_pSubDirs.size(); ++i)
	{
		STreeItem* pDir = m_pSubDirs[i]->FindItem(hItem);
		if (NULL != pDir)
		{
			return pDir;
		}
	}

	return NULL;
}

bool CLiveCreateFilePickerDlg::STreeItem::OnItemExpanded(CTreeCtrl& treeCtrl, HTREEITEM hItem)
{
	if (m_hItem == hItem)
	{
		for (uint i=0; i<m_pSubDirs.size(); ++i)
		{
			m_pSubDirs[i]->Scan();
		}
		
		return true;
	}

	for (uint i=0; i<m_pSubDirs.size(); ++i)
	{
		if (m_pSubDirs[i]->OnItemExpanded(treeCtrl, hItem))
		{
			return true;
		}
	}

	return false;
}

void CLiveCreateFilePickerDlg::STreeItem::CreateRootItem(CTreeCtrl& treeCtrl)
{
	// format console name
	CString targetName = m_pPlatform->GetTargetName();
	targetName += " (";
	targetName += m_pPlatform->GetPlatformName();
	targetName += ")";

	// create root item
	m_hItem = treeCtrl.InsertItem(targetName, 1, 1);
}

bool CLiveCreateFilePickerDlg::STreeItem::CreateItems(CTreeCtrl& treeCtrl)
{
	bool bItemCreated = false;

	// we were not yet created
	if (NULL != m_hItem)
	{
		// create missing directory item
		for (uint i=0; i<m_pSubDirs.size(); ++i)
		{
			STreeItem* pSubDir = m_pSubDirs[i];

			// use different icon for directory and executable
			const int icon = pSubDir->m_bIsExecutable ? 3 : 0; 

			// create item
			if (pSubDir->m_hItem == NULL)
			{
				if ( pSubDir->m_bIsExecutable )
				{
					const CString fileName = CString(" ") + pSubDir->m_name;
					pSubDir->m_hItem = treeCtrl.InsertItem(fileName, icon, icon, m_hItem);
				}
				else
				{
					pSubDir->m_hItem = treeCtrl.InsertItem(pSubDir->m_name, icon, icon, m_hItem);
				}
				bItemCreated = true;

				// all new items are expandable until proven empty :)
				treeCtrl.SetItemState(pSubDir->m_hItem, TVIF_CHILDREN, 1);
			}
			else
			{
				// update item icon
				treeCtrl.SetItemImage(pSubDir->m_hItem, icon, icon);
			}
		}

		// expand
		if (m_bIsExpaned)
		{
			treeCtrl.Expand(m_hItem,TVE_EXPAND);
		}

		if (bItemCreated)
		{
			treeCtrl.SortChildren(m_hItem);
		}

		// update expandable flag
		if (m_bWasScanned)
		{
			const bool bHasChildren = !m_pSubDirs.empty();
			treeCtrl.SetItemState(m_hItem, TVIF_CHILDREN, bHasChildren ? 1 : 0);
		}
	}

	return bItemCreated;
}

//-----------------------------------------------------------------------------

CLiveCreateFilePickerDlg::CLiveCreateFilePickerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLiveCreateFilePickerDlg::IDD, pParent)
	, m_pPlatform(NULL)
	, m_pRoot(NULL)
	, m_pSelected(NULL)
{
}

CLiveCreateFilePickerDlg::~CLiveCreateFilePickerDlg()
{
	// delete directory tree
	SAFE_DELETE(m_pRoot);
}

void CLiveCreateFilePickerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DIRECTORY_TREE, m_treeView);
	DDX_Control(pDX, IDOK, m_btnOK);
}

BEGIN_MESSAGE_MAP(CLiveCreateFilePickerDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CLiveCreateFilePickerDlg::OnBnClickedOk)
	ON_NOTIFY(TVN_ITEMEXPANDED,IDC_DIRECTORY_TREE,&CLiveCreateFilePickerDlg::OnTreeItemExpand)
	ON_NOTIFY(TVN_SELCHANGED,IDC_DIRECTORY_TREE,&CLiveCreateFilePickerDlg::OnTreeItemSelected)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CLiveCreateFilePickerDlg::Setup(LiveCreate::IPlatformHandler* pPlatform, const CString& executableName)
{
	m_pPlatform = pPlatform;

	// extract propper directory and file name
	{
		char szPathDrive[MAX_PATH];
		char szPathDir[MAX_PATH];
		char szPathName[MAX_PATH];
		char szPathExt[MAX_PATH];
		_splitpath_s((const char*)executableName,
			szPathDrive, MAX_PATH,
			szPathDir, MAX_PATH, 
			szPathName, MAX_PATH, 
			szPathExt, MAX_PATH );

		m_selectedDirectory = szPathDrive;
		m_selectedDirectory += szPathDir;

		m_selectedExecutable = szPathName;
		m_selectedExecutable += szPathExt;
	}

	// create root directory
	m_pRoot = new STreeItem(NULL,"", m_pPlatform);
	m_pRoot->Scan();

	// create path items along the way
	if (!m_selectedDirectory.IsEmpty())
	{
		std::vector<CString> directories;
		SplitString(m_selectedDirectory, directories, '\\');

		STreeItem* pCurDir = m_pRoot;
		for (int i=1; i<directories.size(); ++i)
		{
			const CString& dirName = directories[i];
			pCurDir = pCurDir->CreateSubDirectory(dirName, false);
			pCurDir->Scan();
		}

		// create last entry - executable
		if (executableName.IsEmpty())
		{
			m_pSelected = pCurDir;
		}
		else
		{
			m_pSelected = pCurDir->CreateExecutableFile(m_selectedExecutable,false);
		}
	}

	// use root as default selection
	if (NULL == m_pSelected)
	{
		m_pSelected = m_pRoot;
	}
}

BOOL CLiveCreateFilePickerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Attach image list
	CMFCUtils::LoadTrueColorImageList( m_cImageList,IDB_TREE_VIEW,16,RGB(255,0,255) );
	m_treeView.SetImageList(&m_cImageList, TVSIL_NORMAL);

	SetTimer(kTimerRefresh, kRefreshTimerPeriod,NULL);

	// create items that already exists
	m_pRoot->CreateRootItem(m_treeView);
	m_pRoot->CreateItemsRecursive(m_treeView);

	// select the current item
	m_treeView.EnsureVisible(m_pSelected->m_hItem);
	m_treeView.SelectItem(m_pSelected->m_hItem);

	UpdateOKButton();
	return TRUE;
}

void CLiveCreateFilePickerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == kTimerRefresh)
	{
		if (m_pRoot->Update(m_treeView))
		{
			m_treeView.RedrawWindow();
		}

		SetTimer(kTimerRefresh, kRefreshTimerPeriod,NULL);
	}
}

void CLiveCreateFilePickerDlg::OnBnClickedOk()
{
	if (m_pSelected != NULL && m_pSelected->m_bIsExecutable)
	{
		// get executable name and parent directory
		m_selectedExecutable = m_pSelected->m_name;
		m_selectedDirectory = m_pSelected->m_pParent->FormatFullPath();

		// we can exit
		CDialog::OnOK();
	}
}

void CLiveCreateFilePickerDlg::OnTreeItemSelected(NMHDR* pNMHDR, LRESULT* pResult)
{
	UpdateOKButton();
}

void CLiveCreateFilePickerDlg::OnTreeItemExpand(NMHDR* pNMHDR, LRESULT* pResult)
{
	 LPNMTREEVIEW pItem = (LPNMTREEVIEW) pNMHDR;
	 HTREEITEM hItem = pItem->itemNew.hItem;
	 m_pRoot->OnItemExpanded(m_treeView, hItem);
}

void CLiveCreateFilePickerDlg::UpdateOKButton()
{
	HTREEITEM hItem = m_treeView.GetSelectedItem();
	if (hItem != NULL)
	{
		m_pSelected = m_pRoot->FindItem(hItem);
	}
	else
	{
		m_pSelected = NULL;
	}

	// we can exit this dialog only with valid selection
	if (m_pSelected != NULL && m_pSelected->m_bIsExecutable && !m_pSelected->m_name.IsEmpty())
	{
		m_btnOK.EnableWindow(TRUE);
	}
	else
	{
		m_btnOK.EnableWindow(FALSE);
	}
}

#endif