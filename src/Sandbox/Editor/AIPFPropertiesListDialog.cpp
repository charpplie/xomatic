#include "StdAfx.h"
#include "AIPFPropertiesListDialog.h"

#include "AI/AIManager.h"
#include "IAgent.h"


IMPLEMENT_DYNAMIC(CAIPFPropertiesListDialog, CDialog)

CAIPFPropertiesListDialog::CAIPFPropertiesListDialog(CWnd* pParent)
: CDialog(IDD_AIPFPROPERTIESLIST, pParent),
  m_scriptFileName("Scripts/AI/pathfindProperties.lua")
{
}

void CAIPFPropertiesListDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE, m_TreeCtrl);
}


BEGIN_MESSAGE_MAP(CAIPFPropertiesListDialog, CDialog)
	ON_BN_CLICKED   (IDEDIT,      OnBnClickedEdit)
	ON_BN_CLICKED   (IDREFRESH,   OnBnClickedRefresh)
	
	ON_WM_SHOWWINDOW()

	ON_NOTIFY   (TVN_KEYDOWN,    IDC_TREE,   OnTVKeyDown)
	ON_NOTIFY   (NM_CLICK,       IDC_TREE,   OnTVClick)
	ON_NOTIFY   (NM_DBLCLK,      IDC_TREE,   OnTVDblClk)
	ON_NOTIFY   (TVN_SELCHANGED, IDC_TREE,   OnTVSelChanged)
END_MESSAGE_MAP()


void CAIPFPropertiesListDialog::OnBnClickedEdit()
{
	CFileUtil::EditTextFile(m_scriptFileName);
}

void CAIPFPropertiesListDialog::OnBnClickedRefresh()
{
	bool success = CFileUtil::CompileLuaFile(m_scriptFileName)
		&& GetIEditor()->GetSystem()->GetIScriptSystem()->ReloadScript(m_scriptFileName);

	if (success)
	{
		UpdateList();
	}
	else
	{
		char error[1000];
		sprintf(error, "Script file %s reload failure!", m_scriptFileName);
		MessageBox(error, "Sandbox Error", MB_ICONERROR | MB_OK);
	}
}

void CAIPFPropertiesListDialog::OnTVClick(NMHDR* pNMHdr, LRESULT* pResult)
{
	TVHITTESTINFO hti;
	GetCursorPos(&hti.pt);
	m_TreeCtrl.ScreenToClient(&hti.pt);
	m_TreeCtrl.HitTest(&hti);
	if (hti.hItem && hti.flags == TVHT_ONITEMSTATEICON)
	{
		m_TreeCtrl.SelectItem(hti.hItem);
		ToggleItemState();
	}
}

void CAIPFPropertiesListDialog::OnTVDblClk(NMHDR*, LRESULT*)
{
	TVHITTESTINFO hti;
	GetCursorPos(&hti.pt);
	m_TreeCtrl.ScreenToClient(&hti.pt);
	m_TreeCtrl.HitTest(&hti);
	if (hti.hItem && hti.flags != TVHT_ONITEMSTATEICON)
	{
		m_TreeCtrl.SelectItem(hti.hItem);
		ToggleItemState("Do SetCheck(...)");
	}
}

void CAIPFPropertiesListDialog ::OnTVKeyDown(NMHDR* pNMHdr, LRESULT* pResult)
{
	NMTVKEYDOWN* nmkd = (NMTVKEYDOWN*) pNMHdr;
	if (nmkd->wVKey == ' ')
		ToggleItemState();
}

void CAIPFPropertiesListDialog::OnTVSelChanged(NMHDR*, LRESULT*)
{
	UpdateDescription();
}

void CAIPFPropertiesListDialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
		PostMessage(WM_COMMAND, (BN_CLICKED << 16) | IDREFRESH, (LPARAM) GetDlgItem(IDREFRESH)->m_hWnd);
}

void CAIPFPropertiesListDialog::UpdateList()
{
	m_TreeCtrl.DeleteAllItems();
	m_setPFProperties.clear();
	m_mapPFProperties.clear();

	CString sTempPFPropertiesList = m_sPFPropertiesList;
	CString token;
	int index = 0;
	while (!(token = sTempPFPropertiesList.Tokenize(" ,", index)).IsEmpty())
	{
		HTREEITEM item = m_TreeCtrl.InsertItem(token, TVI_ROOT, TVI_SORT);
		m_TreeCtrl.SelectItem(item);
		ToggleItemState("Do SetCheck(...)");
	}

	CAIManager* pAIManager = GetIEditor()->GetAI();
	CString pathTypeNames = pAIManager->GetPathTypeNames();

	index = 0;
	while (!(token = pathTypeNames.Tokenize(" ", index) ).IsEmpty())
	{
		m_mapPFProperties[token] = pAIManager->GetPFPropertiesOfPathType(string(token.GetString()));
		if (m_setPFProperties.find(token) == m_setPFProperties.end())
			m_TreeCtrl.InsertItem(token, TVI_ROOT, TVI_SORT);
	}

	UpdateDescription();
}

void CAIPFPropertiesListDialog::UpdateDescription()
{
	char description[1000] = { 0 };
	HTREEITEM item = m_TreeCtrl.GetSelectedItem();
	if (item)
	{
		CString pathTypeName = m_TreeCtrl.GetItemText(item);
		const AgentPathfindingProperties* p = m_mapPFProperties[pathTypeName];
		if (p)
		{
			sprintf(description,

				"%s:\r\n"
				"navCapMask = %u\r\n"
				"triangularResistanceFactor = %f\r\n"
				"waypointResistanceFactor = %f\r\n"
				"flightResistanceFactor = %f\r\n"
				"volumeResistanceFactor = %f\r\n"
				"roadResistanceFactor = %f\r\n"
				"waterResistanceFactor = %f\r\n"
				"maxWaterDepth = %f\r\n"
				"minWaterDepth = %f\r\n"
				"exposureFactor = %f\r\n"
				"dangerCost = %f\r\n"
				"zScale = %f\r\n"
				"customNavCapsMask = %u\r\n"
				"radius = %f\r\n"
				"height = %f\r\n"
				"maxSlope = %f",

				pathTypeName.GetString(),
				p->navCapMask,
				p->triangularResistanceFactor,
				p->waypointResistanceFactor,
				p->flightResistanceFactor,
				p->volumeResistanceFactor,
				p->roadResistanceFactor,
				p->waterResistanceFactor,
				p->maxWaterDepth, 
				p->minWaterDepth,
				p->exposureFactor,
				p->dangerCost,
				p->zScale,
				p->customNavCapsMask,
				p->radius,
				p->height,
				p->maxSlope);
		}
		else
		{
			sprintf(description, "%s:\r\n\r\nPathfinding properties unknown.", pathTypeName.GetString());
		}
	}
	SetDlgItemText(IDC_DESCRIPTION, description);
}

void CAIPFPropertiesListDialog::UpdatePFPropertiesString()
{
	m_sPFPropertiesList.Empty();
	std::set<CString>::iterator it, itEnd = m_setPFProperties.end();
	for (it = m_setPFProperties.begin(); it != itEnd; ++it)
	{
		if (!m_sPFPropertiesList.IsEmpty())
			m_sPFPropertiesList += ", ";
		m_sPFPropertiesList += *it;
	}
	SetDlgItemText(IDC_SELECTIONLIST, m_sPFPropertiesList);
}

void CAIPFPropertiesListDialog::ToggleItemState(bool bInvokeSetCheck)
{
	HTREEITEM item = m_TreeCtrl.GetSelectedItem();
	if (item)
	{
		bool bOn = ! (m_TreeCtrl.GetItemState(item, TVIS_BOLD) & TVIS_BOLD);

		m_TreeCtrl.SetItemState(item, bOn ? TVIS_BOLD : 0, TVIS_BOLD);
		
		if (bInvokeSetCheck)
			m_TreeCtrl.SetCheck(item, bOn);
		
		if (bOn)
			m_setPFProperties.insert(m_TreeCtrl.GetItemText(item));
		else
			m_setPFProperties.erase(m_TreeCtrl.GetItemText(item));
		
		UpdatePFPropertiesString();
	}
}
