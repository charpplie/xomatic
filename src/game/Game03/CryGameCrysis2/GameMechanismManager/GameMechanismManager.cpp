#include "StdAfx.h"
#include "GameMechanismManager.h"
#include "GameMechanismBase.h"
#include "Utility/CryWatch.h"

CGameMechanismManager * CGameMechanismManager::s_instance = NULL;

#if defined(_RELEASE)
#define MechanismManagerLog(...)    (void)(0)
#define MechanismManagerWatch(...)  (void)(0)
#else
#define MechanismManagerLog(...)    do { if(m_cvarLogEnabled) CryLogAlways("[CGameMechanismManager] " __VA_ARGS__); } while(0)
#define MechanismManagerWatch(...)  do { if(m_cvarWatchEnabled) CryWatch("[CGameMechanismManager] " __VA_ARGS__); } while(0)
#endif

CGameMechanismManager::CGameMechanismManager()
{
	MechanismManagerLog ("Manager created");

	m_firstMechanism = NULL;

#if !defined(_RELEASE)
	m_cvarWatchEnabled = false;
	m_cvarLogEnabled = true;

	IConsole * console = GetISystem()->GetIConsole();
	assert (console);

	console->Register("g_mechanismMgrWatch", & m_cvarWatchEnabled, m_cvarWatchEnabled, 0, "MECHANISM MANAGER: On-screen watches enabled");
	console->Register("g_mechanismMgrLog", & m_cvarLogEnabled, m_cvarLogEnabled, 0, "MECHANISM MANAGER: Log messages enabled");
#endif

	assert (s_instance == NULL);
	s_instance = this;
}

CGameMechanismManager::~CGameMechanismManager()
{
	MechanismManagerLog ("Unregistering all remaining mechanisms:");

	while (m_firstMechanism)
	{
		delete m_firstMechanism;
	};

	MechanismManagerLog ("No game mechanisms remaining; game mechanism manager destroyed");

#if !defined(_RELEASE)
	IConsole * console = GetISystem()->GetIConsole();
	assert (console);

	console->UnregisterVariable("g_mechanismMgrWatch", true);
	console->UnregisterVariable("g_mechanismMgrLog", true);
#endif

	assert (s_instance == this);
	s_instance = NULL;
}

void CGameMechanismManager::RegisterMechanism(CGameMechanismBase * mechanism)
{
	MechanismManagerLog ("Registering %p: %s", mechanism, mechanism->GetName());

	if (m_firstMechanism)
	{
		CGameMechanismBase::SLinkedListPointers * oldFirstPointers = m_firstMechanism->GetLinkedListPointers();
		assert (oldFirstPointers->m_prevMechanism == NULL);
		oldFirstPointers->m_prevMechanism = mechanism;
	}

	CGameMechanismBase::SLinkedListPointers * newFirstPointers = mechanism->GetLinkedListPointers();
	newFirstPointers->m_nextMechanism = m_firstMechanism;
	m_firstMechanism = mechanism;
}

void CGameMechanismManager::UnregisterMechanism(CGameMechanismBase * removeThis)
{
	CGameMechanismBase::SLinkedListPointers * removeThisPointers = removeThis->GetLinkedListPointers();
	MechanismManagerLog ("Unregistering %p: %s", removeThis, removeThis->GetName());

	if (removeThisPointers->m_nextMechanism)
	{
		CGameMechanismBase::SLinkedListPointers * afterPointers = removeThisPointers->m_nextMechanism->GetLinkedListPointers();
		assert (afterPointers->m_prevMechanism == removeThis);
		afterPointers->m_prevMechanism = removeThisPointers->m_prevMechanism;
	}

	if (removeThisPointers->m_prevMechanism)
	{
		CGameMechanismBase::SLinkedListPointers * beforePointers = removeThisPointers->m_prevMechanism->GetLinkedListPointers();
		assert (beforePointers->m_nextMechanism == removeThis);
		assert (m_firstMechanism != removeThis);
		beforePointers->m_nextMechanism = removeThisPointers->m_nextMechanism;
	}
	else
	{
		assert (m_firstMechanism == removeThis);
		m_firstMechanism = removeThisPointers->m_nextMechanism;
	}
}

void CGameMechanismManager::Update(float dt)
{
	for (CGameMechanismBase * eachMechanism = m_firstMechanism; eachMechanism; eachMechanism = eachMechanism->GetLinkedListPointers()->m_nextMechanism)
	{
		MechanismManagerWatch ("Updating %s", eachMechanism->GetName());
		eachMechanism->Update(dt);
	}
}