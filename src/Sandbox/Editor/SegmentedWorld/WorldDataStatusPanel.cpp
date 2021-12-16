#include "StdAfx.h"
#include "WorldDataStatusPanel.h"
#include "InternalCommon.h"
#include "GameEngine.h"
#include <ISourceControl.h>

using namespace sw;

//////////////////////////////////////////////////////////////////////////
CWorldDataStatusEntry::CWorldDataStatusEntry(EWDBType eType)
: m_eType(eType)
{
	CreateItems();
	m_filename = Path::AddPathSlash(GetIEditor()->GetGameEngine()->GetLevelPath()) + GetWorldDataFileName(eType);

	UpdateStatus();
}

void CWorldDataStatusEntry::CreateItems()
{
	SetName(GetWorldDataBlockName(m_eType));
	CreateStdItems();
}

uint32 CWorldDataStatusEntry::GetFileSCMAttributes()
{
	if(!CFileUtil::FileExists(m_filename))
		return 0;
	return GetIEditor()->GetSourceControl()->GetFileAttributes(m_filename);
}

//////////////////////////////////////////////////////////////////////////
CWorldDataStatusPanel::CWorldDataStatusPanel(CWnd *pParent)
: CSLDataPanel(CWorldDataStatusPanel::IDD, pParent)
{
	Create(IDD, pParent);
}

BOOL CWorldDataStatusPanel::OnInitDialog()
{
	CSLDataPanel::OnInitDialog();

	UpdateEntries();

	return TRUE;
}

void CWorldDataStatusPanel::UpdateEntries()
{
	m_tree.BeginUpdate();
	m_tree.DeleteAllItems();
	
	for(int i = 0; i < WDB_COUNT; i++)
	{
		m_tree.AddTreeRecord(new CWorldDataStatusEntry((EWDBType)i), 0);
	}

	m_tree.EndUpdate();
	m_tree.Populate();
}