#include "pch.h"
#include "PropertyGridObjectItem.h"

#include "..\Dialogs\SimpleEditPopup.h"

using namespace CryGame;


CPropertyGridObjectItem::CPropertyGridObjectItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
	: CPropertyGridItem(pGrid, pParent, pObject, pProperty, isElement, name)
{}

void CPropertyGridObjectItem::OnInit()
{
	SetReadOnly();
}

void CPropertyGridObjectItem::OnPropertyValueChanged()
{
	// Clear child grid items
	CXTPPropertyGridItems* pChildren = GetChilds();
	if (pChildren != NULL)
		pChildren->Clear();

	Value value;
	GetPropertyValue(value);
	m_pOwnerGrid->ProcessProperty(this, _smart_ptr<CReflectedObject>(value));  // Update the object

	// Initialize new children
	InitChildren();

	CPropertyGridItem::OnPropertyValueChanged();
}

void CPropertyGridObjectItem::ToString(string& out)
{
	Value value;
	GetPropertyValue(value);

	_smart_ptr<CReflectedObject> pObject = value;
	if (pObject != NULL)
	{
		out = pObject->GetClass()->GetName();
	}
}

void CPropertyGridObjectItem::Reset()
{
	if (HasChanged() == false)
	{
		// Same class instance

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
	else
	{
		// Different class instance
		CPropertyGridItem::Reset();
	}
}

bool CPropertyGridObjectItem::BuildMenu(CMenu& menu)
{
	IProperty* pProperty = m_data.m_pProperty;
	if (pProperty == NULL)
		return false;  // Invalid property (e.g. root)

	CReflectedObject* pObject = m_data.m_pObject;

	if (m_data.m_isElement == true)  // Array object element
	{
		int iIndex = GetRelativeIndex();
		CReflectedObject* pValueObject = _smart_ptr<CReflectedObject>(pProperty->GetElementAt(pObject, iIndex));
		if (pValueObject)
		{
			BuildClassMenu(menu, pValueObject, "Change [%s]", false /*Do not allow NULL*/);
		}
	}
	else  // Inline object
	{
		CReflectedObject* pValueObject = _smart_ptr<CReflectedObject>(pProperty->GetValue(pObject));

		string format = (pValueObject != NULL ? "Change [%s]" : "Create [%s]");
		BuildClassMenu(menu, pValueObject, format, pValueObject != NULL);
	}

	bool result = CPropertyGridItem::BuildMenu(menu);

	// Append the Assign Categories menu option to the bottom.
	// Could do this inside the // Inline object above, but doing it here makes the context menu flow better.
	if (!m_data.m_isElement)
	{
		CReflectedObject* pValueObject = _smart_ptr<CReflectedObject>(pProperty->GetValue(pObject));

		AddMenuSeparator(menu);
		BuildClassMenu(menu, pValueObject, "Assign Categories [", false, true);
	}

	return result;
}

void CPropertyGridObjectItem::HandleMenuSelection(int nSelection)
{

	if (nSelection >= MENU_NULL)
	{
		_smart_ptr<CReflectedObject> pNewObject = NULL;
		if (nSelection == MENU_NULL)
		{
			// NULL selected
			SetPropertyValue(pNewObject);  // Update the property grid
		}
		else if (nSelection <= MENU_CLASSES_END) // Class selected
		{
			nSelection -= MENU_CLASSES_BEGIN;  // Normalize index
			pNewObject = m_subClasses[nSelection]->CreateObject();
			if (pNewObject == NULL)
			{
				CRY_ASSERT_MESSAGE(FALSE, "Failed to create object!");
				return;
			}

			SetPropertyValue(pNewObject);  // Update the property grid
		}
		else	
		{
			nSelection -= MENU_CATEGORIES_BEGIN;  // Normalize index
			
			pNewObject = m_subClasses[nSelection]->CreateObject();

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
	}
	else  // Fall through
	{
		CPropertyGridItem::HandleMenuSelection(nSelection);
	}
}