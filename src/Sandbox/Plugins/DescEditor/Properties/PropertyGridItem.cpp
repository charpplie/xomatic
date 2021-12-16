#include "pch.h"
#include "PropertyGridItem.h"
#include "PropertyGridArrayItem.h"

#include "..\Dialogs\SimpleEditPopup.h"

using namespace CryGame;


IMPLEMENT_DYNAMIC(CPropertyGridItem, CXTPPropertyGridItem)


CPropertyGridItem::CPropertyGridItem()
	: CXTPPropertyGridItem("", "")
	, m_pOwnerGrid(NULL)
	, m_iDirtyRefCount(0)
	, m_dirty(false)
	, m_initialized(false)
{}

CPropertyGridItem::CPropertyGridItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name, const char* value)
	: CXTPPropertyGridItem(name, value)
{
	OnCreate(pGrid, pParent, pObject, pProperty, isElement, name, value);
}

CPropertyGridItem::~CPropertyGridItem()
{
	UpdateParentDirtyRefCount(-m_iDirtyRefCount);
}

void CPropertyGridItem::OnCreate(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty,
								 bool isElement, const char* name, const char* value /* = NULL */, const char* variation /* = NULL */)
{
	SetCaption(name);
	SetValue(value);

	m_pOwnerGrid = pGrid;
	m_data = CPropertyGridData(pObject, pProperty, isElement);
	m_iDirtyRefCount = 0;
	m_dirty = false;
	m_initialized = false;
	m_variation = variation;
	m_bIsGroupHeader = false;

	if (pParent != NULL)
	{
		pParent->AddChildItem(this);
	}
}

void CPropertyGridItem::Init()
{
	if (m_initialized == true)
		return;  // Already initialized

	if (m_data.m_pProperty != NULL)
	{
		// Grab all sub-classes including self
		IClass* pClass = m_data.m_pProperty->GetClassType(m_data.m_pObject);
		if (pClass != NULL)
		{
			m_subClasses = pClass->GetSubClasses(true);
		}
	}

	// Only grab the default property if this is not a root (e.g category)
	if (GetRelativeIndex() >= 0)
	{
		// Cache original value for comparison
		GetPropertyValue(m_data.m_origValue);
	}

	OnInit();  // Allow inherited class to Init

	if (m_pParent != NULL && !m_bIsGroupHeader)
	{
		UpdateIcon();
	}

	UpdateText();

	// Set data from profile
	CPropertyProfile* pPropertyProfile = GetBasePropertyProfile(false);
	if (pPropertyProfile)
	{
		SetDescription(pPropertyProfile->GetDescription());
	}

	// Init children
	InitChildren();

	m_initialized = true;
}

void CPropertyGridItem::InitChildren()
{
	CXTPPropertyGridItems* pChildren = GetChilds();
	if (pChildren != NULL)
	{
		int nCount = (int)pChildren->GetCount();
		for (int i = 0; i < nCount; ++i)
		{
			CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
			if (pChild != NULL)
			{
				pChild->Init();
			}
		}

		///> Update children positions from class profile

		if (nCount <= 0)
			return;  // No children to process

		if (m_data.m_pProperty != NULL && m_data.m_pProperty->IsArray() && m_data.m_isElement == false)
			return;  // Ignore array headers

		CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(0));
		CClassProfile* pClassProfile = pChild->GetClassProfile();
		if (pClassProfile)
		{
			int nPropertyCount = pClassProfile->GetPropertyProfileCount();
			for (int i = 0; i < nPropertyCount; ++i)
			{
				CPropertyProfile* pPropertyProfile = pClassProfile->GetPropertyProfile(i);
				int nIndex = pPropertyProfile->GetIndex();
				if (nIndex < 0)
					continue;

				// Adjust child positions in grid according to class profile if applicable
				for (int j = 0; j < nCount;  ++j)
				{
					CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(j));
					if (pChild == NULL)
						continue;  // Invalid child item

					if (pPropertyProfile == pChild->GetPropertyProfile())
					{
						pChild->Move(this, nIndex);
					}
				}
			}
		}
	}
}

void CPropertyGridItem::UpdateIcon()
{
	// Set icon
	CXTPPropertyGridItemMetrics* pCaptionMetrics = GetCaptionMetrics();
	pCaptionMetrics->m_nImage = GetPropertyType();
}

CPropertyGridItem* CPropertyGridItem::GetParent()
{
	return static_cast<CPropertyGridItem*>(m_pParent);
}

void CPropertyGridItem::SetParent(CPropertyGridItem* pParent)
{
	UpdateParentDirtyRefCount(-m_iDirtyRefCount);
	Move(pParent);
	UpdateParentDirtyRefCount(m_iDirtyRefCount);

	if (pParent)
	{
		m_nIndent = pParent->GetIndent();
	}
}

CClassProfile* CPropertyGridItem::GetClassProfile(bool createIfMissing, bool resolveObjects) const
{
	if (m_data.m_pObject == NULL)
		return NULL;

	IClass* pClass = m_data.m_pObject->GetClass();
	if (resolveObjects && GetPropertyType() == eVType_Object)
	{
		// resolveObjects now defaults to false since the following appears unnecessary -jh 09/10/13
		pClass = m_data.m_pProperty->GetClassType(m_data.m_pObject);
	}

	if (pClass == NULL)
		return NULL;

	return CClassProfileManager::GetInstance()->GetClassProfile(pClass->GetName(), createIfMissing);
}

CPropertyProfile* CPropertyGridItem::GetPropertyProfile(bool createIfMissing, bool resolveObjects) const
{
	if (m_data.m_pProperty == NULL)
		return NULL;

	CClassProfile* pClassProfile = GetClassProfile(createIfMissing, resolveObjects);
	if (pClassProfile == NULL)
		return NULL;

	return pClassProfile->GetPropertyProfile(m_data.m_pProperty->GetName());
}

CPropertyProfile* CPropertyGridItem::GetBasePropertyProfile(bool createIfMissing) const
{
	if (m_data.m_pProperty == NULL)
		return NULL;

	CClassProfile* pClassProfile = GetClassProfile(createIfMissing, false);
	if (pClassProfile == NULL)
		return NULL;

	return pClassProfile->GetBasePropertyProfile(m_data.m_pProperty->GetName(), createIfMissing);
}

void CPropertyGridItem::OnValueChanged(CString strValue)
{
	SetValueFromText((LPCSTR)strValue);
}

void CPropertyGridItem::UpdateText()
{
	string str;
	ToString(str);
	SetValue(str.c_str());
}

bool CPropertyGridItem::HasChanged()
{
	Value value;
	GetPropertyValue(value);

	return (value != m_data.m_origValue);
}

void CPropertyGridItem::Reset()
{
	SetPropertyValue(m_data.m_origValue);
}

void CPropertyGridItem::SendEvent(const EPropertyGridEvents& event)
{
	// Send auxiliary notification if applicable
	if (m_pOwnerGrid != NULL)
	{
		m_pOwnerGrid->SendPropertyEvent(event, m_pOwnerGrid, &m_data);
	}
}

void CPropertyGridItem::SetPropertyValue(const Value& value)
{
	if (m_data.m_pObject == NULL || m_data.m_pProperty == NULL)
		return;  // Some property classes do not store properties nor objects

	if (m_data.m_isElement)
	{
		// Handle arrays
		int iIndex = GetRelativeIndex();
		m_data.m_pProperty->SetValueAt(m_data.m_pObject, iIndex, value);
	}
	else
	{
		// Handle base types
		m_data.m_pProperty->SetValue(m_data.m_pObject, value);
	}

	OnPropertyValueChanged();
}

void CPropertyGridItem::OnPropertyValueChanged()
{
	UpdateText();

	// Update dirty flags
	UpdateDirtyStatus();

	// Notify parent if applicable
	if (m_pParent != NULL)
	{
		// NOTE: GetParent casts m_pParent (a xtkp variable) to CPropertyGridItem*
		GetParent()->OnChildValueChanged(this);
	}

	// Notify listeners
	SendEvent(ePGE_OnChanged);
}

void CPropertyGridItem::GetPropertyValue(Value& value)
{
	if (m_data.m_pObject == NULL || m_data.m_pProperty == NULL)
		return;  // Some property classes do not store properties nor objects

	if (m_data.m_isElement == true)
	{
		// Handle arrays
		int iIndex = GetRelativeIndex();
		value = m_data.m_pProperty->GetElementAt(m_data.m_pObject, iIndex);
	}
	else
	{
		// Handle base types
		value = m_data.m_pProperty->GetValue(m_data.m_pObject);
	}
}

int CPropertyGridItem::GetPropertyType() const
{
	if (m_data.m_pObject == NULL || m_data.m_pProperty == NULL)
		return eVType_None;  // Some property classes do not store properties nor objects

	return m_data.m_pProperty->GetType(m_data.m_pObject);
}

int CPropertyGridItem::GetRelativeIndex()
{
	CXTPPropertyGridItem* pParent = GetParentItem();
	if (pParent)
	{
		// Get index in childs array
		CXTPPropertyGridItems* pChildren = pParent->GetChilds();
		if (pChildren != NULL)
		{
			return pChildren->Find(this);
		}
	}

	return -1;
}

int CPropertyGridItem::GetPropertyProfileIndex()
{
	// Find out how many transient group items are in the list above this and subtract them from the index.
	int groups = 0;
	int absoluteIndex = 0;

	CPropertyGridItem* pRoot = static_cast<CPropertyGridItem*>(this->GetParent());
	if (pRoot)
	{
		if (pRoot->IsGroupHeader())
		{
			absoluteIndex = pRoot->GetPropertyProfileIndex();
		}

		CXTPPropertyGridItems* pChildren = pRoot->GetChilds();
		for (int i = 0; i < pChildren->GetCount(); ++i)
		{
			CPropertyGridItem* item = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
			if (item)
			{
				if (item == this)
				{
					return absoluteIndex - groups;
				}

				if (item->IsGroupHeader())
				{
					groups++;

					// Add the children of the group
					absoluteIndex += item->GetChilds()->GetCount();
				}
			}

			// Increment for this child
			absoluteIndex++;
		}
	}

	return -1;
}

void CPropertyGridItem::UpdateDirtyStatus()
{
	// Update dirty status
	bool changed = HasChanged();
	if (changed == true && m_dirty == false)   // value changed from original
	{
		// First time dirty
		m_dirty = true;
		UpdateDirtyRefCount(1);

		// Notify listeners
		SendEvent(ePGE_OnDirty);
	}
	else if (changed == false && m_dirty == true)  // Back to original value
	{
		// First time cleaned
		m_dirty = false;
		UpdateDirtyRefCount(-1);

		// Notify listeners
		SendEvent(ePGE_OnClean);
	}
}

void CPropertyGridItem::UpdateDirtyRefCount(int nDelta)
{
	if (nDelta == 0)
		return;

	int nPrevDirtyRefCount = m_iDirtyRefCount;

	m_iDirtyRefCount += nDelta;
	CRY_ASSERT_MESSAGE(m_iDirtyRefCount >= 0, "Invalid ref count!");

	if (nPrevDirtyRefCount == 0 && m_iDirtyRefCount > 0)
	{
		UpdateDirtyMetrics(true);
	}
	else if (nPrevDirtyRefCount > 0 && m_iDirtyRefCount == 0)
	{
		UpdateDirtyMetrics(false);
	}

	// Notify parent if applicable
	UpdateParentDirtyRefCount(nDelta);
}

void CPropertyGridItem::UpdateParentDirtyRefCount(int nDelta)
{
	// Notify parent if applicable
	if (m_pParent != NULL)
	{
		GetParent()->UpdateDirtyRefCount(nDelta);
	}
}

void CPropertyGridItem::UpdateDirtyMetrics(bool dirty)
{
	CXTPPropertyGridItemMetrics* pCaptionMetrics = GetCaptionMetrics();
	pCaptionMetrics->m_clrFore = (dirty) ? RGB(192, 100, 0) : RGB(192, 192, 192);
}

// TODO: There has to be a better way to handle this.
static CMenu s_editorsMenu;
typedef std::map<string, CMenu*> MenuMap;
static MenuMap s_editorCategoryMenus;

void CPropertyGridItem::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_pOwnerGrid->ClientToScreen(&point);

	// Change the focus so any current edits get applied.
	m_pOwnerGrid->SetFocus();

	CMenu menu;
	menu.CreatePopupMenu();

	// First item in the menu is the current editor type.
	{
		string name = "Default";

		// Use variation name if it exists
		if (m_variation.empty() == false)
		{
			name = m_variation;
		}
		else if (m_data.m_pObject && m_data.m_pProperty)
		{
			// Otherwise, use the property editor name if it exists
			CPropertyProfile* pPropertyProfile = CClassProfileManager::GetInstance()->GetPropertyProfile(m_data.m_pObject->GetClass()->GetName(),
												 m_data.m_pProperty->GetName());
			if (pPropertyProfile)
			{
				const string& editorName = pPropertyProfile->GetEditorClass();
				if (editorName.empty() == false)
				{
					name = editorName;
				}
			}
		}

		CString editorType;
		editorType.Format("[%s]", name.c_str());
		menu.AppendMenu(MF_STRING | MF_DISABLED, MENU_EDITOR_TYPE, editorType);
	}

	if (BuildMenu(menu))
	{
		int ret = CXTPCommandBars::TrackPopupMenu(&menu, TPM_RETURNCMD|TPM_VCENTERALIGN, point.x, point.y, m_pOwnerGrid);
		if (ret > 0)
		{
			ExternalAddRef();
			HandleMenuSelection(ret);
			ExternalRelease();
		}

		// Clean up menus
		s_editorsMenu.DestroyMenu();
		MenuMap::iterator iter;
		for (iter = s_editorCategoryMenus.begin(); iter != s_editorCategoryMenus.end(); ++iter)
		{
			iter->second->DestroyMenu();
			delete iter->second;
		}
		s_editorCategoryMenus.clear();
	}
}

bool CPropertyGridItem::BuildMenu(CMenu& menu)
{
	///> Description
	int nSelectionCount = m_pOwnerGrid->GetSelectedItems(NULL /*count only*/);
	if (nSelectionCount == 1 && m_data.m_isElement == false && m_data.m_pProperty != NULL)
	{
		// Only allow description editing if a single item is selected
		AddMenuSeparator(menu);
		menu.AppendMenu(MF_STRING, MENU_EDIT_DESCRIPTION, "Edit Description");

		///> Editor Types

		BuildEditorMenu(menu, GetRuntimeClass());
	}

	///> Reset
	AddMenuSeparator(menu);
	menu.AppendMenu(MF_STRING, MENU_RESET, "Reset");

	///> Group & Context menu categories
	if (IsGroupHeader() || (GetParent() && GetParent()->IsGroupHeader()))
	{
		menu.AppendMenu(MF_STRING, MENU_RENAMEGROUP, "Rename Group");
		menu.AppendMenu(MF_STRING, MENU_UNGROUP, "Remove Group");
	}
	else if (GetParent())
	{
		menu.AppendMenu(MF_STRING, MENU_GROUP, "Create Group");
	}

	///> Delete
	if (m_data.m_isElement)
	{
		AddMenuSeparator(menu);

		// Array elements can be deleted
		menu.AppendMenu(MF_STRING, MENU_DELETE, "Delete");
	}

	AddMenuSeparator(menu);
	menu.AppendMenu(MF_STRING, MENU_EXPAND_ALL, "Expand All");
	menu.AppendMenu(MF_STRING, MENU_COLLAPSE_ALL, "Collapse All");

	return true;
}

void CPropertyGridItem::BuildEditorMenu(CMenu& menu, CRuntimeClass* pCurrentEditorClass)
{
	int nPropertyType = GetPropertyType();
	if (m_data.m_isElement || nPropertyType == eVType_Object)
		return;  // Ignore array elements and objects

	if (m_data.m_pProperty == NULL)
		return;  // Invalid property

	if (m_data.m_pProperty->IsEnum() || m_data.m_pProperty->IsDesc())
		return;  // Ignore enums and descs

	// Only allow primitive types to have custom editors

	const PropertyEditorDataVector& editorTypes = CClassProfileManager::GetInstance()->GetPropertyEditors(nPropertyType);
	int nEditorCount = editorTypes.size();
	if (nEditorCount > 0)
	{
		bool valid = false;

		s_editorsMenu.CreatePopupMenu();

		static int s_nRange = MENU_EDITORS_END - MENU_EDITORS_BEGIN;
		for (int i = 0; i < nEditorCount && i <= s_nRange; ++i)
		{
			const SPropertyEditorData& data = editorTypes[i];
			if (pCurrentEditorClass == data.EditorClass && data.Name.compareNoCase(m_variation) == 0)
			{
				continue;  // Ignore same class type
			}

			if (data.Category.empty())
			{
				// Append editor type to root editor menu.
				s_editorsMenu.AppendMenu(MF_STRING, MENU_EDITORS_BEGIN + i, data.Name);
			}
			else
			{
				// Create a new sub menu if one doesn't already exist with this name.
				CMenu* pCategoryMenu = NULL;
				MenuMap::iterator iter = s_editorCategoryMenus.find(data.Category);
				if (iter != s_editorCategoryMenus.end())
				{
					pCategoryMenu = iter->second;
				}
				else
				{
					pCategoryMenu = new CMenu();
					s_editorCategoryMenus[data.Category] = pCategoryMenu;
					pCategoryMenu->CreatePopupMenu();
					s_editorsMenu.AppendMenu(MF_STRING | MF_POPUP, (UINT)pCategoryMenu->m_hMenu, data.Category.c_str());
				}

				CRY_ASSERT_MESSAGE(pCategoryMenu, "Invalid sub menu!");

				// Append editor type to category sub menu
				pCategoryMenu->AppendMenu(MF_STRING, MENU_EDITORS_BEGIN + i, data.Name);
			}

			valid = true;
		}

		if (valid)
		{
			AddMenuSeparator(menu);
			menu.AppendMenu(MF_STRING | MF_POPUP, (UINT)s_editorsMenu.m_hMenu, "Editors");
		}
	}
}

bool CPropertyGridItem::BuildClassMenu(CMenu& menu, CReflectedObject* pObject, const char* szFormat, bool showNULL, bool IsCategories)
{
	string tempString;

	if (showNULL == true)
	{
		// Add separator
		AddMenuSeparator(menu);

		// If this object is not null, add "NULL" for the first entry
		tempString.Format(szFormat, "NULL");
		menu.AppendMenu(MF_STRING, MENU_NULL, tempString);
	}

	int iCount = 0;
	if (m_subClasses.empty() == false)
	{
		string title(szFormat);
		title = title.Left(title.find('[')) + "...";

		HMENU subMenu = CreatePopupMenu();
		AppendMenu(menu, MF_POPUP, (UINT)subMenu, title);

		std::map<string, HMENU> categoryMap;
		string category;
		HMENU* pMenu;

		int s_nRange = IsCategories ? MENU_CATEGORIES_END - MENU_CATEGORIES_BEGIN : MENU_CLASSES_END - MENU_CLASSES_BEGIN;
		for (int i = 0; i < m_subClasses.size() && i <= s_nRange; ++i)
		{
			// Dave requested to see self ( CDelayAction )
			// if (pObject != NULL && m_subClasses[i] == pObject->GetClass())
			//  continue;  // Don't list the current class

			// Look for categories to stuff into a submenu
			GetObjectCategory((CReflectedObject*)(m_subClasses[i]->CreateObject()), category);
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

			AppendMenu(*pMenu, MF_STRING, IsCategories ? MENU_CATEGORIES_BEGIN + i : MENU_CLASSES_BEGIN + i, m_subClasses[i]->GetName().c_str());
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

void CPropertyGridItem::AddMenuSeparator(CMenu& menu)
{
	if (menu.GetMenuItemCount() > 0)
		menu.AppendMenu(MF_SEPARATOR);
}

void CPropertyGridItem::HandleMenuSelection(int nSelection)
{
	if (nSelection >= MENU_EDITORS_BEGIN && nSelection <= MENU_EDITORS_END)
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

		ConvertTo(pEditorClass, variation);
	}
	else  // Otherwise...
	{
		switch (nSelection)
		{
		case MENU_DELETE:
			{
				CPropertyGridArrayItem* pArrayParent = static_cast<CPropertyGridArrayItem*>(GetParent());
				CRY_ASSERT_MESSAGE(pArrayParent, "Invalid array parent");
				pArrayParent->RemoveElement(this);
			}
			break;

		case MENU_RESET:
			{
				// Handle multiple selection
				PropertyGridItemList selectedProperties;
				m_pOwnerGrid->GetSelectedProperties(selectedProperties);
				for (size_t i = 0; i < selectedProperties.size(); ++i)
				{
					selectedProperties[i]->Reset();
				}
			}
			break;

		case MENU_EDIT_DESCRIPTION:
			{
				CPropertyProfile* pPropertyProfile = GetBasePropertyProfile(true);
				if (pPropertyProfile)
				{
					string prevDescription = pPropertyProfile->GetDescription();
					string newDescription = prevDescription;

					CSimpleEditPopup dialog;
					dialog.SetDialogTitle("Edit Description");
					string label;
					label.Format("Enter a brief description for %s.", (LPCSTR)GetCaption());
					dialog.SetDialogLabel(label);
					dialog.SetEditString(newDescription);
					dialog.SetEditHeight(50);
					if (dialog.DoModal() == IDOK && newDescription != prevDescription)
					{
						newDescription.Trim();
						pPropertyProfile->SetDescription(newDescription);
						SetDescription(newDescription);
						CClassProfileManager::GetInstance()->Save();

						m_pOwnerGrid->UpdateDescriptionText(this);
					}
				}
			}
			break;

		case MENU_GROUP:
			{
				string name;
				CSimpleEditPopup dialog;
				dialog.SetDialogTitle("Add new Group");
				dialog.SetDialogLabel("Enter group name");
				dialog.SetEditString(name);
				if (dialog.DoModal() == IDOK && !name.empty())
				{
					CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(GetParent()->GetChilds()->FindItem(name));
					if (pItem && pItem->IsGroupHeader())
					{
						MessageBox(NULL, "Group name in use", "Fail", MB_OK);
						HandleMenuSelection(nSelection);
						return;
					}

					PropertyGridItemList selectedProperties;
					m_pOwnerGrid->GetSelectedProperties(selectedProperties);
					if (!selectedProperties.empty())
					{
						// Get the selection before adding a new group since the selection will be lost.
						PropertyGridItemList itemList;
						m_pOwnerGrid->GetSelectedProperties(itemList);

						CPropertyGridItem* pFirstSelection = selectedProperties[0];
						if (pFirstSelection)
						{
							// Grab parent before moving the selection
							CPropertyGridItem* pParent = pFirstSelection->GetParent();
							m_pOwnerGrid->MoveSelectedToGroup(itemList, m_pOwnerGrid->AddGroup(pFirstSelection, name));
							m_pOwnerGrid->SavePropertyProfiles(pParent);
						}
					}
				}
			}
			break;

		case MENU_RENAMEGROUP:
			{
				string name;
				CSimpleEditPopup dialog;
				dialog.SetDialogTitle("Rename Group");
				dialog.SetDialogLabel("Enter group name");
				dialog.SetEditString(name);
				if (dialog.DoModal() == IDOK && !name.empty())
				{
					CPropertyGridItem* pItem = static_cast<CPropertyGridItem*>(GetParent()->GetChilds()->FindItem(name));
					if (pItem && pItem->IsGroupHeader())
					{
						MessageBox(NULL, "Group name in use", "Fail", MB_OK);
						HandleMenuSelection(nSelection);
						return;
					}

					SetCaption(name);

					m_pOwnerGrid->SavePropertyProfiles(this->GetParent());
				}
			}
			break;

		case MENU_UNGROUP:
			{
				if (MessageBox(NULL, "Remove group?", "Confirm", MB_YESNO) == IDYES)
				{
					CPropertyGridItem* pItem;

					if (IsGroupHeader())
					{
						pItem = static_cast<CPropertyGridItem*>(this);
					}
					else
					{
						pItem = static_cast<CPropertyGridItem*>(GetParent());
					}

					if (pItem)
					{
						// Grab parent before removing the group
						CPropertyGridItem* pParent = pItem->GetParent();

						while (pItem->GetChilds()->GetCount())
						{
							CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pItem->GetChilds()->GetAt(0));
							if (pChild)
							{
								pChild->Move(pParent, pItem->GetRelativeIndex());
							}
						}

						pItem->Remove();

						// Write out profile changes
						m_pOwnerGrid->SavePropertyProfiles(pParent);
					}
				}
			}
			break;

		case MENU_EXPAND_ALL:
			{
				m_pOwnerGrid->ExpandOrCollapseAll(this, true);
			}
			break;

		case MENU_COLLAPSE_ALL:
			{
				m_pOwnerGrid->ExpandOrCollapseAll(this, false);
			}
			break;
		}
	}
}

void CPropertyGridItem::Copy(CPropertyGridItem* pOther)
{
	// Dirty status
	m_data.m_origValue = pOther->m_data.m_origValue;
	UpdateDirtyRefCount(pOther->m_iDirtyRefCount);
	m_dirty = pOther->m_dirty;
}

void CPropertyGridItem::ConvertTo(CRuntimeClass* pEditorClass, const char* variation /* = NULL */)
{
	if (pEditorClass == NULL)
		return; // Invalid class

	CPropertyGridItem* pParent = GetParent();
	int nRelativeIndex = GetRelativeIndex();

	// Create replacement editor
	CPropertyGridItem* pReplacementItem = m_pOwnerGrid->CreateProperty(pEditorClass, GetParent(), m_data.m_pObject,
										  m_data.m_pProperty, m_data.m_isElement, GetCaption(), variation);

	if (pReplacementItem != NULL)
	{
		// Inc ref count to ensure this object isn't destroyed.
		ExternalAddRef();

		// Remove this editor before moving to keep count the same.
		Remove();

		// Init and move replacement editor to this editors position
		pReplacementItem->Init();
		pReplacementItem->Copy(this);
		pReplacementItem->Move(pParent, nRelativeIndex);

		// Force text update once index has been established
		pReplacementItem->UpdateText();

		// Update property profile
		CPropertyProfile* pPropertyProfile = pReplacementItem->GetPropertyProfile(true);
		if (pPropertyProfile)
		{
			pPropertyProfile->SetEditorClass(pEditorClass->m_lpszClassName);
			pPropertyProfile->SetEditorClassVariation(variation);
			CClassProfileManager::GetInstance()->Save();
		}

		if (m_pOwnerGrid)
		{
			// Update all IDs
			m_pOwnerGrid->UpdateItemIDs();

			// Move items into groups
			m_pOwnerGrid->PopulateGroups();
		}

		ExternalRelease();
	}
}

void CPropertyGridItem::SaveState(SPropertyGridItemState& state)
{
	state.ID = GetID();
	state.OriginalValue = m_data.m_origValue;
	state.DirtyRefCount = m_iDirtyRefCount;
	state.Expanded = (IsExpanded() == TRUE);
	state.Selected = (IsSelected() == TRUE);
	state.Name = GetCaption();

	state.ChildStates.clear();

	CXTPPropertyGridItems* pChildren = GetChilds();
	if (pChildren != NULL)
	{
		int nCount = (int)pChildren->GetCount();
		state.ChildStates.resize(nCount);

		for (int i = 0; i < nCount; ++i)
		{
			CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
			CRY_ASSERT(pChild);
			if (pChild != NULL)
			{
				pChild->SaveState(state.ChildStates[i]);
			}
		}
	}
}

void CPropertyGridItem::RestoreState(const SPropertyGridItemState& state)
{
	// ID
	SetID(state.ID);

	// Dirty status
	if (state.OriginalValue.GetType() != eVType_None)
	{
		// Restore the original value
		m_data.m_origValue = state.OriginalValue;
	}
	else
	{
		GetPropertyValue(m_data.m_origValue);
	}

	m_iDirtyRefCount = state.DirtyRefCount;
	m_dirty = (m_iDirtyRefCount > 0);
	UpdateDirtyMetrics(m_dirty);

	// Expand/Collapse
	(state.Expanded) ? Expand() : Collapse();

	// Selection
	if (state.Selected)
	{
		Select();
	}

	// Children
	CXTPPropertyGridItems* pChildren = GetChilds();
	if (pChildren != NULL)
	{
		int nCount = (int)pChildren->GetCount();
		CRY_ASSERT_MESSAGE(nCount == state.ChildStates.size(), "Child size mismatch!");

		if (nCount != state.ChildStates.size())
		{
			// Todo: fix so that prop window state is restored in this situation
			return;
		}

		for (int i = 0; i < nCount; ++i)
		{
			CPropertyGridItem* pChild = static_cast<CPropertyGridItem*>(pChildren->GetAt(i));
			CRY_ASSERT(pChild);
			if (pChild != NULL)
			{
				pChild->RestoreState(state.ChildStates[i]);
			}
		}
	}
}

void CPropertyGridItem::OnProfileEvent(uint32 event, CClassProfileManager* _dispatcher, void* data)
{
	if (event == ePType_Class)
	{
		// Class

		CClassProfile* pProfile = reinterpret_cast<CClassProfile*>(data);
		if (pProfile == GetClassProfile())
		{

		}
	}
	else
	{
		// Property

		CPropertyProfile* pProfile = reinterpret_cast<CPropertyProfile*>(data);
		if (pProfile == GetPropertyProfile())
		{

		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

void CPropertyGridItem::GetValueCaption(const Value& value, string& caption)
{
	_smart_ptr<CReflectedObject> obj;
	value.Get(obj);
	GetObjectCaption(obj, caption);
}

void CPropertyGridItem::GetObjectCaption(CReflectedObject* pObject, string& caption)
{
	caption.clear();
	caption.reserve(128);

	if (pObject != NULL)
	{
		// First, see if there's a custom caption.
		if (!pObject->GetCaption(caption))
		{
			IClass* pClass = pObject->GetClass();

			// If there isn't a custom caption, go through all the properties looking for string
			// properties with one of the common names we use for names.
			while (pClass != NULL)
			{
				CPropertyComponent* pProperties = ((CPropertyComponent*)pClass->GetComponent(eCCType_Properties));
				for (int i = 0; i < pProperties->GetPropertyCount(); ++i)
				{
					IProperty* pProperty = pProperties->GetPropertyAt(i);

					if (pProperty->GetType(pObject) == eVType_String &&
							(strcmp(pProperty->GetName(), "Name") == 0 ||
							 strcmp(pProperty->GetName(), "NameID") == 0))
					{
						Value nameStr = pProperty->GetValue(pObject);
						nameStr.Get(caption);
						return;
					}
				}

				pClass = pClass->GetBaseClass();
			}

			// Final fallback, if we still haven't come up with a caption just use the class name.
			caption = pObject->GetClass()->GetName();
		}
	}
}

void CPropertyGridItem::GetObjectCategory(CReflectedObject* pObject, string& category)
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
