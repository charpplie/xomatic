////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 2013-3014
// -------------------------------------------------------------------------
//  File Name        : PropertyGridAncestorItem.cpp
//  Version          : v1.00
//  Created          : 5/20/2014 by Jack Harmon
//  Description      : Property editor for choosing an ancestor to populate this combo box.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "PropertyGridAncestorItem.h"
#include "ClassProfile.h"

// GameDll
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IGameInterface.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IObjectDesc.h>

using namespace CryGame;

IMPLEMENT_SERIAL(CPropertyGridAncestorItem, CPropertyGridStringItem, 0)

void CPropertyGridAncestorItem::OnInit()
{
	__super::OnInit();

	// Add combo items
	if (CXTPPropertyGridItemConstraints* pConstraints = GetConstraints())
	{
		pConstraints->RemoveAll();

		CDescEditor* pDescEditor = CClassProfileManager::GetInstance()->GetDescEditor();
		if (pDescEditor && !m_variation.empty())
		{
			std::map<string, IClass*> ancestors;
			pDescEditor->GetPropertyAncestors(ancestors);

			if (IClass* pClass = ancestors[m_variation])
			{
				// Found the ancestor, now fill the combo box with the property elements
				std::vector<string> list;

				if (pClass->GetName() == "CActorStateDesc")
				{
					// Special handling for states
					// Get a list of all associated descs from the CActorStateExtensionDesc and add them to the list
					HandleActorStateDescs(list);
				}
				
				if (list.size() == 0)
				{
					pDescEditor->GetPropertyElementList(pClass, list);
				}

				for (std::vector<string>::iterator it = list.begin(); it != list.end(); ++it)
				{
					pConstraints->AddConstraint(*it);
				}
			}
		}

		pConstraints->Sort();
		
		// Only enable combo button if there are items available.  Otherwise default to string edit.
		if (pConstraints->GetCount() > 0)
		{
			AddComboButton();
			SetFlags(xtpGridItemHasComboButton|xtpGridItemHasEdit);
			SetConstraintEdit(FALSE);
		}
	}
}

void CPropertyGridAncestorItem::HandleActorStateDescs(std::vector<string>& list)
{
	IObjectDescFactory* pObjectDescFactory = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetObjectDescFactory();
	CDescEditor* pDescEditor = CClassProfileManager::GetInstance()->GetDescEditor();

	if (!pObjectDescFactory || !pDescEditor)
		return;

	if (_smart_ptr<CObjectDesc> pDesc = pDescEditor->GetSelectedObjectDesc())
	{
		// Determine the CActorStateExtensionDesc filename.
		string filename = pDesc->GetFilename();
		string path = pObjectDescFactory->GetPath(pObjectDescFactory->Get("cactorstateextensiondesc", filename), true);

		if (path.empty())
		{
			// Strip everything after and including the last _ if it exists.
			if (filename.find_last_of('_') != -1)
			{
				filename = filename.Left(filename.find_last_of('_'));
				path = pObjectDescFactory->GetPath(pObjectDescFactory->Get("cactorstateextensiondesc", filename), true);
			}
		}

		// Can't load the actual desc classes here, so handle it via xml
		if (XmlNodeRef in = GetISystem()->GetXmlUtils()->LoadXmlFromFile(path))
		{
			if (XmlNodeRef stateLists = in->findChild("StateLists"))
			{
				for (int i = 0; i < stateLists->getChildCount(); ++i)
				{
					string filename = stateLists->getChild(i)->getAttr("Value");
					path = pObjectDescFactory->GetPath(pObjectDescFactory->Get("cactorstatelistdesc", filename), true);

					if (XmlNodeRef in = GetISystem()->GetXmlUtils()->LoadXmlFromFile(path))
					{
						if (XmlNodeRef states = in->findChild("States"))
						{
							for (int j = 0; j < states->getChildCount(); ++j)
							{
								//Todo: if an item being added already exists, warn the user of the duplicate.
								list.push_back(states->getChild(j)->getAttr("NameID"));
							}
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropertyGridAncestorItem::UpdateText()
{
	__super::UpdateText();

	CXTPPropertyGridItemConstraints* pConstraints = GetConstraints();
	if (pConstraints && pConstraints->GetCount() > 0)
	{
		Value v;
		GetPropertyValue(v);
		
		// Set property item text red if it doesn't exist.
		CXTPPropertyGridItemMetrics* pMetrics = GetValueMetrics();
		pMetrics->m_clrFore = (pConstraints->FindConstraint(string(v).c_str()) != -1) ? RGB(192, 192, 192) : RGB(200, 0, 0);
	}
}