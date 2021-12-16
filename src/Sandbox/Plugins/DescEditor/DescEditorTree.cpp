////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 2013-3013
// -------------------------------------------------------------------------
//  File Name        : DescEditorTree.cpp
//  Version          : v1.00
//  Created          : 5/20/2013 by Jack Harmon
//  Description      : Custom CXTPReportControl for use with desc objects in the Desc Editor
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "DescEditorTree.h"

#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IGameInterface.h>

using namespace CryGame;

#define COLUMN_DESC_NAME                0
#define DIRTY_DESC_NAME					"Other dirty descs"

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CDescEditorTree, CXTPReportControl)
	ON_WM_RBUTTONUP()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
DROPEFFECT CDescEditorTree::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point, int nState)
{
	m_nOLEDropMode = 1;

	return CXTPReportControl::OnDragOver(pDataObject, dwKeyState, point, nState);
}

//////////////////////////////////////////////////////////////////////////
BOOL CDescEditorTree::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	if (!m_pDescEditor)
	{
		return CXTPReportControl::OnDrop(pDataObject, dropEffect, point);
	}

	CDescEditorTree* pClipboardTree = m_pDescEditor->GetClipboardTree();
	bool draggingFromClipboard = false;
	if (pClipboardTree->m_bInternalDrag)
	{
		draggingFromClipboard = true;
	}

	CXTPReportRow* targetRow = HitTest(point);
	if (targetRow && targetRow->GetControl() != pClipboardTree)
	{
		// Dropped in the m_objectDesc tree
		CDescPropertyInfo* sourceItemData;
		CDescPropertyInfo* targetItemData = GetItemData(targetRow);

		// Determine if we're cloning from clipboard
		if (draggingFromClipboard)
		{
			sourceItemData = GetItemData(pClipboardTree->GetSelectedDescRecord());
			if (sourceItemData && targetItemData)
			{
				if (!sourceItemData->GetElement())
				{
					// We're cloning an object property array
					if (sourceItemData->GetClass())
					{
						if (sourceItemData->GetClass() == targetItemData->GetClass())
						{
							CXTPReportRecords* records = sourceItemData->GetRecord()->GetChilds();
							for (int i = 0; i < records->GetCount(); ++i)
							{
								m_pDescEditor->CloneDescElement(GetItemData(records->GetAt(i)), targetItemData, this);
							}
						}
					}
					else
					{
						// Primitive data type
						m_pDescEditor->CloneDescElement(sourceItemData, targetItemData, this);
					}
				}
				else
				{
					m_pDescEditor->CloneDescElement(sourceItemData, targetItemData, this);
				}
			}

			return CXTPReportControl::OnDrop(pDataObject, DROPEFFECT_NONE, point);
		}

		// We're just moving a record inside the same tree
		sourceItemData = GetItemData(GetSelectedDescRecord());

		if (sourceItemData && targetItemData)
		{
			// Verify we're working the same parent record
			if (sourceItemData->GetParentRecord() == targetItemData->GetParentRecord() || sourceItemData->GetParentRecord() == targetRow->GetRecord())
			{
				CReflectedObject* sourceObject = sourceItemData->GetObject();
				IProperty* sourceProperty = sourceItemData->GetProperty();

				if (sourceObject && sourceProperty)
				{
					// Default the index to 0 in case the drop target was the parent record
					int newIndex = 0;
					int pIndex = sourceItemData->GetElementIndex();

					// If we targeted (between) a record inside the parent record of sourceItemData, get the index
					if (sourceItemData->GetParentRecord() == targetItemData->GetParentRecord())
					{
						newIndex = targetItemData->GetElementIndex();

						// Determine if we're dropping before or after the target row
						if (newIndex > -1 && !m_nOLEDropAbove)
						{
							newIndex++;
						}
					}
					else
					{
						if (m_nOLEDropAbove)
						{
							// User was wanting to drop above the parent record
							newIndex = -1;
						}
					}

					if (pIndex > -1 && newIndex > -1 && pIndex != newIndex)
					{
						// Move the element
						sourceProperty->MoveElement(sourceObject, pIndex, newIndex);
						GetSelectedDescRecord()->GetParentRecord()->GetChilds()->MoveRecord(newIndex, GetSelectedDescRecord(), TRUE);

						// Mark as dirty
						sourceItemData->IncrementDirty();
						if (m_pDescEditor)
						{
							m_pDescEditor->UpdateDirtyStatus(GetSelectedDescRecord(), false);
						}
					}
				}
			}
		}
	}
	else
	{
		// Todo: Allow moving the order of items inside the clipboard

		if (!draggingFromClipboard)
		{
			CloneToClipboard();
		}

		if (this == pClipboardTree)
		{
			dropEffect = DROPEFFECT_NONE;
		}
	}

	return CXTPReportControl::OnDrop(pDataObject, dropEffect, point);
}

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::CloneToClipboard()
{
	// Dropped into an empty clipboard - manually copy since the flags for the object tree are set to move
	CDescPropertyInfo* sourceItemData = new CDescPropertyInfo(*GetItemData(m_pDescEditor->GetObjectTree()->GetSelectedDescRecord()));
	if (sourceItemData)
	{
		IClassRegistry* m_classRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
		_smart_ptr<CReflectedObject> objectClone = m_classRegistry->CloneObject(sourceItemData->GetObject());
		CDescPropertyInfo* targetItemData = new CDescPropertyInfo();

		if (sourceItemData->GetElement())
		{
			_smart_ptr<CReflectedObject> elementClone = m_classRegistry->CloneObject(sourceItemData->GetElement());
			targetItemData->SetElement(elementClone);
		}

		targetItemData->SetObject(objectClone);
		targetItemData->SetProperty(sourceItemData->GetProperty());
		targetItemData->SetClass(sourceItemData->GetClass());
		targetItemData->SetParentDesc(sourceItemData->GetParentDesc());

		// Create a record on the clipboard
		CXTPReportRecord* baseRecord = AddRecord(new CXTPReportRecord());
		CXTPReportRecordItem* item = baseRecord->AddItem(new CXTPReportRecordItem());

		// Set the caption here in case there is no element to pull one from / this is a property array
		item->SetCaption(sourceItemData->GetRecord()->GetItem(COLUMN_DESC_NAME)->GetCaption(0));

		targetItemData->SetRecord(baseRecord);
		baseRecord->GetItem(COLUMN_DESC_NAME)->SetItemData((DWORD_PTR(targetItemData)));

		m_pDescEditor->CloneDescElement(sourceItemData, targetItemData, m_pDescEditor->GetClipboardTree());
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::CleanupAllUserdata()
{
	if (CXTPReportRecords* records = GetRecords())
	{
		for (int i = 0; i < records->GetCount(); ++i)
		{
			CleanupChildrenUserdata(records->GetAt(i));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::CleanupChildrenUserdata(CXTPReportRecord* pRecord)
{
	if (!pRecord)
		return;

	// Ignore items under "Other dirty descs" since they persist
	if (CXTPReportRecordItem* pItem = pRecord->GetItem(COLUMN_DESC_NAME))
		if (pItem->GetCaption(0) == DIRTY_DESC_NAME)
			return;

	for (int i = 0; i < pRecord->GetChilds()->GetCount(); ++i)
	{
		if(CXTPReportRecord* pChildRecord = pRecord->GetChilds()->GetAt(i))
		{
			CleanupChildrenUserdata(pChildRecord);

			if (CXTPReportRecordItem* pItem = pChildRecord->GetItem(COLUMN_DESC_NAME))
			{
				if (CDescPropertyInfo* pInfo = (CDescPropertyInfo*)pItem->GetItemData())
				{
					delete pInfo;
					pItem->SetItemData(NULL);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::UpdateTree()
{
	if (m_pSelectedRecord)
	{
		UpdateRecord(m_pSelectedRecord, TRUE);
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::SetSelectedDescRecord(CXTPReportRecord* pValue)
{
	if (m_pDescEditor)
	{
		// Notify desc editor that the selection has changed,
		// giving it a chance to clean up.
		m_pDescEditor->OnBeforeDescElementSelectionChanged();

		m_pSelectedRecord = pValue;

		// Update the record item
		SetSelectedDescRecordItem(m_pSelectedRecord ? m_pSelectedRecord->GetItem(COLUMN_DESC_NAME) : NULL);

		// Notify the desc editor that the selection has changed
		m_pDescEditor->OnDescElementSelectionChanged(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::OnSelectionChanged()
{
	CXTPReportControl::OnSelectionChanged();

	POSITION pos = GetSelectedRows()->GetFirstSelectedRowPosition();
	if (pos == NULL)
		return;  // Invalid pos

	CXTPReportRow* pRow = GetSelectedRows()->GetNextSelectedRow(pos);
	if (pRow == NULL)
		return;  // Invalid row

	SetSelectedDescRecord(pRow->GetRecord());
};

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_pDescEditor)
	{
		void* pData = NULL;
		POSITION pos = GetSelectedRows()->GetFirstSelectedRowPosition();
		if (pos)
		{
			pData = GetItemData(GetSelectedRows()->GetNextSelectedRow(pos)->GetRecord());
		}

		m_pDescEditor->OnRButtonUpDescElementPanel(this, pData, point);
		return;
	}

	CXTPReportControl::OnRButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
CXTPReportRecord* CDescEditorTree::GetRootRecord()
{
	for (int i = 0; i < GetRecords()->GetCount(); ++i)
	{
		if (GetRecords()->GetAt(i)->IsVisible())
		{
			return GetRecords()->GetAt(i);
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CDescEditorTree::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportControl::GetItemMetrics(pDrawArgs, pItemMetrics);

	// This is used to highlight the selected row when the tree isn't in focus.
	if (pDrawArgs->pRow->IsSelected())
	{
		CXTPReportPaintManager* pPaintManager = pDrawArgs->pControl->GetPaintManager();

		// If the item is dirty, force the text color to red
		if (pDrawArgs->pItem->GetTextColor() == RGB(192, 100, 0))
		{
			pPaintManager->m_clrHighlightText = RGB(192, 0, 0);
		}
		else
		{
			pPaintManager->m_clrHighlightText = RGB(255, 255, 255);
		}

		pItemMetrics->clrForeground = pPaintManager->m_clrHighlightText;
		if (m_pDescEditor && m_pDescEditor->GetActiveTree() != this)
		{
			pItemMetrics->clrBackground = RGB(92, 92, 92);
		}
		else
		{
			pItemMetrics->clrBackground = pPaintManager->m_clrHighlight;
		}

		// Marked to be deleted
		if (!pDrawArgs->pItem->IsFocusable())
		{
			pItemMetrics->clrBackground = RGB(92, 92, 92);
		}
	}
	else if ((pDrawArgs->pRow->GetIndex() % 2) && !pDrawArgs->pItem->IsPreviewItem())
	{
		// Shade every other line slightly more gray as long as it's not selected
		pItemMetrics->clrBackground += RGB(64, 64, 64);
	}

	if (!pDrawArgs->pItem->IsFocusable())
	{
		pItemMetrics->strText = "DELETED (" + pItemMetrics->strText + ")";
	}
}

//////////////////////////////////////////////////////////////////////////
CDescPropertyInfo* CDescEditorTree::GetItemData(CXTPReportRow* pRow)
{
	if (pRow)
	{
		return GetItemData(pRow->GetRecord());
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
CDescPropertyInfo* CDescEditorTree::GetItemData(CXTPReportRecord* pRecord)
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
void CryGame::CDescEditorTree::OnSetFocus(CWnd* pOldWnd)
{
	CXTPReportControl::OnSetFocus(pOldWnd);

	if (m_pDescEditor && m_pDescEditor->GetActiveTree() != this)
	{
		// Set this as the active tree on the desc editor
		m_pDescEditor->SetActiveTree(this);

		SetSelectedDescRecord(NULL);
		OnSelectionChanged();
	}
}
