////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 2013-3013
// -------------------------------------------------------------------------
//  File Name        : PropertyGrid.cpp
//  Version          : v1.00
//  Created          : 5/8/2013 by John Mena
//  Description      : Custom property grid designed to reflect CReflectedObjects
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "PropertyGrid.h"
#include "PropertyGridItem.h"
#include "PropertyGridArrayItem.h"
#include "PropertyGridEnumItem.h"
#include "PropertyGridNumberItem.h"
#include "PropertyGridStringItem.h"
#include "PropertyGridBoolItem.h"
#include "PropertyGridVec3Item.h"
#include "PropertyGridObjectItem.h"
#include "PropertyGridDescItem.h"
#include "ClassProfile.h"

using namespace CryGame;

BEGIN_MESSAGE_MAP(CPropertyGridView, CXTPPropertyGridView)
	//{{AFX_MSG_MAP(CPropertyGrid)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
// Property Grid
//////////////////////////////////////////////////////////////////////////

CPropertyGrid::CPropertyGrid()
	: m_nFlags(ePFG_ShowAll)
{
	// Set image icons
	m_icons.Create(IDB_VALUE_TYPES, 16, 1, RGB(192, 192, 192));
	GetImageManager()->SetImageList(m_icons, 0);
	m_borderBgColor = RGB(92, 92, 92);
}

BOOL CPropertyGrid::Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwListStyle)
{
	if (CXTPPropertyGrid::Create(rect, pParentWnd, nID, dwListStyle) == FALSE)
		return FALSE;  // Unable to create

	// NOTE: For some reason the line color matches that of the category color; fix that here.
	CXTPPropertyGridItemMetrics* pMetrics = m_pPaintManager->GetItemMetrics();
	pMetrics->m_clrLine.SetCustomValue(RGB(128, 128, 128));
	RedrawControl();

	EnableMultiSelect(TRUE);                // Enable multi-select
	NavigateItems(TRUE, FALSE);         // Enable Tab-based navigation
	ShowHelp(FALSE);                    // By default, don't show help
	SetShowInplaceButtonsAlways(TRUE);  // Always show dropdown, browser, etc. buttons.

	return TRUE;
}

CXTPPropertyGridView* CPropertyGrid::CreateView() const
{
	return new CPropertyGridView();
}

void CPropertyGrid::OnNavigate(XTPPropertyGridUI nUIElement, BOOL bForward, CXTPPropertyGridItem* pItem)
{
	if (nUIElement == xtpGridUIView || nUIElement == xtpGridUIInplaceEdit)
	{
		// Since this is multiselect and the view expects single select, handle the tab movement here.

		int increment = bForward ? +1 : -1;
		int currentIndex = GetGridView().GetCurSel();
		int newIndex = currentIndex + increment;

		if (currentIndex > 0 && newIndex > 0 && newIndex < GetGridView().GetCount())
		{
			CXTPPropertyGridView& rWndView = GetGridView();

			rWndView.SetFocus();

			// Deselect old item then select the new item
			rWndView.SelItemRange(FALSE, 0, GetGridView().GetCount() - 1);
			rWndView.SetSel(newIndex, TRUE);

			rWndView.SetCurSel(newIndex);
			pItem = rWndView.GetItem(newIndex);
		}
	}
	else
	{
		CXTPPropertyGrid::OnNavigate(nUIElement, bForward, pItem);
	}
}

CPropertyGridItem* CPropertyGrid::Populate(CReflectedObject* pObject, IProperty* pProperty)
{
	if (pObject == NULL)
		return NULL;

	ResetContent();  // Remove existing items

	string name;
	CPropertyGridItem* pRoot = NULL;
	bool success = false;

	if (pProperty == NULL)
	{
		name = pObject->GetClass()->GetName();
		pRoot = new CPropertyGridItem(this, NULL, pObject, pProperty, false, name);
		AddCategory(name, pRoot);
		// Populate from an entire object
		ProcessProperty(pRoot, pObject);
	}
	else
	{
		name = pProperty->GetName();
		if (pProperty->IsArray())
		{
			pRoot = new CPropertyGridArrayItem(this, NULL, pObject, pProperty, name);
			AddCategory(name, pRoot);
		}
		else
		{
			pRoot = new CPropertyGridItem(this, NULL, pObject, pProperty, false, name);
			AddCategory(name, pRoot);
			// Populate from property
			ProcessProperty(pRoot, pObject, pProperty);
		}
	}

	// Turn off updates
	pRoot->SetHidden(true);

	pRoot->Init();
	pRoot->Expand();

	m_groups.clear();

	// Update all IDs
	UpdateItemIDs();

	// Move items into groups
	PopulateGroups();

	// Turn on updates
	pRoot->SetHidden(false);

	return pRoot;
}

void CPropertyGrid::Repopulate()
{
	CPropertyGridItem* pRoot = static_cast<CPropertyGridItem*>(GetCategories()->GetAt(0));
	if (pRoot)
	{
		Populate(pRoot->GetObject(), pRoot->GetProperty());
		// Todo: reselect previous selection?
	}
}

void CPropertyGrid::PopulateGroups()
{
	CXTPPropertyGridItem* pProperty = NULL;
	if (GetCategories()->GetCount() > 0)
	{
		pProperty = GetCategories()->GetAt(0);
	}

	if (pProperty)
	{
		CXTPPropertyGridItems* pChildren = pProperty->GetChilds();
		if (pChildren == NULL)
			return;

		for (int i = 0; i < m_groups.size(); ++i)
		{
			CPropertyGridItem* item = m_groups[i];
			if (item && item->GetParent() && !item->GetParent()->IsGroupHeader())
			{
				CPropertyProfile* pProfile = item->GetPropertyProfile(false, false);
				if (pProfile)
				{
					// Handle group creation / movements
					if (!pProfile->GetGroup().empty())
					{
						CPropertyGridItem* pGroup = static_cast<CPropertyGridItem*>(FindGroupItem(item->GetParent()->GetChilds(), pProfile->GetGroup()));
						if (!pGroup)
						{
							// Create group
							pGroup = AddGroup(item, pProfile->GetGroup());
							pGroup->Collapse();
						}

						// Add the item to the group
						int count = pGroup->GetChilds()->GetCount();
						item->Move(pGroup, count);
					}
				}
			}
		}
	}
}

void CPropertyGrid::ProcessProperty(CPropertyGridItem* pParentProperty, CReflectedObject* pObject)
{
	if (pObject == NULL)
		return;

	IClass* pClass = pObject->GetClass();
	if (pClass == NULL)
		return;

	while (pClass != NULL)
	{
		CPropertyComponent* properties = ((CPropertyComponent*)pClass->GetComponent(eCCType_Properties));
		for (int i = 0; i < properties->GetPropertyCount(); ++i)
		{
			IProperty* p = properties->GetPropertyAt(i);

			if (p->IsTransient())
				continue;

			ProcessProperty(pParentProperty, pObject, p);
		}

		// Traverse base class
		pClass = pClass->GetBaseClass();
	}
}

CPropertyGridItem* CPropertyGrid::ProcessProperty(CPropertyGridItem* pParentProperty, CReflectedObject* o, IProperty* p)
{
	if (p->IsArray())
	{
		// Process Array

		// HACK: To get around inline objects having arrays within them.
		bool bArrayHackOverride = false;
		if (pParentProperty)
		{
			CXTPPropertyGridItem* pParentOfParent = pParentProperty->GetParentItem();
			bArrayHackOverride = (pParentOfParent != NULL);  // Not the root
		}

		if (m_nFlags & ePFG_ShowArrays || bArrayHackOverride == true)
		{
			return new CPropertyGridArrayItem(this, pParentProperty, o, p, p->GetName());
		}
	}
	else if (p->IsEnum())
	{
		// Process enumeration
		return new CPropertyGridEnumItem(this, pParentProperty, o, p, false, p->GetName());
	}
	else if (p->IsDesc())
	{
		return new CPropertyGridDescItem(this, pParentProperty, o, p, false, p->GetName());
	}
	else
	{
		// Process types
		return ProcessProperty(pParentProperty, o, p, false, p->GetValue(o), p->GetName());
	}

	return NULL;
}

CPropertyGridItem* CPropertyGrid::ProcessProperty(CPropertyGridItem* pParentProperty, CReflectedObject* o, IProperty* p, bool isElement, Value& value, const char* name)
{
	CRuntimeClass* pEditorClass = NULL;
	string variation;

	// Attempt to create custom editor if applicable
	CPropertyProfile* pPropertyProfile = CClassProfileManager::GetInstance()->GetPropertyProfile(o->GetClass()->GetName(), p->GetName());
	if (pPropertyProfile != NULL)
	{
		if (pPropertyProfile->GetEditorClass().empty() == false)
		{
			pEditorClass = CRuntimeClass::FromName(pPropertyProfile->GetEditorClass());
			if (pEditorClass)
			{
				variation = pPropertyProfile->GetEditorClassVariation();
			}
		}
	}

	if (pEditorClass == NULL)
	{
		// Create default editor.
		int nType = value.GetType();

		// If we're dealing with an object or an enum, process them as such; otherwise, create a default property.
		if ((nType == eVType_Object && m_nFlags & ePGF_ShowObjects) || p->IsEnum())
		{
			CPropertyGridItem* pEditor = NULL;

			// Add entry regardless of whether the class has been instantiated.
			if (p->IsEnum()) pEditor = new CPropertyGridEnumItem(this, pParentProperty, o, p, isElement, name);
			else pEditor = new CPropertyGridObjectItem(this, pParentProperty, o, p, isElement, name);

			CReflectedObject* pChildObject = _smart_ptr<CReflectedObject>(value);
			ProcessProperty(pEditor, pChildObject);
			return pEditor;
		}
		else
		{
			const SPropertyEditorData* const pEditorData = CClassProfileManager::GetInstance()->GetDefaultPropertyEditor(nType);
			if (pEditorData)
			{
				pEditorClass = pEditorData->EditorClass;
				variation = pEditorData->Name;
			}
			else
			{
				CRY_ASSERT_MESSAGE(0, "Unhandled type!");
			}
		}
	}

	return CreateProperty(pEditorClass, pParentProperty, o, p, isElement, name, variation);
}

CPropertyGridItem* CPropertyGrid::CreateProperty(CRuntimeClass* pClass, CPropertyGridItem* pParentProperty, CReflectedObject* o, IProperty* p, bool isElement, const char* name, const char* variation /* = NULL */)
{
	if (pClass == NULL || pClass->IsDerivedFrom(RUNTIME_CLASS(CPropertyGridItem)) == FALSE)
		return NULL;  // Invalid class

	CPropertyGridItem* pEditor = static_cast<CPropertyGridItem*>(pClass->CreateObject());
	if (pEditor)
	{
		pEditor->OnCreate(this, pParentProperty, o, p, isElement, name, "", variation);
	}

	return pEditor;
}

CPropertyGridItem* CPropertyGrid::AddGroup(CPropertyGridItem* pInsertBefore, const char* name)
{
	int insertIndex = pInsertBefore->GetRelativeIndex();
	CPropertyGridGroupItem* pGroup = new CPropertyGridGroupItem(this, pInsertBefore->GetParent(), name);
	pInsertBefore->GetParent()->InsertChildItem(insertIndex, pGroup);

	pGroup->Init();

	return pGroup;
}

void CPropertyGrid::MoveSelectedToGroup(PropertyGridItemList& itemList, CPropertyGridItem* pGroup)
{
	int nCount = itemList.size();
	if (nCount > 0)
	{
		for (int i = 0; i < nCount; ++i)
		{
			if (!itemList[i]->IsGroupHeader())
			{
				itemList[i]->Move(pGroup, i);
			}
		}
	}
}

void CPropertyGrid::SavePropertyProfiles(CXTPPropertyGridItem* pRoot)
{
	if (pRoot)
	{
		SaveChildPropertyProfiles(pRoot->GetChilds());
		CClassProfileManager::GetInstance()->Save();
	}
}

void CPropertyGrid::SaveChildPropertyProfiles(CXTPPropertyGridItems* pChildren)
{
	if (pChildren && pChildren->GetCount() > 0)
	{
		std::vector<CPropertyGridItem*> unusedGroups;
		string groupName;
		CPropertyGridItem* pParent = static_cast<CPropertyGridItem*>(pChildren->GetAt(0)->GetParentItem());
		if (pParent && pParent->IsGroupHeader())
		{
			groupName = pParent->GetCaption();
		}

		for (int i = 0; i < pChildren->GetCount(); ++i)
		{
			CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
			if (pItem)
			{
				if (pItem->IsGroupHeader())
				{
					CXTPPropertyGridItems* pChildren = pItem->GetChilds();

					if (pChildren->IsEmpty())
					{
						unusedGroups.push_back(pItem);
					}
					else
					{
						SaveChildPropertyProfiles(pChildren);
					}
				}
				else
				{
					CPropertyProfile* pPropertyProfile = pItem->GetPropertyProfile(true, false);
					if (pPropertyProfile)
					{
						pPropertyProfile->SetIndex(pItem->GetPropertyProfileIndex());
						pPropertyProfile->SetGroup(groupName);
					}
				}
			}
		}

		while (!unusedGroups.empty())
		{
			unusedGroups.back()->Remove();
			unusedGroups.pop_back();
		}
	}
}

int CPropertyGrid::GetSelectedProperties(PropertyGridItemList& out)
{
	CXTPPropertyGridView* pView = &GetGridView();
	int nCount = pView->GetSelCount();
	
	if (nCount == 0)
		return 0;  // Only a single item selected

	LPINT items = new int [nCount];
	pView->GetSelItems(nCount, items);

	int nIndex = 0;
	for (int i = 0; i < nCount; ++i)
	{
		nIndex = items[i];
		out.push_back(static_cast<CPropertyGridItem*>(this->GetItem(nIndex)));
	}

	delete items;

	return nCount;
}

void CPropertyGrid::OnSelectionChanged(CXTPPropertyGridItem* pItem)
{
	CPropertyGridItem* pProperty = static_cast<CPropertyGridItem*>(pItem);

	if (pProperty != NULL)
	{
		// Grab currently selected properties
		PropertyGridItemList propertyList;
		int nCount = GetSelectedProperties(propertyList);
		if (nCount > 1)
		{
			// More than one item selected.
			for (int i = 0; i < nCount; ++i)
			{
				if (propertyList[i]->GetObject() != pProperty->GetObject() ||
						propertyList[i]->GetIndent() != pProperty->GetIndent() ||
						propertyList[i]->IsElement() == true)
				{
					// The object types do not match, preform a single select on the new item
					pProperty->Select();
					return;
				}
			}
		}
	}

	UpdateDescriptionText(pProperty);

	CXTPPropertyGrid::OnSelectionChanged(pProperty);
}

void CPropertyGrid::UpdateDescriptionText(CPropertyGridItem* pProperty)
{
	if (pProperty == NULL)
		return;  // Invalid property

	CPropertyProfile* pPropertyProfile = pProperty->GetBasePropertyProfile(false);
	if (pPropertyProfile == NULL || pPropertyProfile->GetDescription().empty())
	{
		ShowHelp(FALSE);
	}
	else
	{
		CDC* pDC = GetDC();

		ShowHelp(TRUE);

		// NOTE: Values pulled from CXTPPropertyGridPaintManager::FillPropertyGrid
		static int nWidthPadding = 6 + 6;  // HACK: Adding extra padding until word width calc is sorted.
		static int nHeightPadding = 3;

		CRect rc;
		GetWindowRect(rc);
		int nWindowWidth = rc.Width();
		nWindowWidth -= nWidthPadding;  // Account for borders and the like

		CXTPPropertyGridItemMetrics* pMetrics = m_pPaintManager->GetItemMetrics();
		HGDIOBJ prevObject = pDC->SelectObject(pMetrics->m_fontNormal);

		// Get space size
		const CSize spaceSize = pDC->GetTextExtent(CString(" "));

		// Cache description
		const string& description = pPropertyProfile->GetDescription();

		// Iterate over each linebreak
		bool firstWord = true;
		int nLineCount = 0;
		int lineIdx = 0;
		string lineToken;
		while ((lineToken = description.Tokenize("\r\n", lineIdx)).empty() == false)
		{
			++nLineCount;

			// Iterate over each word, determining their size.
			int nLineWidth = 0;
			int wordIdx = 0;
			string wordToken;
			while ((wordToken = lineToken.Tokenize(" ", wordIdx)).empty() == false)
			{
				// Account for space if this isn't the first word
				if (firstWord == false)
				{
					if (nLineWidth + spaceSize.cx <= nWindowWidth)
					{
						nLineWidth += spaceSize.cx;
					}
					else
					{
						++nLineCount;
						nLineWidth = spaceSize.cx;
					}
				}
				else
				{
					firstWord = false;
				}

				// Get width of word
				int nWordWidth = pDC->GetTextExtent(CString(wordToken)).cx;
				if (nLineWidth + nWordWidth <= nWindowWidth)
				{
					nLineWidth += nWordWidth;
				}
				else
				{
					++nLineCount;
					nLineWidth = nWordWidth;
				}
			}
		}

		pDC->SelectObject(prevObject);

		int nHeight = nLineCount;
		nHeight += 2;  // Account for bold header and blank line prior to our description
		nHeight *= spaceSize.cy;
		nHeight += nHeightPadding;

		SetHelpHeight(nHeight);
	}
}

void CPropertyGrid::UpdateItemIDs(CXTPPropertyGridItem* pProperty /* = NULL */, int nID /* = 0 */)
{
	if (pProperty == NULL && nID == 0)
	{
		if (GetCategories()->GetCount() > 0)
		{
			pProperty = GetCategories()->GetAt(0);
		}
	}

	if (pProperty == NULL)
		return;  // Invalid parent

	pProperty->SetID(nID);

	CXTPPropertyGridItems* pChildren = pProperty->GetChilds();
	if (pChildren == NULL)
		return;

	for (int nItem = 0; nItem < pChildren->GetCount(); nItem++)
	{
		CXTPPropertyGridItem* pItem = pChildren->GetAt(nItem);
		UpdateItemIDs(pItem, ++nID);

		// Store unhandled group members.
		CPropertyGridItem* item = static_cast<CPropertyGridItem*>(pItem);
		if (item && item->GetParent() && !item->GetParent()->IsGroupHeader())
		{
			CPropertyProfile* pProfile = item->GetPropertyProfile(false, false);
			if (pProfile)
			{
				if (!pProfile->GetGroup().empty())
				{
					m_groups.push_back(item);
				}
			}
		}
	}
}

void CPropertyGrid::SaveState(SPropertyGridState& state)
{
	state.TopIndex = m_pView->GetTopIndex();
	state.Count = m_pView->GetCount();

	if (state.Count > 0)
	{
		static_cast<CPropertyGridItem*>(m_pView->GetItem(0))->SaveState(state.RootState);
	}
}

void CPropertyGrid::RestoreState(const SPropertyGridState& state)
{
	if (state.Count <= 0)
		return;

	static_cast<CPropertyGridItem*>(m_pView->GetItem(0))->RestoreState(state.RootState);
	m_pView->SetTopIndex(state.TopIndex);
}

void CPropertyGrid::ExpandOrCollapseAll(CXTPPropertyGridItem* pItem, bool expand)
{
	if (pItem)
	{
		// Hide the item and redraw only after all elements are addressed
		pItem->SetHidden(TRUE);
		expand ? pItem->Expand() : pItem->Collapse();

		CXTPPropertyGridItems* pChildren = pItem->GetChilds();
		if (pChildren)
		{
			for (int i = 0; i < pChildren->GetCount(); ++i)
			{
				ExpandOrCollapseAll(pChildren->GetAt(i), expand);
			}
		}

		pItem->SetHidden(FALSE);
	}
}

void CPropertyGrid::Reset(CXTPPropertyGridItem* pItem)
{
	if (!pItem)
	{
		pItem = GetItem(0);
	}

	if (pItem)
	{
		CPropertyGridItem* pItemCast = static_cast<CPropertyGridItem*>(pItem);
		if (pItemCast)
		{
			pItemCast->Reset();

			CXTPPropertyGridItems* pChildren = pItem->GetChilds();
			if (pChildren)
			{
				for (int i = 0; i < pChildren->GetCount(); ++i)
				{
					Reset(pChildren->GetAt(i));
				}
			}
		}
	}
}

CXTPPropertyGridItem* CPropertyGrid::FindGroupItem(const CXTPPropertyGridItems* pItemList, const char* pCaption) const
{
	for (int i = 0; i < pItemList->GetCount(); i++)
	{
		CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(pItemList->GetAt(i));
		if (pItem->IsGroupHeader() && pItem->GetCaption().Compare(pCaption) == 0)
			return pItem;

		CXTPPropertyGridItem* pChild = FindGroupItem(pItem->GetChilds(), pCaption);
		if (pChild)
			return pChild;
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
// Property Grid View
//////////////////////////////////////////////////////////////////////////

#define TID_SCROLLDOWN  100
#define TID_SCROLLUP    101

CPropertyGridView::CPropertyGridView()
	: m_nDragToIndex(-1)
	, m_nDrawLineIndex(-1)
	, m_nScrollInterval(0)
{}

void CPropertyGridView::OnLButtonDown(UINT nFlags, CPoint point)
{
	//clear all the flags
	m_nDragToIndex = -1;
	m_nScrollInterval = 0;
	m_dragging = false;

	// If holding ctrl + shift, enter dragging mode
	// NOTE: ctrl or shift used exclusively are for multiple selection
	if (nFlags & MK_CONTROL && nFlags & MK_SHIFT)
	{
		CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(ItemFromPoint(point));
		if (pItem && pItem->IsElement() == false)
		{
			// Cache the currently selected item as the destination index immediately
			// to allow quick ctrl+click moves without actually needing to drag.
			m_nDragToIndex = pItem->GetRelativeIndex();

			// Grab selected items
			m_draggingItems.clear();
			((CPropertyGrid*)m_pGrid)->GetSelectedProperties(m_draggingItems);

			if (m_draggingItems.empty() == false)
			{
				for (size_t i = 0; i < m_draggingItems.size(); ++i)
				{
					SetSel(m_draggingItems[i]->GetIndex());
				}
			}
			else  // No items selected so select the one under the mouse and assume it is meant to be moved.
			{
				SetSel(m_nDragToIndex);
				m_draggingItems.push_back(pItem);
			}

			m_dragging = true;
		}
	}

	if (m_dragging == false)
	{
		CXTPPropertyGridView::OnLButtonDown(nFlags, point);
	}
}

void CPropertyGridView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_dragging == true)
	{
		m_dragging = false;

		KillTimer(TID_SCROLLDOWN);
		KillTimer(TID_SCROLLUP);
		m_nScrollInterval = 0;

		CRect Rect;
		GetClientRect(&Rect);
		//if they are still within the listbox window
		if (Rect.PtInRect(point))
		{
			// Grab shared parent
			CPropertyGridItem* pParent = m_draggingItems[0]->GetParent();

			// Determine the group if any that the dragged items are in.
			CPropertyGridItem* pDragGroup = NULL;
			if (m_draggingItems[0]->IsGroupHeader())
			{
				pDragGroup = m_draggingItems[0];
			}
			else if (pParent->IsGroupHeader())
			{
				pDragGroup = pParent;
			}

			// NOTE: Treat the count as the max index so items are placed at the end
			int nMaxIndex = pParent->GetChilds()->GetCount();
			int nRelDragToIndex = m_nDragToIndex;

			int nNewIndex = 0;
			if (nRelDragToIndex > nMaxIndex || m_nDragToIndex < 0 /*off bottom of client rect*/)
			{
				// Index below lower bound
				nNewIndex = nMaxIndex;
			}
			else if (nRelDragToIndex < 0)
			{
				// Index above upper bound
				nNewIndex = 0;
			}
			else
			{
				// Index in range
				nNewIndex = nRelDragToIndex;
			}

			// Make sure parents are the same
			bool bValidIndex = true;
			bool bChangingParents = false;
			CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(ItemFromPoint(point));
			if (pItem)
			{
				if (pItem->GetParent() == m_draggingItems[0])
				{
					// Dropped group header inside itself.
					bValidIndex = false;
				}
				else if (pItem->GetParent() != pParent && pItem != pParent)
				{
					// Parents aren't the same
					if (pItem->GetParent()->IsGroupHeader() && pItem->GetParent()->GetParent() == pParent)
					{
						// Is valid - moving an item into the group
						pParent = pItem->GetParent();
						bChangingParents = true;
					}
					else if (pParent->IsGroupHeader() && pItem->GetParent() == pParent->GetParent())
					{
						// Is valid - moving an item from a group to group's parent
						pParent = pItem->GetParent();
						bChangingParents = true;
					}
					else if (pParent->IsGroupHeader() && (pItem->IsGroupHeader() || pItem->GetParent()->IsGroupHeader()))
					{
						// Group to group under same parent

						CPropertyGridItem* pParentTarget;
						(pItem->IsGroupHeader()) ? pParentTarget = pItem : pParentTarget = pItem->GetParent();

						if (pParent->GetParent() == pParentTarget->GetParent())
						{
							pParent = pParentTarget;
							bChangingParents = true;
						}
						else
						{
							bValidIndex = false;
						}
					}
					else
					{
						bValidIndex = false;
					}
				}
				else if (pItem->GetChilds() && pItem->IsExpanded() && pItem != pParent)
				{
					// Target is a container and is expanded
					bValidIndex = false;
				}
			}
			else
			{
				// Default drop to last index
				pItem = static_cast<CPropertyGridItem*>(pParent->GetChilds()->GetAt(nMaxIndex - 1));
			}

			// Determine the group if any that the dragged items are dropped into
			CPropertyGridItem* pDropGroup = NULL;
			if (pItem->IsGroupHeader())
			{
				pDropGroup = pItem;
			}
			else if (pItem->GetParent()->IsGroupHeader())
			{
				pDropGroup = pItem->GetParent();
			}

			if (bValidIndex && !bChangingParents)
			{
				// Ensure we haven't chosen an index in our selection group.
				for (int i = 0; i < m_draggingItems.size(); ++i)
				{
					int nItemIndex = m_draggingItems[i]->GetRelativeIndex();
					if (nItemIndex == nRelDragToIndex)
					{
						bValidIndex = false;
						break;
					}
				}
			}

			if (bValidIndex)
			{
				std::vector<int> newIndexes(m_draggingItems.size());

				int nItemIndex = m_draggingItems[0]->GetRelativeIndex();

				if (bChangingParents && !pItem->GetParent()->IsGroupHeader())
				{
					// Moving from inside group to group parent - set nItemIndex to group index
					nItemIndex = m_draggingItems[0]->GetParent()->GetRelativeIndex() - 1;
					nNewIndex = pItem->GetRelativeIndex();
				}

				int nOriginalIndex = __max(0, nItemIndex);
				if (nNewIndex > nOriginalIndex)
				{
					///> Moved Down
					if (!bChangingParents)
					{
						nNewIndex--;
					}

					// Move all selected items
					for (int i = 0; i < m_draggingItems.size(); ++i)
					{
						m_draggingItems[i]->Move(pParent, nNewIndex);
						newIndexes[i] = (nNewIndex + i);
					}
				}
				else
				{
					///> Moved Up
					// Reverse move order to retain original order
					int nEndIndex = (m_draggingItems.size() - 1);
					for (int i = nEndIndex; i >= 0; --i)
					{
						m_draggingItems[i]->Move(pParent, nNewIndex);
						newIndexes[i] = (nNewIndex + (i - nEndIndex));
					}
				}

				// Deselect all items
				SelItemRange(FALSE, 0, GetCount() - 1);

				// Select moved items
				for (size_t i = 0; i < m_draggingItems.size(); ++i)
				{
					SetSel(m_draggingItems[i]->GetIndex());
				}

				// Write out profile changes
				if (pItem->GetParent()->IsGroupHeader())
				{
					((CPropertyGrid*)m_pGrid)->SavePropertyProfiles(pItem->GetParent()->GetParent());
				}
				else
				{
					((CPropertyGrid*)m_pGrid)->SavePropertyProfiles(pItem->GetParent());
				}
			}
		}

		Invalidate();
		UpdateWindow();

		m_nDragToIndex = -1;
	}

	CXTPPropertyGridView::OnLButtonUp(nFlags, point);
}

void CPropertyGridView::OnMouseMove(UINT nFlags, CPoint point)
{
	CXTPPropertyGridView::OnMouseMove(nFlags, point);

	if (nFlags & MK_LBUTTON && m_dragging == true)
	{
		// Shift the selection when passing the halfway point of an item.
		point.y += m_pGrid->GetItem(0)->GetItemRect().Height() / 2;

		CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(ItemFromPoint(point));
		int Index = -1;
		bool Invalid = true;

		// Make m_nDragToIndex invalid / off the bottom of the list so it will default to move items there.
		m_nDragToIndex = 1000;

		//if they our not on a particular item
		if (pItem == NULL)
		{
			CRect ClientRect;
			GetClientRect(&ClientRect);

			//if they are still within the listbox window, then
			//simply select the last item as the drop point
			//else if they are outside the window then scroll the items
			if (ClientRect.PtInRect(point))
			{
				KillTimer(TID_SCROLLDOWN);
				KillTimer(TID_SCROLLUP);
				m_nScrollInterval = 0;
				Index = -1;
				Invalid = false;
			}
			else
			{
				HandleScrolling(point, ClientRect);
			}
		}
		else
		{
			Index = pItem->GetIndex();

			m_nDragToIndex = pItem->GetRelativeIndex();

			Invalid = false;

			KillTimer(TID_SCROLLDOWN);
			KillTimer(TID_SCROLLUP);
			m_nScrollInterval = 0;
		}

		if (Index != m_nDrawLineIndex && !Invalid)
		{
			UpdateLines(Index);
		}
	}
}

void CPropertyGridView::HandleScrolling(CPoint Point, CRect ClientRect)
{
	if (Point.y > ClientRect.Height())
	{
		DWORD Interval = 250 / (1 + ((Point.y - ClientRect.Height()) / GetItemHeight(0)));
		if (Interval != m_nScrollInterval)
		{
			m_nScrollInterval = Interval;
			SetTimer(TID_SCROLLDOWN, Interval, NULL);
			OnTimer(TID_SCROLLDOWN);
		}
	}
	else if (Point.y < 0)
	{
		DWORD Interval = 250 / (1 + (abs(Point.y) / GetItemHeight(1)));
		if (Interval != m_nScrollInterval)
		{
			m_nScrollInterval = Interval;
			SetTimer(TID_SCROLLUP, Interval, NULL);
			OnTimer(TID_SCROLLUP);
		}
	}
	else
	{
		KillTimer(TID_SCROLLDOWN);
		KillTimer(TID_SCROLLUP);
		m_nScrollInterval = 0;
	}
}

void CPropertyGridView::UpdateLines(int nIndex)
{
	if (m_nDrawLineIndex != nIndex)
	{
		// Clear previous line
		DrawLine(m_nDrawLineIndex, RGB(128, 128, 128));
	}

	m_nDrawLineIndex = nIndex;

	if (nIndex != -1)
	{
		DrawLine(m_nDrawLineIndex, RGB(192, 100, 0));
	}
}

void CPropertyGridView::DrawLine(int nIndex, COLORREF color)
{
	CRect ClientRect;
	GetClientRect(&ClientRect);

	CDC* pDC = GetDC();
	CRect Rect;

	CPen Pen(PS_SOLID, 1, color);
	CPen* pOldPen = pDC->SelectObject(&Pen);

	if (nIndex != -1)
	{
		// Draw at desired position
		GetItemRect(m_nDrawLineIndex, &Rect);
		if (ClientRect.PtInRect(Rect.TopLeft()))
		{
			pDC->MoveTo(Rect.left, Rect.top - 1);
			pDC->LineTo(Rect.right, Rect.top - 1);
		}
	}
	else
	{
		// Draw at bottom
		GetItemRect(GetCount() - 1, &Rect);
		if (ClientRect.PtInRect(CPoint(0, Rect.bottom)))
		{
			pDC->MoveTo(Rect.left, Rect.bottom);
			pDC->LineTo(Rect.right, Rect.bottom);
		}
	}

	pDC->SelectObject(pOldPen);
	ReleaseDC(pDC);
}

void CPropertyGridView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TID_SCROLLDOWN)
	{
		UpdateLines(m_nDrawLineIndex + 1);
		SetTopIndex(GetTopIndex() + 1);
	}
	else if (nIDEvent == TID_SCROLLUP)
	{
		UpdateLines(m_nDrawLineIndex - 1);
		SetTopIndex(GetTopIndex() - 1);
	}

	CXTPPropertyGridView::OnTimer(nIDEvent);
}


