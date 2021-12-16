#include "stdafx.h"
#include "EntityLayer.h"
#include "Entity.h"


CEntityLayer::CEntityLayer(const char* name)
{
	m_name = name;
	m_isEnabled = true;
}


CEntityLayer::~CEntityLayer()
{
	for (std::vector<EntityProp>::iterator it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		CEntity* pEntity = (CEntity*) ((*it).m_pEntity);
		if((*it).m_bIsNoAwake && pEntity->GetPhysicalProxy() && pEntity->GetPhysicalProxy()->GetPhysicalEntity())
		{
			pe_action_awake aa;
			aa.bAwake = false;
			pEntity->GetPhysicalProxy()->GetPhysicalEntity()->Action(&aa);
		}
		(*it).m_bIsNoAwake=false;
	}
}


void CEntityLayer::AddChild(CEntityLayer* pLayer)
{
	m_childs.push_back(pLayer);
}


void CEntityLayer::AddObject(IEntity* pEntity)
{
	m_entities.push_back(EntityProp(pEntity, false, pEntity->IsHidden()));
}


void CEntityLayer::Enable(bool isEnable)
{
	if(m_isEnabled==isEnable)
		return;
	m_isEnabled=isEnable;

	pe_action_awake noAwake;
	noAwake.bAwake = false;

	for (std::vector<EntityProp>::iterator it = m_entities.begin(); it != m_entities.end(); ++it)
	{
		CEntity* pEntity = (CEntity*) ((*it).m_pEntity);
		if(!isEnable)
			(*it).m_bIsHidden=pEntity->IsHidden();
		if((*it).m_bIsHidden)
			continue;

		if(isEnable)
		{
			pEntity->Hide(!isEnable);
			pEntity->Activate(isEnable);

			if((*it).m_bIsNoAwake && pEntity->GetPhysicalProxy() && pEntity->GetPhysicalProxy()->GetPhysicalEntity())
				pEntity->GetPhysicalProxy()->GetPhysicalEntity()->Action(&noAwake);

			(*it).m_bIsNoAwake=false;
		}
		else
		{
			(*it).m_bIsNoAwake=false;
			CPhysicalProxy* pPhProxy = pEntity->GetPhysicalProxy();
			if(pPhProxy)
			{
				pe_status_awake isawake;
				IPhysicalEntity* pPhEnt = pPhProxy->GetPhysicalEntity();
				if(pPhEnt && pPhEnt->GetStatus(&isawake)==0)
					(*it).m_bIsNoAwake=true;
			}
			pEntity->Hide(!isEnable);
			pEntity->Activate(isEnable);
			if((*it).m_bIsNoAwake && pEntity->GetPhysicalProxy() && pEntity->GetPhysicalProxy()->GetPhysicalEntity())
				pEntity->GetPhysicalProxy()->GetPhysicalEntity()->Action(&noAwake);
		}
	}

	for (std::vector<CEntityLayer*>::iterator it = m_childs.begin(); it != m_childs.end(); ++it)
		(*it)->Enable(isEnable);
}