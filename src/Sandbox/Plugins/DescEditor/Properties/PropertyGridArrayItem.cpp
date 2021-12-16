#include "pch.h"
#include "PropertyGridArrayItem.h"
#include "ClassProfile.h"

using namespace CryGame;


CPropertyGridArrayItem::CPropertyGridArrayItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, const char* name)
	: CPropertyGridItem(pGrid, pParent, pObject, pProperty, false, name)
{}

void CPropertyGridArrayItem::OnInit()
{
	SetReadOnly();

	string caption, elementCaption;
	elementCaption.reserve(128);

	// Process array elements
	int nCount = m_data.m_pProperty->GetElementCount(m_data.m_pObject);
	for (int i = 0; i < nCount; ++i)
	{
		Value elementValue = m_data.m_pProperty->GetElementAt(m_data.m_pObject, i);

		GetValueCaption(elementValue, elementCaption);

		caption.Format("{%d} %s", i, elementCaption.c_str());
		m_pOwnerGrid->ProcessProperty(this, m_data.m_pObject, m_data.m_pProperty, true, elementValue, caption);
	}
}

void CPropertyGridArrayItem::ToString(string& out)
{
	int nCount = m_data.m_pProperty->GetElementCount(m_data.m_pObject);
	out.Format("{Count = %d}", nCount);
}

void CPropertyGridArrayItem::Reset()
{
	// Reset children
	CXTPPropertyGridItems* pChildren = GetChilds();
	if (pChildren != NULL)
	{
		int iCount = (int)pChildren->GetCount();
		for (int i = 0; i < iCount; ++i)
		{
			CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
			if (pChild != NULL)
				pChild->Reset();
		}
	}
}

void CPropertyGridArrayItem::AddElement()
{
	Value value = Value::CreateDefault(GetPropertyType());
	AddElement(value);
}

void CPropertyGridArrayItem::AddElement(Value& value)
{
	m_data.m_pProperty->AddElement(m_data.m_pObject, value);

	// Get child count
	int iCount = 0;
	CXTPPropertyGridItems* pChildren = GetChilds();
	if (pChildren != NULL)
		iCount = (int)pChildren->GetCount();

	// Add new value to this property
	// NOTE: The caption will be updated in UpdateChildCaptions to the appropriate index
	CPropertyGridItem* pChild = m_pOwnerGrid->ProcessProperty(this, m_data.m_pObject, m_data.m_pProperty, true, value, "{-1}");

	// Initialize new property
	pChild->Init();

	// Update text
	UpdateText();
	UpdateChildCaptions(iCount);

	if (m_pOwnerGrid)
	{
		// Update all IDs
		m_pOwnerGrid->UpdateItemIDs();

		// Move items into groups
		m_pOwnerGrid->PopulateGroups();
	}

	SendEvent(ePGE_OnElementAdded);
}

void CPropertyGridArrayItem::RemoveElement(CPropertyGridItem* pPropertyItem)
{
	if (pPropertyItem == NULL)
		return;  // Invalid property item

	int iIndex = pPropertyItem->GetRelativeIndex();
	m_data.m_pProperty->RemoveElementAt(m_data.m_pObject, iIndex);

	// Remove from tree
	pPropertyItem->Remove();

	// Update text
	UpdateText();
	UpdateChildCaptions(iIndex);

	SendEvent(ePGE_OnElementRemoved);
}

void CPropertyGridArrayItem::UpdateChildCaptions(int nIndex)
{
	CXTPPropertyGridItems* pChildren = GetChilds();
	if (pChildren == NULL)
		return;

	string caption, elementCaption;

	int iCount = (int)pChildren->GetCount();
	for (int i = nIndex; i < iCount; ++i)
	{
		CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
		if (pChild != NULL)
		{
			Value value;
			pChild->GetPropertyValue(value);

			GetValueCaption(value, elementCaption);
			caption.Format("{%d} %s", i, elementCaption.c_str());

			pChild->SetCaption(caption.c_str());
		}
	}
}

bool CPropertyGridArrayItem::BuildMenu(CMenu& menu)
{
	bool bSuccess = false;

	int nType = GetPropertyType();
	if (nType == eVType_Object)  // Class type
	{
		bSuccess |= BuildClassMenu(menu, m_data.m_pObject, "Add [%s]", false /*Do not allow NULL*/);
	}
	else  // Primitive type
	{
		string menuItem;
		GetValueTypeName(nType, menuItem);
		menuItem = string().Format("Add [%s]", menuItem.c_str());
		menu.AppendMenu(MF_ENABLED, MENU_ADD, menuItem);
		bSuccess = true;
	}

	AddMenuSeparator(menu);
	menu.AppendMenu(MF_STRING, MENU_EDIT_DESCRIPTION, "Edit Description");

	if (bSuccess)
	{
		CXTPPropertyGridItems* pChildren = GetChilds();
		if (pChildren != NULL && pChildren->GetCount() > 0)
		{
			// NOTE: All children should be of the same editor type.
			BuildEditorMenu(menu, pChildren->GetAt(0)->GetRuntimeClass());
		}
	}

	return bSuccess;
}

void CPropertyGridArrayItem::HandleMenuSelection(int nSelection)
{
	if (nSelection >= MENU_CLASSES_BEGIN && nSelection <= MENU_CLASSES_END)  // Class type
	{
		// Create the new object if a class was chosen

		nSelection -= MENU_CLASSES_BEGIN;  // Normalize index
		_smart_ptr<CReflectedObject> pNewObject = m_subClasses[nSelection]->CreateObject();
		if (pNewObject == NULL)
		{
			CRY_ASSERT_MESSAGE(FALSE, "Failed to create object!");
			return;
		}

		AddElement(Value(pNewObject));
	}
	else if (nSelection >= MENU_EDITORS_BEGIN && nSelection <= MENU_EDITORS_END)
	{
		// Editor type
		// Change this property's editor type (destroying this instance)

		nSelection -= MENU_EDITORS_BEGIN;  // Normalize index)

		// Grab editor class
		int nPropertyType = GetPropertyType();
		const PropertyEditorDataVector& editorTypes = CClassProfileManager::GetInstance()->GetPropertyEditors(nPropertyType);
		CRY_ASSERT_MESSAGE(editorTypes.size() > nSelection, "Editor type array size mismatch!");
		CRuntimeClass* pEditorClass = editorTypes[nSelection].EditorClass;
		const string& variation = editorTypes[nSelection].Name;

		CXTPPropertyGridItems* pChildren = GetChilds();
		if (pChildren == NULL)
			return;

		// Convert all children to this editor type
		int iCount = (int)pChildren->GetCount();
		for (int i = 0; i < iCount; ++i)
		{
			CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
			if (pChild != NULL)
			{
				// NOTE: This will remove the child editor, add a new one, and situate it at the same location
				//       therefore there is no need to worry about corruption during iteration as the cursor and
				//       count will remain the same.
				pChild->ConvertTo(pEditorClass, variation);
			}
		}
	}
	else if (nSelection == MENU_ADD)  // Primitive type
	{
		AddElement();
	}
	else  // Fall through
	{
		CPropertyGridItem::HandleMenuSelection(nSelection);
	}
}