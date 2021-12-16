/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 23:8:2004   15:52 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ActorSystem.h"
#include "Network/GameServerChannel.h"
#include "Network/GameServerNub.h"
#include "CryPath.h"
#include "CryAction.h"

//------------------------------------------------------------------------
void CActorSystem::DemoSpectatorSystem::SwitchSpectator(TActorMap *pActors, EntityId id)
{
	if(pActors->size() <= 1) 
		return;	

	IActor *nextActor, *lastActor;

	lastActor = (*pActors)[m_currentActor];

	if(id == 0)
	{
		TActorMap::const_iterator it = pActors->begin();
		while(it->first != m_currentActor)
			++it;
		++it;
		if(it == pActors->end())
			it = pActors->begin();
		nextActor = it->second;
	}
	else
	{
		nextActor = (*pActors)[id];
		if(!nextActor)
			return;
	}

	if(nextActor == lastActor)
		return;

	m_currentActor = nextActor->GetEntityId();
	if(IView* view = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView())
		view->LinkTo(nextActor->GetEntity());

	nextActor->SwitchDemoModeSpectator(true);
	lastActor->SwitchDemoModeSpectator(false);

	gEnv->pSoundSystem->SetListenerEntity( LISTENERID_STANDARD, m_currentActor );
}

//------------------------------------------------------------------------
CActorSystem::CActorSystem(ISystem *pSystem, IEntitySystem *pEntitySystem)
: m_pSystem(pSystem),
	m_pEntitySystem(pEntitySystem)
{
	m_demoPlaybackMappedOriginalServerPlayer = 0;
}

// Iterators now have their destructors called before they enter the pool - so we only need to free the memory here {2008/12/09}
void DeleteActorIterator(IActorIterator *ptr) { operator delete(ptr); }

//------------------------------------------------------------------------
CActorSystem::~CActorSystem()
{
	std::for_each(m_iteratorPool.begin(), m_iteratorPool.end(), DeleteActorIterator);
	// delete the created userdata in each class
	for (TActorClassMap::iterator it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		IEntityClass *pClass = m_pEntitySystem->GetClassRegistry()->FindClass(it->first.c_str());
		if( pClass )
			delete (char*)pClass->GetUserProxyData();
	}
}

void CActorSystem::Reset()
{
	if(GetISystem()->IsSerializingFile() == 1)
	{
		TActorMap::iterator it = m_actors.begin();
		IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
		for (; it != m_actors.end(); ++it)
		{
			EntityId id = it->first;
			IEntity *pEntity = pEntitySystem->GetEntity(id);
			if(!pEntity)
			{
				m_actors.erase(it);
				--it;
			}
		}
	}
}

//------------------------------------------------------------------------
IActor *CActorSystem::GetActor(EntityId entityId)
{
	TActorMap::iterator it = m_actors.find(entityId);

	if (it != m_actors.end())
	{
		return it->second;
	}

	return 0;
}

//------------------------------------------------------------------------
IActor *CActorSystem::GetActorByChannelId(uint16 channelId)
{
	for (TActorMap::iterator it = m_actors.begin(); it != m_actors.end(); it++)
	{
		if (it->second->GetGameObject()->GetChannelId() == channelId)
		{
			return it->second;
		}
	}

	return 0;
}

//------------------------------------------------------------------------
IActor *CActorSystem::CreateActor(uint16 channelId, const char *name, const char *actorClass, const Vec3 &pos, const Quat &rot, const Vec3 &scale, EntityId id)
{
	// get the entity class
	IEntityClass *pEntityClass = m_pEntitySystem->GetClassRegistry()->FindClass(actorClass);

	if (!pEntityClass)
	{
		CRY_ASSERT(pEntityClass);

		return 0;
	}

	// if a channel is specified and already has a player,
	// use that entity id

	if (channelId)
	{
		if (CGameServerNub * pGameServerNub = CCryAction::GetCryAction()->GetGameServerNub())
			if (CGameServerChannel * pGameServerChannel = pGameServerNub->GetChannel(channelId))
				if (pGameServerChannel->GetNetChannel()->IsLocal())
				{
					id = LOCAL_PLAYER_ENTITY_ID;
					if(IsDemoPlayback()) //if playing a demo - spectator id is changed
						m_demoSpectatorSystem.m_currentActor = m_demoSpectatorSystem.m_originalActor = id;
				}
	}

	IGameObjectSystem::SEntitySpawnParamsForGameObjectWithPreactivatedExtension userData;
	userData.hookFunction = HookCreateActor;
	userData.pUserData = &channelId;

	SEntitySpawnParams params;
	params.id = id;
	params.pUserData = (void *)&userData;
	params.sName = name;
	params.vPosition = pos;
	params.qRotation = rot;
	params.vScale = scale;
	params.nFlags = ENTITY_FLAG_TRIGGER_AREAS;
	if (channelId)
		params.nFlags |= ENTITY_FLAG_NEVER_NETWORK_STATIC;
	params.pClass = pEntityClass;

	IEntity *pEntity = m_pEntitySystem->SpawnEntity(params, true);
	CRY_ASSERT(pEntity);

	if (!pEntity)
	{
		return 0;
	}

	return GetActor(pEntity->GetId());
}

//------------------------------------------------------------------------
void CActorSystem::SetDemoPlaybackMappedOriginalServerPlayer(EntityId id)
{
	m_demoPlaybackMappedOriginalServerPlayer = id;
}

EntityId CActorSystem::GetDemoPlaybackMappedOriginalServerPlayer() const
{
	return m_demoPlaybackMappedOriginalServerPlayer;
}

//------------------------------------------------------------------------
void CActorSystem::SwitchDemoSpectator(EntityId id)
{
	m_demoSpectatorSystem.SwitchSpectator(&m_actors, id);
}

//------------------------------------------------------------------------
IActor* CActorSystem::GetCurrentDemoSpectator()
{
	return m_actors[m_demoSpectatorSystem.m_currentActor];
}

//------------------------------------------------------------------------
IActor* CActorSystem::GetOriginalDemoSpectator()
{
	return m_actors[m_demoSpectatorSystem.m_originalActor];
}

//------------------------------------------------------------------------
void CActorSystem::RegisterActorClass(const char *name, IGameFramework::IActorCreator * pCreator, bool isAI)
{
	IEntityClassRegistry::SEntityClassDesc actorClass;

	char scriptName[1024] = {0};
	if(!isAI)
		sprintf(scriptName, "Scripts/Entities/Actor/%s.lua", name);
	else
		sprintf(scriptName, "Scripts/Entities/AI/%s.lua", name);

	// Allow the name to contain relative path, but use only the name part as class name.
	string	className(PathUtil::GetFile(name));
	actorClass.sName = className.c_str();
	actorClass.sScriptFile = scriptName;
	if(!isAI)
		actorClass.flags |= ECLF_INVISIBLE;

	CCryAction::GetCryAction()->GetIGameObjectSystem()->RegisterExtension(className.c_str(), pCreator, &actorClass);

	m_classes.insert(TActorClassMap::value_type(name, pCreator));
}

//------------------------------------------------------------------------
void CActorSystem::AddActor(EntityId entityId, IActor *pProxy)
{
	m_actors.insert(TActorMap::value_type(entityId, pProxy));
}

//------------------------------------------------------------------------
void CActorSystem::RemoveActor(EntityId entityId)
{
	stl::member_find_and_erase(m_actors, entityId);
}

//------------------------------------------------------------------------
bool CActorSystem::HookCreateActor( IEntity *pEntity, IGameObject *pGameObject, void *pUserData )
{
	pGameObject->SetChannelId( *static_cast<uint16*>(pUserData) );
	return true;
}

//---------------------------------------------------------------------
void CActorSystem::Reload()
{
	Reset();
	Scan(m_actorParamsFolder.c_str());
}

//---------------------------------------------------------------------
void CActorSystem::Scan(const char *folderName)
{
	stack_string folder = folderName;
	stack_string search = folder;
	stack_string subName;
	stack_string xmlFile;
	search += "/*.*";

	ICryPak *pPak = gEnv->pCryPak;

	_finddata_t fd;
	intptr_t handle = pPak->FindFirst(search.c_str(), &fd);
	//Scan only one directory, not recursion supported (add it if need it)
	if (handle > -1)
	{
		do
		{
			if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
				continue;

			const char* fileExtension = PathUtil::GetExt(fd.name);
			if (stricmp(fileExtension, "xml"))
			{
				GameWarning("ActorSystem: File '%s' does not have 'xml' extension, skipping.", fd.name );
				continue;
			}

			xmlFile = folder + "/" + fd.name;
			XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFile(xmlFile.c_str());

			if (!rootNode)
			{
				ActorSystemErrorMessage(xmlFile.c_str(), "Root xml node couldn't be loaded", true);
				continue;
			}

			if (!ScanXML(rootNode, xmlFile.c_str()))
			{
				continue;
			}

		} while (pPak->FindNext(handle, &fd) >= 0);
	}

	m_actorParamsFolder = folderName;
}

//--------------------------------------------------------------------
bool CActorSystem::ScanXML(XmlNodeRef &root, const char *xmlFile)
{
	if (strcmpi(root->getTag(), "ActorParams"))
	{
		CryFixedStringT<128> errorBuffer;
		errorBuffer.Format("Root tag is '%s', expecting 'ActorParams', Skipping...");
		ActorSystemErrorMessage(xmlFile, errorBuffer.c_str(), true);
		return false;
	}

	const char *type = root->getAttr("type");
	if (!type)
	{
		ActorSystemErrorMessage(xmlFile, "Actor params file does not contain attribute 'type'! Skipping...", true);
		return false;
	}

	TActorParamsMap::iterator dit = m_actorParams.find(CONST_TEMP_STRING(type));

	//Insert only if new, might be reloading...
	if (dit == m_actorParams.end())
	{
		std::pair<TActorParamsMap::iterator, bool> result = m_actorParams.insert(TActorParamsMap::value_type(type, SActorParamsDesc()));
		dit = result.first;
	}

	SActorParamsDesc &desc = dit->second;

	if (desc.params)
		desc.params->Release();

	desc.params = new CItemParamsNode();
	desc.params->ConvertFromXML(root);

	return true;
}

//-----------------------------------------------------------------
const IItemParamsNode *CActorSystem::GetActorParams(const char *actorClass) const
{
	TActorParamsMap::const_iterator it = m_actorParams.find(CONST_TEMP_STRING(actorClass));
	if (it != m_actorParams.end())
	{
		return it->second.params;
	}

	GameWarning("Trying to get xml description for actor of type '%s'!", actorClass);
	return 0;
}

//------------------------------------------------------------------------
#if 0
IEntityProxy *CActorSystem::CreateActor(IEntity *pEntity, SEntitySpawnParams &params, void *pUserData)
{
	/*
	SActorUserData *pActorUserData = reinterpret_cast<SActorUserData *>(pUserData);
	string className = pActorUserData->className;

	CActorSystem *pActorSystem = reinterpret_cast<CActorSystem *>(pActorUserData->pActorSystem);
	CRY_ASSERT(pActorSystem);

	const SSpawnUserData * pSpawnUserData = static_cast<const SSpawnUserData *>(params.pUserData);

	TActorClassMap::iterator it = pActorSystem->m_classes.find(className);

	if (it == pActorSystem->m_classes.end())
	{
		GameWarning( "Unknown Actor class '%s'", className );

		return 0;
	}

	IActor *pActor = (*it->second)();
	CActor *pActor = new CActor(pActorSystem, className.c_str(), pEntity, pActor);
	CRY_ASSERT(pActor);
	*/

	pActor->GetGameObject()->SetChannelId( pSpawnUserData ? pSpawnUserData->channelId : 0);
	
	//pEntity->Activate(true);

	//pActorSystem->AddActor(pEntity->GetId(), pActor);

	return pActor;
}
#endif

IActorIteratorPtr CActorSystem::CreateActorIterator()
{
	if (m_iteratorPool.empty())
	{
		return new CActorIterator(this);
	}
	else
	{
		CActorIterator* pIter = m_iteratorPool.back();
		m_iteratorPool.pop_back();
		new (pIter) CActorIterator(this);
		return pIter;
	}
}

void CActorSystem::ActorSystemErrorMessage( const char* fileName, const char* errorInfo, bool displayErrorDialog )
{
	if ((fileName == NULL) || (errorInfo == NULL))
		return;

	CryFixedStringT<1024> messageBuffer;
	messageBuffer.Format("ERROR: Failed to load '%s'. Required data missing, which could lead to un-expected game behavior or crashes. ( %s )", fileName, errorInfo);

	CryLogAlways(messageBuffer.c_str());

	if (displayErrorDialog)
	{
		CryMessageBox( messageBuffer.c_str(), "Error", 0);
	}
}

void CActorSystem::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "ActorSystem");

	s->AddContainer(m_actors);
	s->AddContainer(m_classes);
	s->AddContainer(m_iteratorPool);
	for (size_t i=0; i<m_iteratorPool.size(); i++)
		s->Add(*m_iteratorPool[i]);

	int thisSize = sizeof(*this);

	int paramsSize = 0;
	for (TActorParamsMap::iterator iter = m_actorParams.begin(); iter != m_actorParams.end(); ++iter)
	{
		if (iter->second.params)
			iter->second.params->GetMemoryUsage(s);
	}

	s->AddObject(this, (thisSize + paramsSize));
}

