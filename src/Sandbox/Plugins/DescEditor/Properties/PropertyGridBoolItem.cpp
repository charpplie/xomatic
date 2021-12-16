#include "pch.h"
#include "PropertyGridBoolItem.h"
#include "ClassProfile.h"

using namespace CryGame;


IMPLEMENT_SERIAL(CPropertyGridBoolItem, CPropertyGridItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridBoolItem, Default, eVType_Bool, "")


CPropertyGridBoolItem::CPropertyGridBoolItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
	: CPropertyGridItem(pGrid, pParent, pObject, pProperty, isElement, name)
{}

void CPropertyGridBoolItem::OnInit()
{
	// TODO: Checkbox
	// Convert to combo box
	AddComboButton();
	SetFlags(xtpGridItemHasComboButton);
	SetConstraintEdit();

	// Add combo items
	GetConstraints()->AddConstraint("False");
	GetConstraints()->AddConstraint("True");
}

void CPropertyGridBoolItem::SetValueFromText(const string& text)
{
	// Grab the constraint index that was chosen
	int nIndex = GetConstraints()->GetCurrent();

	// Apply the value to the property on the object.  Signed/Unsigned must be accounted for
	// to ensure proper casting during assignment.
	SetPropertyValue((bool)nIndex);
}

void CPropertyGridBoolItem::ToString(string& out)
{
	CXTPPropertyGridItemConstraints* pConstraints = GetConstraints();
	if (pConstraints == NULL || pConstraints->GetCount() == 0)
		return;  // Not yet initialized

	Value value;
	GetPropertyValue(value);

	// Set initial value
	bool v = value;
	int index = (v ? 1 : 0);
	pConstraints->SetCurrent(index);
	out = (LPCSTR)pConstraints->GetAt(index);
}