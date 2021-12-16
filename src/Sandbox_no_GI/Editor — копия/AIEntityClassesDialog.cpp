#include "StdAfx.h"
#include "AIEntityClassesDialog.h"

#include "AI/AIManager.h"
#include "IAgent.h"


IMPLEMENT_DYNAMIC(CAIEntityClassesDialog, CDialog)

CAIEntityClassesDialog::CAIEntityClassesDialog(CWnd* pParent) :
	CDialog(IDD_AIENTITYCLASSES, pParent)
{
}


void CAIEntityClassesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE, m_TreeCtrl);
}


BEGIN_MESSAGE_MAP(CAIEntityClassesDialog, CDialog)
	ON_BN_CLICKED   (IDREFRESH,   UpdateList)

	ON_WM_SHOWWINDOW()

	ON_NOTIFY   (TVN_KEYDOWN,    IDC_TREE,   OnTVKeyDown)
	ON_NOTIFY   (NM_CLICK,       IDC_TREE,   OnTVClick)
	ON_NOTIFY   (NM_DBLCLK,      IDC_TREE,   OnTVDblClk)
	ON_NOTIFY   (TVN_SELCHANGED, IDC_TREE,   OnTVSelChanged)
END_MESSAGE_MAP()


void CAIEntityClassesDialog::OnTVClick(NMHDR*, LRESULT*)
{
	TVHITTESTINFO hti;
	GetCursorPos(&hti.pt);
	m_TreeCtrl.ScreenToClient(&hti.pt);
	m_TreeCtrl.HitTest(&hti);
	if (hti.hItem && (hti.flags == TVHT_ONITEMSTATEICON))
	{
		m_TreeCtrl.SelectItem(hti.hItem);
		ToggleItemState();
	}
}


void CAIEntityClassesDialog::OnTVDblClk(NMHDR*, LRESULT*)
{
	TVHITTESTINFO hti;
	GetCursorPos(&hti.pt);
	m_TreeCtrl.ScreenToClient(&hti.pt);
	m_TreeCtrl.HitTest(&hti);
	if (hti.hItem && (hti.flags != TVHT_ONITEMSTATEICON))
	{
		m_TreeCtrl.SelectItem(hti.hItem);
		ToggleItemState("Do SetCheck(...)");
	}
}


void CAIEntityClassesDialog::OnTVKeyDown(NMHDR* pNMHdr, LRESULT* pResult)
{
	NMTVKEYDOWN* nmkd = (NMTVKEYDOWN*) pNMHdr;
	if (nmkd->wVKey == ' ')
	{
		ToggleItemState();
	}
}


void CAIEntityClassesDialog::OnTVSelChanged(NMHDR*, LRESULT*)
{
	UpdateDescription();
}


void CAIEntityClassesDialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow)
	{
		CWnd* pWnd = GetDlgItem(IDREFRESH);
		PostMessage(WM_COMMAND, (BN_CLICKED << 16) | IDREFRESH, (LPARAM) pWnd->m_hWnd);
		pWnd->ShowWindow(SW_HIDE);
	}
}


void CAIEntityClassesDialog::UpdateList()
{
	m_TreeCtrl.DeleteAllItems();
	m_setAIEntityClasses.clear();

	CString sTempAIEntityClasses = m_sAIEntityClasses;
	CString token;
	int index = 0;
	while (!(token = sTempAIEntityClasses.Tokenize(" ,", index)).IsEmpty())
	{
		HTREEITEM item = m_TreeCtrl.InsertItem(token, TVI_ROOT, TVI_SORT);
		m_TreeCtrl.SelectItem(item);
		ToggleItemState("Do SetCheck(...)");
	}

	const CString sStartPath1 = "Scripts/Entities/AI/NewHumans/Human";
	const CString sStartPath2 = "Scripts/Entities/AI/NewAliens/Alien";

	IEntityClass* pEntityClass;
	IEntityClassRegistry* pEntityClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
	for (pEntityClassRegistry->IteratorMoveFirst(); pEntityClass = pEntityClassRegistry->IteratorNext(); )
	{
		
		CString sFileName = pEntityClass->GetScriptFile();

		assert(sStartPath1.GetLength() == sStartPath2.GetLength());	// We implicitly use it in the next line.
		if (!sFileName.IsEmpty() && (sFileName.GetLength() > sStartPath1.GetLength()))
		{
			if ((sFileName.Left(sStartPath1.GetLength()) == sStartPath1) ||
					(sFileName.Left(sStartPath2.GetLength()) == sStartPath2))
			{
				// check the table has properties (a good filter)
				SmartScriptTable pSSTable;
				gEnv->pScriptSystem->GetGlobalValue(pEntityClass->GetName(), pSSTable);
				if (pSSTable && pSSTable->HaveValue("Properties"))
				{
					CString sClassName = pEntityClass->GetName();
					if (m_setAIEntityClasses.find(sClassName) == m_setAIEntityClasses.end())
						m_TreeCtrl.InsertItem(sClassName.GetString(), TVI_ROOT, TVI_SORT);
				}
			}
		}
	}

	UpdateDescription();
}


void CAIEntityClassesDialog::UpdateDescription()
{
	HTREEITEM item = m_TreeCtrl.GetSelectedItem();
	if (item)
	{
		CString sAIEntityClass = m_TreeCtrl.GetItemText(item);
		for (size_t i = 0; i < sAIEntityClass.GetLength(); ++i)
		{
			if (isupper(sAIEntityClass[i]))
			{
				sAIEntityClass.Insert(i, ' ');
				i += 2;
			}
		}
		SetDlgItemText(IDC_DESCRIPTION, sAIEntityClass.GetString());
	}
}


void CAIEntityClassesDialog::UpdateAIEntityClassesString()
{
	m_sAIEntityClasses.Empty();
	std::set<CString>::iterator it, itEnd = m_setAIEntityClasses.end();
	for (it = m_setAIEntityClasses.begin(); it != itEnd; ++it)
	{
		if (!m_sAIEntityClasses.IsEmpty())
		{
			m_sAIEntityClasses += ", ";
		}
		m_sAIEntityClasses += *it;
	}
	SetDlgItemText(IDC_SELECTIONLIST, m_sAIEntityClasses);
}


void CAIEntityClassesDialog::ToggleItemState(bool bInvokeSetCheck)
{
	HTREEITEM item = m_TreeCtrl.GetSelectedItem();
	if (item)
	{
		bool bOn = ! (m_TreeCtrl.GetItemState(item, TVIS_BOLD) & TVIS_BOLD);

		m_TreeCtrl.SetItemState(item, bOn ? TVIS_BOLD : 0, TVIS_BOLD);
		
		if (bInvokeSetCheck)
		{
			m_TreeCtrl.SetCheck(item, bOn);
		}
		
		if (bOn)
		{
			m_setAIEntityClasses.insert(m_TreeCtrl.GetItemText(item));
		}
		else
		{
			m_setAIEntityClasses.erase(m_TreeCtrl.GetItemText(item));
		}
		
		UpdateAIEntityClassesString();
	}
}
