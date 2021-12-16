#include "pch.h"
#include "PropertyGridVec3Item.h"
#include "ClassProfile.h"

using namespace CryGame;

IMPLEMENT_SERIAL(CPropertyGridVec3fItem, CPropertyGridItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridVec3fItem, Default, eVType_Vec3, "")

IMPLEMENT_SERIAL(CPropertyGridVec3iItem, CPropertyGridItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridVec3iItem, Default, eVType_Vec3i, "")

//////////////////////////////////////////////////////////////////////////////////////////

CPropertyGridVec3fComponentItem::CPropertyGridVec3fComponentItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, const char* name)
	: CPropertyGridFloatItem(pGrid, pParent, NULL, NULL, false, name)
{
}

void CPropertyGridVec3fComponentItem::SetPropertyValue(const Value& value)
{
	Value parentValue;
	GetParent()->GetPropertyValue(parentValue);

	Vec3 v = parentValue;
	v[GetRelativeIndex()] = value;

	GetParent()->SetPropertyValue(v);

	OnPropertyValueChanged();
}

void CPropertyGridVec3fComponentItem::GetPropertyValue(Value& value)
{
	Value parentValue;
	GetParent()->GetPropertyValue(parentValue);

	value = ((Vec3)parentValue)[GetRelativeIndex()];
}

//////////////////////////////////////////////////////////////////////////////////////////

CPropertyGridVec3iComponentItem::CPropertyGridVec3iComponentItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, const char* name)
	: CPropertyGridInt32Item(pGrid, pParent, NULL, NULL, false, name)
{
}

void CPropertyGridVec3iComponentItem::SetPropertyValue(const Value& value)
{
	Value parentValue;
	GetParent()->GetPropertyValue(parentValue);

	Vec3i v = parentValue;
	v[GetRelativeIndex()] = value;

	GetParent()->SetPropertyValue(v);

	OnPropertyValueChanged();
}

void CPropertyGridVec3iComponentItem::GetPropertyValue(Value& value)
{
	Value parentValue;
	GetParent()->GetPropertyValue(parentValue);

	value = ((Vec3i)parentValue)[GetRelativeIndex()];
}