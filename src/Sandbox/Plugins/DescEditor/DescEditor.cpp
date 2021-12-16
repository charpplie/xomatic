////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 2013-3013
// -------------------------------------------------------------------------
//  File Name        : DescEditor.cpp
//  Version          : v1.00
//  Created          : 3/21/2013 by Jack Harmon
//  Description      : Custom editor dialog for ObjectDescFactory objects
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "DescEditor.h"

#include <IGame.h>
#include <ISystem.h>
#include "functor.h"

#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IGameInterface.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\Serialize.h>

#include "PropertiesPanel.h"
#include "GameEngine.h"
#include "..\Editor\Controls\PropertyItem.h"
#include "Dialogs\SimpleEditPopup.h"
#include "EngineSettingsManager.h"


// PropertyGrid
#include "Properties\PropertyGridArrayItem.h"


using namespace CryGame;


#define IDC_DESC_CONTROL				7778
#define COLUMN_DESC_NAME                0
#define DESC_ICON_OFFSET                13
#define DESC_CATEGORY_DEFAULT           0
#define DESC_CATEGORY_DIRTY             1
#define DIRTY_DESC_NAME					"Other dirty descs"

enum
{
	PMENU_SAVE = 1,
	PMENU_DELETE,
	PMENU_RESET,
	PMENU_RENAME,
	PMENU_DUPLICATE,
	PMENU_UNDO,
	PMENU_EXPAND,
	PMENU_COLLAPSE,
	MENU_CLASSES_BEGIN = 100,
	MENU_CLASSES_END = 199,
	MENU_CATEGORIES_BEGIN = 200,
	MENU_CATEGORIES_END = 299
};

enum
{
	IDW_FIRST_PANE = AFX_IDW_CONTROLBAR_FIRST + 13,
	IDW_SECOND_PANE,
	IDW_THIRD_PANE,
	IDW_FOURTH_PANE,
	IDW_FIFTH_PANE
};

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CDescFilter, CDialog)
	ON_CONTROL(CBN_SELCHANGE, ID_DESC_EDITOR_DROPDOWN_DESCLIST, OnComboBoxSelectDesc)
	ON_COMMAND(ID_CE_FILTER_LABEL, OnCheckboxDesc)
	ON_COMMAND(ID_CE_FILTER_TEXT, OnCheckboxProperties)
	ON_NOTIFY(EN_CHANGE, ID_MOTION_HISTORY_LABEL, OnFilterText)
	ON_NOTIFY(EN_KILLFOCUS, ID_MOTION_HISTORY_LABEL, OnFilterText)
	ON_COMMAND(ID_CE_RELOAD, OnResetFilter)
	ON_WM_SIZE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
BOOL CryGame::CDescFilter::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Load previous session settings from registry
	char buffer[1024];
	CEngineSettingsManager esm;
	esm.GetModuleSpecificStringEntryUtf8("DEd_Filter", SettingsManagerHelpers::CCharBuffer(buffer, sizeof(buffer)));
	esm.GetModuleSpecificBoolEntry("DEd_FilterDescs", m_filterDesc);
	esm.GetModuleSpecificBoolEntry("DEd_FilterProps", m_filterProperties);
	m_filterText = buffer;

	// Create filter Toolbar.
	VERIFY(m_wndToolBar.CreateToolBar(WS_VISIBLE | WS_CHILD | CBRS_TOOLTIPS | CBRS_GRIPPER, this, AFX_IDW_TOOLBAR));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_MOTION_BROWSER_TOOLBAR));
	m_wndToolBar.SetFlags(xtpFlagAlignTop | xtpFlagStretched);

	CXTPControl* pCtrl;
	int nIndex;

	// Reuse the existing search toolbar.  This could bite us in the ass if it gets changed inside the SDK.
	// Filter reset
	pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_CE_RELOAD, TRUE, FALSE);
	if (pCtrl)
	{
		nIndex = pCtrl->GetIndex();
		CXTPControlButton* pButton = (CXTPControlButton*)m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlButton);
		pButton->SetTooltip("Reset filter");
	}

	// Filter checkbox for filtering the desc tree
	pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_CE_FILTER_LABEL, TRUE, FALSE);
	if (pCtrl)
	{
		nIndex = pCtrl->GetIndex();
		m_pCheckBoxDescs = (CXTPControlCheckBox*)m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlCheckBox);
		m_pCheckBoxDescs->SetTooltip("Filter descs");
		m_pCheckBoxDescs->SetCaption("D");
		m_pCheckBoxDescs->SetChecked(m_filterDesc);
	}

	// Filter checkbox for filtering the object tree
	pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_CE_FILTER_TEXT, TRUE, FALSE);
	if (pCtrl)
	{
		nIndex = pCtrl->GetIndex();
		m_pCheckBoxProperties = (CXTPControlCheckBox*)m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlCheckBox);
		m_pCheckBoxProperties->SetTooltip("Filter selected desc's properties");
		m_pCheckBoxProperties->SetCaption("P");
		m_pCheckBoxProperties->SetChecked(m_filterProperties);
	}

	// Filter label
	pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_MOTION_BROWSER_SELECT_CHARACTER, TRUE, FALSE);
	if (pCtrl)
	{
		nIndex = pCtrl->GetIndex();
		CXTPControlLabel* pLabelCtrl = (CXTPControlLabel*)m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlLabel);
		pLabelCtrl->SetCaption("Filter");
		pLabelCtrl->SetTooltip("Filter");
		pLabelCtrl->SetStyle(xtpButtonCaption);
	}

	// Filter control to filter items in the desc or object trees
	pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_MOTION_HISTORY_LABEL, TRUE, FALSE);
	if (pCtrl)
	{
		nIndex = pCtrl->GetIndex();
		m_pEditFilter = (CXTPControlEdit*)m_wndToolBar.GetControls()->SetControlType(nIndex, xtpControlEdit);
		m_pEditFilter->SetTooltip("Filter text");
	}

	// Unused so make invisible
	pCtrl = m_wndToolBar.GetControls()->FindControl(xtpControlButton, ID_MOTION_BROWSER_HISTORY, TRUE, FALSE);
	if (pCtrl)
	{
		pCtrl->SetVisible(FALSE);
	}

	LayoutControls();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::OnSize(UINT nType, int cx, int cy)
{
	if (m_wndToolBar)
	{
		// Resize the combo box and filter when the parent dialog size changes.
		LayoutControls();
	}

	CDialog::OnSize(nType, cx, cy);
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::OnClose()
{
	// Store session settings to registry
	CEngineSettingsManager esm;
	esm.SetModuleSpecificStringEntryUtf8("DEd_Filter", m_filterText);
	esm.SetModuleSpecificBoolEntry("DEd_FilterDescs", m_filterDesc);
	esm.SetModuleSpecificBoolEntry("DEd_FilterProps", m_filterProperties);

	CDialog::OnClose();
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::LayoutControls()
{
	CRect rcClient;
	CRect rctb;

	// Find the parent dialog dimensions
	GetClientRect(rcClient);

	// Stretch the combo box to fit the docking pane width
	CComboBox* pComboCtrlDescs = (CComboBox*)GetDlgItem(ID_DESC_EDITOR_DROPDOWN_DESCLIST);
	if (pComboCtrlDescs)
	{
		rctb = rcClient;
		rctb.left += 3;
		rctb.top += 3;
		rctb.right -= 1;
		rctb.bottom = rctb.top + 12;
		pComboCtrlDescs->MoveWindow(rctb, true);
	}

	// Shift the toolbar below the combo box and stretch the filter to fit the docking pane width
	DWORD dwMode = LM_HORZ | LM_HORZDOCK | LM_STRETCH | LM_COMMIT;
	CSize sz = m_wndToolBar.CalcDockingLayout(32000, dwMode);

	rctb = rcClient;
	rctb.top += 27;
	rctb.bottom = rctb.top + sz.cy;
	m_wndToolBar.MoveWindow(rctb);

	if (m_pEditFilter->GetType() == xtpControlEdit)
	{
		// Resize filter box
		m_pEditFilter->SetWidth(rctb.right - 144);
	}

	if (m_pEditFilter && m_pEditFilter->GetType() == xtpControlEdit)
	{
		m_pEditFilter->GetEditCtrl()->SetWindowText(m_filterText);
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::OnComboBoxSelectDesc()
{
	if (m_pDescEditor)
	{
		m_pDescEditor->OnComboBoxSelectDesc();
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::DoFilter()
{
	if (m_filterDesc)
	{
		FilterDescs();
	}

	if (m_filterProperties)
	{
		FilterProperties();
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::OnFilterText(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = FALSE; //Unhandled

	// Empty the cached text and get the latest.
	m_filterText = "";
	if (m_pEditFilter && m_pEditFilter->GetType() == xtpControlEdit)
	{
		m_pEditFilter->GetEditCtrl()->GetWindowText(m_filterText);
	}

	DoFilter();
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::OnResetFilter()
{
	m_filterText = "";

	if (m_pEditFilter && m_pEditFilter->GetType() == xtpControlEdit)
	{
		m_pEditFilter->GetEditCtrl()->SetWindowText(m_filterText);
	}

	FilterDescs();
	FilterProperties();
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::FilterDescs()
{
	if (!m_pDescEditor)
		return;

	// If filter is empty and the combo box has a selection, make sure it's the only thing visible
	if (m_filterText.IsEmpty() || !m_filterDesc)
	{
		OnComboBoxSelectDesc();
	}

	if (!m_filterText.IsEmpty())
	{
		if (m_filterDesc)
		{
			// Show all descs in all CObjectDesc inherited classes that match the filter and rebuild their properties for the objectree
			m_pDescEditor->GetDescTree()->GetRecords()->RemoveAll();
			int count = 0;
			CComboBox* pComboCtrlDescs = (CComboBox*)GetDlgItem(ID_DESC_EDITOR_DROPDOWN_DESCLIST);
			if (pComboCtrlDescs)
			{
				count = pComboCtrlDescs->GetCount();
			}

			for (int i = 0; i < count; ++i)
			{
				m_pDescEditor->GatherDescs(i, false);
			}
		}
	}

	if (m_filterDesc)
	{
		m_pDescEditor->GetDescTree()->SetRecordsTreeFilterMode(xtpReportFilterTreeByParentAndChildren);
		m_pDescEditor->GetDescTree()->SetFilterText(m_filterText);
		m_pDescEditor->GetDescTree()->Populate();
	}
	else
	{
		m_pDescEditor->GetDescTree()->SetRecordsTreeFilterMode(xtpReportFilterTreeByParentAndChildren);
		m_pDescEditor->GetDescTree()->SetFilterText("");
		m_pDescEditor->GetDescTree()->Populate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::FilterProperties()
{
	if (!m_pDescEditor)
		return;

	if (m_filterProperties)
	{
		m_pDescEditor->GetObjectTree()->SetRecordsTreeFilterMode(xtpReportFilterTreeByParentAndChildren);
		m_pDescEditor->GetObjectTree()->SetFilterText(m_filterText);
		m_pDescEditor->GetObjectTree()->Populate();
	}
	else
	{
		m_pDescEditor->GetObjectTree()->SetRecordsTreeFilterMode(xtpReportFilterTreeByParentAndChildren);
		m_pDescEditor->GetObjectTree()->SetFilterText("");
		m_pDescEditor->GetObjectTree()->Populate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::OnCheckboxDesc()
{
	if (m_pCheckBoxDescs && m_pCheckBoxDescs->GetType() == xtpControlCheckBox)
	{
		m_filterDesc = !m_filterDesc;
		m_pCheckBoxDescs->SetChecked(m_filterDesc);
		FilterDescs();
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescFilter::OnCheckboxProperties()
{
	if (m_pCheckBoxProperties && m_pCheckBoxProperties->GetType() == xtpControlCheckBox)
	{
		m_filterProperties = !m_filterProperties;
		m_pCheckBoxProperties->SetChecked(m_filterProperties);
		FilterProperties();
	}
}

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CDescEditor, CXTPFrameWnd)

BEGIN_MESSAGE_MAP(CDescEditor, CXTPFrameWnd)
	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)
	ON_COMMAND(ID_DESC_EDITOR_TOOLBAR_ITEM_2, OnSave)
	ON_COMMAND(ID_DESC_EDITOR_TOOLBAR_ITEM_1, SaveAll)
	ON_COMMAND(ID_DESC_EDITOR_TOOLBAR_ITEM_6, OnReset)
	ON_COMMAND(ID_DESC_EDITOR_TOOLBAR_ITEM_5, ResetAll)
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CDescEditor::CDescEditor()
	: m_pDockPaneFilter(NULL)
	, m_pDockPaneClipboard(NULL)
	, m_pDockPaneDescs(NULL)
	, m_pDockPanePropertyObjects(NULL)
	, m_pDockPanePropertyGrid(NULL)
	, m_pTreeDescSelection(NULL)
	, m_pTreePropertyObjects(NULL)
	, m_pTreeClipboard(NULL)
	, m_pActiveTree(NULL)
	, m_pPropertyGrid(NULL)
	, m_pFilterDescTreeBar(NULL)
	, m_pSelectedObject(NULL)
	, m_rootPath("Game_P1/Data/")
	, m_objectDescManager(NULL)
{
	CRect rc(0, 0, 0, 0);
	Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rc, AfxGetMainWnd());

	GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CDescEditor::~CDescEditor()
{
	m_pActiveTree = NULL;

	m_pTreePropertyObjects->CleanupAllUserdata();
	m_pTreeDescSelection->CleanupAllUserdata();
	m_pTreeClipboard->CleanupAllUserdata();

	while (!m_objectDescManager.empty())
	{
		delete m_objectDescManager.back();
		m_objectDescManager.pop_back();
	}

	if (m_pFilterDescTreeBar)
	{
		delete m_pFilterDescTreeBar;
	}

	GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
LRESULT CDescEditor::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass(new CDescEditorViewClass);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	if (GetSafeHwnd() == NULL)
		return;

	switch (event)
	{
	case eNotify_OnIdleUpdate:
	case eNotify_OnQuit:
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CDescEditor::OnInitDialog()
{
	__super::OnInitDialog();

	ModifyStyle(0, WS_CLIPCHILDREN);

	CRect rc;
	GetClientRect(rc);
	CSize filterSZ(300, 51);
	CSize clipboardSZ(300, 100);

	CXTPToolBar* pLibToolBar = GetCommandBars()->Add(_T("ToolBar"), xtpBarLeft);
	VERIFY(pLibToolBar->LoadToolBar(IDR_DESC_EDITOR_TOOLBAR));

	///////////////////////////////////////////////////////////////////////
	// Paint manager used for m_descTree and m_objectTree
	CXTPReportPaintManager* pPMgr = new CXTPReportPaintManager();
	pPMgr->m_nTreeIndent = 0x0a;
	pPMgr->m_bShadeSortColumn = false;
	pPMgr->SetGridStyle(FALSE, xtpGridNoLines);
	pPMgr->SetGridStyle(TRUE, xtpGridNoLines);
	pPMgr->m_bHideSelection = TRUE; // Hide the selection and handle it with CDescEditorTree::GetItemMetrics

	///////////////////////////////////////////////////////////////////////
	// Icon manager used for m_descTree and m_objectTree
	// Add icons to the tree items using CXTPReportRecordItem::SetIconIndex()
	CXTPImageManager* pImageManager = new CXTPImageManager();
	m_icons.Create(IDB_VALUE_TYPES, 16, 1, RGB(192, 192, 192));
	pImageManager->SetImageList(m_icons, 0);

	///////////////////////////////////////////////////////////////////////
	// Create the panes for the desc editor
	m_pDockPaneDescs = CreateDockingPane("Base Descs", this, IDW_FIRST_PANE, CRect(0, 0, filterSZ.cx, 1000), xtpPaneDockLeft);
	m_pDockPaneDescs->SetOptions(xtpPaneNoCloseable | xtpPaneNoHideable);

	m_pDockPanePropertyObjects = CreateDockingPane("Desc Objects", this, IDW_SECOND_PANE, CRect(0, 0, 800, 600), xtpPaneDockRight, m_pDockPaneDescs);
	m_pDockPanePropertyObjects->SetOptions(xtpPaneNoCloseable | xtpPaneNoHideable);

	m_pDockPanePropertyGrid = CreateDockingPane("Properties", this, IDW_THIRD_PANE, CRect(0, 0, 1000, 600), xtpPaneDockRight, m_pDockPanePropertyObjects);
	m_pDockPanePropertyGrid->SetOptions(xtpPaneNoCloseable | xtpPaneNoHideable);

	m_pDockPaneFilter = CreateDockingPane("Filter", this, IDW_FOURTH_PANE, CRect(0, 0, filterSZ.cx, filterSZ.cy), xtpPaneDockTop, m_pDockPaneDescs);
	m_pDockPaneFilter->SetOptions(xtpPaneNoCaption | xtpPaneNoCloseable | xtpPaneNoHideable);
	m_pDockPaneFilter->SetMinTrackSize(filterSZ);

	m_pDockPaneClipboard = CreateDockingPane("Clipboard", this, IDW_FIFTH_PANE, CRect(0, 0, clipboardSZ.cx, 600), xtpPaneDockBottom, m_pDockPaneDescs);
	m_pDockPaneClipboard->SetOptions(xtpPaneNoCloseable | xtpPaneNoHideable);
	m_pDockPaneClipboard->SetMinTrackSize(clipboardSZ);

	///////////////////////////////////////////////////////////////////////
	// Desc Tree
	m_pTreeDescSelection = new CDescEditorTree(this);
	m_pTreeDescSelection->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rc, this, IDC_DESC_CONTROL);
	m_pTreeDescSelection->ModifyStyleEx(0, WS_EX_STATICEDGE);

	CXTPReportColumn* pCol = new CXTPReportColumn(COLUMN_DESC_NAME, _T("Objects"), 200, TRUE, XTP_REPORT_NOICON, TRUE, TRUE);
	CXTPReportColumn* pTreeCol = m_pTreeDescSelection->AddColumn(pCol);
	pTreeCol->SetTreeColumn(true);
	pTreeCol->SetSortable(TRUE);
	pTreeCol->SetAutoSortWhenGrouped(TRUE);
	m_pTreeDescSelection->SetSortRecordChilds(TRUE);
	m_pTreeDescSelection->GetColumns()->SetSortColumn(pTreeCol, true);

	m_pTreeDescSelection->GetReportHeader()->AllowColumnRemove(FALSE);
	m_pTreeDescSelection->ShadeGroupHeadings(FALSE);
	m_pTreeDescSelection->SkipGroupsFocus(TRUE);
	m_pTreeDescSelection->SetMultipleSelection(FALSE);
	m_pTreeDescSelection->SetPaintManager(pPMgr);
	m_pTreeDescSelection->SetImageManager(pImageManager);
	m_pTreeDescSelection->ShowHeader(FALSE);

	m_pDockPaneDescs->Attach(m_pTreeDescSelection);

	///////////////////////////////////////////////////////////////////////
	// Desc Object Tree
	m_pTreePropertyObjects = new CDescEditorTree(this);
	m_pTreePropertyObjects->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rc, this, IDC_DESC_CONTROL);
	m_pTreePropertyObjects->ModifyStyleEx(0, WS_EX_STATICEDGE);

	pCol = new CXTPReportColumn(COLUMN_DESC_NAME, _T("Object Descs"), 200, TRUE, XTP_REPORT_NOICON, FALSE, TRUE);
	pTreeCol = m_pTreePropertyObjects->AddColumn(pCol);
	pTreeCol->SetTreeColumn(true);
	pTreeCol->SetSortable(FALSE);
	m_pTreePropertyObjects->GetReportHeader()->AllowColumnRemove(FALSE);
	m_pTreePropertyObjects->ShadeGroupHeadings(FALSE);
	m_pTreePropertyObjects->SkipGroupsFocus(TRUE);
	m_pTreePropertyObjects->SetMultipleSelection(FALSE);
	m_pTreePropertyObjects->SetPaintManager(pPMgr);
	m_pTreePropertyObjects->SetImageManager(pImageManager);
	m_pTreePropertyObjects->ShowHeader(FALSE);
	m_pTreePropertyObjects->EnableDragDrop("m_objectTree", xtpReportAllowDragMove | xtpReportAllowDrop);

	m_pDockPanePropertyObjects->Attach(m_pTreePropertyObjects);

	///////////////////////////////////////////////////////////////////////
	// Clipboard Tree
	m_pTreeClipboard = new CDescEditorTree(this);
	m_pTreeClipboard->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rc, this, IDC_DESC_CONTROL);
	m_pTreeClipboard->ModifyStyleEx(0, WS_EX_STATICEDGE);

	pCol = new CXTPReportColumn(COLUMN_DESC_NAME, _T("Clipboard"), 200, TRUE, XTP_REPORT_NOICON, FALSE, TRUE);
	pTreeCol = m_pTreeClipboard->AddColumn(pCol);
	pTreeCol->SetTreeColumn(true);
	pTreeCol->SetSortable(FALSE);
	m_pTreeClipboard->GetReportHeader()->AllowColumnRemove(FALSE);
	m_pTreeClipboard->ShadeGroupHeadings(FALSE);
	m_pTreeClipboard->SkipGroupsFocus(TRUE);
	m_pTreeClipboard->SetMultipleSelection(FALSE);
	m_pTreeClipboard->SetPaintManager(pPMgr);
	m_pTreeClipboard->SetImageManager(pImageManager);
	m_pTreeClipboard->ShowHeader(FALSE);
	m_pTreeClipboard->EnableDragDrop("m_clipboardTree", xtpReportAllowDragCopy | xtpReportAllowDrop);

	m_pDockPaneClipboard->Attach(m_pTreeClipboard);

	///////////////////////////////////////////////////////////////////////
	// Property Grid

	m_pPropertyGrid = new CPropertyGrid();
	m_pPropertyGrid->Create(CRect(4, 4, 350, 1000), this, NULL);
	m_pPropertyGrid->ClearFlag(ePFG_ShowArrays);
	m_pPropertyGrid->AddPropertyListener(this);  // Listen for property changes

	m_pDockPanePropertyGrid->Attach(m_pPropertyGrid);

	///////////////////////////////////////////////////////////////////////
	// Desc List toolbar for selecting m_descTree items

	m_pFilterDescTreeBar = new CDescFilter(this);
	m_pFilterDescTreeBar->Create(IDD_DESC_EDITOR_FILTER, this);
	m_pDockPaneFilter->Attach(m_pFilterDescTreeBar);

	///////////////////////////////////////////////////////////////////////
	RecalcLayout();
	LayOutControls();

	// Make sure we can find the game dll
	if (!GetISystem()->GetIGame() || !GetISystem()->GetIGame()->GetGameInterface())
	{
		gEnv->pLog->LogError("Missing or unsupported GameDLL.  Aborting DescEditor desc loading.");
		return TRUE;
	}

	BuildDescList();

	m_pFilterDescTreeBar->DoFilter();

	// Set the View's pointer to the desc editor.  This functionality needs to change if multiple 
	// Desc Editors are introduced.
	if (IClassDesc* pClassDesc = GetIEditor()->GetClassFactory()->FindClass(DESC_EDITOR_TOOL_NAME))
	{
		IViewPaneClass *pViewPaneClass = NULL;
		pClassDesc->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewPaneClass);
		if (pViewPaneClass)
		{
			if (CDescEditorViewClass* pView = (CDescEditorViewClass*)pViewPaneClass)
			{
				pView->SetDescEditor(this);
			}
		}
	}

	return TRUE;
}

void CDescEditor::LayOutControls()
{
	if (!m_pTreeClipboard)
		return;

	CRect rcClient;
	m_pTreeClipboard->GetClientRect(rcClient);
	rcClient.top = rcClient.top + 50;
	m_pTreeClipboard->MoveWindow(rcClient, true);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	LayOutControls();
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnDestroy()
{
	// Make sure we can find the game dll
	if (GetISystem()->GetIGame())
	{
		m_pActiveTree = NULL;

		// Check dirty and request save
		SaveAll();

		// Reload the descs from disc to clean out any unsaved changes we made.
		BuildDescList();

		if (m_pFilterDescTreeBar)
		{
			m_pFilterDescTreeBar->OnClose();
		}
	}

	// Todo: Only continue if success?
	// Need to trap onclose instead?
	__super::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnPropertyEvent(uint32 event, CPropertyGrid* pGrid, const CPropertyGridData* pData)
{
	if (!m_pActiveTree)
	{
		return;
	}

	switch (event)
	{
	case ePGE_OnChanged:
	{
		// Every time a property changes
		// Update the caption for the parent array element in case it changed
		if (pData != NULL && pData->m_pProperty && pData->m_pObject)
		{
			// Exclude primitive types
			if (!pData->m_isElement)
			{
				// Don't update the object tree's root record - leave it as the name of the .xml file
				if (m_pActiveTree == m_pTreePropertyObjects && m_pActiveTree->GetSelectedDescRecord() == m_pActiveTree->GetRootRecord())
					break;

				CXTPPropertyGridItem* pGridRootItem = pGrid->GetCategories()->GetAt(0);
				CXTPPropertyGridItem* pParentItem = pGrid->GetSelectedItem()->GetParentItem();

				if (pGridRootItem != pParentItem && (CPropertyGridGroupItem*)pParentItem)
				{
					// Grab the parent of a group header
					pParentItem = pParentItem->GetParentItem();
				}

				// If this is one of the base properties, check to see if the caption was modified.
				if (pParentItem == pGridRootItem)
				{
					// Extract the caption and update if applicable
					string caption;
					CPropertyGridItem::GetObjectCaption(pData->m_pObject, caption);

					CString ccaption(caption);

					if (m_pActiveTree->GetSelectedDescRecordItem() && m_pActiveTree->GetSelectedDescRecord())
					{
						if (m_pActiveTree->GetSelectedDescRecordItem()->GetCaption(COLUMN_DESC_NAME) != ccaption)
						{
							m_pActiveTree->GetSelectedDescRecordItem()->SetCaption(ccaption);
							m_pActiveTree->UpdateRecord(m_pActiveTree->GetSelectedDescRecord(), TRUE);
						}
					}
				}
			}
		}
	}
	break;

	case ePGE_OnDirty:
	case ePGE_OnClean:
	{
		// First time a property is marked dirty/clean
		CXTPReportRecordItem* pItem = m_pActiveTree->GetSelectedDescRecordItem();
		if (pItem)
		{
			CDescPropertyInfo* pInfo = reinterpret_cast<CDescPropertyInfo*>(pItem->GetItemData());
			if (pInfo)
			{
				// Inc/Dec dirty count
				(event == ePGE_OnDirty) ? pInfo->IncrementDirty() : pInfo->DecrementDirty();
				if (m_pActiveTree == m_pTreeClipboard)
				{
					UpdateDirtyStatus(pItem->GetRecord());
				}
				else
				{
					UpdateDirtyStatus();
					UpdateDirtyStatus(m_pTreeDescSelection);
				}
			}
		}
	}
	break;

	case ePGE_OnElementAdded:
	case ePGE_OnElementRemoved:
	{
		// Update the count on the parent tree element if a native element is removed
		if (pData && pData->m_pProperty && pData->m_pObject && m_pActiveTree->GetSelectedDescRecordItem())
		{
			CDescPropertyInfo* pInfo = (CDescPropertyInfo*)m_pActiveTree->GetSelectedDescRecordItem()->GetItemData();
			if (pInfo)
			{
				if (pInfo->GetObject() == pData->m_pObject)
				{
					if (pInfo->GetRecord() && pInfo->GetRecord()->GetItem(COLUMN_DESC_NAME))
					{
						SetArrayCaption(m_pActiveTree->GetSelectedDescRecordItem(), pData->m_pProperty, pData->m_pObject);
					}
				}
			}
		}
	}
	break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::SaveAll()
{
	for (int i = 0; i < m_objectDescManager.size(); ++i)
	{
		for (int j = 0; j < m_objectDescManager[i]->GetObjDescs().size(); ++j)
		{
			CDescEdObjectDesc* pDesc = m_objectDescManager[i]->GetObjDescs()[j];
			if (CheckDirtyDesc(pDesc))
			{
				if (RequestSave(pDesc->GetObjectDesc()))
				{
					if (SaveDesc(pDesc->GetObjectDesc()))
					{
						UpdateDirtyStatus(pDesc->GetObjectTreeRootRecord(), true);

						// Todo: Tell property panel that it's now clean
					}
					else
					{
						MessageBox("Save failed.", "Save Failed", MB_OK | MB_ICONSTOP);
					}
				}
			}
		}
	}

	// Call to cleanup m_descTree "Other Dirty Descs" category
	UpdateDirtyStatus(NULL, false);

	// Call to reflect dirty objects in the m_descTree
	UpdateDirtyStatus(m_pTreeDescSelection);
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::RequestSave(CObjectDesc* pDesc)
{
	CString message;
	message.Format("%s.xml [%s] is unsaved.  Save changes?", pDesc->GetFilename().c_str(), pDesc->GetClass()->GetName().c_str());
	if (MessageBox(message, "Notice", MB_YESNO) == IDYES)
	{
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnReset()
{
	ResetDesc(GetItemData(m_pTreeDescSelection->GetSelectedDescRecord()));
	m_pPropertyGrid->Reset();
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::ResetAll()
{
	for (int i = 0; i < m_objectDescManager.size(); ++i)
	{
		for (int j = 0; j < m_objectDescManager[i]->GetObjDescs().size(); ++j)
		{
			CDescEdObjectDesc* pDesc = m_objectDescManager[i]->GetObjDescs()[j];
			if (CheckDirtyDesc(pDesc))
			{
				ResetDesc(GetItemData(pDesc->GetObjectTreeRootRecord()));
			}
		}
	}

	UpdateDirtyStatus(NULL, false);

	// Call to reflect dirty objects in the m_descTree
	UpdateDirtyStatus(m_pTreeDescSelection);

	m_pPropertyGrid->Reset();
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::ResetDesc(CDescPropertyInfo* pInfo, bool forceReset)
{
	if (!pInfo || !pInfo->GetParentDesc())
	{
		return;
	}

	CObjectDesc* pDesc = pInfo->GetParentDesc()->GetObjectDesc();
	if (pDesc)
	{
		bool reset = forceReset;

		if (!reset)
		{
			CString message;
			message.Format("Reset unsaved changes on %s.xml [%s]", pDesc->GetFilename(), pDesc->GetClass()->GetName());
			reset = (MessageBox(message, "Notice", MB_YESNO) == IDYES);
		}

		if (reset)
		{
			// Clean up any entries flagged to be deleted.
			ResolveDeletedElements(pDesc);

			pInfo->ResetDirtyCount();

			// Force everything clean with true flag
			UpdateDirtyStatus(NULL, true);

			pInfo->SetRecord(NULL);

			// Cleanup existing panels
			ResetObjectPropertyTree();

			IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
			
			// Don't clone transient properties when using a modpack because a file could have been deleted that had fewer properties in an array than the original.
			// If this becomes a problem, then find out if the deleted file was from a modpack and only skip the clone then.
			_smart_ptr<CObjectDesc> pNewDesc = pObjectDescFactory->Get(pDesc->GetClass(), pDesc->GetFilename(), true, !pObjectDescFactory->IsUsingModpack());
			pInfo->GetParentDesc()->SetObjectDesc(pNewDesc);
			pInfo->GetParentDesc()->GetObjectTreeRootRecord()->RemoveAll();
			pInfo->GetParentDesc()->SetObjectTreeRootRecord(NULL);
			pInfo->SetObject(pNewDesc);

			// Call to reflect dirty objects in the m_descTree
			UpdateDirtyStatus(m_pTreeDescSelection);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnSave()
{
	if (CXTPReportRecordItem* pItem = m_pTreeDescSelection->GetSelectedDescRecordItem())
	{
		CDescPropertyInfo* pInfo = GetItemData(pItem->GetRecord());
		if (pInfo && pInfo->GetParentDesc())
		{
			CObjectDesc* pDesc = pInfo->GetParentDesc()->GetObjectDesc();
			if (pDesc && SaveDesc(pDesc))
			{
				int iconIndex = pDesc->GetIsModpackObj() ? DESC_ICON_OFFSET + 1 : DESC_ICON_OFFSET;
				pItem->SetIconIndex(iconIndex); // File icon

				pInfo->ResetDirtyCount(true);
				UpdateDirtyStatus(NULL, true);
				UpdateDirtyStatus(m_pTreeDescSelection);
			}
			else
			{
				MessageBox("Save failed.", "Save Failed", MB_OK | MB_ICONSTOP);
			}
		}	
	}
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::CheckDirtyDesc(CDescEdObjectDesc* pDesc)
{
	CDescPropertyInfo* pInfo = GetItemData(pDesc->GetObjectTreeRootRecord());
	if (pInfo)
	{
		if (pInfo->GetDirtyCount())
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::BuildDescList()
{
	// Only happens when CDescEditor is created or ResetAll() is called.

	IClassRegistry* pClassRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
	IClass* pBaseClass = pClassRegistry->GetClass("CObjectDesc");

	CComboBox* pComboCtrl = (CComboBox*)m_pFilterDescTreeBar->GetDlgItem(ID_DESC_EDITOR_DROPDOWN_DESCLIST);
	if (pComboCtrl)
	{
		pComboCtrl->ResetContent();
	}

	// Cleanup existing panels
	m_objectDescManager.clear();
	ResetDescTree();

	IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();

	DynArray<IClass*> subClasses;
	subClasses = pBaseClass->GetSubClasses();
	for (int i = 0; i < subClasses.size(); ++i)
	{
		if (subClasses[i]->GetBaseClass() == pBaseClass)
		{
			CDescEdObjectDescMgr* pDescEdObjectDesc = new CDescEdObjectDescMgr();
			pDescEdObjectDesc->SetDescClass(subClasses[i]);

			if (pComboCtrl)
			{
				string className = subClasses[i]->GetName();

				// Trim the 'C' off the front of the class name and break up camel case
				// words, for readability
				if (className[0] == 'C')
					className.erase(0, 1);
				ExpandCamelCase(className);

				int index = pComboCtrl->AddString(className);
				pComboCtrl->SetItemData(index, (DWORD_PTR)subClasses[i]);
			}

			ObjectDescVector& pDescList = pObjectDescFactory->GetDescList(subClasses[i]->GetName());
			for (int j = 0; j < pDescList.size(); ++j)
			{
				static const string invalidDescNameid("Fail");
				if (pDescList[j]->GetNameID() != invalidDescNameid)
				{
					AddDescFile(subClasses[i], Path::GetFileName(pDescList[j]->GetFilename().c_str()), *pDescEdObjectDesc);
				}
			}

			m_objectDescManager.push_back(pDescEdObjectDesc);
		}
	}

	if (pComboCtrl)
	{
		pComboCtrl->SendMessage(CB_SETCUEBANNER, 0, (LPARAM)L"Choose a Desc");
	}
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::AddDescFile(IClass* pClass, const char* pFilename, CDescEdObjectDescMgr& rDescMgr)
{
	IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();

	// Force it to reload from disc.
	_smart_ptr<CObjectDesc> pDesc = pObjectDescFactory->Get(pClass->GetName().c_str(), pFilename, true);
	if (pDesc)
	{
		CDescEdObjectDesc* pObject = new CDescEdObjectDesc();
		pObject->SetObjectDesc(pDesc);
		rDescMgr.GetObjDescs().push_back(pObject);
		return true;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::RemoveDescFile(IClass* pClass, CObjectDesc* pDesc)
{
	IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
	CDescEdObjectDescMgr* pDescMgr = NULL;

	// Clean up any entries flagged to be deleted.
	ResolveDeletedElements(pDesc);

	// Find the desc manager for the file so it can be removed
	if (pClass->GetBaseClass()->GetName() != string("CObjectDesc"))
	{
		pDescMgr = FindDescEdObjectDescMgr(pClass->GetBaseClass());
	}
	else
	{
		pDescMgr = FindDescEdObjectDescMgr(pClass);
	}

	// Remove the desc from the desc manager
	if (pDescMgr)
	{
		pDescMgr->RemoveObjDesc(pDesc);
	}

	// Force it to reload the desc from disk.  This will set the desc as "Fail" so it's no longer used.
	_smart_ptr<CObjectDesc> pLocalDesc = pObjectDescFactory->Get(pClass->GetName().c_str(), pDesc->GetFilename().c_str(), true);
	if (pLocalDesc)
	{
		return true;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::CreateDescFile(IClass* pClass)
{
	if (!pClass)
	{
		return FALSE;
	}

	// Pop up a simple dialog to request the desired filename
	CSimpleEditPopup dialog;
	string filename;
	string nameID;

	dialog.SetDialogTitle("Enter Filename");
	dialog.SetEditString(filename);
	dialog.SetDialogLabel("Enter filename excluding extension");

	if (dialog.DoModal() == IDOK && !filename.empty())
	{
		// Find the base class for the path
		IClass* pBaseClass = pClass;
		while (pBaseClass->GetBaseClass()->GetName() != "CObjectDesc")
		{
			pBaseClass = pBaseClass->GetBaseClass();
		}

		// Verify it doesn't exist.
		// Todo: Replace with !CFileUtil::FileExists()
		CFileUtil::FileArray fArray;
		string path = m_rootPath + pBaseClass->GetName() + string("/");
		CFileUtil::ScanDirectory(path.c_str(), "*.xml", fArray, true);
		for (int i = 0; i < fArray.size(); ++i)
		{
			if (Path::GetFileName(fArray[i].filename) == CString(filename))
			{
				CString message;
				message.Format("%s already exists", filename);
				MessageBox(message, "Fail", MB_OK | MB_ICONSTOP);
				return false;
			}
		}

		// Create the new desc
		_smart_ptr<CReflectedObject> pObject = pClass->CreateObject();
		CObjectDesc* pDesc = (CObjectDesc*)pObject.get();
		pDesc->SetFilename(filename);
		nameID = filename;
		nameID.MakeLower();
		pDesc->SetNameID(nameID);
		CDescEdObjectDescMgr* pDescMgr = NULL;

		// Track down the local Desc Manager to to add the new desc to.
		pDescMgr = FindDescEdObjectDescMgr(pBaseClass);

		// Save the desc and add it to the desc panel list.
		if (pDescMgr)
		{
			if (SaveDesc(pDesc))
			{
				return AddDescFile(pClass, filename, *pDescMgr);
			}
		}
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnComboBoxSelectDesc()
{
	m_pTreeDescSelection->SetSelectedDescRecord(NULL);

	CComboBox* pComboCtrl = (CComboBox*)m_pFilterDescTreeBar->GetDlgItem(ID_DESC_EDITOR_DROPDOWN_DESCLIST);
	if (!pComboCtrl)
		return;

	BuildTreeDescSelection(pComboCtrl->GetCurSel());
}

//////////////////////////////////////////////////////////////////////////
// Called when the user selects a desc from the combo box.  The index is the combo box selection index.
void CDescEditor::BuildTreeDescSelection(int index)
{
	BuildTreePropertyObjects(NULL);

	m_pTreeDescSelection->CleanupAllUserdata();
	m_pTreeDescSelection->GetRecords()->RemoveAll();

	if (index == -1)
		return;

	m_pTreeDescSelection->BeginUpdate();
	GatherDescs(index, true);
	GatherDirtyDescs(index);
	m_pTreeDescSelection->EndUpdate();
	m_pTreeDescSelection->Populate();
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::BuildTreePropertyObjects(CDescEdObjectDesc* pObjectDesc)
{
	// Cleanup existing panel
	ResetPropertyGrid();

	if (m_pTreePropertyObjects->GetRootRecord())
	{
		m_pTreePropertyObjects->BeginUpdate();
		m_pTreePropertyObjects->GetRootRecord()->SetVisible(FALSE);
		m_pTreePropertyObjects->EndUpdate();
		m_pTreePropertyObjects->Populate();
	}

	if (pObjectDesc)
	{
		if (pObjectDesc->GetObjectTreeRootRecord())
		{
			m_pTreePropertyObjects->BeginUpdate();
			pObjectDesc->GetObjectTreeRootRecord()->SetVisible(TRUE);
			m_pTreePropertyObjects->EndUpdate();
			m_pTreePropertyObjects->Populate();

			CXTPReportRow* pRow = m_pTreePropertyObjects->GetRows()->Find(pObjectDesc->GetObjectTreeSelectedRecord());
			if (pRow)
			{
				m_pTreePropertyObjects->GetSelectedRows()->Select(pRow);
				m_pTreePropertyObjects->SetFocusedRow(pRow);
			}
			else
			{
				pRow = m_pTreePropertyObjects->GetRows()->GetAt(0);
				m_pTreePropertyObjects->GetSelectedRows()->Select(pRow);
				m_pTreePropertyObjects->SetFocusedRow(pRow);
			}
		}
		else
		{
			m_pTreePropertyObjects->BeginUpdate();
			CXTPReportRecord* pBaseRecord;
			pBaseRecord = m_pTreePropertyObjects->AddRecord(new CXTPReportRecord());
			pObjectDesc->SetObjectTreeRootRecord(pBaseRecord);

			CXTPReportRecordItem* pItem = new CXTPReportRecordItem();
			CString caption;
			caption.Format("%s.xml", pObjectDesc->GetObjectDesc()->GetFilename().c_str());
			pItem->SetCaption(caption);
			CDescPropertyInfo* pInfo = new CDescPropertyInfo(pObjectDesc->GetObjectDesc(), NULL, NULL, NULL, pObjectDesc);

			pInfo->SetParentRecord(NULL);
			pInfo->SetRecord(pBaseRecord);

			pItem->SetItemData(DWORD_PTR(pInfo));
			pBaseRecord->AddItem(pItem);
			pBaseRecord->SetExpanded(TRUE);

			PopulateObjectProperties(pBaseRecord, pInfo);
			m_pTreePropertyObjects->EndUpdate();
			m_pTreePropertyObjects->Populate();

			// Set the root record as the selected record so the properties can be displayed.
			CXTPReportRow* pRow = m_pTreePropertyObjects->GetRows()->GetAt(0);
			m_pTreePropertyObjects->GetSelectedRows()->Select(pRow);
			m_pTreePropertyObjects->SetFocusedRow(pRow);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CDescPropertyInfo* CDescEditor::AddObjectPropArrayElement(CDescPropertyInfo* pInfo, IClass* pClass, CDescEditorTree* pTree, int index)
{
	if (pInfo == NULL)
		return NULL;

	_smart_ptr<CReflectedObject> pObj = pInfo->GetObject();

	if (pClass != NULL)
	{
		// Create a new object of the specified type
		_smart_ptr<CReflectedObject> pNewObject = pClass->CreateObject();
		if (pNewObject == NULL)
			return NULL;  // Failed to create object

		// Add the newly created object to the actual array
		if (index > -1)
		{
			pInfo->GetProperty()->AddElementAt(pInfo->GetObject(), index, Value(pNewObject));
		}
		else
		{
			pInfo->GetProperty()->AddElement(pInfo->GetObject(), Value(pNewObject));
		}

		// Create user data
		CDescPropertyInfo* pNewInfo = new CDescPropertyInfo(*pInfo);
		pNewInfo->SetDirtyCount(0);
		pNewInfo->SetParentInfo(pInfo);
		pNewInfo->SetElement(pNewObject);
		pNewInfo->SetRecord(NULL);
		pNewInfo->IncrementDirty();

		// Find array root branch (TODO: Store this on the user data?)
		CXTPReportRecord* pRecord = pInfo->GetParentRecord();
		string caption = pNewInfo->GetProperty()->GetName();
		CXTPReportRecordItem* pChildRecordItem = NULL;

		for (int i = 0; i < pRecord->GetChilds()->GetCount(); ++i)
		{
			CXTPReportRecord* pChildRecord = pRecord->GetChilds()->GetAt(i);
			CDescPropertyInfo* pData = GetItemData(pChildRecord);
			if (pData)
			{
				IProperty* pProperty = pData->GetProperty();
				if (pProperty)
				{
					if (pProperty->GetName() == caption)
					{
						pChildRecordItem = pChildRecord->GetItem(COLUMN_DESC_NAME);
						break;
					}
				}
			}
		}

		if (pChildRecordItem)
		{
			// Create a leaf and add to branch.
			CXTPReportRecord* pNewRecord = new CXTPReportRecord();
			if (index > -1)
				pChildRecordItem->GetRecord()->GetChilds()->InsertAt(index, pNewRecord);
			else
				pChildRecordItem->GetRecord()->GetChilds()->Add(pNewRecord);

			pNewInfo->SetParentRecord(pChildRecordItem->GetRecord());
			pNewInfo->SetRecord(pNewRecord);

			CXTPReportRecordItem* pItem = AddObjectPropArrayElementChild(pNewRecord, pNewInfo);
			PopulateObjectProperties(pNewRecord, pNewInfo);
			pTree->SetSelectedDescRecordItem(pItem);
			pNewRecord->SetExpanded(TRUE);
			pNewRecord->GetParentRecord()->SetExpanded(TRUE);

			if (pTree != m_pTreeClipboard)
			{
				pTree->UpdateRecord(pChildRecordItem->GetRecord(), TRUE);
				UpdateDirtyStatus();
				UpdateDirtyStatus(m_pTreeDescSelection);
			}
			return pNewInfo;
		}
	}
	else
	{
		CPropertyGridArrayItem* pRoot = static_cast<CPropertyGridArrayItem*>(m_pPropertyGrid->GetCategories()->GetAt(0));
		pRoot->AddElement();  // Add a new element

		pInfo->IncrementDirty();
		UpdateDirtyStatus();
		UpdateDirtyStatus(m_pTreeDescSelection);
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::DeleteObjectPropArrayElement(CDescPropertyInfo* pInfo)
{
	if (!pInfo)
		return;

	pInfo->GetProperty()->RemoveElementAt(pInfo->GetObject(), pInfo->GetElementIndex());
	if (pInfo->GetParentInfo()->GetDirtyCount() == 0)
	{
		pInfo->GetParentInfo()->IncrementDirty();
	}
	else
	{
		pInfo->GetParentInfo()->DecrementDirty(pInfo->GetDirtyCount());
	}

	m_pActiveTree->SetSelectedDescRecord(pInfo->GetParentRecord());
	BuildPropertyGrid(NULL);

	CXTPReportRecord* pParentRecord = pInfo->GetParentRecord();
	m_pActiveTree->BeginUpdate();
	m_pActiveTree->CleanupChildrenUserdata(pInfo->GetRecord());
	pInfo->GetRecord()->RemoveAll();
	pParentRecord->GetChilds()->RemoveRecord(pInfo->GetRecord());
	m_pActiveTree->EndUpdate();
	m_pActiveTree->Populate();

	UpdateDirtyStatus();
	UpdateDirtyStatus(m_pTreeDescSelection);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::CloneDescElement(CDescPropertyInfo* pSourceObject, CDescPropertyInfo* pTargetObject, CDescEditorTree* pTree)
{
	if (!pSourceObject || !pTargetObject)
	{
		return;
	}

	bool add = false;
	CDescPropertyInfo* pInfo = pTargetObject;
	CDescPropertyInfo* pTargetRecordInfo;

	// If the target isn't the clipboard, we plan to add the source object to a valid target property array
	if (pTree != m_pTreeClipboard)
	{
		// Determine if this is a primitive array and handle it here if it is
		if (!pSourceObject->GetElement() && !pSourceObject->GetClass())
		{
			if (pSourceObject->GetProperty() && pSourceObject->GetObject() && pTargetObject->GetProperty() && pTargetObject->GetObject())
			{
				// Verify the target is compatible by verifying that the types are the same
				int sourceType = pSourceObject->GetProperty()->GetType(pSourceObject->GetObject());
				if (sourceType != eVType_None)
				{
					int targetType = pTargetObject->GetProperty()->GetType(pTargetObject->GetObject());
					if (sourceType == targetType)
					{
						// Target is valid, add the elements to the target array
						int count = pSourceObject->GetProperty()->GetElementCount(pSourceObject->GetObject());
						if (count)
						{
							for (int i = 0; i < count; ++i)
							{
								pTargetObject->GetProperty()->AddElement(pTargetObject->GetObject(), pSourceObject->GetProperty()->GetElementAt(pSourceObject->GetObject(), i));
							}

							SetArrayCaption(pTargetObject->GetRecord()->GetItem(COLUMN_DESC_NAME), pTargetObject->GetProperty(), pTargetObject->GetObject());

							pTargetObject->IncrementDirty();
							UpdateDirtyStatus(m_pTreePropertyObjects->GetRootRecord());
							UpdateDirtyStatus(m_pTreeDescSelection);

							// Reset the state of the property grid so it can be rebuilt with the new entries
							pTargetObject->GetPropertyGridState() = SPropertyGridState();
						}
					}
				}
			}

			return;
		}

		add = true;

		// Determine if we can be dropped at this location.
		pTargetRecordInfo = GetValidCloneTarget(pSourceObject, pTargetObject);
		if (!pTargetRecordInfo)
		{
			// Bail if the drop location can't contain what was dropped.
			return;
		}

		// Create a new dummy desc element to store the cloned data.
		pInfo = AddObjectPropArrayElement(pTargetRecordInfo, pSourceObject->GetElement()->GetClass(), pTree, pTargetObject->GetElementIndex());
	}

	if (pInfo != NULL)
	{
		CXTPReportRecordItem* pItem = pInfo->GetRecord()->GetItem(COLUMN_DESC_NAME);
		if (pItem)
		{
			pInfo->ResetDirtyCount(true);
			m_pPropertyGrid->RestoreState(pInfo->GetPropertyGridState());

			if (add)
			{
				// Clone the object
				IClassRegistry* pClassRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
				_smart_ptr<CReflectedObject> pClone = pClassRegistry->CloneObject(pSourceObject->GetElement());

				// Move the cloned object into the dummy desc element.
				pInfo->GetProperty()->SetValueAt(pInfo->GetObject(), pInfo->GetElementIndex(), Value(pClone));
				pInfo->SetElement(pClone);

				pItem->SetItemData(DWORD_PTR(pInfo));
			}

			// Pull caption from element
			if (pInfo->GetElement())
			{
				string caption;
				CPropertyGridItem::GetObjectCaption(pInfo->GetElement(), caption);
				CString ccaption(caption);
				pItem->SetCaption(ccaption);
			}

			// Create the records for all child properties
			pTree->BeginUpdate();
			pInfo->GetRecord()->GetChilds()->RemoveAll();
			PopulateObjectProperties(pInfo->GetRecord(), pInfo);
			pTree->EndUpdate();
			pTree->Populate();

			// Update parent array caption.
			if (add)
			{
				SetArrayCaption(pTargetRecordInfo->GetRecord()->GetItem(COLUMN_DESC_NAME), pTargetRecordInfo->GetProperty(), pTargetRecordInfo->GetObject());
				pInfo->IncrementDirty();
				UpdateDirtyStatus(m_pTreePropertyObjects->GetRootRecord());
				UpdateDirtyStatus(m_pTreeDescSelection);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Find a property array / record to host the source object
CDescPropertyInfo* CDescEditor::GetValidCloneTarget(CDescPropertyInfo* pSourceObject, CDescPropertyInfo* pTargetObject)
{
	if (pTargetObject->GetClass() && pSourceObject->GetElement())
	{
		// Figure out who will own this new element.  Handle dropping the element on the array element itself or its children.
		IClass* pSourceClass = pSourceObject->GetElement()->GetClass();
		CDescPropertyInfo* pTargetInfo = pTargetObject;

		// If dropped on something with an element ( non container ), check parent.
		if (pTargetInfo->GetElement())
		{
			pTargetInfo = pTargetInfo->GetParentInfo();
		}

		// Bail if the drop location can't contain what was dropped.
		if (!pTargetInfo->GetElement() && pSourceClass->InstanceOf(pTargetInfo->GetClass()))
		{
			return pTargetInfo;
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnDescElementSelectionChanged(CDescEditorTree* pTree)
{
	CXTPReportRecordItem* pItem = pTree->GetSelectedDescRecordItem();
	if (pItem)
	{
		CDescPropertyInfo* pInfo = (CDescPropertyInfo*)pItem->GetItemData();
		if (pInfo && pInfo->GetObject())
		{
			// Cache the selected desc
			m_pSelectedDesc = pInfo->GetParentDesc();
			m_pActiveTree = pTree;

			if (pTree == m_pTreeDescSelection)
			{
				// Desc tree
				m_pActiveTree = m_pTreePropertyObjects;
				BuildTreePropertyObjects(pInfo->GetParentDesc());
			}
			else
			{
				if (pTree == m_pTreePropertyObjects)
				{
					// Store the current selection on object tree elements so they can be reselected when this desc is chosen.
					CDescPropertyInfo* pObjectInfo = GetItemData(m_pTreePropertyObjects->GetRootRecord());
					if (pObjectInfo)
					{
						if (pObjectInfo->GetParentDesc())
						{
							pObjectInfo->GetParentDesc()->SetObjectTreeSelectedRecord(m_pTreePropertyObjects->GetSelectedDescRecord());
						}
					}
				}

				BuildPropertyGrid(pInfo);
			}
		}
		else
		{
			// Clear the property panel
			BuildPropertyGrid(NULL);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::GatherDescs(int index, bool useCategories)
{
	if (index >= m_objectDescManager.size())
	{
		// Invalid index;
		return;
	}

	IClass* pBaseClass = m_objectDescManager[index]->GetDescClass();
	CXTPReportRecord* pBaseRecord;
	CXTPReportRecordItem* pItem;
	CDescPropertyInfo* pInfo;
	CString caption;
	CString baseCaption;
	bool dirty = false;
	
	// Add a base expandable record for this specific desc base class
	pBaseRecord = m_pTreeDescSelection->AddRecord(new CXTPReportRecord());
	pItem = new CXTPReportRecordItem();
	caption = baseCaption = pBaseClass->GetName().c_str();
	pItem->SetCaption(caption);
	pInfo = new CDescPropertyInfo();
	pInfo->SetClass(pBaseClass);
	pItem->SetItemData(DWORD_PTR(pInfo));
	pBaseRecord->AddItem(pItem);
	pBaseRecord->SetExpanded(TRUE);

	if (useCategories && m_objectDescManager[index]->GetObjDescs().size() > 0)
	{
		// If there is only 1 class, disable categories
		useCategories = false;
		IClass* pFirstClass = NULL;
		for (int i = 0; i < m_objectDescManager[index]->GetObjDescs().size(); ++i)
		{
			if (CObjectDesc* pObjectDesc = m_objectDescManager[index]->GetObjDescs()[i]->GetObjectDesc())
			{
				if (IClass* pArrayClass = pObjectDesc->GetClass())
				{
					if (!pFirstClass)
					{
						pFirstClass = pArrayClass;
					}
					else if (pArrayClass != pFirstClass)
					{
						useCategories = true;
						break;
					}
				}
			}
		}
	}

	for (int i = 0; i < m_objectDescManager.size(); ++i)
	{
		// Parse the descs and populate the tree
		for (int j = 0; j < m_objectDescManager[i]->GetObjDescs().size(); ++j)
		{
			dirty = false;

			// Handle dirty descs
			CXTPReportRecord* pThisRecord = m_objectDescManager[i]->GetObjDescs()[j]->GetObjectTreeRootRecord();
			CDescPropertyInfo* pThisInfo = NULL;
			// Set this m_pTreeDescSelection entry as dirty if the m_pTreePropertyObjects root object is dirty
			if (pThisRecord)
			{
				CXTPReportRecordItem* pThisItem = pThisRecord->GetItem(COLUMN_DESC_NAME);
				if (pThisItem)
				{
					pThisInfo = (CDescPropertyInfo*)pThisItem->GetItemData();
					if (pThisInfo->GetDirtyCount() > 0)
					{
						dirty = true;
					}
				}
			}

			// Handle the specific desc class that has been selected
			if (i == index)
			{
				// These are individual file names
				CObjectDesc* pObjectDesc = m_objectDescManager[i]->GetObjDescs()[j]->GetObjectDesc();
				if (pObjectDesc == NULL)
				{
					// Object desc not found
					// NOTE: This can occur if a local file has been deleted.
					continue;
				}

				CXTPReportRecord* pClassRecord = pBaseRecord;
				IClass* pArrayClass = pObjectDesc->GetClass();

				// Split the descs into subclass categories
				if (useCategories && pArrayClass)
				{
					CXTPReportRecords* pChilds = pBaseRecord->GetChilds();
					CXTPReportRecordItem* pClassItem = pChilds->FindRecordItem(0, pChilds->GetCount(), 0, COLUMN_DESC_NAME, 0, 0, pArrayClass->GetName().c_str(), 0);
					if (!pClassItem)
					{
						CXTPReportRecord* pClassRecord = new CXTPReportRecord();
						pChilds->Add(pClassRecord);
						pClassItem = new CXTPReportRecordItem();
						
						// Move base class items into a special category that remains at the top of the desc list
						if (pArrayClass == pBaseClass)
						{
							pClassItem->SetCaption(string("<" + pArrayClass->GetName() + ">").c_str());
						}
						else
						{
							pClassItem->SetCaption(pArrayClass->GetName().c_str());
						}

						pClassRecord->AddItem(pClassItem);
						pClassRecord->SetExpanded(FALSE);
					}

					pClassRecord = pClassItem->GetRecord();
				}

				caption.Format("%s.xml", pObjectDesc->GetFilename().c_str());
				pItem = new CXTPReportRecordItem();
				pItem->SetCaption(caption);
				pItem->SetGroupCaption(baseCaption + CString(":") + caption);

				int iconIndex = pObjectDesc->GetIsModpackObj() ? DESC_ICON_OFFSET + 1 : DESC_ICON_OFFSET;
				pItem->SetIconIndex(iconIndex); // File icon

				// Only add non-dirty records for the specified index
				CXTPReportRecord* pDescFactoryRecord = new CXTPReportRecord();
				CDescPropertyInfo* pNewInfo = new CDescPropertyInfo(pObjectDesc, pArrayClass, NULL, NULL, m_objectDescManager[i]->GetObjDescs()[j]);
				pNewInfo->SetParentRecord(pClassRecord);
				pNewInfo->SetRecord(pDescFactoryRecord);
				pItem->SetItemData(DWORD_PTR(pNewInfo));
				if (dirty)
				{
					pItem->SetTextColor(RGB(192, 100, 0));
				}
				pDescFactoryRecord->AddItem(pItem);
				pClassRecord->GetChilds()->Add(pDescFactoryRecord);
			}
		}
	}

	// Expand top category
	if (useCategories)
	{
		m_pTreeDescSelection->Populate();

		if ( m_pTreeDescSelection->GetRows()->GetCount() > 1)
		{
			m_pTreeDescSelection->GetRows()->GetAt(1)->SetExpanded(TRUE);
		}
	}
}

void CDescEditor::GatherDirtyDescs(int index)
{	
	CXTPReportRecord* pBaseDirtyRecord;
	CXTPReportRecordItem* pItem;
	CString caption;
	CString baseCaption;
	bool dirty = false;

	// Add a base expandable record for all dirty descs
	pBaseDirtyRecord = m_pTreeDescSelection->AddRecord(new CXTPReportRecord());
	pItem = new CXTPReportRecordItem();
	pItem->SetCaption(DIRTY_DESC_NAME);
	pBaseDirtyRecord->AddItem(pItem);
	pBaseDirtyRecord->SetExpanded(TRUE);

	for (int i = 0; i < m_objectDescManager.size(); ++i)
	{
		// Parse the descs and populate the tree
		for (int j = 0; j < m_objectDescManager[i]->GetObjDescs().size(); ++j)
		{
			dirty = false;

			CXTPReportRecord* pThisRecord = m_objectDescManager[i]->GetObjDescs()[j]->GetObjectTreeRootRecord();
			CDescPropertyInfo* pThisInfo = NULL;
			// Set this m_descTree entry as dirty if the m_objectTree root object is dirty
			if (pThisRecord)
			{
				CXTPReportRecordItem* pThisItem = pThisRecord->GetItem(COLUMN_DESC_NAME);
				if (pThisItem)
				{
					pThisInfo = (CDescPropertyInfo*)pThisItem->GetItemData();
					if (pThisInfo->GetDirtyCount() > 0)
					{
						dirty = true;
					}
				}
			}

			if (dirty && i != index)
			{
				// These are individual file names
				CObjectDesc* pObjectDesc = m_objectDescManager[i]->GetObjDescs()[j]->GetObjectDesc();
				if (pObjectDesc == NULL)
				{
					// Object desc not found
					// NOTE: This can occur if a local file has been deleted.
					continue;
				}

				caption.Format("%s.xml", pObjectDesc->GetFilename().c_str());
				pItem = new CXTPReportRecordItem();
				pItem->SetCaption(caption);
				pItem->SetGroupCaption(baseCaption + CString(":") + caption);

				int iconIndex = pObjectDesc->GetIsModpackObj() ? DESC_ICON_OFFSET + 1 : DESC_ICON_OFFSET;
				pItem->SetIconIndex(iconIndex); // File icon

				// Only add non-dirty records for the specified index
				if (i != index && dirty)
				{
					CXTPReportRecord* pDirtyRecord = new CXTPReportRecord();
					pItem->SetItemData(DWORD_PTR(new CDescPropertyInfo(*pThisInfo)));
					pItem->SetTextColor(RGB(192, 100, 0));
					pDirtyRecord->AddItem(pItem);
					pBaseDirtyRecord->GetChilds()->Add(pDirtyRecord);
				}
			}
		}
	}

	// Get rid of the dirty category if there are no dirty items.
	if (pBaseDirtyRecord->GetChilds()->GetCount() == 0)
	{
		pBaseDirtyRecord->RemoveAll();
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::PopulateObjectProperties(CXTPReportRecord* pRecord, CDescPropertyInfo* pInfo)
{
	CXTPReportRecordItem* pItem;
	_smart_ptr<CReflectedObject> pObject;

	// This lands when cloning a property array that has no element
	if (!pInfo->GetElement() && pInfo->GetProperty())
	{
		PopulateObjectPropArrayElements(pRecord, pInfo);
		return;
	}

	pInfo->GetElement() ? pObject = pInfo->GetElement() : pObject = pInfo->GetObject();
	if (pObject != NULL)
	{
		IClass* pClass = pObject->GetClass();
		while (pClass != NULL)
		{
			CPropertyComponent* pProperties = ((CPropertyComponent*)pClass->GetComponent(eCCType_Properties));
			for (int i = 0; i < pProperties->GetPropertyCount(); ++i)
			{
				IProperty* p = pProperties->GetPropertyAt(i);

				// Only interested in arrays
				if (p->IsArray() == false)
					continue;

				IClass* pArrayClass = p->GetClassType(pObject);

				// Create a new branch for this array
				CXTPReportRecord* pDescFileRecord = new CXTPReportRecord();
				pRecord->GetChilds()->Add(pDescFileRecord);  // Add the branch to the tree

				// Create user data
				CDescPropertyInfo* pParentInfo = new CDescPropertyInfo(pObject, pArrayClass, p, NULL, pInfo->GetParentDesc());
				pParentInfo->SetParentInfo(pInfo);
				pParentInfo->SetParentRecord(pRecord);
				pParentInfo->SetRecord(pDescFileRecord);

				// Add a header leaf to the branch
				pItem = new CXTPReportRecordItem();

				// Create a caption for the new array item
				SetArrayCaption(pItem, p, pObject);
				pItem->SetIconIndex(p->GetType(pObject));
				pItem->SetItemData(DWORD_PTR(pParentInfo));  // Store user data on leaf
				pDescFileRecord->AddItem(pItem);  // Add the leaf to the branch

				PopulateObjectPropArrayElements(pDescFileRecord, pParentInfo);
			}

			pClass = pClass->GetBaseClass();
		}
	}
}

void CDescEditor::PopulateObjectPropArrayElements(CXTPReportRecord* pRecord, CDescPropertyInfo* pInfo)
{
	_smart_ptr<CReflectedObject> pObject = pInfo->GetObject();
	IProperty* p = pInfo->GetProperty();

	// If this array contains objects for elements then create a new
	// branch for it.
	if (p->GetType(pObject) != eVType_Object)
	{
		return;
	}

	// Add array elements to header leaf
	int numelements = p->GetElementCount(pObject);
	for (int j = 0; j < numelements; j++)
	{
		Value element = p->GetElementAt(pObject, j);

		_smart_ptr<CReflectedObject> pRefObject = _smart_ptr<CReflectedObject>(element);

		// Create a new branch for this element
		CXTPReportRecord* pNewRecord = new CXTPReportRecord();
		pRecord->GetChilds()->Add(pNewRecord);  // Add the leaf to the header

		// Create user data
		CDescPropertyInfo* pNewInfo = new CDescPropertyInfo(pObject, pRefObject->GetClass(), p, pRefObject, pInfo->GetParentDesc());
		pNewInfo->SetParentInfo(pInfo);
		pNewInfo->SetParentRecord(pRecord);
		pNewInfo->SetRecord(pNewRecord);

		// Add header leaf and set caption
		AddObjectPropArrayElementChild(pNewRecord, pNewInfo);

		// Recurse for object properties
		PopulateObjectProperties(pNewRecord, pNewInfo);
	}
}

//////////////////////////////////////////////////////////////////////////
CXTPReportRecordItem* CDescEditor::AddObjectPropArrayElementChild(CXTPReportRecord* pParentRecord, CDescPropertyInfo* pInfo)
{
	string caption;
	CPropertyGridItem::GetObjectCaption(pInfo->GetElement(), caption);
	CString ccaption(caption);

	CXTPReportRecordItem* pItem = NULL;
	pItem = new CXTPReportRecordItem();
	pItem->SetIconIndex(eVType_Object);
	pItem->SetItemData(DWORD_PTR(pInfo));
	pItem->SetCaption(ccaption);
	pParentRecord->AddItem(pItem);

	return pItem;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::ResolveDeletedElements(CObjectDesc* pDesc)
{
	if (!pDesc)
		return;

	m_pTreePropertyObjects->BeginUpdate();

	// Delete items flagged as IsDeleted()
	std::vector<CDescPropertyInfo*>::iterator i = m_deletedProperties.begin();
	std::vector<CDescPropertyInfo*>::iterator end = m_deletedProperties.end();
	while (i != end)
	{
		CDescPropertyInfo* pInfo = *i;
		if (pInfo && pInfo->GetParentDesc()->GetObjectDesc() == pDesc)
		{
			if (pInfo->IsDeleted())
			{
				pInfo->SetIsDeleted(false);
				DeleteObjectPropArrayElement(pInfo);

				if (pInfo->GetParentRecord() && pInfo->GetParentRecord()->GetItem(COLUMN_DESC_NAME))
				{
					if (pInfo->GetParentInfo() && pInfo->GetParentInfo()->GetProperty() && pInfo->GetParentInfo()->GetObject())
					{
						// Create a caption for the array item
						SetArrayCaption(pInfo->GetParentRecord()->GetItem(COLUMN_DESC_NAME), pInfo->GetParentInfo()->GetProperty(), pInfo->GetParentInfo()->GetObject());
					}
				}
			}

			i = m_deletedProperties.erase(i);
			end = m_deletedProperties.end();
		}
		else
		{
			++i;
		}
	}

	m_pTreePropertyObjects->EndUpdate();
	m_pTreePropertyObjects->Populate();
}

//////////////////////////////////////////////////////////////////////////
class ValidationAggregator
{
public:
	void Log(bool error, const char* pMessage)
	{
		m_Str += pMessage;
		m_Str += "\n";
	}

	string m_Str;
};

bool CDescEditor::SerializeDesc(CObjectDesc* pDesc)
{
	IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
	if (!pObjectDescFactory)
		return false;

	ResolveDeletedElements(pDesc);

	bool retval = pObjectDescFactory->Serialize(pDesc);

	IClassRegistry* pClassRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
	if (pClassRegistry)
	{
		ValidationAggregator va;

		string path = pDesc->GetClass()->GetName() + string("/") + pDesc->GetFilename() + ".xml";

		// Validate the current state of the object and display a message if there are issues.
		pClassRegistry->ValidateObject(pDesc, path.c_str(), functor(va, &ValidationAggregator::Log));

		if (!va.m_Str.empty())
		{
		    MessageBox(va.m_Str.c_str(), "Validation failed", MB_OK|MB_ICONWARNING);
		}
	}

	return retval;
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::SaveDesc(CObjectDesc* pDesc)
{
	IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
	if (!pObjectDescFactory)
		return false;

	string path = string(pObjectDescFactory->GetPath(pDesc, true));

	if (path)
	{
		if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
		{
			// Already exists - check out before save
			// Check out from source control
			do
			{
				if (m_sourceControl.CheckoutFile(path))
				{
					return SerializeDesc(pDesc);
				}
			}
			while (MessageBox("Unable to check out from source control.  Cancel to save without.\n" + path, "Save Failed", MB_RETRYCANCEL) == IDRETRY);

			if (MessageBox("Attempt to save without source control?\n" + path, "Notice", MB_YESNO) == IDYES)
			{
				return SerializeDesc(pDesc);
			}
		}
		else
		{
			// Does not exist, create it then add to source control
			if (SerializeDesc(pDesc))
			{
				// Add to source control
				do
				{
					if (m_sourceControl.AddFile(path))
					{
						return true;
					}
				}
				while (MessageBox("Unable to add to source control.  Retry?\n" + path, "Save Failed", MB_RETRYCANCEL) == IDRETRY);

				// Failed to add to source control but file was created
				MessageBox("File was created but was unable to be added to source control.  Add manually.\n" + path, "Notice", MB_OK);
				return true;
			}
			else
			{
				// Failed to create file
				MessageBox("Unable to create the file.  Check folder permissions.\n" + path, "Notice", MB_OK);
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::SetActiveTree(CDescEditorTree* pTree)
{
	if (m_pActiveTree == pTree)
		return;  // Same; abort.

	if (m_pActiveTree)
	{
		// Allow the currently active tree a chance to clean up
		OnBeforeDescElementSelectionChanged();
	}

	m_pActiveTree = pTree;

	ResetPropertyGrid();
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnBeforeDescElementSelectionChanged()
{
	if (m_pActiveTree && m_pPropertyGrid->GetCount() > 0)
	{
		CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(m_pPropertyGrid->GetSelectedItem());
		if (pItem)
		{
			// Deselect the item so any edits are applied to the current selected desc.
			pItem->Deselect();
		}

		CDescPropertyInfo* pInfo = NULL;
		if (m_pActiveTree == m_pTreeDescSelection)
		{
			// If the active tree is the desc tree then grab the Info off of the
			// object tree instead.

			if (m_pTreePropertyObjects)
			{
				CXTPReportRecord* pRecord = m_pTreePropertyObjects->GetSelectedDescRecord();
				if (pRecord)
				{
					pInfo = GetItemData(pRecord);
				}
			}
		}
		else
		{
			pInfo = GetItemData(m_pActiveTree->GetSelectedDescRecord());
		}

		if (pInfo)
		{
			m_pPropertyGrid->SaveState(pInfo->GetPropertyGridState());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::BuildPropertyGrid(CDescPropertyInfo* pInfo)
{
	ResetPropertyGrid();  // Clear current data

	if (pInfo == NULL || pInfo->IsDeleted())
		return;  // Invalid Info

	_smart_ptr<CReflectedObject> pObject = pInfo->GetElement();
	if (!pObject)
	{
		// If this isn't an element in an array then it should be an inline object
		pObject = pInfo->GetObject();
	}

	// If this is an element in an array, do not pass the associated property as it is the actual array property, not the element itself.
	IProperty* pProperty = (pInfo->GetElement() != NULL) ? NULL : pInfo->GetProperty();
	if (pProperty != NULL && pProperty->GetType(pObject) == eVType_Object)
	{
		// If this is the actual array property (not an element of it) and it's a list of objects
		// don't display them in the property grid as the outline panel handles adding/removing these
		// unlike primitive types which do need to be handled by the property grid.
		return;
	}

	if (m_pPropertyGrid)
	{
		m_pPropertyGrid->Populate(pObject, pProperty);  // Build
		m_pPropertyGrid->RestoreState(pInfo->GetPropertyGridState());  // Restore state
	}
}

//////////////////////////////////////////////////////////////////////////
int CDescEditor::HandlePopupMenu(CMenu* pMenu, CPoint& rPoint, CWnd* pParent)
{
	pParent->ClientToScreen(&rPoint);

	// Return -1 if we fail so that the return value can be used as an index.
	return CXTPCommandBars::TrackPopupMenu(pMenu, TPM_RETURNCMD|TPM_VCENTERALIGN, rPoint.x, rPoint.y, pParent);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::SetArrayCaption(CXTPReportRecordItem* pItem, IProperty* pProperty, CReflectedObject* pObject)
{
	if (pItem && pProperty && pObject)
	{
		string caption;
		caption.Format("%s {%d}", pProperty->GetName(), pProperty->GetElementCount(pObject));
		pItem->SetCaption(caption);
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::UpdateDirtyStatus(CDescEditorTree* pTree)
{
	if (pTree == m_pTreeDescSelection)
	{
		m_pTreeDescSelection->BeginUpdate();

		// Desc tree items 
		CXTPReportRecords* records = m_pTreeDescSelection->GetRecords();
		for (int i = 0; i < records->GetCount(); ++i)
		{
			if (CXTPReportRecord* pBaseRecord = records->GetAt(i))
			{
				for (int j = 0; j < pBaseRecord->GetChilds()->GetCount(); ++j)
				{
					if (CXTPReportRecord* pRecord = pBaseRecord->GetChilds()->GetAt(j))
					{
						UpdateDirtyStatusItem(pRecord, false, true);

						if (CXTPReportRecordItem* pItem = pRecord->GetItem(COLUMN_DESC_NAME))
						{
							if (_smart_ptr<CObjectDesc> pDesc = GetObjectDesc(pItem))
							{
								int iconIndex = pDesc->GetIsModpackObj() ? DESC_ICON_OFFSET + 1 : DESC_ICON_OFFSET;
								pItem->SetIconIndex(iconIndex); // File icon
							}
						}
					}
				}
			}
		}

		m_pTreeDescSelection->EndUpdate();
		m_pTreeDescSelection->Populate();
	}
	else
	{
		UpdateDirtyStatus(pTree->GetSelectedDescRecord(), false);
	}
}

//////////////////////////////////////////////////////////////////////////
// Update the active and desc trees dirty flags on records / items
void CDescEditor::UpdateDirtyStatus(CXTPReportRecord* pRecord, bool clean)
{
	if (!pRecord && m_pActiveTree)
	{
		// Snag the top level record from the active tree.
		pRecord = m_pActiveTree->GetRootRecord();
		if (pRecord)
		{
			CDescPropertyInfo* pInfo = GetItemData(pRecord);
			if (pInfo)
			{
				if (clean)
				{
					pInfo->SetDirtyCount(0);
					pInfo->GetPropertyGridState().ResetDirtyStatus(true);
				}

				if (m_pActiveTree != m_pTreeClipboard)
				{
					m_pActiveTree->BeginUpdate();
					UpdateItemDirtyTextColor(m_pActiveTree->GetSelectedDescRecordItem());
					m_pActiveTree->EndUpdate();
				}
			}
		}
	}

	// Check dirty status of entries under Dirty Descs on the m_descTree
	CXTPReportRecords* pRecords = m_pTreeDescSelection->GetRecords();
	if (pRecords)
	{
		if (pRecords->GetCount() > 1)
		{
			CXTPReportRecordItem* item = m_pTreeDescSelection->GetRecords()->FindRecordItem(0, m_pTreeDescSelection->GetRecords()->GetCount(), 0, 0, 0, 0, DIRTY_DESC_NAME, 0);
			if (item)
			{
				CXTPReportRecord* pLocalRecord = item->GetRecord();
				if (pLocalRecord)
				{
					m_pTreeDescSelection->BeginUpdate();

					for (int i = 0; i < pLocalRecord->GetChilds()->GetCount(); ++i)
					{
						CDescPropertyInfo* pInfo = GetItemData(pLocalRecord->GetChilds()->GetAt(i));
						if (pInfo)
						{
							pInfo = (CDescPropertyInfo*)pInfo->GetParentDesc()->GetObjectTreeRootRecord()->GetItem(COLUMN_DESC_NAME)->GetItemData();
							if (pInfo)
							{
								if (pInfo->GetDirtyCount() <= 0)
								{
									pLocalRecord->GetChilds()->RemoveAt(i);
								}
							}
						}
					}

					m_pTreeDescSelection->EndUpdate();
					m_pTreeDescSelection->Populate();
				}
			}
		}
	}

	// Check all of the individual items & children inside the supplied record
	
	UpdateDirtyStatusItem(pRecord, clean);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::UpdateDirtyStatusItem(CXTPReportRecord* pRecord, bool clean, bool isDescTreeItem)
{
	if (pRecord)
	{
		CXTPReportRecordItem* pItem = pRecord->GetItem(COLUMN_DESC_NAME);
		if (pItem)
		{
			CDescPropertyInfo* pInfo = (CDescPropertyInfo*)pItem->GetItemData();
			if (isDescTreeItem)
			{
				if (pInfo && pInfo->GetParentDesc())
				{
					CXTPReportRecord* pObjectTreeRecord = pInfo->GetParentDesc()->GetObjectTreeRootRecord();
					if (pObjectTreeRecord)
					{
						UpdateItemDirtyTextColor(pItem, GetItemData(pObjectTreeRecord));
					}
					else
					{
						// Must be clean
						pItem->SetTextColor(0xFFFFFFFF);
					}
				}
			}
			else
			{
				if (pInfo)
				{
					if (clean)
					{
						pInfo->SetDirtyCount(0);
						pInfo->GetPropertyGridState().ResetDirtyStatus(true);
					}

					UpdateItemDirtyTextColor(pItem);
				}
			}
		}

		if (pRecord->GetChilds())
		{
			for (int i = 0; i < pRecord->GetChilds()->GetCount(); ++i)
			{
				UpdateDirtyStatusItem(pRecord->GetChilds()->GetAt(i), clean, isDescTreeItem);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::UpdateItemDirtyTextColor(CXTPReportRecordItem* pItem, CDescPropertyInfo* pInfo)
{
	if (pItem)
	{
		if (!pInfo)
		{
			pInfo = (CDescPropertyInfo*)pItem->GetItemData();
		}

		if (pInfo && pInfo->GetDirtyCount() > 0)
		{
			pItem->SetTextColor(RGB(192, 100, 0));
		}
		else
		{
			pItem->SetTextColor(0xFFFFFFFF);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::ResetDescTree()
{
	ResetObjectPropertyTree();

	// Clear descpanelselection
	m_pTreeDescSelection->CleanupAllUserdata();
	m_pTreeDescSelection->GetRecords()->RemoveAll();
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::ResetObjectPropertyTree()
{
	ResetPropertyGrid();
	m_pTreePropertyObjects->SetSelectedDescRecord(NULL);
	BuildTreePropertyObjects(NULL);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::ResetPropertyGrid()
{
	if (m_pPropertyGrid)
	{
		m_pPropertyGrid->ResetContent();
	}
}

//////////////////////////////////////////////////////////////////////////
CDescEdObjectDescMgr* CDescEditor::FindDescEdObjectDescMgr(IClass* pClass)
{
	for (int i = 0; i < m_objectDescManager.size(); ++i)
	{
		if (m_objectDescManager[i]->GetDescClass() == pClass)
		{
			return m_objectDescManager[i];
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::ExpandAll(CXTPReportRecord* pRecord)
{
	if (pRecord)
	{
		pRecord->SetExpanded(TRUE);

		CXTPReportRecords* pChildren = pRecord->GetChilds();
		if (pChildren)
		{
			for (int i = 0; i < pChildren->GetCount(); ++i)
			{
				ExpandAll(pChildren->GetAt(i));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::CollapseAll(CXTPReportRecord* pRecord)
{
	if (pRecord)
	{
		pRecord->SetExpanded(FALSE);

		CXTPReportRecords* pChildren = pRecord->GetChilds();
		if (pChildren)
		{
			for (int i = 0; i < pChildren->GetCount(); ++i)
			{
				CollapseAll(pChildren->GetAt(i));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CDescPropertyInfo* CDescEditor::GetItemData(CXTPReportRow* pRow)
{
	if (pRow)
	{
		return GetItemData(pRow->GetRecord());
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
CDescPropertyInfo* CDescEditor::GetItemData(CXTPReportRecord* pRecord)
{
	if (pRecord)
	{
		CXTPReportRecordItem* pItem = pRecord->GetItem(COLUMN_DESC_NAME);
		if (pItem)
		{
			return (CDescPropertyInfo*)pItem->GetItemData();
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::BuildClassMenu(CMenu& menu, IClass* pClass, const char* title, bool IsCategories)
{
	if (!pClass)
		return false;

	DynArray<IClass*> pSubClasses = pClass->GetSubClasses(true);

	int iCount = 0;
	if (!pSubClasses.empty())
	{
		HMENU subMenu = CreatePopupMenu();
		AppendMenu(menu, MF_POPUP, (UINT)subMenu, title);

		std::map<string, HMENU> categoryMap;
		string category;
		HMENU* pMenu;

		int s_nRange = IsCategories ? MENU_CATEGORIES_END - MENU_CATEGORIES_BEGIN : MENU_CLASSES_END - MENU_CLASSES_BEGIN;
		for (int i = 0; i < pSubClasses.size() && i <= s_nRange; ++i)
		{
			// Look for categories to stuff into a submenu
			GetObjectCategory((CReflectedObject*)(pSubClasses[i]->CreateObject()), category);
			if (!category.empty())
			{
				// Find category in map and if it doesn't exist, create it
				if (!categoryMap[category])
				{
					categoryMap[category] = CreatePopupMenu();
				}

				pMenu = &categoryMap[category];
			}
			else
			{
				pMenu = &subMenu;
			}

			AppendMenu(*pMenu, MF_STRING, IsCategories ? MENU_CATEGORIES_BEGIN + i : MENU_CLASSES_BEGIN + i, pSubClasses[i]->GetName().c_str());
			++iCount;
		}

		// Place classes that are in categories at the top.
		int index = 0;
		for (std::map<string, HMENU>::iterator it = categoryMap.begin(); it != categoryMap.end(); ++it)
		{
			InsertMenu(subMenu, index++, MF_BYPOSITION | MF_POPUP, (UINT)it->second, it->first + " ...");
		}
	}

	return (iCount > 0);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::GetObjectCategory(CReflectedObject* pObject, string& category)
{
	category.clear();
	category.reserve(128);

	if (pObject)
	{
		pObject->GetEditCategory(category);

		// If no code driven category
		if (category.empty())
		{
			if (IClass* pClass = pObject->GetClass())
			{
				if (CClassProfile* pClassProfile = CClassProfileManager::GetInstance()->GetClassProfile(pClass->GetName(), false))
				{
					if (!pClassProfile->GetCategory().empty())
					{
						category = pClassProfile->GetCategory();
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<CObjectDesc> CDescEditor::GetObjectDesc(CXTPReportRecord* pRecord)
{
	if (pRecord)
	{
		if (CXTPReportRecordItem* pItem = pRecord->GetItem(COLUMN_DESC_NAME))
		{
			return GetObjectDesc(pItem);
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<CObjectDesc> CDescEditor::GetObjectDesc(CXTPReportRecordItem* pItem)
{
	if (pItem)
	{
		if (CDescPropertyInfo* pInfo = (CDescPropertyInfo*)pItem->GetItemData())
		{
			if (CDescEdObjectDesc* pDescEd = pInfo->GetParentDesc())
			{
				return pDescEd->GetObjectDesc();
			}
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnRButtonUpDescElementPanel(CDescEditorTree* pTree, void* pData, CPoint& rPoint)
{
	if (pTree == m_pTreeClipboard)
	{
		OnRButtonUpClipboardTree(pTree, pData, rPoint);
	}
	else if (pTree == m_pTreeDescSelection)
	{
		OnRButtonUpDescTree(pTree, pData, rPoint);
	}
	else if (pTree == m_pTreePropertyObjects)
	{
		OnRButtonUpObjectTree(pTree, pData, rPoint);
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnRButtonUpClipboardTree(CDescEditorTree* pTree, void* pData, CPoint& rPoint)
{
	CDescPropertyInfo* pInfo = (CDescPropertyInfo*)pData;

	CString name;
	CMenu menu;
	menu.CreatePopupMenu();

	CXTPReportRecord* pRecord = m_pTreeClipboard->GetSelectedDescRecord();
	if (pRecord)
	{
		while (pRecord->GetParentRecord())
		{
			pRecord = pRecord->GetParentRecord();
		}

		CMenu menu;
		menu.CreatePopupMenu();

		if (pInfo->GetRecord()->HasChildren())
		{
			menu.AppendMenu(MF_ENABLED, PMENU_EXPAND, "Expand All");
			menu.AppendMenu(MF_ENABLED, PMENU_COLLAPSE, "Collapse All");
			menu.AppendMenu(MF_SEPARATOR);
		}

		CXTPReportRecordItem* pItem = pRecord->GetItem(COLUMN_DESC_NAME);
		CString caption = "Remove [" + pItem->GetCaption(0) + "]";
		menu.AppendMenu(MF_ENABLED, 1, caption);

		int ret = HandlePopupMenu(&menu, rPoint, pTree);
		if (ret > 0)
		{
			// Todo: Allow user to delete elements out of cloned items on the clipboard instead of the entire entry

			// Todo: Extra clean up on parent Info ( object = new CDescEdObjectDesc(); )

			switch (ret)
			{
			case PMENU_EXPAND:
				{
					ExpandAll(pInfo->GetRecord());
					pTree->Populate();
				}
				break;

			case PMENU_COLLAPSE:
				{
					CollapseAll(pInfo->GetRecord());
					pTree->Populate();
				}
				break;

			default:
				{
					m_pTreeClipboard->BeginUpdate();
					m_pTreeClipboard->CleanupChildrenUserdata(pRecord);
					pRecord->Delete();
					m_pTreeClipboard->EndUpdate();
					m_pTreeClipboard->Populate();

					m_pTreeClipboard->SetSelectedDescRecord(NULL);
				}
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnRButtonUpDescTree(CDescEditorTree* pTree, void* pData, CPoint& rPoint)
{
	CDescPropertyInfo* pInfo = (CDescPropertyInfo*)pData;

	CString name;
	CMenu menu;
	menu.CreatePopupMenu();

	// If pData has a parent desc set, it means the user RClicked the top record in the object tree so treat it like they RClicked the desc tree
	if (pInfo && pInfo->GetParentDesc())
	{
		// Find the m_objectTree record that this m_descTree record is pointing to
		if (CXTPReportRecordItem* pItem = pInfo->GetParentDesc()->GetObjectTreeRootRecord()->GetItem(COLUMN_DESC_NAME))
		{
			pInfo = (CDescPropertyInfo*)pItem->GetItemData();
		}
		else
		{
			// Didn't find the m_objectTree record so bail.
			return;
		}
	}
	else
	{
		menu.AppendMenu(MF_ENABLED, PMENU_EXPAND, "Expand All");
		menu.AppendMenu(MF_ENABLED, PMENU_COLLAPSE, "Collapse All");

		DynArray<IClass*> subClasses;
		if (pInfo)
		{
			// RClicked the base desc in the desc tree so create a list of descs to add
			menu.AppendMenu(MF_SEPARATOR);

			IClass* pClass = pInfo->GetClass();
			if (pClass)
			{
				subClasses = pClass->GetSubClasses(true);

				for (int i = 0; i < subClasses.size(); ++i)
				{
					if (!subClasses[i]->IsAbstract())
					{
						name.Format("Add [%s]", subClasses[i]->GetName().c_str());
						menu.AppendMenu(MF_ENABLED, MENU_CLASSES_BEGIN + i, name);
					}
				}
			}
		}

		int ret = HandlePopupMenu(&menu, rPoint, pTree);
		if (ret > 0)
		{
			switch (ret)
			{
			case PMENU_EXPAND:
				{
					ExpandAll(pTree->GetSelectedDescRecord());
					pTree->Populate();
				}
				break;

			case PMENU_COLLAPSE:
				{
					CollapseAll(pTree->GetSelectedDescRecord());
					pTree->Populate();
				}
				break;

			default:
				{
					IClass* pClass = subClasses[ret - MENU_CLASSES_BEGIN];

					// Attempt to create the new desc.
					if (CreateDescFile(pClass))
					{
						// Update the Desc Panel to reflect the new desc
						OnComboBoxSelectDesc();
					}
				}
				break;
			}
		}

		return;
	}

	// Handle RClicking a desc itself and handle save, rename, delete, etc. on the desc
	if (pInfo)
	{
		if (CObjectDesc* pDesc = pInfo->GetParentDesc()->GetObjectDesc())
		{
			name.Format("Save [%s]", pDesc->GetFilename().c_str());
			menu.AppendMenu(MF_ENABLED, PMENU_SAVE, name);

			menu.AppendMenu(MF_SEPARATOR);
			menu.AppendMenu(MF_ENABLED, PMENU_RESET, "Reset");

			if (pTree == m_pTreeDescSelection)
			{
				menu.AppendMenu(MF_SEPARATOR);
				name.Format("Delete [%s]", pDesc->GetFilename().c_str());
				menu.AppendMenu(MF_ENABLED, PMENU_DELETE, name);
				menu.AppendMenu(MF_ENABLED, PMENU_RENAME, "Rename");
				menu.AppendMenu(MF_ENABLED, PMENU_DUPLICATE, "Duplicate");
			}

			if (pTree != m_pTreeDescSelection)
			{
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu(MF_ENABLED, PMENU_EXPAND, "Expand All");
				menu.AppendMenu(MF_ENABLED, PMENU_COLLAPSE, "Collapse All");
			}

			int ret = HandlePopupMenu(&menu, rPoint, pTree);
			if (ret > 0)
			{
				switch (ret)
				{
				case PMENU_EXPAND:
					{
						ExpandAll(pTree->GetSelectedDescRecord());
						pTree->Populate();
					}
					break;

				case PMENU_COLLAPSE:
					{
						CollapseAll(pTree->GetSelectedDescRecord());
						pTree->Populate();
					}
					break;

				case PMENU_SAVE:
					{
						if (SaveDesc(pDesc))
						{
							pInfo->ResetDirtyCount();

							// Set the selected object tree record to the root record so the property window can operate on it.
							m_pTreePropertyObjects->SetSelectedDescRecord(m_pTreePropertyObjects->GetRootRecord());
							m_pTreePropertyObjects->GetRows()->GetAt(0)->SetSelected(TRUE);

							CDescPropertyInfo* pLocalInfo = GetItemData(m_pTreePropertyObjects->GetRootRecord());
							pLocalInfo->ResetDirtyCount(true);

							UpdateDirtyStatus(NULL, true);
							UpdateDirtyStatus(m_pTreeDescSelection);
						}
						else
						{
							MessageBox("Save failed.", "Save Failed", MB_OK | MB_ICONSTOP);
						}
					}
					break;

				case PMENU_RESET:
					{
						ResetDesc(pInfo);
					}
					break;

				case PMENU_RENAME:
					{
						RenameDesc(pDesc, pInfo);
					}
					break;

				case PMENU_DUPLICATE:
					{
						DuplicateDesc(pDesc);
					}
					break;

				case PMENU_DELETE:
					{
						if (DeleteDesc(pDesc))
						{
							ResetDesc(pInfo, true);
						}
					}
					break;

				default:
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::OnRButtonUpObjectTree(CDescEditorTree* pTree, void* pData, CPoint& rPoint)
{
	CDescPropertyInfo* pInfo = (CDescPropertyInfo*)pData;

	CString name;
	CMenu menu;
	menu.CreatePopupMenu();

	if (!pInfo->GetElement())
	{
		if (pInfo->GetProperty())
		{
			_smart_ptr<CReflectedObject> pObj = pInfo->GetObject();
			IProperty* pProperty = pInfo->GetProperty();
			IClass* pClass = pProperty->GetClassType(pObj);

			if (pClass)
			{
				if (pInfo->GetRecord()->HasChildren())
				{
					menu.AppendMenu(MF_ENABLED, PMENU_EXPAND, "Expand All");
					menu.AppendMenu(MF_ENABLED, PMENU_COLLAPSE, "Collapse All");
					menu.AppendMenu(MF_SEPARATOR);
				}

				BuildClassMenu(menu, pClass, "Add...");
				menu.AppendMenu(MF_SEPARATOR);
				BuildClassMenu(menu, pClass, "Assign Categories...", true);

				DynArray<IClass*> subClasses;
				subClasses = pClass->GetSubClasses(true);

				int ret = HandlePopupMenu(&menu, rPoint, pTree);
				if (ret > 0)
				{
					if (ret >= MENU_CLASSES_BEGIN && ret < MENU_CATEGORIES_BEGIN)
					{
						AddObjectPropArrayElement(pInfo, subClasses[ret  - MENU_CLASSES_BEGIN], m_pTreePropertyObjects);

						if (pInfo->GetRecord() && pInfo->GetRecord()->GetItem(COLUMN_DESC_NAME))
						{
							// Create a caption for the array item
							SetArrayCaption(pInfo->GetRecord()->GetItem(COLUMN_DESC_NAME), pProperty, pObj);
						}
					}
					else if (ret >= MENU_CATEGORIES_BEGIN)
					{
						ret -= MENU_CATEGORIES_BEGIN;  // Normalize index

						_smart_ptr<CReflectedObject> pNewObject = subClasses[ret]->CreateObject();

						if (pNewObject)
						{
							CSimpleEditPopup dialog;
							dialog.SetDialogLabel("Enter a category name for this class");
							dialog.SetDialogTitle("Assign class category");

							string category;
							GetObjectCategory(pNewObject, category);
							dialog.SetEditString(category);

							if (dialog.DoModal() == IDOK)
							{
								// Save category to profiles
								if (IClass* pClass = pNewObject->GetClass())
								{
									if (CClassProfile* pClassProfile = CClassProfileManager::GetInstance()->GetClassProfile(pClass->GetName(), true))
									{
										pClassProfile->SetCategory(category);

										CClassProfileManager::GetInstance()->Save();
									}
								}
							}
						}
					}
					else
					{
						switch (ret)
						{
						case PMENU_EXPAND:
							{
								ExpandAll(pInfo->GetRecord());
								m_pTreePropertyObjects->Populate();
							}
							break;

						case PMENU_COLLAPSE:
							{
								CollapseAll(pInfo->GetRecord());
								m_pTreePropertyObjects->Populate();
							}
							break;
						}
					}
				}
			}
			else  // Native Type
			{
				string type;
				GetValueTypeName(pProperty->GetType(pObj), type);

				name.Format("Add [%s]", type.c_str());
				menu.AppendMenu(MF_ENABLED, MENU_CLASSES_BEGIN, name);

				int ret = HandlePopupMenu(&menu, rPoint, pTree);
				if (ret > 0)
				{
					AddObjectPropArrayElement(pInfo, pInfo->GetClass(), m_pTreePropertyObjects);

					if (pInfo->GetRecord() && pInfo->GetRecord()->GetItem(COLUMN_DESC_NAME))
					{
						// Create a caption for the array item
						SetArrayCaption(pInfo->GetRecord()->GetItem(COLUMN_DESC_NAME), pProperty, pObj);
					}
				}
			}
		}
		else
		{
			// If there is no element or property then the user RClicked the top record ( desc ) so treat as if they RClicked the desc tree
			OnRButtonUpDescTree(pTree, (void*)m_pTreePropertyObjects->GetSelectedDescRecordItem()->GetItemData(), rPoint);
		}
	}
	else
	{
		if (pInfo->GetObject())
		{
			// User RClicked an existing element in the tree.  Create a context menu of options.

			string caption;
			CPropertyGridItem::GetObjectCaption(pInfo->GetElement(), caption);

			if (pInfo->GetRecord()->HasChildren())
			{
				menu.AppendMenu(MF_ENABLED, PMENU_EXPAND, "Expand All");
				menu.AppendMenu(MF_ENABLED, PMENU_COLLAPSE, "Collapse All");
				menu.AppendMenu(MF_SEPARATOR);
			}

			if (!pInfo->IsDeleted())
			{
				name.Format("Delete [%s]", caption.c_str());
				menu.AppendMenu(MF_ENABLED, PMENU_DELETE, name);
			}
			else
			{
				menu.AppendMenu(MF_ENABLED, PMENU_UNDO, "UNDO delete");
			}

			int ret = HandlePopupMenu(&menu, rPoint, pTree);
			if (ret > 0)
			{
				switch (ret)
				{
				case PMENU_EXPAND:
					{
						ExpandAll(pInfo->GetRecord());
						m_pTreePropertyObjects->Populate();
					}
					break;

				case PMENU_COLLAPSE:
					{
						CollapseAll(pInfo->GetRecord());
						m_pTreePropertyObjects->Populate();
					}
					break;

				// Delete an array element
				case PMENU_DELETE:
					{
						// Mark to be deleted when saved
						pInfo->SetIsDeleted(true);
						m_deletedProperties.push_back(pInfo);
						pInfo->IncrementDirty();
						ResetPropertyGrid();

						// Hide the children
						pTree->BeginUpdate();
						CXTPReportRecord* pRecord = pInfo->GetRecord();
						pRecord->GetItem(0)->SetFocusable(FALSE);
						for (int i = 0; i < pRecord->GetChilds()->GetCount(); ++i)
						{
							pRecord->GetChilds()->GetAt(i)->SetVisible(FALSE);
						}
						pTree->EndUpdate();
						pTree->Populate();

						UpdateDirtyStatus();
						UpdateDirtyStatus(m_pTreeDescSelection);
					}
					break;

				// Undo array element deletion
				case PMENU_UNDO:
					{
						// Remove the mark to be deleted when saved
						pInfo->SetIsDeleted(false);
						pInfo->DecrementDirty();

						// Unhide the children
						pTree->BeginUpdate();
						CXTPReportRecord* pRecord = pInfo->GetRecord();
						pRecord->GetItem(0)->SetFocusable(TRUE);
						for (int i = 0; i < pRecord->GetChilds()->GetCount(); ++i)
						{
							pRecord->GetChilds()->GetAt(i)->SetVisible(TRUE);
						}
						pTree->EndUpdate();
						pTree->Populate();

						UpdateDirtyStatus();
						UpdateDirtyStatus(m_pTreeDescSelection);
						BuildPropertyGrid(pInfo);
					}
					break;

				default:
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::RenameDesc(CObjectDesc* pDesc, CDescPropertyInfo* pInfo)
{
	string filename;
	CSimpleEditPopup dialog;
	dialog.SetDialogTitle("Rename");
	dialog.SetDialogLabel("Enter new name");
	dialog.SetEditString(filename);
	if (dialog.DoModal() == IDOK && !filename.empty())
	{
		IClassRegistry* pClassRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
		_smart_ptr<CReflectedObject> pClone = pClassRegistry->CloneObject(pDesc);
		CObjectDesc* pNewDesc = (CObjectDesc*)pClone.get();
		pNewDesc->SetFilename(filename);
		pNewDesc->SetNameID(filename.MakeLower());

		IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
		string oldPath = pObjectDescFactory->GetPath(pDesc, true);
		string newPath = pObjectDescFactory->GetPath(pNewDesc, true);

		if (!CFileUtil::FileExists(oldPath.c_str()))
		{
			MessageBox("Source file doesn't exist (mod pack loaded?)", "Notice", MB_OK | MB_ICONSTOP);
			return;
		}

		if (CFileUtil::FileExists(newPath.c_str()))
		{
			MessageBox("File already exists", "Notice", MB_OK | MB_ICONSTOP);
			return;
		}

		if (!m_sourceControl.RenameFile(oldPath, newPath))
		{
			if (MessageBox("Source control failed to rename the file. Rename locally without source control?", "Notice", MB_YESNO | MB_ICONSTOP) != IDYES)
			{
				return;
			}
			else
			{
				if (CFileUtil::MoveFile(oldPath.c_str(), newPath.c_str(), false) != CFileUtil::ETREECOPYOK)
				{
					MessageBox("Failed to rename", "Notice", MB_OK | MB_ICONSTOP);
					return;
				}
			}
		}

		if (pObjectDescFactory->IsUsingModpack())
		{
			// Reset the original desc so it will reload from disk
			ResetDesc(pInfo, true);

			// Find the base class for the path
			IClass* pClass = pNewDesc->GetClass();
			IClass* pBaseClass = pClass;
			while (pBaseClass->GetBaseClass()->GetName() != "CObjectDesc")
			{
				pBaseClass = pBaseClass->GetBaseClass();
			}

			// Track down the local Desc Manager to to add the new desc to.
			CDescEdObjectDescMgr* pDescMgr = FindDescEdObjectDescMgr(pBaseClass);
			std::vector<CDescEdObjectDesc*> objDescs = pDescMgr->GetObjDescs();

			// Determine if a record with filename already exists and if so, overwrite it otherwise create new
			for (std::vector<CDescEdObjectDesc*>::iterator it = objDescs.begin(); it != objDescs.end(); ++it)
			{
				_smart_ptr<CObjectDesc> objectDesc = ((CDescEdObjectDesc*)*it)->GetObjectDesc();
				if (objectDesc && objectDesc->GetFilename() == filename)
				{
					IClass* pClass = pDesc->GetClass();
					RemoveDescFile(pClass, objectDesc);
					break;
				}
			}

			// Add the new desc to the desc panel list.
			if (pDescMgr)
			{
				if (AddDescFile(pClass, filename, *pDescMgr))
				{
					// Update the Desc Panel to reflect the new desc
					OnComboBoxSelectDesc();
				}
			}

			// Todo: Update object tree with new filename
		}
		else
		{
			// Update desc with new filename
			pDesc->SetFilename(filename);
			pDesc->SetNameID(filename.MakeLower());

			// Update both trees with new name
			CXTPReportRecord* pRecord = pInfo->GetRecord();
			if (pRecord)
			{
				CXTPReportRecordItem* pItem = pRecord->GetItem(COLUMN_DESC_NAME);
				if (pItem)
				{
					pItem->SetCaption(CString(pDesc->GetFilename()) + ".xml");
				}

				pItem = m_pTreeDescSelection->GetSelectedDescRecordItem();
				if (pItem)
				{
					pItem->SetCaption(CString(pDesc->GetFilename()) + ".xml");
				}

				m_pTreePropertyObjects->Populate();
				m_pTreeDescSelection->Populate();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CDescEditor::DeleteDesc(CObjectDesc* pDesc)
{
	if (!pDesc)
		return false;

	// Verify the user intended to do delete the file
	CString message;
	message.Format("Delete [%s]?", pDesc->GetFilename());
	if (MessageBox(message, "Confirm", MB_YESNO | MB_ICONSTOP) == IDYES)
	{
		// Delete the file
		IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
		string path = pObjectDescFactory->GetPath(pDesc, true);

		// Use source control to delete
		if (!m_sourceControl.DeleteFile(path))
		{
			MessageBox("Failed to delete the file.  Check permissions.", "Notice", MB_OK | MB_ICONSTOP);
			return false;
		}

		// If not using a mod pack, or if the file no longer exists in either a mod pack or original depot, remove it from the list
		if (!pObjectDescFactory->IsUsingModpack() || !CFileUtil::FileExists(pObjectDescFactory->GetPath(pDesc, false)))
		{
			IClass* pClass = pDesc->GetClass();

			// Remove the desc from CObjectDesc.
			RemoveDescFile(pClass, pDesc);

			// Update m_descTree and m_objectTree to reflect the change.
			CXTPReportRecord* pRecord = m_pTreeDescSelection->GetSelectedDescRecord();
			if (pRecord)
			{
				// Reset the internal storage of selected desc record and item
				m_pTreeDescSelection->SetSelectedDescRecord(NULL);

				// Clean out m_objectTree since that desc is gone.
				ResetObjectPropertyTree();

				// Delete the m_descTree record for the delete desc.
				m_pTreeDescSelection->BeginUpdate();
				pRecord->Delete();
				m_pTreeDescSelection->EndUpdate();
				m_pTreeDescSelection->Populate();

				// Rebuild the m_descTree to reflect the missing xml file and update record indexes.
				OnComboBoxSelectDesc();
			}
		}
		else
		{
			// Returning true calls reset on the desc. Return true since the file still exists in another folder.  
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::DuplicateDesc(CObjectDesc* pDesc)
{
	if (!pDesc)
		return;

	string filename;
	CSimpleEditPopup dialog;
	dialog.SetDialogTitle("Duplicate");
	dialog.SetDialogLabel("Enter name for duplicate");
	dialog.SetEditString(filename);
	if (dialog.DoModal() == IDOK && !filename.empty())
	{
		IClass* pClass = pDesc->GetClass();

		// Find the base class for the path
		IClass* pBaseClass = pClass;
		while (pBaseClass->GetBaseClass()->GetName() != "CObjectDesc")
		{
			pBaseClass = pBaseClass->GetBaseClass();
		}

		IClassRegistry* pClassRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
		_smart_ptr<CReflectedObject> pClone = pClassRegistry->CloneObject(pDesc);
		CObjectDesc* pNewDesc = (CObjectDesc*)pClone.get();
		pNewDesc->SetFilename(filename);
		pNewDesc->SetNameID(filename.MakeLower());
		pClass = pNewDesc->GetClass();

		IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
		CString newPath;
		// Make sure the file doesn't already exist in the mod pack
		if (pObjectDescFactory->IsUsingModpack())
		{
			newPath = pObjectDescFactory->GetPath(pNewDesc, true);
			if (CFileUtil::FileExists(newPath))
			{
				MessageBox("File already exists", "Notice", MB_OK | MB_ICONSTOP);
				return;
			}
		}

		// Make sure the file doesn't already exist in the main depot
		newPath = pObjectDescFactory->GetPath(pNewDesc, false);
		if (CFileUtil::FileExists(newPath))
		{
			MessageBox("File already exists", "Notice", MB_OK | MB_ICONSTOP);
			return;
		}

		// Track down the local Desc Manager to to add the new desc to.
		CDescEdObjectDescMgr* pDescMgr = FindDescEdObjectDescMgr(pBaseClass);

		// Save the desc and add it to the desc panel list.
		if (pDescMgr)
		{
			if (SaveDesc(pNewDesc))
			{
				if (AddDescFile(pClass, filename, *pDescMgr))
				{
					// Update the Desc Panel to reflect the new desc
					OnComboBoxSelectDesc();
				}
				else
				{
					MessageBox("Saved desc but was unable to add desc to desc panel. Restart Desc Editor.", "Notice", MB_OK | MB_ICONSTOP);
				}
			}
			else
			{
				MessageBox("Save failed.", "Save Failed", MB_OK | MB_ICONSTOP);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::GetPropertyAncestors(std::map<string, IClass*>& list)
{
	list.clear();

	if (CXTPReportRecord* pRecord = m_pTreePropertyObjects->GetSelectedDescRecord())
	{
		// Set pRecord to immediate parent of the selected record.
		if (pRecord = pRecord->GetParentRecord())
		{
			// Start finding ancestors at "grandparent" so that siblings are ignored.
			while (pRecord = pRecord->GetParentRecord())
			{
				if (CDescPropertyInfo* pInfo = GetItemData(pRecord))
				{
					if (IProperty* pProperty = pInfo->GetProperty())
					{
						if (pProperty->GetType(pInfo->GetObject()) == eVType_Object && pProperty->IsArray())
						{
							if (!pInfo->GetElement())
							{
								list[pProperty->GetName()] = pInfo->GetClass();
							}
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditor::GetPropertyElementList(IClass* pFindClass, std::vector<string>& list)
{
	list.clear();

	if (CXTPReportRecord* pRecord = m_pTreePropertyObjects->GetSelectedDescRecord())
	{
		CXTPReportRecord* pParentRecord = pRecord->GetParentRecord();

		while (pParentRecord)
		{
			if (CDescPropertyInfo* pInfo = GetItemData(pParentRecord))
			{
				if (pInfo->GetClass() == pFindClass)
				{
					pParentRecord = pParentRecord->GetParentRecord();
					CXTPReportRecords* pChilds = pParentRecord->GetChilds();
					// Fill context menu with all children of pParentItem
					for (int i = 0; i < pChilds->GetCount(); ++i)
					{
						list.push_back(pChilds->GetAt(i)->GetItem(COLUMN_DESC_NAME)->GetCaption(COLUMN_DESC_NAME).GetString());
					}

					break;
				}
			}

			pParentRecord = pParentRecord->GetParentRecord();
		}
	}
}

_smart_ptr<CObjectDesc> CDescEditor::GetSelectedObjectDesc()
{ 
	return GetObjectDesc(m_pTreePropertyObjects->GetRootRecord()); 
}

