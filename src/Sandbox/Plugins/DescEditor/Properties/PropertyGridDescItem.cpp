#include "pch.h"
#include "PropertyGridDescItem.h"
#include "ClassProfile.h"

// GameDll
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IGameInterface.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IObjectDesc.h>

using namespace CryGame;


IMPLEMENT_SERIAL(CPropertyGridDescItem, CPropertyGridStringItem, 0)
// NOTE: All desc editor types are registered by the CClassProfileManager

CPropertyGridDescItem::CPropertyGridDescItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
	: CPropertyGridStringItem(pGrid, pParent, pObject, pProperty, isElement, name)
{
	// If this is a desc property, set the variation now so it will automatically set to the property type.
	// Also, because this property only accepts specific desc types, don't allow the type changed.
	if (pProperty->IsDesc())
	{
		IClass* pClass = pProperty->GetClassType(pObject);
		if (pClass)
		{
			m_variation = pClass->GetName().c_str();
		}
	}
}

void CPropertyGridDescItem::OnInit()
{
	__super::OnInit();

	AddComboButton();
	SetFlags(xtpGridItemHasComboButton);
	SetConstraintEdit();

	// Add combo items
	CXTPPropertyGridItemConstraints* pConstraints = GetConstraints();
	if (pConstraints)
	{
		IClass* pClass = GetDescClass();
		if (pClass)
		{
			// Find all inherited classes of the editor type for this item
			DynArray<IClass*> subClasses;
			subClasses = pClass->GetSubClasses(true);

			// Get the base class so we can iterate over all the inherited descs to find a match
			IClass* pBaseClass;
			(pClass->GetBaseClass()->GetName() == "CObjectDesc") ? pBaseClass = pClass : pBaseClass = pClass->GetBaseClass();

			IObjectDescFactory* pDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();

			// Get a list of all descs that inherit from the base class
			ObjectDescVector descs = pDescFactory->GetDescList(pBaseClass->GetName());
			for (int i = 0; i < descs.size(); ++i)
			{
				CObjectDesc* pDesc = descs[i];
				for (int j = 0; j < subClasses.size(); ++j)
				{
					if (pDesc->GetClass() == subClasses[j])
					{
						pConstraints->AddConstraint(pDesc->GetNameID());
					}
				}
			}

			pConstraints->Sort();
		}
	}
}

IClass* CPropertyGridDescItem::GetDescClass()
{
	return ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry()->GetClass(m_variation);
}

void CPropertyGridDescItem::UpdateText()
{
	__super::UpdateText();

	IObjectDescFactory* pDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
	if (pDescFactory)
	{
		Value v;
		GetPropertyValue(v);

		IClass* pClass = GetDescClass();
		if (pClass)
		{
			// Render red if invalid
			CObjectDesc* pDesc = pDescFactory->Get(pClass->GetName(), (string)v);
			CXTPPropertyGridItemMetrics* pMetrics = GetValueMetrics();
			pMetrics->m_clrFore = (pDesc) ? RGB(192, 192, 192) : RGB(200, 0, 0);
		}
	}
}