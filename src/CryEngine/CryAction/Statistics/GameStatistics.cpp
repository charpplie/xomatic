#include "StdAfx.h"
#include "GameStatistics.h"
#include "GameUtils.h"
#include "ScriptBind_GameStatistics.h"
#include "IGameRulesSystem.h"
#include "CryAction.h"
#include "CryActionCVars.h"
#include "StatsSizer.h"

static const int MAX_SAVE_ATTEMPTS = 100;

//////////////////////////////////////////////////////////////////////////
// CGameScope
//////////////////////////////////////////////////////////////////////////

CGameScopes::CGameScopes()
{
}

bool CGameScopes::RegisterGameScopes(const SGameScopeDesc scopeDescs[], size_t numScopes)
{
	CRY_ASSERT(scopeDescs);
	CRY_ASSERT(numScopes > 0);

	if (!ValidateScopes(scopeDescs, numScopes))
		return false;

	GrowRegistry(numScopes);
	InsertScopesInRegistry(scopeDescs, numScopes);

	return true;
}

CStatsTracker* CGameScopes::PushGameScope(size_t scopeID, uint32 timestamp, CGameStatistics* gameStatistics /* would be good to get rid of this somehow */)
{
	CRY_ASSERT(scopeID < m_scopeRegistry.size());
	if(scopeID >= m_scopeRegistry.size())
		return 0;

	// Check that same scope isn't already on the stack
	size_t pos = FindScopePos(scopeID);
	CRY_ASSERT_MESSAGE(pos == m_scopeStack.size(), "Same scope can't be pushed twice");
	if(pos != m_scopeStack.size())
		return 0;

	SNodeLocator locator(scopeID);
	locator.timeStamp = timestamp;
	CStatsTracker* tracker = new CStatsTracker(locator, gameStatistics);
	m_scopeStack.push_back(SScopeData(locator, tracker));

	return tracker;
}

void CGameScopes::PopGameScope(size_t checkScopeID)
{
}

size_t CGameScopes::FindScopePos(size_t scopeID) const
{
	size_t size = m_scopeStack.size();
	for(size_t i = 0; i < size; ++i)
	{
		if(m_scopeStack[i].locator.scopeID == scopeID)
		{
			return i;
		}
	}
	return size;
}

size_t CGameScopes::Count() const
{
	return m_scopeRegistry.size();
}

size_t CGameScopes::GetStackSize() const
{
	return m_scopeStack.size();
}

const SScopeData& CGameScopes::GetLastScopeDataInStack() const
{
	return m_scopeStack.back();
}

bool CGameScopes::ValidateScopes(const SGameScopeDesc scopeDescs[], size_t numScopes)
{
	const size_t firstFreeID = m_scopeRegistry.size();

	std::set<const char*, stl::less_strcmp<const char*> > scriptNames;

	for(size_t i = 0; i != numScopes; ++i)
	{
		if (!scopeDescs[i].IsValid())
			return false;

		if (scopeDescs[i].statID < firstFreeID || scopeDescs[i].statID >= (firstFreeID + numScopes))
			return false;

		if (m_scopeMap.find(scopeDescs[i].scriptName) != m_scopeMap.end()) // name collision
			return false;

		if(!scriptNames.insert(scopeDescs[i].scriptName.c_str()).second)
			return false;
	}

	return true;
}

void CGameScopes::GrowRegistry(size_t numScopes)
{
	m_scopeRegistry.resize(m_scopeRegistry.size() + numScopes);
}

void CGameScopes::InsertScopesInRegistry(const SGameScopeDesc scopeDescs[], size_t numScopes)
{
	for(size_t i = 0; i != numScopes; ++i)
	{
		CRY_ASSERT(m_scopeRegistry[scopeDescs[i].statID].scriptName.empty());

		m_scopeRegistry[scopeDescs[i].statID] = scopeDescs[i];

		m_scopeMap.insert(std::make_pair(scopeDescs[i].scriptName, &m_scopeRegistry[scopeDescs[i].statID]));
	}
}

//////////////////////////////////////////////////////////////////////////
// CStatRegistry
//////////////////////////////////////////////////////////////////////////

bool CStatRegistry::Register(const SGameStatDesc statDescs[], size_t numStats)
{
	CRY_ASSERT(statDescs);
	if(!numStats) return true;

	const size_t firstFreeID = m_statRegistry.size();

	if(!ValidateRegistration(statDescs, numStats))
		return false;

	// Copy new events to registry and map
	m_statRegistry.resize(firstFreeID + numStats);

	for(size_t i = 0; i != numStats; ++i)
	{
		// Check for overwrite
		CRY_ASSERT(m_statRegistry[statDescs[i].statID].scriptName.empty());

		m_statRegistry[statDescs[i].statID] = statDescs[i];

		m_statMap.insert(std::make_pair(statDescs[i].scriptName, &m_statRegistry[statDescs[i].statID]));
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

size_t CStatRegistry::Register(const char* scriptName, const char* serializeName)
{
	SGameStatDesc newStat(m_statRegistry.size(), scriptName, serializeName);

	if(!ValidateRegistration(&newStat, 1))
		return INVALID_STAT_ID;

	m_statRegistry.push_back(newStat);
	m_statMap.insert(std::make_pair(scriptName, &m_statRegistry.back()));

	return m_statRegistry.back().statID;
}

//////////////////////////////////////////////////////////////////////////

size_t CStatRegistry::Count() const
{
	return m_statRegistry.size();
}

//////////////////////////////////////////////////////////////////////////

size_t CStatRegistry::GetID(const char* scriptName) const
{
	CRY_ASSERT(scriptName);

	TStatMap::const_iterator it = m_statMap.find(scriptName);
	return (it == m_statMap.end()) ? 
		INVALID_STAT_ID :
		it->second->statID;
}

//////////////////////////////////////////////////////////////////////////

const SGameStatDesc* CStatRegistry::GetDesc(size_t statID) const
{
	return (statID >= m_statRegistry.size()) ?
		0 :
		&m_statRegistry[statID];
}

//////////////////////////////////////////////////////////////////////////

bool CStatRegistry::ValidateRegistration(const SGameStatDesc *statDescs, size_t numStats)
{
	const size_t firstFreeID = m_statRegistry.size();

	std::set<const char*, stl::less_strcmp<const char*> > scriptNames;

	for(size_t i = 0; i != numStats; ++i)
	{
		if(!statDescs[i].IsValid())
			return false;

		if(statDescs[i].statID < firstFreeID || statDescs[i].statID >= (firstFreeID + numStats)) // bad id
			return false;

		if((m_statMap.find(statDescs[i].scriptName) != m_statMap.end()) ) // name collision
			return false;

		if(!scriptNames.insert(statDescs[i].scriptName.c_str()).second)
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

void CStatRegistry::GetMemoryStatistics(ICrySizer *pSizer)const
{
	pSizer->AddContainer(m_statRegistry);
	pSizer->AddContainer(m_statMap);
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CElemRegistry
//////////////////////////////////////////////////////////////////////////

bool CElemRegistry::Register(const SGameElementDesc elemDescs[], size_t numElems)
{
	CRY_ASSERT(elemDescs);
	if(!numElems) return true;

	const size_t firstFreeID = m_elemRegistry.size();

	if(!ValidateRegistration(elemDescs, numElems))
		return false;

	// Copy new events to registry and map
	m_elemRegistry.resize(firstFreeID + numElems);

	for(size_t i = 0; i != numElems; ++i)
	{
		// Check for overwrite
		CRY_ASSERT(m_elemRegistry[elemDescs[i].statID].scriptName.empty());

		m_elemRegistry[elemDescs[i].statID] = elemDescs[i];

		m_elemMap.insert(std::make_pair(elemDescs[i].scriptName, &m_elemRegistry[elemDescs[i].statID]));
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

size_t CElemRegistry::Count() const
{
	return m_elemRegistry.size();
}

//////////////////////////////////////////////////////////////////////////

size_t CElemRegistry::GetID(const char* scriptName) const
{
	CRY_ASSERT(scriptName);

	TElemMap::const_iterator it = m_elemMap.find(scriptName);
	return (it == m_elemMap.end()) ? 
		INVALID_STAT_ID :
		it->second->statID;
}

//////////////////////////////////////////////////////////////////////////

const SGameElementDesc* CElemRegistry::GetDesc(size_t statID) const
{
	return (statID >= m_elemRegistry.size()) ?
		0 :
		&m_elemRegistry[statID];
}

//////////////////////////////////////////////////////////////////////////

bool CElemRegistry::ValidateRegistration(const SGameElementDesc *elemDescs, size_t numElems)
{
	const size_t firstFreeID = m_elemRegistry.size();

	std::set<const char*, stl::less_strcmp<const char*> > scriptNames;

	for(size_t i = 0; i != numElems; ++i)
	{
		if(!elemDescs[i].IsValid())
			return false;

		if(elemDescs[i].statID < firstFreeID || elemDescs[i].statID >= (firstFreeID + numElems)) // bad id
			return false;

		if((m_elemMap.find(elemDescs[i].scriptName) != m_elemMap.end()) ) // name collision
			return false;

		if(!scriptNames.insert(elemDescs[i].scriptName.c_str()).second)
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

void CElemRegistry::GetMemoryStatistics(ICrySizer *pSizer)const
{
	pSizer->AddContainer(m_elemRegistry);
	pSizer->AddContainer(m_elemMap);
}

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// CGameStatistics
//////////////////////////////////////////////////////////////////////////

CGameStatistics::CGameStatistics()
: m_gsCallback(0)
, m_memoryLimit((size_t)-1)
, m_currTimeStamp(0)
, m_cachedMemUsage(0)
{
	// Register common game events
	SGameStatDesc commonEvents[eSE_Num] = {
		GAME_STAT_DESC(eSE_Kill,			"kill"			),
		GAME_STAT_DESC(eSE_Score,			"score"			),
		GAME_STAT_DESC(eSE_Shot,			"shot"			),
		GAME_STAT_DESC(eSE_Throw,			"throw"			),
		GAME_STAT_DESC(eSE_Hit,				"hit"				),
		GAME_STAT_DESC(eSE_Activate,	"activate"	),
		GAME_STAT_DESC(eSE_Explode,		"explode"		),
		GAME_STAT_DESC(eSE_Death,			"death"			),
		GAME_STAT_DESC(eSE_Reload,		"reload"		),
		GAME_STAT_DESC(eSE_Position,	"position"	),
		GAME_STAT_DESC(eSE_Health,		"health"		),
		GAME_STAT_DESC(eSE_Stamina,		"stamina"		),
		GAME_STAT_DESC(eSE_LookDir,		"lookdir"		),
		GAME_STAT_DESC(eSE_Weapon,		"weapon"		),
		GAME_STAT_DESC(eSE_Consume,		"consume"		),
		GAME_STAT_DESC(eSE_Connect,		"connect"		),
		GAME_STAT_DESC(eSE_Disconnect,"disconnect"),
		GAME_STAT_DESC(eSE_TeamChange,"team_change"),
		GAME_STAT_DESC(eSE_Lifetime,	"lifetime"	),
		GAME_STAT_DESC(eSE_Damage,		"damage"		),
		GAME_STAT_DESC(eSE_Action,		"action"		),
		GAME_STAT_DESC(eSE_Enable,		"enable"		),
	};
	RegisterGameEvents(commonEvents, eSE_Num);


	SGameStatDesc commonStates[eSS_Num] = {
		GAME_STAT_DESC(eSS_GameSettings,	"game_settings"	),
		GAME_STAT_DESC(eSS_Map,						"map"						),
		GAME_STAT_DESC(eSS_Gamemode,			"gamemode"			),
		GAME_STAT_DESC(eSS_Team,					"team"					),
		GAME_STAT_DESC(eSS_Winner,				"winner"				),
		GAME_STAT_DESC(eSS_Weapons,				"weapons"				),
		GAME_STAT_DESC(eSS_Ammos,					"ammos"					),
		GAME_STAT_DESC(eSS_PlayerName,		"name"					),
		GAME_STAT_DESC(eSS_ProfileId,			"profile_id"		),
		GAME_STAT_DESC(eSS_PlayerInfo,		"player_info"		),
		GAME_STAT_DESC(eSS_EntityId,			"entity_id"			),
		GAME_STAT_DESC(eSS_Kind,					"kind"					),
		GAME_STAT_DESC(eSS_TriggerParams,	"trigger_params"),
		GAME_STAT_DESC(eSS_Score,					"score"					),
	};
	RegisterGameStates(commonStates, eSS_Num);
}

CGameStatistics::~CGameStatistics()
{
	CRY_ASSERT_MESSAGE(!m_gameScopes.m_scopeStack.size(), "Stack should be empty now");

	// Remove all data left on the stack and do clean up
	while(m_gameScopes.m_scopeStack.size())
		PopGameScope(INVALID_STAT_ID);
}

//////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////

bool CGameStatistics::RegisterGameEvents(const SGameStatDesc *eventDescs, size_t numEvents)
{
	return m_eventRegistry.Register(eventDescs, numEvents);
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::RegisterGameEvent(const char* scriptName, const char* serializeName)
{
	return m_eventRegistry.Register(scriptName, serializeName);
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetEventCount() const
{
	return m_eventRegistry.Count();
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetEventID(const char* scriptName) const
{
	return m_eventRegistry.GetID(scriptName);
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetEventIDBySerializeName(const char* serializeName) const
{
	for(size_t index = 0; index < m_eventRegistry.Count(); ++index)
	{
		const SGameStatDesc* pDesc = m_eventRegistry.GetDesc(index);
		if(pDesc && !strcmp(serializeName, pDesc->serializeName))
			return index;
	}
	
	return INVALID_STAT_ID;
}

//////////////////////////////////////////////////////////////////////////

const SGameStatDesc* CGameStatistics::GetEventDesc(size_t eventID) const
{
	return m_eventRegistry.GetDesc(eventID);
}

//////////////////////////////////////////////////////////////////////////
// States
//////////////////////////////////////////////////////////////////////////

bool CGameStatistics::RegisterGameStates(const SGameStatDesc *stateDescs, size_t numStates)
{
	return m_stateRegistry.Register(stateDescs, numStates);
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::RegisterGameState(const char* scriptName, const char* serializeName)
{
	return m_stateRegistry.Register(scriptName, serializeName);
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetStateCount() const
{
	return m_stateRegistry.Count();
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetStateID(const char* scriptName) const
{
	return m_stateRegistry.GetID(scriptName);
}

//////////////////////////////////////////////////////////////////////////

const SGameStatDesc* CGameStatistics::GetStateDesc(size_t stateID) const
{
	return m_stateRegistry.GetDesc(stateID);
}

//////////////////////////////////////////////////////////////////////////
// Scopes
//////////////////////////////////////////////////////////////////////////

bool CGameStatistics::RegisterGameScopes(const SGameScopeDesc *scopeDescs, size_t numScopes)
{
	m_gameScopes.RegisterGameScopes(scopeDescs, numScopes);

	return true;
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetScopeCount() const
{
	return m_gameScopes.Count();
}

//////////////////////////////////////////////////////////////////////////

IStatsTracker* CGameStatistics::PushGameScope(size_t scopeID)
{
	CStatsTracker* tracker = m_gameScopes.PushGameScope(scopeID, ++m_currTimeStamp, this);
	if (tracker == 0)
		return 0;

	if(m_gameScopes.m_scopeStack.size() == 1)
	{
		CRY_ASSERT(!m_scriptBind.get());
		m_scriptBind.reset(new CScriptBind_GameStatistics(this));
	}

	if(m_gsCallback)
		m_gsCallback->OnNodeAdded(tracker->GetLocator());

	m_scriptBind->BindTracker(GetGameRulesTable(), m_gameScopes.m_scopeRegistry[scopeID].trackerName, tracker);

	return tracker;
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::PopGameScope(size_t checkScopeID)
{
	m_gameScopes.PopGameScope(checkScopeID);

	CRY_ASSERT(m_gameScopes.m_scopeStack.size());
	if(!m_gameScopes.m_scopeStack.size())
		return;

//	SNodeLocator locator = m_gameScopes.m_scopeStack.back().locator;
//	CStatsTracker* tracker = m_gameScopes.m_scopeStack.back().tracker;

	SNodeLocator locator = m_gameScopes.GetLastScopeDataInStack().locator;
	CStatsTracker* tracker = m_gameScopes.GetLastScopeDataInStack().tracker;

	CRY_ASSERT(checkScopeID == INVALID_STAT_ID || checkScopeID == locator.scopeID);
	if(checkScopeID != INVALID_STAT_ID && checkScopeID != locator.scopeID)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Scope validation failed when popping game scope");
		return;
	}

	RemoveAllElements(locator.scopeID);

	m_scriptBind->UnbindTracker(GetGameRulesTable(), m_gameScopes.m_scopeRegistry[locator.scopeID].trackerName, tracker);

	SDeadStatNode* deadScope = new SDeadStatNode(locator, tracker);
	deadScope->children.swap(m_gameScopes.m_scopeStack.back().deadNodes);

	m_gameScopes.m_scopeStack.pop_back();

	if(m_gameScopes.m_scopeStack.size())
		m_gameScopes.m_scopeStack.back().deadNodes.push_back(deadScope);

	if(m_gsCallback)
		m_gsCallback->OnNodeRemoved(locator, tracker);

	// Last scope removed from the stack?
	if(!m_gameScopes.m_scopeStack.size())
	{
		SaveDeadNodesRec(deadScope);
		m_scriptBind.reset();
	}
	else
	{
		CheckMemoryOverflow();
	}
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetScopeStackSize() const
{
	return m_gameScopes.m_scopeStack.size();
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetScopeID(size_t depth) const
{
	CRY_ASSERT(depth < m_gameScopes.m_scopeStack.size());
	if(depth < m_gameScopes.m_scopeStack.size())
		return m_gameScopes.m_scopeStack[m_gameScopes.m_scopeStack.size() - depth - 1].locator.scopeID;

	return INVALID_STAT_ID;
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetScopeID(const char *scriptName) const
{
	CRY_ASSERT(scriptName);
	TScopeMap::const_iterator it = m_gameScopes.m_scopeMap.find(scriptName);
	return it == m_gameScopes.m_scopeMap.end() ?
					INVALID_STAT_ID :
					it->second->statID;
}

//////////////////////////////////////////////////////////////////////////

const SGameScopeDesc* CGameStatistics::GetScopeDesc(size_t scopeID) const
{
	return (scopeID >= m_gameScopes.m_scopeRegistry.size()) ?
					0 :
					&m_gameScopes.m_scopeRegistry[scopeID];
}

//////////////////////////////////////////////////////////////////////////

XmlNodeRef CGameStatistics::CreateStatXMLNode(const char* tag)
{
	return GetISystem()->GetXmlUtils()->CreateStatsXmlNode(tag);
}

//////////////////////////////////////////////////////////////////////////
// Elements
//////////////////////////////////////////////////////////////////////////

bool CGameStatistics::RegisterGameElements(const SGameElementDesc *elemDescs, size_t numElems)
{
	return m_elemRegistry.Register(elemDescs, numElems);
}

//////////////////////////////////////////////////////////////////////////

IStatsTracker* CGameStatistics::AddGameElement(const SNodeLocator& locator, IScriptTable* pTable)
{
	const SGameElementDesc* desc = m_elemRegistry.GetDesc(locator.elemID);
	CRY_ASSERT(desc);
	CRY_ASSERT(!locator.isScope());
	CRY_ASSERT(desc->locatorID == locator.locatorType);

	if(!desc || locator.isScope())
		return 0;

	// Find scope on the stack
	size_t stackPos = FindScopePos(locator.scopeID);

	CRY_ASSERT_MESSAGE(stackPos != m_gameScopes.m_scopeStack.size(), "Elements can be added only to the scopes on the stack");
	if(stackPos == m_gameScopes.m_scopeStack.size())
		return 0;

	SNodeLocator newLocator(locator);
	newLocator.timeStamp = ++m_currTimeStamp;
	CStatsTracker* tracker = new CStatsTracker(newLocator, this, pTable);

	m_gameScopes.m_scopeStack[stackPos].elements.insert(std::make_pair(newLocator, tracker));

	if(pTable)
		m_scriptBind->BindTracker(pTable, desc->trackerName, tracker);

	if(m_gsCallback)
		m_gsCallback->OnNodeAdded(newLocator);

	return tracker;
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::RemoveElement(const SNodeLocator& locator)
{
	size_t stackPos = FindScopePos(locator.scopeID);

	CRY_ASSERT_MESSAGE(stackPos != m_gameScopes.m_scopeStack.size(), "No such scope on the stack");
	if(stackPos == m_gameScopes.m_scopeStack.size())
		return;

	SScopeData::TElements& elems = m_gameScopes.m_scopeStack[stackPos].elements;

	SScopeData::TElements::iterator it = elems.find(locator);
	if(it != elems.end())
	{
		DoRemoveElement(stackPos, it->first, it->second);
		elems.erase(it);
		CheckMemoryOverflow();
	}
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::DoRemoveElement(size_t scopePos, const SNodeLocator& locator, CStatsTracker* tracker)
{
	const SGameElementDesc* desc = m_elemRegistry.GetDesc(locator.elemID);
	CRY_ASSERT(desc);
	if(!desc)
		return;

	m_gameScopes.m_scopeStack[scopePos].deadNodes.push_back(new SDeadStatNode(locator, tracker));

	if(tracker->GetScriptTable())
		m_scriptBind->UnbindTracker(tracker->GetScriptTable(), desc->trackerName, tracker);

	if(m_gsCallback)
		m_gsCallback->OnNodeRemoved(locator, tracker);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::RemoveAllElements(size_t scopeID)
{
	size_t stackPos = FindScopePos(scopeID);

	CRY_ASSERT_MESSAGE(stackPos != m_gameScopes.m_scopeStack.size(), "No such scope on the stack");
	if(stackPos == m_gameScopes.m_scopeStack.size()) return;

	SScopeData::TElements& elems = m_gameScopes.m_scopeStack[stackPos].elements;

	for(SScopeData::TElements::iterator it = elems.begin(); it != elems.end(); ++it)
		DoRemoveElement(stackPos, it->first, it->second);

	elems.clear();
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetElementCount() const
{
	return m_elemRegistry.Count();
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetElementID(const char* scriptName) const
{
	return m_elemRegistry.GetID(scriptName);
}

//////////////////////////////////////////////////////////////////////////

const SGameElementDesc* CGameStatistics::GetElementDesc(size_t elemID) const
{
	return m_elemRegistry.GetDesc(elemID);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IStatsTracker* CGameStatistics::GetTracker(const SNodeLocator& locator) const
{
	CRY_ASSERT(locator.scopeID < m_gameScopes.m_scopeRegistry.size());

	if(locator.isScope())
	{
		CRY_ASSERT(locator.elemID == INVALID_STAT_ID);

		size_t pos = FindScopePos(locator.scopeID);

		if(pos != m_gameScopes.m_scopeStack.size())
			return m_gameScopes.m_scopeStack[pos].tracker;
	}
	else
	{
		const SGameElementDesc* desc = m_elemRegistry.GetDesc(locator.elemID);
		CRY_ASSERT(desc);
		CRY_ASSERT(desc->locatorID == locator.locatorType);

		size_t pos = FindScopePos(locator.scopeID);

		if(pos != m_gameScopes.m_scopeStack.size())
		{
			const SScopeData::TElements& elems = m_gameScopes.m_scopeStack[pos].elements;
			SScopeData::TElements::const_iterator it = elems.find(locator);
			if(it != elems.end())
				return it->second;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////

SNodeLocator CGameStatistics::GetTrackedNode(IStatsTracker *tracker) const
{
	CRY_ASSERT(tracker);
	if(!tracker) 
		return SNodeLocator();

	CStatsTracker* track = static_cast<CStatsTracker*>(tracker);
	return track->GetLocator();
}

//////////////////////////////////////////////////////////////////////////
// Misc
//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::FindScopePos(size_t scopeID) const
{
	return m_gameScopes.FindScopePos(scopeID);
}

//////////////////////////////////////////////////////////////////////////

// Wrapper for XML data for IXMLSerializable
class CXMLStatsWrapper : public CXMLSerializableBase
{
public:
	CXMLStatsWrapper(const XmlNodeRef& node) : m_node(node) 
	{ }

	virtual XmlNodeRef GetXML(IGameStatistics* pGS) 
	{ return m_node; }

	virtual void GetMemoryStatistics(ICrySizer* pSizer) const
	{ pSizer->Add(*this); }

private:
	XmlNodeRef m_node;
};

//////////////////////////////////////////////////////////////////////////

IXMLSerializable* CGameStatistics::WrapXMLNode(const XmlNodeRef &node)
{
	return new CXMLStatsWrapper(node);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::PreprocessScriptedEventParameter(size_t eventID, SStatAnyValue& value)
{
	if(m_gsCallback)
		m_gsCallback->PreprocessScriptedEventParameter(eventID, value);
}

void CGameStatistics::PreprocessScriptedStateParameter(size_t stateID, SStatAnyValue& value)
{
	if(m_gsCallback)
		m_gsCallback->PreprocessScriptedStateParameter(stateID, value);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::SetStatisticsCallback(IGameStatisticsCallback* pCallback)
{
	// Allow reset only
	CRY_ASSERT(!m_gsCallback || !pCallback);

	m_gsCallback = pCallback;
}

//////////////////////////////////////////////////////////////////////////

IGameStatisticsCallback* CGameStatistics::GetStatisticsCallback() const
{
	return m_gsCallback;
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::RegisterSerializer(IStatsSerializer* serializer)
{
	CRY_ASSERT(serializer);
	if(!serializer) 
		return;

	m_serializers.push_back(serializer);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::SetMemoryLimit(size_t kb)
{
	m_memoryLimit = kb << 10;
	CheckMemoryOverflow();
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::GetMemoryStatistics(ICrySizer *pSizer)const
{
	SIZER_COMPONENT_NAME(pSizer, "Statistics");
	pSizer->Add(*this);
	
	m_eventRegistry.GetMemoryStatistics(pSizer);
	m_stateRegistry.GetMemoryStatistics(pSizer);
	m_elemRegistry.GetMemoryStatistics(pSizer);

	pSizer->AddContainer(m_gameScopes.m_scopeRegistry);
	pSizer->AddContainer(m_gameScopes.m_scopeMap);
	pSizer->AddContainer(m_gameScopes.m_scopeStack);

	for(size_t i = 0; i != m_gameScopes.m_scopeStack.size(); ++i)
		m_gameScopes.m_scopeStack[i].GetMemoryStatistics(pSizer);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::CheckMemoryOverflow()
{
	if(m_memoryLimit != -1 && GetDeadNodesMemory() > m_memoryLimit)
	{
		SaveScopesRec();
		m_cachedMemUsage = 0;
	}
}

//////////////////////////////////////////////////////////////////////////

size_t CGameStatistics::GetDeadNodesMemory()
{
	CStatsSizer sizer;

	for(size_t i = 0; i != m_gameScopes.m_scopeStack.size(); ++i)
		m_gameScopes.m_scopeStack[i].GetDeadNodesMemory(&sizer);

	m_cachedMemUsage = sizer.GetTotalSize();
	return m_cachedMemUsage;
}

//////////////////////////////////////////////////////////////////////////

const char* CGameStatistics::GetSerializeName(const SNodeLocator& locator) const
{
	return locator.isScope() 
		? GetScopeDesc(locator.scopeID)->serializeName
		: GetElementDesc(locator.elemID)->serializeName;
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::SaveDeadNodesRec(SDeadStatNode* node)
{
	const char* serializeName = GetSerializeName(node->locator);

	NotifyNodeAdded(node->locator, serializeName, *node->tracker->GetStatsContainer(), eSNS_Dead);

	for(size_t i = 0; i != node->children.size(); ++i)
		SaveDeadNodesRec(node->children[i]);

	node->children.clear();
	NotifyNodeRemoved(node->locator, serializeName, *node->tracker->GetStatsContainer(), eSNS_Dead);

	delete node;
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::SaveScopesRec(size_t scopePos)
{
	SScopeData& sd = m_gameScopes.m_scopeStack[scopePos];

	const char* sname = GetSerializeName(sd.locator);
	NotifyNodeAdded(sd.locator, sname, *sd.tracker->GetStatsContainer(), eSNS_Alive);

	if((scopePos + 1) < m_gameScopes.m_scopeStack.size())
		SaveScopesRec(scopePos + 1);

	for(size_t i = 0; i != sd.deadNodes.size(); ++i)
		SaveDeadNodesRec(sd.deadNodes[i]);
	sd.deadNodes.clear();

	NotifyNodeRemoved(sd.locator, sname, *sd.tracker->GetStatsContainer(), eSNS_Alive);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::NotifyNodeAdded(const SNodeLocator& locator, const char* serializeName, IStatsContainer& container, EStatNodeState state)
{
	for(size_t i = 0; i != m_serializers.size(); ++i)
		m_serializers[i]->AddNode(locator, serializeName, container, state);
}

void CGameStatistics::NotifyNodeRemoved(const SNodeLocator& locator, const char* serializeName, IStatsContainer& container, EStatNodeState state)
{
	for(size_t i = 0; i != m_serializers.size(); ++i)
		m_serializers[i]->RemoveNode(locator, serializeName, container, state);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::GrowCachedMemSize(const SStatAnyValue& value)
{
	CStatsSizer sizer;
	value.GetMemoryStatistics(&sizer);
	m_cachedMemUsage += sizer.GetTotalSize();

	if(m_cachedMemUsage > m_memoryLimit)
	{
		SaveScopesRec();
		m_cachedMemUsage = 0;
	}
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::OnTrackedEvent(const SNodeLocator& locator, size_t eventID, const CTimeValue& timeVal, const SStatAnyValue& value)
{
	GrowCachedMemSize(value);

	if(!m_gsCallback)
		return;

	if(value.type == eSAT_TXML && value.pSerializable)
		value.pSerializable->DispatchEventToCallback(locator, eventID, timeVal, m_gsCallback);
	else
		m_gsCallback->OnEvent(locator, eventID, timeVal, value);
}

//////////////////////////////////////////////////////////////////////////

void CGameStatistics::OnTrackedState(const SNodeLocator& locator, size_t stateID, const SStatAnyValue& value)
{
	GrowCachedMemSize(value);

	if(!m_gsCallback)
		return;

	if(value.type == eSAT_TXML && value.pSerializable)
		value.pSerializable->DispatchStateToCallback(locator, stateID, m_gsCallback);
	else
		m_gsCallback->OnState(locator, stateID, value);
}

//////////////////////////////////////////////////////////////////////////

IScriptTable* CGameStatistics::GetGameRulesTable()
{
	IScriptTable *gameRulesTable = CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRules()->GetEntity()->GetScriptTable();
	CRY_ASSERT(gameRulesTable);
	return gameRulesTable;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CStatsTracker
//////////////////////////////////////////////////////////////////////////

CStatsTracker::CStatsTracker(const SNodeLocator& locator, CGameStatistics* pGameStats, IScriptTable* pTable)
: m_pGameStats(pGameStats)
, m_locator(locator)
, m_scriptTable(pTable)
{
	int eventCount = pGameStats != 0 ? pGameStats->GetEventCount() : 0;
	int stateCount = pGameStats != 0 ? pGameStats->GetStateCount() : 0;
	m_container.reset(new CStatsContainer(eventCount, stateCount));
}

//////////////////////////////////////////////////////////////////////////

SNodeLocator CStatsTracker::GetLocator() const
{
	return m_locator;
}

//////////////////////////////////////////////////////////////////////////

IScriptTable* CStatsTracker::GetScriptTable() const
{
	return m_scriptTable.GetPtr();
}

//////////////////////////////////////////////////////////////////////////

CStatsContainer* CStatsTracker::GetStatsContainer()
{
	return m_container.get();
}

//////////////////////////////////////////////////////////////////////////

void CStatsTracker::StateValue(size_t stateID, const SStatAnyValue& value)
{
#if STATS_MODE_CVAR
	if(CCryActionCVars::Get().g_statisticsMode != 2)
		return;
#endif

	m_container->AddState(stateID, value);

	m_pGameStats->OnTrackedState(m_locator, stateID, value);
}

//////////////////////////////////////////////////////////////////////////

void CStatsTracker::Event(size_t eventID, const SStatAnyValue& value)
{
#if STATS_MODE_CVAR
	if(CCryActionCVars::Get().g_statisticsMode != 2)
		return;
#endif

	CTimeValue timeVal = gEnv->pTimer->GetFrameStartTime();
	m_container->AddEvent(eventID, timeVal, value);

	m_pGameStats->OnTrackedEvent(m_locator, eventID, timeVal, value);
}

//////////////////////////////////////////////////////////////////////////

void CStatsTracker::GetMemoryStatistics(ICrySizer *pSizer)
{
	pSizer->Add(*this);
	m_container->GetMemoryStatistics(pSizer);
}

//////////////////////////////////////////////////////////////////////////
// CStatsContainer
//////////////////////////////////////////////////////////////////////////

CStatsContainer::CStatsContainer(size_t numEvents, size_t numStates)
{
	m_events.reserve(numEvents);
	m_states.reserve(numStates);
}

//////////////////////////////////////////////////////////////////////////

void CStatsContainer::AddEvent(size_t eventID, const CTimeValue& time, const SStatAnyValue& val)
{
	// Do we have a track for this event?
	if(eventID >= m_events.size())
	{
		// Events must be obsolete, extending event set
		m_events.resize(eventID + 1);
	}

	m_events[eventID].push_back(std::make_pair(time, val));
}

//////////////////////////////////////////////////////////////////////////

void CStatsContainer::AddState(size_t stateID, const SStatAnyValue &val)
{
	// Do we have a track for this event?
	if(stateID >= m_states.size())
	{
		// Events must be obsolete, extending event set
		m_states.resize(stateID + 1);
	}

	m_states[stateID] = val;
}

//////////////////////////////////////////////////////////////////////////

size_t CStatsContainer::GetEventTrackLength(size_t eventID) const
{
	return eventID >= m_events.size() ? 0 : m_events[eventID].size();
}

//////////////////////////////////////////////////////////////////////////

void CStatsContainer::GetEventInfo(size_t eventID, size_t idx, CTimeValue& outTime, SStatAnyValue& outParam) const
{
	if(eventID < m_events.size() && idx < m_events[eventID].size())
	{
		outTime = m_events[eventID][idx].first;
		outParam = m_events[eventID][idx].second;
	}
	else
	{
		outTime = CTimeValue();
		outParam = SStatAnyValue();
	}
}

//////////////////////////////////////////////////////////////////////////

void CStatsContainer::GetStateInfo(size_t stateID, SStatAnyValue& outValue) const
{
	if(stateID < m_states.size())
		outValue = m_states[stateID];
	else
		outValue = SStatAnyValue();
}

//////////////////////////////////////////////////////////////////////////

void CStatsContainer::Clear()
{
	for(size_t i = 0; i != m_events.size(); ++i)
		m_events[i].clear();

	for(size_t i = 0; i != m_states.size(); ++i)
		m_states[i].type = eSAT_NONE;
}

//////////////////////////////////////////////////////////////////////////

void CStatsContainer::GetMemoryStatistics(ICrySizer *pSizer)
{
	pSizer->Add(*this);
	pSizer->AddContainer(m_events);
	pSizer->AddContainer(m_states);

	for(size_t i = 0; i != m_events.size(); ++i)
	{
		const TEventTrack& track = m_events[i];
		pSizer->AddContainer(track);
		for(size_t j = 0; j != track.size(); ++j)
			track[j].second.GetMemoryStatistics(pSizer);
	}

	for(size_t i = 0; i != m_states.size(); ++i)
		m_states[i].GetMemoryStatistics(pSizer);
}

//////////////////////////////////////////////////////////////////////////
