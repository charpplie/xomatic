/************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 29:9:2004   18:02 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ItemSystem.h"
#include <CryPath.h>
//#include "ScriptBind_ItemSystem.h"
#include "IActionMapManager.h"
#include "GameObjects/GameObject.h"
#include "CryAction.h"
#include "IGameObjectSystem.h"
#include "IActorSystem.h"

#include <IEntitySystem.h>
#include <ICryAnimation.h>
#include <IVehicleSystem.h>
#include "ItemParams.h"
#include "EquipmentManager.h"
#include "CryActionCVars.h"

ICVar *CItemSystem::m_pPrecache = 0;
ICVar *CItemSystem::m_pItemLimit = 0;

//This is used for AI offhands, if we want more classes, perhaps it's better
//if every class can specify the size of its shared pool
#define ITEM_SHARED_POOL_MAX_SIZE	5

//------------------------------------------------------------------------
CItemSystem::CItemSystem(IGameFramework *pGameFramework, ISystem *pSystem)
: m_pGameFramework(pGameFramework),
	m_pSystem(pSystem),
	m_pEntitySystem(gEnv->pEntitySystem),
	m_spawnCount(0),
	m_grenadeType("explosivegrenade"),
	m_precacheTime(0.0f),
	m_reloading(false),
	m_recursing(false)
{
	RegisterCVars();
	m_pEquipmentManager = new CEquipmentManager(this);
	m_itemClassIterator = m_params.begin();
}

//------------------------------------------------------------------------
CItemSystem::~CItemSystem()
{
	SAFE_DELETE(m_pEquipmentManager);
	UnregisterCVars();

	ClearGeometryCache();
	ClearSoundCache();
}

//------------------------------------------------------------------------
void CItemSystem::OnLoadingStart(ILevelInfo *pLevelInfo)
{
	Reset();

	ClearGeometryCache();
	ClearSoundCache();

  if (gEnv->bMultiplayer)
  {
    SetConfiguration("mp");
  }
  else
  {
    ICVar* pVar = gEnv->pConsole->GetCVar("g_editor_seizure");
    if (pVar && pVar->GetIVal()!=0)
    {
      SetConfiguration("mp");
    }
    else
    {
        SetConfiguration("");
    }
  }
}

//------------------------------------------------------------------------
void CItemSystem::OnLoadingComplete(ILevel *pLevel)
{
	// marcio: precaching of items enabled by default for now
//	ICVar *sys_preload=gEnv->pConsole->GetCVar("sys_preload");
//	if ((!sys_preload || sys_preload->GetIVal()) && m_pPrecache->GetIVal())
	{
		PrecacheLevel();
	}
}

//------------------------------------------------------------------------
void CItemSystem::RegisterItemClass(const char *name, IGameFramework::IItemCreator *pCreator)
{
	m_classes.insert(TItemClassMap::value_type(name, SItemClassDesc(pCreator)));
}

//------------------------------------------------------------------------
void CItemSystem::Update()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	ITimer *pTimer = gEnv->pTimer;
	IActor *pActor = m_pGameFramework->GetClientActor();
	
	if (pActor)
	{
		IInventory* pInventory = pActor->GetInventory();
		if (pInventory)
		{
      IItem *pCurrentItem = GetItem(pInventory->GetCurrentItem());      
      if(pCurrentItem && pCurrentItem->IsSelected())
				pCurrentItem->UpdateFPView(pTimer->GetFrameTime());

			IItem *pLastItem = GetItem(pInventory->GetLastItem());
			if (pLastItem && pLastItem != pCurrentItem && pLastItem->IsSelected())
				pLastItem->UpdateFPView(pTimer->GetFrameTime());

			// marcok: argh ... we depend on Crysis game stuff here ... REMOVE
			IEntityClass* pOffHandClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("OffHand");
			IItem *pOffHandItem = GetItem(pInventory->GetItemByClass(pOffHandClass));

			// HAX: update offhand - shouldn't be here
			if(pOffHandItem)
				pOffHandItem->UpdateFPView(pTimer->GetFrameTime());
		}
	}

	if(CCryActionCVars::Get().debugItemMemStats!=0)
	{
		DisplayItemSystemStats();
	}

}

//------------------------------------------------------------------------
const IItemParamsNode *CItemSystem::GetItemParams(const char *itemName) const
{
	TItemParamsMap::const_iterator it = m_params.find(CONST_TEMP_STRING(itemName));
	if (it != m_params.end())
	{
		if (m_config.empty())
			return it->second.params;

		std::map<string, CItemParamsNode *>::const_iterator cit=it->second.configurations.find(m_config);
		if (cit != it->second.configurations.end())
			return cit->second;

		return it->second.params;
	}

	GameWarning("Trying to get xml description for item '%s'! Something very wrong has happened!", itemName);
	return 0;
}

//------------------------------------------------------------------------
uint8 CItemSystem::GetItemPriority(const char *itemName) const
{
	TItemParamsMap::const_iterator it = m_params.find(CONST_TEMP_STRING(itemName));
	if (it != m_params.end())
		return it->second.priority;

	GameWarning("Trying to get priority for item '%s'! Something very wrong has happened!", itemName);
	return 0;
}

//------------------------------------------------------------------------
const char *CItemSystem::GetItemCategory(const char *itemName) const
{
	TItemParamsMap::const_iterator it = m_params.find(CONST_TEMP_STRING(itemName));
	if (it != m_params.end())
		return it->second.category.c_str();

	GameWarning("Trying to get category for item '%s'! Something very wrong has happened!", itemName);
	return 0;
}

//-------------------------------------------------------------------------
uint8 CItemSystem::GetItemUniqueId(const char *itemName) const
{
	TItemParamsMap::const_iterator it = m_params.find(CONST_TEMP_STRING(itemName));
	if (it != m_params.end())
		return it->second.uniqueId;

	GameWarning("Trying to get uniqueId for item '%s'! Something very wrong has happened!", itemName);
	return 0;
}

//------------------------------------------------------------------------
bool CItemSystem::IsItemClass(const char *name) const
{
	TItemParamsMap::const_iterator it = m_params.find(CONST_TEMP_STRING(name));
	return (it != m_params.end());
}

//------------------------------------------------------------------------
const char* CItemSystem::GetFirstItemClass()
{
	m_itemClassIterator = m_params.begin();

	if(m_itemClassIterator != m_params.end())
		return m_itemClassIterator->first;
	return 0;
}

//------------------------------------------------------------------------
const char* CItemSystem::GetNextItemClass() 
{
	if(m_itemClassIterator != m_params.end())
	{
		++m_itemClassIterator;
		if(m_itemClassIterator != m_params.end())
			return m_itemClassIterator->first;
	}
	return 0;
}

//------------------------------------------------------------------------
void CItemSystem::RegisterForCollection(EntityId itemId)
{
#if _DEBUG
	IItem *pItem=GetItem(itemId);
	CRY_ASSERT(pItem);

	if (pItem)
	{
		//CRY_ASSERT(!pItem->GetIItemBox()->GetOwnerId());
		CRY_ASSERT(!pItem->GetOwnerId());
	}
#endif

	CTimeValue now=gEnv->pTimer->GetFrameStartTime();

	TCollectionMap::iterator cit = m_collectionmap.find(itemId);
	if (cit == m_collectionmap.end())
		m_collectionmap.insert(TCollectionMap::value_type(itemId, now));
	else
		cit->second=now;

	if (gEnv->bServer && gEnv->bMultiplayer)
	{
		int limit=0;
		if (m_pItemLimit)
			limit=m_pItemLimit->GetIVal();

		if (limit>0)
		{
			int size=(int)m_collectionmap.size();
			if (size>limit)
			{
				EntityId nItemId=0;
				for (TCollectionMap::const_iterator it=m_collectionmap.begin(); it!=m_collectionmap.end(); ++it)
				{
					if (it->second<now)
					{
						now=it->second;
						nItemId=it->first;
					}
				}

				if (IItem *pItem=GetItem(nItemId))
				{
					//CRY_ASSERT(!pItem->GetIItemBox()->GetOwnerId());
					CRY_ASSERT(!pItem->GetOwnerId());
				
					CryLogAlways("[game] Removing item %s due to i_lying_item_limit!", pItem->GetEntity()->GetName());
					UnregisterForCollection(nItemId);
					gEnv->pEntitySystem->RemoveEntity(nItemId);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CItemSystem::UnregisterForCollection(EntityId itemId)
{
	stl::member_find_and_erase(m_collectionmap, itemId);
}

//------------------------------------------------------------------------
void CItemSystem::Reload()
{
	m_reloading=true;
	
	m_params.clear();

	for (TFolderList::iterator fit=m_folders.begin(); fit!=m_folders.end();++fit)
		Scan(fit->c_str());

	SEntityEvent event(ENTITY_EVENT_RESET);

	for (TItemMap::iterator it=m_items.begin(); it!=m_items.end();++it)
	{
		IEntity *pEntity = m_pEntitySystem->GetEntity(it->first);
		if (pEntity)
			pEntity->SendEvent(event);
	}

	m_reloading=false;
}

//------------------------------------------------------------------------
void CItemSystem::PreReload()
{
	m_reloading=true;

	m_params.clear();

	for (TFolderList::iterator fit=m_folders.begin(); fit!=m_folders.end();++fit)
		Scan(fit->c_str());

	for (TItemMap::iterator it=m_items.begin(); it!=m_items.end();++it)
	{
		IItem *pItem = it->second;
		if (pItem)
			pItem->PreResetParams();
	}
}

//------------------------------------------------------------------------
void CItemSystem::PostReload()
{
	SEntityEvent event(ENTITY_EVENT_RESET);

	for (TItemMap::iterator it=m_items.begin(); it!=m_items.end();++it)
	{
		IItem *pItem = it->second;
		if (pItem)
			pItem->ResetParams();
	}
	
	for (TItemMap::iterator it=m_items.begin(); it!=m_items.end();++it)
	{
		IEntity *pEntity = m_pEntitySystem->GetEntity(it->first);
		if (pEntity)
			pEntity->SendEvent(event);
	}

	m_reloading=false;
}

//------------------------------------------------------------------------
void CItemSystem::Reset()
{
	if(GetISystem()->IsSerializingFile() == 1)
	{
		TItemMap::iterator it = m_items.begin();
		IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
		for (; it != m_items.end(); ++it)
		{
			EntityId id = it->first;
			IEntity *pEntity = pEntitySystem->GetEntity(id);
			it->second->Reset();

			if(!pEntity)
			{
				m_items.erase(it);
				--it;
			}
		}
	}
}

//------------------------------------------------------------------------
void CItemSystem::Scan(const char *folderName)
{
	string folder = folderName;
	string search = folder;
	search += "/*.*";

	ICryPak *pPak = gEnv->pCryPak;

	_finddata_t fd;
	intptr_t handle = pPak->FindFirst(search.c_str(), &fd);

	if (!m_recursing)
		CryLog("Loading item XML definitions from '%s'!", folderName);

	if (handle > -1)
	{
		do
		{
			if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
				continue;

			if (fd.attrib & _A_SUBDIR)
			{
				string subName = folder+"/"+fd.name;
				if (m_recursing)
					Scan(subName.c_str());
				else
				{
					m_recursing=true;
					Scan(subName.c_str());
					m_recursing=false;
				}
				continue;
			}

			const char* fileExtension = PathUtil::GetExt(fd.name);
			if (stricmp(fileExtension, "xml"))
			{
				GameWarning("ItemSystem: File '%s' does not have 'xml' extension, skipping.", fd.name );
				continue;
			}

			string xmlFile = folder + string("/") + string(fd.name);
			XmlNodeRef rootNode = m_pSystem->LoadXmlFile(xmlFile.c_str());

			if (!rootNode)
			{
				ItemSystemErrorMessage(xmlFile.c_str(), "Root xml node couldn't be loaded", true);
				continue;
			}

			if (!ScanXML(rootNode, xmlFile.c_str()))
			{
				continue;
			}

		} while (pPak->FindNext(handle, &fd) >= 0);
	}

	if (!m_recursing)
		CryLog("Finished loading item XML definitions from '%s'!", folderName);

	if (!m_reloading && !m_recursing)
		m_folders.push_back(folderName);
}

#include <iterator> //Diesel new

//------------------------------------------------------------------------
bool CItemSystem::ScanXML(XmlNodeRef &root, const char *xmlFile)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Item XML (%s)", xmlFile);

	if (strcmpi(root->getTag(), "item"))
	{
		// We don't want to report error here, we have other files in the same folder like ammo with different tag
		return false;
	}

	const char *name = root->getAttr("name");
	if (!name)
	{
		ItemSystemErrorMessage(xmlFile, "Item definition file does not contain attribute 'name'! Skipping...", true);
		return false;
	}

	const char *className = root->getAttr("class");
	
	if (!className)
	{
		ItemSystemErrorMessage(xmlFile, "Item definition file does not contain attribute 'class'! Skipping...", true);
		return false;
	}

	TItemClassMap::iterator it = m_classes.find(CONST_TEMP_STRING(className));
	if (it == m_classes.end())
	{
		CryFixedStringT<128> errorBuffer;
		errorBuffer.Format("Unknown item class '%s'! Skipping...", className);
		ItemSystemErrorMessage(xmlFile, errorBuffer.c_str(), true);
		return false;
	}

	const char *configName = root->getAttr("configuration");
	TItemParamsMap::iterator dit=m_params.find(CONST_TEMP_STRING(name));
	
	if (dit == m_params.end())
	{
		const char *scriptName = root->getAttr("script");
		IEntityClassRegistry::SEntityClassDesc classDesc;
		classDesc.sName = name;
		if (scriptName && scriptName[0])
			classDesc.sScriptFile = scriptName;
		else
		{
			classDesc.sScriptFile = DEFAULT_ITEM_SCRIPT;
			CreateItemTable(name);
		}

		int invisible=0;
		root->getAttr("invisible", invisible);

		classDesc.pUserProxyCreateFunc = (IEntityClass::UserProxyCreateFunc)it->second.pCreator;
		classDesc.flags |= invisible?ECLF_INVISIBLE:0;

		if (!m_reloading)
			CCryAction::GetCryAction()->GetIGameObjectSystem()->RegisterExtension( name, it->second.pCreator, &classDesc );

		std::pair<TItemParamsMap::iterator, bool> result = m_params.insert(TItemParamsMap::value_type(name, SItemParamsDesc()));
		dit = result.first;
	}

	SItemParamsDesc &desc = dit->second;

	if (!configName || !configName[0])
	{
		if (desc.params)
			desc.params->Release();

		desc.params = new CItemParamsNode();
		bool filterMP = desc.params->ConvertFromXMLWithFiltering(root, "SP");

		desc.priority = 0;
		int p=0;
		if (desc.params->GetAttribute("priority", p))
			desc.priority=(uint8)p;
		desc.category = desc.params->GetAttribute("category");
		desc.uniqueId = 0;
		int id=0;
		if (desc.params->GetAttribute("uniqueId", id))
			desc.uniqueId = (uint8)id;

		if(filterMP)
		{
			CItemParamsNode *params = new CItemParamsNode();
			params->ConvertFromXMLWithFiltering(root, "MP");
//Diesel cut
			//desc.configurations.insert(std::make_pair<string, CItemParamsNode *>("mp", params));
//Diesel new		
			desc.configurations.insert(std::make_pair("mp", params));

		
		}
	}
	else
	{
		// to check that a weapon has multiple configuration (we support that for different game modes)
//		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Non-empty configuration found in \"%s\"!!!", xmlFile);

		CItemParamsNode *params = new CItemParamsNode();
		params->ConvertFromXML(root);
		
//Diesel cut		
		//desc.configurations.insert(std::make_pair<string, CItemParamsNode *>(configName, params));
	
//Diesel new	
		desc.configurations.insert(std::make_pair(configName, params));

	
	}

	return true;
}

//------------------------------------------------------------------------
void CItemSystem::AddItem(EntityId itemId, IItem *pItem)
{
	m_items.insert(TItemMap::value_type(itemId, pItem));

}


//------------------------------------------------------------------------
void CItemSystem::RemoveItem(EntityId itemId, const char* itemName)
{
	stl::member_find_and_erase(m_items, itemId);
	stl::member_find_and_erase(m_collectionmap, itemId);
}



//------------------------------------------------------------------------
IItem *CItemSystem::GetItem(EntityId itemId) const
{
	TItemMap::const_iterator it = m_items.find(itemId);

	if (it != m_items.end())
	{
		return it->second;
	}
	return 0;
}

//------------------------------------------------------------------------
EntityId CItemSystem::GiveItem(IActor *pActor, const char *item, bool sound, bool select, bool keepHistory, const char *setup)
{
	if (!gEnv->bServer)
	{
		GameWarning("Trying to spawn item of class '%s' on a process which is not server!", item);
		return 0;
	}

	if(gEnv->pSystem->IsSerializingFile())
		return 0;

	CRY_ASSERT(item && pActor);
	static char itemName[65];
	_snprintf(itemName, 64, "%s%.03d", item, ++m_spawnCount);
  itemName[sizeof(itemName)-1] = '\0';
	SEntitySpawnParams params;
	params.sName = itemName;
	params.pClass = m_pEntitySystem->GetClassRegistry()->FindClass(item);
	params.nFlags |= (ENTITY_FLAG_NO_PROXIMITY|ENTITY_FLAG_NEVER_NETWORK_STATIC);
	if (!params.pClass)
	{
		GameWarning("Trying to spawn item of class '%s' which is unknown!", item);
		return 0;
	}

	if (IEntity *pItemEnt = m_pEntitySystem->SpawnEntity(params))
	{
		EntityId itemEntId = pItemEnt->GetId();
		if (IItem * pItem = GetItem(itemEntId))
		{
			// this may remove the entity
			pItem->PickUp(pActor->GetEntityId(), sound, select, keepHistory, setup);

			//[kirill] make sure AI gets notified about new item
			if(IAIObject * pActorAI = pActor->GetEntity()->GetAI())
				gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER, 0, "OnUpdateItems", pActorAI);

			if ((pItemEnt = gEnv->pEntitySystem->GetEntity(itemEntId)) && !pItemEnt->IsGarbage())
			{
				// set properties table to null, since they'll not be used [timur's request]
				if (pItemEnt->GetScriptTable())
					pItemEnt->GetScriptTable()->SetToNull("Properties");

				return pItemEnt->GetId();
			}
		}
	}

	return 0;
}

//------------------------------------------------------------------------
void CItemSystem::SetActorItem(IActor *pActor, EntityId itemId, bool keepHistory)
{
	if(!pActor)
		return;
	IInventory* pInventory = pActor->GetInventory();
	if (!pInventory)
		return;

	EntityId currentItemId = pInventory->GetCurrentItem();

	IItem *pItem = GetItem(itemId);

	if (currentItemId == itemId)
  {
		if (keepHistory)
			pInventory->SetLastItem(currentItemId);
			
    if (pItem)
      pItem->Select(true);
		return;
  }

	if (currentItemId)
	{
		IItem *pCurrentItem = GetItem(currentItemId);
		if (pCurrentItem)
		{
			pInventory->SetCurrentItem(0);
			pCurrentItem->Select(false);
			if (keepHistory)
				pInventory->SetLastItem(currentItemId);	
			//if (pItem)
			//	pItem->EnableSelect(false);
		}
	}

	TListenerVec::iterator iter = m_listeners.begin();
	while (iter != m_listeners.end())
	{
		(*iter)->OnSetActorItem(pActor, pItem);
		++iter;
	}

	CCryAction::GetCryAction()->GetIGameplayRecorder()->Event(pActor->GetEntity(), GameplayEvent(eGE_ItemSelected, 0, 0, (void *)itemId));

	if (!pItem)
		return;

	pInventory->SetCurrentItem(itemId);
	if (pItem)
	{
		pItem->SetHand(IItem::eIH_Right);
		pItem->Select(true);
	}

}

//------------------------------------------------------------------------
void CItemSystem::SetActorAccessory(IActor *pActor, EntityId itemId, bool keepHistory)
{
	if(!pActor) 
		return;
	
	IInventory* pInventory = pActor->GetInventory();
	if (!pInventory) 
		return;

	IItem *pItem = GetItem(itemId);

	TListenerVec::iterator iter = m_listeners.begin();
	while (iter != m_listeners.end())
	{
		(*iter)->OnSetActorAccessory(pActor, pItem);
		++iter;
	}
}

//------------------------------------------------------------------------

void CItemSystem::DropActorItem(IActor *pActor, EntityId itemId)
{
	if(!pActor) 
		return;

	IInventory* pInventory = pActor->GetInventory();
	if (!pInventory) 
		return;

	IItem *pItem = GetItem(itemId);
	if (!pItem) 
		return;

	TListenerVec::iterator iter = m_listeners.begin();
	while (iter != m_listeners.end())
	{
		(*iter)->OnDropActorItem(pActor, pItem);
		++iter;
	}
}

//------------------------------------------------------------------------

void CItemSystem::DropActorAccessory(IActor *pActor, EntityId itemId)
{
	if(!pActor) 
		return;
	
	IInventory* pInventory = pActor->GetInventory();
	if (!pInventory) 
		return;

	IItem *pItem = GetItem(itemId);
	if (!pItem) return;

	TListenerVec::iterator iter = m_listeners.begin();
	while (iter != m_listeners.end())
	{
		(*iter)->OnDropActorAccessory(pActor, pItem);
		++iter;
	}
}

//------------------------------------------------------------------------
void CItemSystem::SetActorItem(IActor *pActor, const char *name, bool keepHistory)
{
	IInventory* pInventory = (pActor != NULL) ? pActor->GetInventory() : NULL;
	if (!pInventory)
		return;

	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
	EntityId itemId = pInventory->GetItemByClass(pClass);
	if (!itemId)
		return;

	SetActorItem(pActor, itemId, keepHistory);
}

//------------------------------------------------------------------------
void CItemSystem::CacheGeometry(const IItemParamsNode *geometry)
{
	if (!geometry)
		return;

	int n = geometry->GetChildCount();
	if (n>0)
	{
		for (int i=0; i<n; i++)
		{
			const IItemParamsNode *slot = geometry->GetChild(i);
			const char *name = slot->GetAttribute("name");
			int useStreaming = 1;
			slot->GetAttribute("useStreaming", useStreaming);

			if (name && name[0])
				CacheObject(name, (useStreaming != 0));
		}
	}
}

//------------------------------------------------------------------------
void CItemSystem::CacheItemGeometry(const char *className)
{
	TItemParamsMap::iterator it=m_params.find(CONST_TEMP_STRING(className));
	if (it==m_params.end())
		return;

	if ((it->second.precacheFlags&SItemParamsDesc::eIF_PreCached_Geometry) == 0)
	{
		if (const IItemParamsNode *root=GetItemParams(className))
		{
			const IItemParamsNode *geometry=root->GetChild("geometry");
			if (geometry)
				CacheGeometry(geometry);
		}

		it->second.precacheFlags|=SItemParamsDesc::eIF_PreCached_Geometry;
	}
}

//------------------------------------------------------------------------
void CItemSystem::CacheItemSound(const char *className)
{
	TItemParamsMap::iterator it=m_params.find(CONST_TEMP_STRING(className));
	if (it==m_params.end())
		return;

	if ((it->second.precacheFlags&SItemParamsDesc::eIF_PreCached_Sound) == 0)
	{
		if (const IItemParamsNode *root=GetItemParams(className))
		{
			const IItemParamsNode *actions=root->GetChild("actions");
			if (actions)
			{
				int n=actions->GetChildCount();
				for (int i=0;i<n;i++)
				{
					const IItemParamsNode *action=actions->GetChild(i);
					if (!stricmp(action->GetName(), "action"))
					{
						int na=action->GetChildCount();
						for (int a=0;a<na;a++)
						{
							const IItemParamsNode *sound=actions->GetChild(i);
							if (!stricmp(sound->GetName(), "sound"))
							{
								const char *soundName=sound->GetNameAttribute();

								gEnv->pSoundSystem->Precache(soundName, 0, FLAG_SOUND_PRECACHE_LOAD_GROUP);
							}
						}
					}
				}
				
			}
		}

		it->second.precacheFlags|=SItemParamsDesc::eIF_PreCached_Sound;
	}
}

//------------------------------------------------------------------------
void CItemSystem::ClearSoundCache()
{
	for (TItemParamsMap::iterator it=m_params.begin(); it!=m_params.end(); ++it)
		it->second.precacheFlags&=~SItemParamsDesc::eIF_PreCached_Sound;
}

//------------------------------------------------------------------------
ICharacterInstance *CItemSystem::GetCachedCharacter(const char *fileName)
{
	CryFixedStringT<256> name(fileName);
	name.replace('\\', '/');
	name.MakeLower();

	TCharacterCacheIt cit = m_characterCache.find(CONST_TEMP_STRING(name.c_str()));

	if (cit == m_characterCache.end())
		return 0;

	return cit->second;
}

//------------------------------------------------------------------------
IStatObj *CItemSystem::GetCachedObject(const char *fileName)
{
	CryFixedStringT<256> name = fileName;
	name.replace('\\', '/');
	name.MakeLower();

	TObjectCacheIt oit = m_objectCache.find(CONST_TEMP_STRING(name.c_str()));

	if (oit == m_objectCache.end())
		return 0;

	return oit->second;
}

//------------------------------------------------------------------------
void CItemSystem::CacheObject(const char *fileName, bool useCgfStreaming)
{
	string name = PathUtil::ToUnixPath(string(fileName));
	name.MakeLower();

	TObjectCacheIt oit = m_objectCache.find(name);
	TCharacterCacheIt cit = m_characterCache.find(name);

	if ((oit != m_objectCache.end()) || (cit != m_characterCache.end()))
	{
		return;
	}

	stack_string ext(PathUtil::GetExt(name.c_str()));

	if ((ext == "cdf") || (ext == "chr") || (ext == "cga"))
	{
		ICharacterInstance *pChar = gEnv->pCharacterManager->CreateInstance(fileName);
		if (pChar)
		{
			pChar->AddRef();
			m_characterCache.insert(TCharacterCache::value_type(name, pChar));
		}
	}
	else
	{
		IStatObj *pStatObj = gEnv->p3DEngine->LoadStatObj(fileName, 0, 0, useCgfStreaming);
		if (pStatObj)
		{
			pStatObj->AddRef();
			m_objectCache.insert(TObjectCache::value_type(name, pStatObj));
		}
	}
}

//------------------------------------------------------------------------
void CItemSystem::ClearGeometryCache()
{	
	for (TObjectCacheIt oit = m_objectCache.begin(); oit != m_objectCache.end(); oit++)
	{
		if (oit->second)
			oit->second->Release();
	}
	m_objectCache.clear();

	for (TCharacterCacheIt cit = m_characterCache.begin(); cit != m_characterCache.end(); cit++)
		cit->second->Release();
	m_characterCache.clear();

	for (TItemParamsMap::iterator it=m_params.begin(); it!=m_params.end(); ++it)
		it->second.precacheFlags&=~SItemParamsDesc::eIF_PreCached_Geometry;
}


//------------------------------------------------------------------------
void CItemSystem::PrecacheLevel()
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_ACTION );
	
	IEntitySystem *pEntitySystem = gEnv->pEntitySystem;

	TItemMap::iterator it = m_items.begin();
	TItemMap::iterator end = m_items.end();
	
	for (; it!=end; ++it)
	{
		if(IEntity *pEntity = pEntitySystem->GetEntity(it->first))
		{
			CacheItemGeometry(pEntity->GetClass()->GetName());
			CacheItemSound(pEntity->GetClass()->GetName());
		}
	}
}

//------------------------------------------------------------------------
void CItemSystem::CreateItemTable(const char *name)
{
	IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
	pScriptSystem->ExecuteFile(DEFAULT_ITEM_SCRIPT);
	pScriptSystem->BeginCall(DEFAULT_ITEM_CREATEFUNC);
	pScriptSystem->PushFuncParam(name);
	pScriptSystem->EndCall();
}

//------------------------------------------------------------------------
void CItemSystem::RegisterCVars()
{
	gEnv->pConsole->Register("i_inventory_capacity", &i_inventory_capacity, 64, VF_CHEAT, "Players inventory capacity");	
	REGISTER_COMMAND("i_giveitem", (ConsoleCommandFunc)GiveItemCmd, VF_CHEAT, "Gives specified item to the player!");
	REGISTER_COMMAND("i_giveallitems", (ConsoleCommandFunc)GiveAllItemsCmd, VF_CHEAT, "Gives all available items to the player!");
	REGISTER_COMMAND("i_givedebugitems", (ConsoleCommandFunc)GiveDebugItemsCmd, VF_CHEAT, "Gives special debug items to the player!");
	REGISTER_COMMAND("i_listitems", ListItemNames, VF_CHEAT, "List all item names matching the string provided as paramater.");
	REGISTER_COMMAND("i_saveweaponposition", (ConsoleCommandFunc)SaveWeaponPositionCmd, VF_CHEAT, "Saves weapon offset");
	REGISTER_COMMAND("i_giveammo", (ConsoleCommandFunc)GiveAmmoCmd, VF_CHEAT, "Gives specified ammo to the player!");

	m_pPrecache = REGISTER_INT("i_precache", 1, VF_DUMPTODISK, "Enables precaching of items during level loading.");
	m_pItemLimit = REGISTER_INT("i_lying_item_limit", 64, 0, "Max number of items lying around in a level. Only works in multiplayer.");
}

//------------------------------------------------------------------------
void CItemSystem::UnregisterCVars()
{
	gEnv->pConsole->RemoveCommand("i_giveitem");
	gEnv->pConsole->RemoveCommand("i_giveallitems");
	gEnv->pConsole->RemoveCommand("i_givedebugitems");
	gEnv->pConsole->RemoveCommand("i_listitems");
	gEnv->pConsole->RemoveCommand("i_giveammo");
	gEnv->pConsole->UnregisterVariable("i_precache", true);
	gEnv->pConsole->UnregisterVariable("i_inventory_capacity", true);
}

//------------------------------------------------------------------------
void CItemSystem::GiveItemCmd(IConsoleCmdArgs *args)
{
	if (args->GetArgCount()<2)
		return;

	IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
	IActorSystem	*pActorSystem = pGameFramework->GetIActorSystem();
	IItemSystem		*pItemSystem = pGameFramework->GetIItemSystem();

	const char *itemName = args->GetArg(1);
	const char *actorName = 0;

	if (args->GetArgCount()>2)
		actorName = args->GetArg(2);

	IActor *pActor = 0;

	if (actorName)
	{
		IEntity *pEntity = gEnv->pEntitySystem->FindEntityByName(actorName);
		if (pEntity)
		{
			pActor = pGameFramework->GetIActorSystem()->GetActor(pEntity->GetId());
		}
	}
	else
		pActor = pGameFramework->GetClientActor();

	if (!pActor)
		return;

	//Check item name before giving (it will resolve case sensitive 'issues')
	itemName = (const char*)(pItemSystem->Query(eISQ_Find_Item_By_Name, itemName));

	if(itemName != NULL)
		pItemSystem->GiveItem(pActor, itemName, true, true, true);
	else
		GameWarning("Trying to spawn item of class '%s' which is unknown!", args->GetArg(1));
}

//------------------------------------------------------------------------
void CItemSystem::ListItemNames(IConsoleCmdArgs *args)
{
	IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
	IItemSystem		*pItemSystem = pGameFramework->GetIItemSystem();

	const char *itemName = NULL;

	if(args->GetArgCount()>0)
		itemName = args->GetArg(1);;

	pItemSystem->Query(eISQ_Dump_Item_Names, itemName);

}

//--------------------------------------------------------------------
void CItemSystem::GiveItemsHelper(IConsoleCmdArgs *args, bool useGiveable, bool useDebug)
{
	if (args->GetArgCount()<1)
		return;

	IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
	IActorSystem	*pActorSystem = pGameFramework->GetIActorSystem();
	CItemSystem		*pItemSystem = static_cast<CItemSystem *>(pGameFramework->GetIItemSystem());

	const char *actorName = 0;

	if (args->GetArgCount()>1)
		actorName = args->GetArg(1);

	IActor *pActor = 0;

	if (actorName)
	{
		IEntity *pEntity = gEnv->pEntitySystem->FindEntityByName(actorName);
		if (pEntity)
		{
			pActor = pGameFramework->GetIActorSystem()->GetActor(pEntity->GetId());
		}
	}
	else
		pActor = pGameFramework->GetClientActor();

	if (!pActor)
		return;


	for (TItemParamsMap::iterator it = pItemSystem->m_params.begin(); it != pItemSystem->m_params.end(); it++)
	{
		const char* item = it->first.c_str();
		CItemParamsNode *params = it->second.params;
		if (!params)
			continue;
		
		const IItemParamsNode *itemParams = params->GetChild("params");
		
		bool give = false;
		bool debug = false;
		if (itemParams)
		{
			int n = itemParams->GetChildCount();
			for (int i=0; i<n; i++)
			{
				const IItemParamsNode *param = itemParams->GetChild(i);
				const char *name = param->GetAttribute("name");
				if (!stricmp(name?name:"", "giveable") && useGiveable)
				{
					int val = 0; param->GetAttribute("value", val);
					give = val != 0;
				}
				if (!stricmp(name?name:"", "debug") && useDebug)
				{
					int val = 0; param->GetAttribute("value", val);
					debug = val != 0;
				}
			}
		}

		if (give || debug)
			pItemSystem->GiveItem(pActor, it->first.c_str(), false);
	}
}


//------------------------------------------------------------------------
void CItemSystem::GiveAllItemsCmd(IConsoleCmdArgs *args)
{
	GiveItemsHelper(args, true, false);
}

//------------------------------------------------------------------------
void CItemSystem::GiveDebugItemsCmd(IConsoleCmdArgs *args)
{
	GiveItemsHelper(args, false, true);
}

//------------------------------------------------------------------------
void CItemSystem::SaveWeaponPositionCmd(IConsoleCmdArgs *args)
{
	IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
	if (IActor *pActor = pGameFramework->GetClientActor())
	{
		if (IItem* pItem = pActor->GetCurrentItem())
			if (IWeapon* pWeapon = pItem->GetIWeapon())
				pWeapon->SaveWeaponPosition();
	}
}

//------------------------------------------------------------------------
void CItemSystem::GiveAmmoCmd(IConsoleCmdArgs *args)
{
	if (args->GetArgCount() < 3)
		return;

	const char *ammo = args->GetArg(1);
	int amount = atoi(args->GetArg(2));

	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo);
	if(!pClass)
		return;

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (!pActor)
		return;

	if (IInventory* pInventory = pActor->GetInventory())
		pInventory->SetAmmoCount(pClass, amount);
}

//------------------------------------------------------------------------
void CItemSystem::RegisterListener(IItemSystemListener *pListener)
{
	stl::push_back_unique(m_listeners, pListener);
}

//------------------------------------------------------------------------
void CItemSystem::UnregisterListener(IItemSystemListener *pListener)
{
	stl::find_and_erase(m_listeners, pListener);
}

void CItemSystem::Serialize(TSerialize ser)
{
	if(ser.GetSerializationTarget() != eST_Network)
	{
		if(ser.IsReading())
			m_playerLevelToLevelSave = 0;

		bool hasInventoryNode = m_playerLevelToLevelSave?true:false;
		ser.Value("hasInventoryNode", hasInventoryNode);

		if(hasInventoryNode)
		{
			if(ser.IsWriting())
			{
				IXmlStringData* xmlData = m_playerLevelToLevelSave->getXMLData(50000);
				string data( xmlData->GetString() );
				ser.Value("LTLInventoryData", data);
				xmlData->Release();
			}
			else
			{
				string data;
				ser.Value("LTLInventoryData", data);

				m_playerLevelToLevelSave = gEnv->pSystem->LoadXmlFromString( data.c_str() );
			}
		}
	}
}

void CItemSystem::SerializePlayerLTLInfo(bool bReading)
{
	if(bReading && !m_playerLevelToLevelSave)
		return;

	if(gEnv->bMultiplayer)
		return;

	if(!bReading)
		m_playerLevelToLevelSave = gEnv->pSystem->CreateXmlNode( "Inventory" );

	IXmlSerializer *pSerializer = gEnv->pSystem->GetXmlUtils()->CreateXmlSerializer();
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	ISerialize *pSer = NULL; 
	if(!bReading)
		pSer = pSerializer->GetWriter(m_playerLevelToLevelSave);
	else
	{
		pSer = pSerializer->GetReader(m_playerLevelToLevelSave);
		m_pEquipmentManager->OnBeginGiveEquipmentPack();
	}
	TSerialize ser = TSerialize(pSer);
	if(pActor)
		pActor->SerializeLevelToLevel(ser);
	pSerializer->Release();

	if (bReading)
		m_pEquipmentManager->OnEndGiveEquipmentPack();
}

//------------------------------------------------------------------------
int CItemSystem::GetItemParamsCount() const
{
	return m_params.size();
}

//------------------------------------------------------------------------
const char* CItemSystem::GetItemParamName(int index) const
{
	// FIXME: maybe return an iterator class, so get rid of advance (it's a map, argh)
	assert (index >= 0 && index < m_params.size());
	TItemParamsMap::const_iterator iter = m_params.begin();
	std::advance(iter, index);
	return iter->first.c_str();
}

//----------------------------------------------------------------------
void* CItemSystem::Query(IItemSystemQuery query, const void* param /* = NULL */)
{
	static CryFixedStringT<64> lastItemName = "";

	switch(query)
	{
		case eISQ_Dump_Item_Names:
			{
				const char* filterName = param? (const char*)(param): NULL;
				DumpItemList(filterName);
			}
			break;

		case eISQ_Find_Item_By_Name:
			{
				CryFixedStringT<32> filterName = param? (const char*)(param): "";
				if(filterName.empty())
					return NULL;

				TItemParamsMap::const_iterator cit = m_params.find(CONST_TEMP_STRING(filterName.c_str()));
				
				//If not found, make lower case and make a non-case sensitive search
				if(cit == m_params.end())
				{
					filterName.MakeLower();
					cit = m_params.begin();
					while (cit != m_params.end())
					{
						CryFixedStringT<32> itemName = cit->first.c_str();
						itemName.MakeLower();
						if(itemName == filterName)
						{
							break;
						}
						++cit;
					}
				}

				//We don't find any match
				if(cit == m_params.end())
					return NULL;

				//Found a match, return its name
				lastItemName = cit->first.c_str();
				return (void*)(lastItemName.c_str());
			}
			break;

		default:
			break;
	}

	return NULL;
}

//--------------------------------------------------------------------
void CItemSystem::DumpItemList(const char* filter)
{
	CryFixedStringT<32> filterName = "";

	if(filter != NULL)
	{
		filterName = filter;
		filterName.MakeLower();
	}

	TItemParamsMap::const_iterator cit = m_params.begin();
	if(filterName.empty())
	{
		while(cit != m_params.end())
		{
			CryLog("\t- %s", cit->first.c_str());
			++cit;	
		}
	}
	else
	{
		while(cit != m_params.end())
		{
			CryFixedStringT<32> itemName = cit->first.c_str();
			itemName.MakeLower();
			if(itemName.find(CONST_TEMP_STRING(filterName.c_str())) == 0)
			{
				CryLog("\t- %s", cit->first.c_str());
			}
			++cit;	
		}
	}

}


//----------------------------------------------------------------------
typedef struct classMemData
{
	uint32 countOfClass;
	uint32 memSize;

	classMemData():countOfClass(0),memSize(0){}
}t_classMemData;

typedef std::map<IEntityClass*,t_classMemData>		TDebugClassesMem;

void CItemSystem::DisplayItemSystemStats()
{
	const float kbInvert = 1.0f/1024.0f;

	TDebugClassesMem debugClassesMap;
	t_classMemData tempData;

	ICrySizer *pSizer = gEnv->pSystem->CreateSizer();

	GetMemoryUsage(pSizer);

	int itemSystemMem = pSizer->GetTotalSize();

	uint32 itemCount = 0;
	uint32 weaponCount = 0;

	uint32 weaponMemSize = 0;
	uint32 lastSizerSize = 0;

	TItemMap::const_iterator cit = m_items.begin();
	TItemMap::const_iterator citEnd = m_items.end();
	while(cit!=citEnd)
	{
		IItem* pItem = GetItem(cit->first);
		lastSizerSize = pSizer->GetTotalSize();
		pItem->GetMemoryUsage(pSizer);
		uint32 itemSize = (pSizer->GetTotalSize() - lastSizerSize);

		if(pItem->GetIWeapon())
		{
			itemCount++;weaponCount++;
			weaponMemSize += itemSize;
		}
		else
		{
			itemCount++;
		}

		TDebugClassesMem::iterator it = debugClassesMap.find(pItem->GetEntity()->GetClass());
		if(it!=debugClassesMap.end())
		{
			it->second.countOfClass++;
			it->second.memSize += itemSize;
		}
		else
		{
			tempData.countOfClass = 1;
			tempData.memSize = itemSize;
			debugClassesMap.insert(TDebugClassesMem::value_type(pItem->GetEntity()->GetClass(),tempData));
		}

		cit++;
	}
	float white[4] = {1.0f,1.0f,1.0f,1.0f};

	//Global
	gEnv->pRenderer->Draw2dLabel(50.0f,50.0f,1.5f,white,false,"Item system mem:		%.2f Kb.",itemSystemMem*kbInvert);

	gEnv->pRenderer->Draw2dLabel(50.0f,65.0f,1.5f,white,false,"Num. Item instances:		%d.		Total mem: %.2f Kb",itemCount,(float)(pSizer->GetTotalSize()-itemSystemMem)*kbInvert);
	gEnv->pRenderer->Draw2dLabel(50.0f,80.0f,1.5f,white,false,"Num. Weapon instances:	%d.		Total mem: %.2f Kb",weaponCount,(float)weaponMemSize*kbInvert);

	//Per item class
	TDebugClassesMem::const_iterator cit2 = debugClassesMap.begin();
	TDebugClassesMem::const_iterator citEnd2 = debugClassesMap.end();

	int i = 0;
	while(cit2!=citEnd2)
	{
		float grey[4] = {0.7f,0.7f,0.7f,1.0f};

		float midSize = (float)cit2->second.memSize/(float)cit2->second.countOfClass;
		gEnv->pRenderer->Draw2dLabel(50.0f,100.0f+(10.0f*i),1.2f,grey,false,"Class %s:	Instances: %d.	MemSize: %.2f Kb.	Instance Size:%.3f Kb",cit2->first->GetName(),cit2->second.countOfClass,(float)cit2->second.memSize*kbInvert,midSize*kbInvert);

		cit2++;i++;
	}

	pSizer->Release();
}

//-----------------------------------------------------------------------
template <class T>
static int AddFactoryMapTo( const std::map<string, T>& m )
{
	int nSize = 0;
	if (!m.empty())
	{
		int nStringSize = 0;
		for (typename std::map<string, T>::const_iterator iter = m.begin(); iter != m.end(); ++iter)
		{
			nStringSize += iter->first.length();
		}
		nSize = m.size()*(sizeof(typename std::map<string, T>::value_type));
	}
	return nSize;
}
void CItemSystem::GetMemoryUsage(ICrySizer * pSizer) const
{
	SIZER_COMPONENT_NAME(pSizer, "ItemSystem");

	pSizer->AddObject( this, sizeof(*this) );
	//pSizer->AddObject(m_objectCache);
	pSizer->AddObject(m_characterCache);
	pSizer->AddObject(m_classes);
	pSizer->AddObject(m_params );
	pSizer->AddObject(m_items);
	pSizer->AddObject(m_listeners);	
	pSizer->AddObject(m_folders);	
	//pSizer->AddObject(m_pEquipmentManager);
	pSizer->AddObject(m_grenadeType);	
}

uint32 CItemSystem::GetItemSocketCount(const char* item) const
{
	const IItemParamsNode *root = GetItemParams(item);
	if (!root)
		return 0;

	const IItemParamsNode *sockets = root->GetChild("sockets");
	if (!sockets)
		return 0;

	int count = 0;
	for (int i=0; i<sockets->GetChildCount(); ++i)
	{
		const char* name = sockets->GetChildName(i);
		if (name && !stricmp(name, "socket"))
			++count;
	}
	return count;
}

const char* CItemSystem::GetItemSocketName(const char* item, int idx) const
{
	const IItemParamsNode *root = GetItemParams(item);
	if (!root)
		return NULL;

	const IItemParamsNode *sockets = root->GetChild("sockets");
	if (!sockets)
		return NULL;

	return sockets->GetChild(idx)->GetAttribute("name");
}

bool CItemSystem::IsCompatible(const char* item, const char* attachment) const
{
	const IItemParamsNode *rootItem = GetItemParams(item);
	if (!rootItem)
		return false;

	const IItemParamsNode *sockets = rootItem->GetChild("sockets");
	if (!sockets)
		return false;

	const IItemParamsNode *rootAtt = GetItemParams(attachment);
	if (!rootAtt)
		return false;

	const IItemParamsNode *types = rootAtt->GetChild("types");
	if (!types)
		return false;

	for (int i=0; i<sockets->GetChildCount(); ++i)
	{
		const IItemParamsNode *socket = sockets->GetChild(i);
		for (int j=0; j<socket->GetChildCount(); ++j)
		{
			const char* socketName = socket->GetChild(j)->GetAttribute("type");
			if (socketName)
				for (int k=0; k<types->GetChildCount(); ++k)
				{
					const char* typeName = types->GetChild(k)->GetAttribute("name");
					if (typeName && !stricmp(socketName, typeName))
						return true;
				}
		}
	}
	return false;
}

bool CItemSystem::GetItemSocketCompatibility(const char* item, const char* socket) const
{
	const IItemParamsNode *itemAtt = GetItemParams(item);
	if (!itemAtt)
		return false;

	const IItemParamsNode *itemTypes = itemAtt->GetChild("types");
	if (!itemTypes)
		return false;

	std::set<string> itemSockets;
	for (int i=0; i<itemTypes->GetChildCount(); ++i)
	{
		const char* socketName = itemTypes->GetChild(i)->GetAttribute("name");
		itemSockets.insert( socketName );
	}

	for ( TItemParamsMap::const_iterator it = m_params.begin(), eIt = m_params.end(); it != eIt;  ++it )
	{
		if ( it->first == item )
			continue;

		if (const IItemParamsNode *itemParams= it->second.params)
		{
			if ( const IItemParamsNode *socketsNode = itemParams->GetChild("sockets") )
			{
				for ( int i = 0; i < socketsNode->GetChildCount(); ++i )
				{
					const IItemParamsNode *childNode = socketsNode->GetChild(i);
					if ( const char* childName = childNode->GetAttribute("name") )
						if ( CONST_TEMP_STRING(childName) == socket )
							for ( int j = 0; j < childNode->GetChildCount(); ++j )
								if ( const IItemParamsNode *innerNode = childNode->GetChild(j) )
									if ( const char* supportType = innerNode->GetAttribute("type") )
										if ( itemSockets.find(supportType) != itemSockets.end() )
											return true;
				}
			}
		}
	}
	
	return false;
}

bool CItemSystem::CanSocketBeEmpty(const char* item, const char* socket) const
{
	const IItemParamsNode *root = GetItemParams(item);
	if (!root)
		return true;

	const IItemParamsNode *sockets = root->GetChild("sockets");
	if (!sockets)
		return true;

	for (int i=0; i<sockets->GetChildCount(); ++i)
	{
		const char* socketName = sockets->GetChild(i)->GetAttribute("name");
		if (socketName && !stricmp(socketName, socket))
		{
			int canBeEmpty = 1;
			sockets->GetChild(i)->GetAttribute("can_be_empty", canBeEmpty);
			return canBeEmpty!=0;
		}
	}
	return true;
}


void CItemSystem::ItemSystemErrorMessage( const char* fileName, const char* errorInfo, bool displayErrorDialog )
{
	if ((fileName == NULL) || (errorInfo == NULL))
		return;

	CryFixedStringT<1024> messageBuffer;
	messageBuffer.Format("Failed to load '%s'. Required data missing, which could lead to un-expected game behavior or crashes. ( %s )", fileName, errorInfo);

	CryLogAlways(messageBuffer.c_str());

	if (displayErrorDialog)
	{
		CryMessageBox( messageBuffer.c_str(), "Error", 0);
	}
}