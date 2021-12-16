
#include "StdAfx.h"
#include "ControlMRU.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_XTP_CONTROL(CControlMRU, CXTPControlRecentFileList)

//////////////////////////////////////////////////////////////////////////
bool CControlMRU::DoesFileExist(CString& sFileName)
{
	return (_access(sFileName.GetBuffer(), 0) == 0);
}

//////////////////////////////////////////////////////////////////////////
void CControlMRU::OnCalcDynamicSize(DWORD dwMode)
{
	CRecentFileList* pRecentFileList = GetRecentFileList();

	if (!pRecentFileList)
		return;

	CString* pArrNames = pRecentFileList->m_arrNames;

	ASSERT(pArrNames!= NULL);
	if (!pArrNames)
		return;

	while (m_nIndex + 1 < m_pControls->GetCount())
	{
		CXTPControl* pControl = m_pControls->GetAt(m_nIndex + 1); ASSERT(pControl);
		if (pControl->GetID() >= GetFirstMruID() && pControl->GetID() <= GetFirstMruID() + pRecentFileList->m_nSize)
		{
			m_pControls->Remove(pControl);
		}
		else break;
	}

	if (m_pParent->IsCustomizeMode())
	{
		m_dwHideFlags = 0;
		SetEnabled(TRUE);
		return;
	}

	if (pArrNames[0].IsEmpty())
	{
		SetCaption("No Recent Files");
		SetDescription("No recently opened files");
		m_dwHideFlags = 0;
		SetEnabled(FALSE);

		return;
	}
	else
	{
		SetCaption("Recent File");
		SetDescription("Open this document");
	}

	m_dwHideFlags |= xtpHideGeneric;

	CString sCurDir = Path::GetExecutableParentDirectory() / Path::GetGameFolder() / "";
	int nCurDir = sCurDir.GetLength();

	CString strName;
	CString strTemp;
	int iLastValidMRU = 0;
	for (int iMRU = 0; iMRU < pRecentFileList->m_nSize; iMRU++)
	{
		if (!pRecentFileList->GetDisplayName(strName, iMRU, sCurDir.GetBuffer(), nCurDir))
			break;

		if (DoesFileExist(pArrNames[iMRU]))
		{
			CString sCurEntryDir = pArrNames[iMRU].Left(nCurDir);
			if (sCurEntryDir.CompareNoCase(sCurDir) != 0)
			{
				//unavailable entry (wrong directory)
				continue;
			}
		}
		else
		{
			//invalid entry (not existing)
			continue;
		}

		int nId = iMRU + GetFirstMruID();

		//CXTPControlMRUEntry* pControl = (CXTPControlMRUEntry*)CXTPControlMRUEntry::CreateObject();
		//pControl = (CXTPControlMRUEntry*)m_pControls->Add((CXTPControl*)pControl, nId, _T(""), m_nIndex + iMRU + 1, TRUE);
		//ASSERT(pControl);
		//pControl->Initialize();
		CXTPControl* pControl = m_pControls->Add(xtpControlButton, nId, _T(""), m_nIndex + iLastValidMRU + 1, TRUE);
		ASSERT(pControl);

		pControl->SetCaption(CXTPControlWindowList::ConstructCaption(strName, iLastValidMRU + 1));
		pControl->SetFlags(xtpFlagManualUpdate);
		pControl->SetBeginGroup(iLastValidMRU == 0 && m_nIndex != 0);
		pControl->SetParameter(pArrNames[iMRU]);

		CString sDescription = "Open file:  " + pArrNames[iMRU];
		pControl->SetDescription(sDescription);

		if ((GetFlags() & xtpFlagWrapRow) && iMRU == 0)
			pControl->SetFlags(pControl->GetFlags() | xtpFlagWrapRow);

		++iLastValidMRU;
	}

	//if no entry was valid, treat as none would exist
	if (iLastValidMRU == 0)
	{
		SetCaption("No Recent Files");
		SetDescription("No recently opened files");
		m_dwHideFlags = 0;
		SetEnabled(FALSE);
	}
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//IMPLEMENT_XTP_CONTROL(CControlMRUEntry, CXTPControlPopup)
//
////////////////////////////////////////////////////////////////////////////
//void CControlMRUEntry::Initialize()
//{
//	//set control type, to mimic visual appearance of button
//	m_controlType = xtpControlButton;
//	m_bTmpDisabled = false;
//}

////////////////////////////////////////////////////////////////////////////
//void CControlMRUEntry::OnClick(BOOL bKeyboard, CPoint pt)
//{
//	//force button behavior and execute on click
//	CXTPControlButton::OnClick(true, pt);
//}
//
////////////////////////////////////////////////////////////////////////////
//void CControlMRUEntry::OnMouseHover()
//{
//	//create "remove" submenu (if not yet exists) and pop it up
//	if (m_bPopup)
//		return;
//
//	if (!m_pCommandBar)
//		m_pCommandBar = CXTPPopupBar::CreatePopupBar(m_pParent ? m_pParent->GetCommandBars() : NULL);
//
//	CXTPControls* pControls = m_pCommandBar->GetControls();
//	assert(pControls);
//
//	if (pControls->GetCount()==0)
//	{
//		CXTPControl* pControl = pControls->Add(xtpControlButton, ID_FILE_MRU_REMOVE_FIRST + (GetID()-ID_FILE_MRU_FIRST), _T(""), -1, TRUE);
//		ASSERT(pControl);
//
//		pControl->SetDescription("Remove this entry");
//		pControl->SetCaption("Remove");
//	}
//
//	//temporarily enable disabled items, but also store this info for re-disabling later on (in OnSetPopup)
//	//this ensures that OnSetPopup handling is called
//	if (!m_bEnabled)
//	{
//		m_bTmpDisabled = true;
//		m_bEnabled = true;
//	}
//
//	GetParent()->SetPopuped(m_nIndex);
//}
//
////////////////////////////////////////////////////////////////////////////
//BOOL CControlMRUEntry::OnSetPopup(BOOL bPopup)
//{
//	//set appearance according to popup state
//	if (bPopup)
//		m_controlType = xtpControlButtonPopup;
//	else
//		m_controlType = xtpControlButton;
//
//	//re-disable if adequate
//	if (m_bTmpDisabled)
//		m_bEnabled = false;
//
//	return CXTPControlPopup::OnSetPopup(bPopup);
//}
