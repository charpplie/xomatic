////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   EquipmentManager.h
//  Version:     v1.00
//  Created:     07/07/2006 by AlexL
//  Compilers:   Visual Studio.NET
//  Description: EquipmentManager to handle item packs
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EQUIPMENTMANAGER_H__
#define __EQUIPMENTMANAGER_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IItemSystem.h>

class CItemSystem;

class CEquipmentManager : public IEquipmentManager
{
public:
	CEquipmentManager(CItemSystem* pItemSystem);
	~CEquipmentManager();

	// Clear all equipment packs
	VIRTUAL void DeleteAllEquipmentPacks();

	// Loads equipment packs from rootNode 
	VIRTUAL void LoadEquipmentPacks(const XmlNodeRef& rootNode);

	// Load all equipment packs from a certain path
	VIRTUAL void LoadEquipmentPacksFromPath(const char* path);

	// Load an equipment pack from an XML node
	VIRTUAL bool LoadEquipmentPack(const XmlNodeRef& rootNode, bool bOverrideExisting=true);

	// Give an equipment pack (resp. items/ammo) to an actor
	VIRTUAL bool GiveEquipmentPack(IActor* pActor, const char* packName, bool bAdd, bool selectPrimary=false);

	// return iterator with all available equipment packs
	VIRTUAL IEquipmentManager::IEquipmentPackIteratorPtr CreateEquipmentPackIterator();

	VIRTUAL void RegisterListener(IListener *pListener);
	VIRTUAL void UnregisterListener(IListener *pListener);

	// listener callbacks
	void OnBeginGiveEquipmentPack();
	void OnEndGiveEquipmentPack();

	void DumpPacks();
	
	struct SEquipmentPack 
	{
		struct SEquipmentItem 
		{
			SEquipmentItem(const char* name, const char* type, const char* setup)
				: m_name(name), m_type(type), m_setup(setup) {}
			string m_name;
			string m_type;
			string m_setup;
		};

		void Init(const char* name)
		{
			m_name.assign(name);
			m_primaryItem.assign("");
			m_items.clear();
			m_ammoCount.clear();
		}

		bool AddItem(const char* name, const char* type, const char* setup)
		{
			if (HasItem(name))
				return false;
			m_items.push_back(SEquipmentItem(name, type, setup));
			return true;
		}

		bool HasItem(const char* name) const
		{
			for (std::vector<SEquipmentItem>::const_iterator iter = m_items.begin(), iterEnd = m_items.end();
				iter != iterEnd; ++iter)
			{
				if (iter->m_name == name)
					return true;
			}
			return false;
		}
		
		inline int NumItems() const
		{
			return m_items.size();
		}

		const SEquipmentItem* GetItemByIndex(int index) const
		{
			if (index < 0 || index >= m_items.size())
				return 0;
			return &m_items[index];
		}

		string												m_name;
		string												m_primaryItem;
		std::vector<SEquipmentItem>		m_items;
		std::map<string, int>					m_ammoCount;
	};
	typedef std::vector<SEquipmentPack*> TEquipmentPackVec;

	SEquipmentPack* GetPack(const char* packName) const;
	void DumpPack(const SEquipmentPack* pPack) const;

	void GetMemoryStatistics(ICrySizer * s);

protected:


	typedef std::vector<IEquipmentManager::IListener*>	TListenerVec;

	friend class CScriptBind_ItemSystem;

	CItemSystem* m_pItemSystem;
	TEquipmentPackVec m_equipmentPacks;
	TListenerVec      m_listeners;
};

#endif
