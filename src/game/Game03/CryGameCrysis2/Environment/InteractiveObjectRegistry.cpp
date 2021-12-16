/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Interactive objects are registered here with required params

-------------------------------------------------------------------------
History:
- 10:12:2009: Created by Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "InteractiveObjectRegistry.h"
#include <IItemSystem.h>

void CInteractiveObjectRegistry::RegisterInteractiveObject( EntityId entityId, const char* interactionName )
{
	TEntityToParamsMap::iterator it = m_entityToParams.find(entityId);

	if (it != m_entityToParams.end())
	{
		//Refresh, might come from a reset
		it->second = interactionName;
	}
	else
	{
		m_entityToParams.insert(TEntityToParamsMap::value_type(entityId, ItemString(interactionName)));
	}

}

void CInteractiveObjectRegistry::UnregisterInteractiveObject( EntityId entityId )
{
	m_entityToParams.erase(entityId);
}

const SInteractionParams* CInteractiveObjectRegistry::GetInteractionParamsForEntity(EntityId entityId) const
{
	TEntityToParamsMap::const_iterator entityCit = m_entityToParams.find(entityId);

	if (entityCit != m_entityToParams.end())
	{
		TInteractionParamsMap::const_iterator paramsCit = m_interactiveObjectParams.find(entityCit->second);
		if (paramsCit != m_interactiveObjectParams.end())
		{
			return &(paramsCit->second);
		}
	}

	return NULL;
}

const SInteractionParams* CInteractiveObjectRegistry::GetInteractionParamsByName( const char* interactionName ) const
{
	TInteractionParamsMap::const_iterator paramsCit = m_interactiveObjectParams.find(CONST_TEMPITEM_STRING(interactionName));
	if (paramsCit != m_interactiveObjectParams.end())
	{
		return &(paramsCit->second);
	}

	return NULL;
}

void CInteractiveObjectRegistry::Init()
{
	ReadInteractionsFromXml("Libs/PlayerInteractive/PlayerInteractions.xml");
}

void CInteractiveObjectRegistry::ReadInteractionsFromXml(const char* fileName)
{
	XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFile(fileName);

	if (!rootNode || strcmpi(rootNode->getTag(), "PlayerInteractions"))
	{
		GameWarning("Could not load Player Interactions. Invalid XML file '%s'! ", fileName);
		return;
	}

	IItemParamsNode *pParamNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	pParamNode->ConvertFromXML(rootNode);

	const int childCount = 	pParamNode->GetChildCount();
	for (int i =  0; i < childCount; ++i)
	{
		const IItemParamsNode* pInteractionNode = pParamNode->GetChild(i);

		const char* interactionName = pInteractionNode->GetAttribute("name");
		interactionName = (interactionName != NULL) ? interactionName : "default";

		SInteractionParams interaction;
		
		pInteractionNode->GetAttribute("speed", interaction.speed);
		pInteractionNode->GetAttribute("delay", interaction.correctionDelay);
		interaction.speed = (interaction.speed > 0.0f) ? interaction.speed : 1.0f;

		if (const IItemParamsNode* pObjectNode = pInteractionNode->GetChild("Object"))
		{
			interaction.objectAnimation = pObjectNode->GetAttributeSafe("animation");
		}

		if (const IItemParamsNode* pUserNode = pInteractionNode->GetChild("User"))
		{
			interaction.userAnimation = pUserNode->GetAttributeSafe("animation");
		}

		if (const IItemParamsNode* pHelperNode = pInteractionNode->GetChild("Helper"))
		{
			interaction.helperName = pHelperNode->GetAttributeSafe("name");
		}

		m_interactiveObjectParams.insert(TInteractionParamsMap::value_type(ItemString(interactionName), interaction));
	}

	pParamNode->Release();
}

void CInteractiveObjectRegistry::Reload()
{
	m_interactiveObjectParams.clear();

	ReadInteractionsFromXml("Libs/PlayerInteractive/PlayerInteractions.xml");
}
