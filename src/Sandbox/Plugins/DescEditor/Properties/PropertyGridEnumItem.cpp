#include "pch.h"
#include "PropertyGridEnumItem.h"

using namespace CryGame;


CPropertyGridEnumItem::CPropertyGridEnumItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
	: CPropertyGridItem(pGrid, pParent, pObject, pProperty, isElement, name)
{
	string enumName = pProperty->GetEnumName();

	bool allPrefixedWithEnum = true;

	// Check if all the enum value names are prefixed with the enum name
	int nSize = m_data.m_pProperty->GetEnumSize();
	for (int i = 0; i < nSize; ++i)
	{
		int key = m_data.m_pProperty->GetEnumValueFromIndex(i);
		string value = m_data.m_pProperty->GetEnumAsStringFromIndex(i);
		value.resize(enumName.size());
		if (value.compareNoCase(enumName) != 0)
		{
			allPrefixedWithEnum = false;
			break;
		}
	}

	// If all are prefixed with the enum name, we strip that off
	if (allPrefixedWithEnum)
	{
		m_CharsToTrim = enumName.length();
	}
	// Otherwise we just strip the first character (we assume it starts with 'e')
	else
	{
		m_CharsToTrim = 1;
	}
}

void CPropertyGridEnumItem::OnInit()
{
	// Convert to combo box
	AddComboButton();
	SetFlags(xtpGridItemHasComboButton);
	SetConstraintEdit();

	// Add combo items
	int nSize = m_data.m_pProperty->GetEnumSize();
	for (int i = 0; i < nSize; ++i)
	{
		int key = m_data.m_pProperty->GetEnumValueFromIndex(i);
		string value = GetEnumString(m_data.m_pProperty, i);
		GetConstraints()->AddConstraint(value, key);
	}
}

void CPropertyGridEnumItem::SetValueFromText(const string& text)
{
	// Grab the constraint index that was chosen
	int nIndex = GetConstraints()->GetCurrent();
	// Grab the user data which is the enum value
	int32 nValue = GetConstraints()->GetConstraintAt(nIndex)->m_dwData;

	// Apply the value to the property on the object.  Signed/Unsigned must be accounted for
	// to ensure proper casting during assignment.
	SetPropertyValue((GetPropertyType() == eVType_Int) ? nValue : (uint32)nValue);
}

void CPropertyGridEnumItem::ToString(string& out)
{
	CXTPPropertyGridItemConstraints* pConstraints = GetConstraints();
	if (pConstraints == NULL || pConstraints->GetCount() == 0)
		return;  // Not yet initialized

	Value value;
	GetPropertyValue(value);

	int32 nValue = (GetPropertyType() == eVType_Int) ? value : (int32)(uint32)value;
	int nIndex = m_data.m_pProperty->GetEnumIndexFromKey(nValue);

	if (nIndex >= 0)
	{
		pConstraints->SetCurrent(nIndex);
		out = GetEnumString(m_data.m_pProperty, nIndex);
	}
}

string CPropertyGridEnumItem::GetEnumString(IProperty* pProperty, int index)
{
	// Trim the enum name (or 'e') off the front of the string.
	string ret = pProperty->GetEnumAsStringFromIndex(index).substr(m_CharsToTrim);

	// Trim any underscore off the front
	ret.TrimLeft('_');

	ExpandCamelCase(ret);

	return ret;
}
