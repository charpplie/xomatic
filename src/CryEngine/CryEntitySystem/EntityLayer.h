/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2010.
 -------------------------------------------------------------------------
  Description: Entity layer container. 
  
 -------------------------------------------------------------------------
  History:
  - 11:2:2010   10:50 : Created by Sergiy Shaykin

*************************************************************************/
#ifndef __ENTITYLAYER_H__
#define __ENTITYLAYER_H__

#if _MSC_VER > 1000
# pragma once
#endif




class CEntityLayer
{

	struct EntityProp
	{
		EntityProp(IEntity* pEntity, bool bIsNoAwake, bool bIsHidden)
			:m_pEntity(pEntity), m_bIsNoAwake(bIsNoAwake), m_bIsHidden(bIsHidden) {}

		IEntity* m_pEntity;
		bool m_bIsNoAwake;
		bool m_bIsHidden;
	};


public:
	CEntityLayer(const char* name);
	virtual ~CEntityLayer();

	void AddChild(CEntityLayer* pLayer);
	
	void AddObject(IEntity* pEntity);
	void Enable(bool isEnable);

private:
	string m_name;
	bool m_isEnabled;
	std::vector<CEntityLayer*>m_childs;
	std::vector<EntityProp>m_entities;

friend class CEntitySystem;
};

#endif //__ENTITYLAYER_H__