#include "pch.h"
#include "PropertyGridPathAgentItem.h"

#include <INavigationSystem.h>

using namespace CryGame;


IMPLEMENT_SERIAL(CPropertyGridPathAgentItem, CPropertyGridStringItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridPathAgentItem, PathAgent, eVType_Int, "")
REGISTER_PROPERTY_EDITOR(CPropertyGridPathAgentItem, PathAgent, eVType_UInt, "")

void CPropertyGridPathAgentItem::OnInit()
{
	__super::OnInit();

	AddComboButton();
	SetFlags(xtpGridItemHasComboButton);
	SetConstraintEdit();

	// Add combo items
	CXTPPropertyGridItemConstraints* pConstraints = GetConstraints();
	if (pConstraints)
	{
		INavigationSystem* pNavSystem = gEnv->pAISystem->GetNavigationSystem();
		if (pNavSystem)
		{
			int nCount = pNavSystem->GetAgentTypeCount();
			for (int i = 0; i < nCount; ++i)
			{
				NavigationAgentTypeID agentID = pNavSystem->GetAgentTypeID(i);
				const char* agentName = pNavSystem->GetAgentTypeName(agentID);
				pConstraints->AddConstraint(agentName, agentID);
			}
		}
	}
}

void CPropertyGridPathAgentItem::SetValueFromText(const string& text)
{
	// Grab the constraint index that was chosen
	int nIndex = GetConstraints()->GetCurrent();
	// Grab the user data which is the enum value
	int32 nValue = GetConstraints()->GetConstraintAt(nIndex)->m_dwData;

	// Apply the value to the property on the object.  Signed/Unsigned must be accounted for
	// to ensure proper casting during assignment.
	SetPropertyValue((GetPropertyType() == eVType_Int) ? nValue : (uint32)nValue);
}

void CPropertyGridPathAgentItem::ToString(string& out)
{
	CXTPPropertyGridItemConstraints* pConstraints = GetConstraints();
	if (pConstraints == NULL || pConstraints->GetCount() == 0)
		return;  // Not yet initialized

	Value value;
	GetPropertyValue(value);

	int32 nValue = (GetPropertyType() == eVType_Int) ? value : (int32)(uint32)value;

	out = gEnv->pAISystem->GetNavigationSystem()->GetAgentTypeName(NavigationAgentTypeID((uint32)nValue));
}