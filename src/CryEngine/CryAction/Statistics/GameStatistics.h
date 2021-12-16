
#ifndef		__GAMESTATISTICS_H__
#define		__GAMESTATISTICS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#define STATS_MODE_CVAR 1

#include <IScriptSystem.h>
#include <IGameStatistics.h>

class CGameStatistics;

class CStatsContainer : public IStatsContainer
{
	typedef std::vector<std::pair<CTimeValue, SStatAnyValue> > TEventTrack;
	typedef std::vector< TEventTrack > TEventTracks;
	typedef std::vector< SStatAnyValue > TStateTrack;

public:

	CStatsContainer(size_t numEvents, size_t numStates);
	virtual void AddEvent(size_t eventID, const CTimeValue& time, const SStatAnyValue& val);
	virtual void AddState(size_t stateID, const SStatAnyValue& val);
	virtual size_t GetEventTrackLength(size_t eventID) const;
	virtual void GetEventInfo(size_t eventID, size_t idx, CTimeValue& outTime, SStatAnyValue& outParam) const;
	virtual void GetStateInfo(size_t stateID, SStatAnyValue& outValue) const;
	virtual void Clear();
	virtual void GetMemoryStatistics(ICrySizer *pSizer);

private:
	TEventTracks	m_events;
	TStateTrack		m_states;
};

//////////////////////////////////////////////////////////////////////////

class CStatsTracker : public IStatsTracker
{
public:
	CStatsTracker(const SNodeLocator&locator, CGameStatistics* pGameStats, IScriptTable* pTable = 0);
	virtual void StateValue(size_t stateID, const SStatAnyValue& value);
	virtual void Event(size_t eventID, const SStatAnyValue& value);
	virtual CStatsContainer* GetStatsContainer();
	virtual void GetMemoryStatistics(ICrySizer *pSizer);

	SNodeLocator GetLocator() const;
	IScriptTable* GetScriptTable() const;

private:
	SNodeLocator m_locator;
	SmartScriptTable m_scriptTable;
	CGameStatistics* m_pGameStats;
	std::auto_ptr<CStatsContainer>	m_container;
};

//////////////////////////////////////////////////////////////////////////

struct SDeadStatNode
{
	typedef std::vector<SDeadStatNode*> TChildren;

	SNodeLocator locator;
	CStatsTracker* tracker;
	TChildren children;

	SDeadStatNode(const SNodeLocator& loc, CStatsTracker* track)
		: locator(loc), tracker(track)
	{ }

	void GetMemoryStatistics(ICrySizer *pSizer)const
	{
		pSizer->Add(*this);
		pSizer->AddContainer(children);
		tracker->GetMemoryStatistics(pSizer);

		for(size_t i = 0; i != children.size(); ++i)
			children[i]->GetMemoryStatistics(pSizer);
	}
};

//////////////////////////////////////////////////////////////////////////

struct SScopeData
{
	typedef std::map<SNodeLocator, CStatsTracker*> TElements;
	typedef std::vector<SDeadStatNode*> TDeadNodes;

	SNodeLocator		locator;
	CStatsTracker*	tracker;
	TElements				elements;
	TDeadNodes			deadNodes;

	void GetMemoryStatistics(ICrySizer *pSizer)const
	{
		pSizer->Add(*this);
		pSizer->AddContainer(elements);
		pSizer->AddContainer(deadNodes);

		for(TElements::const_iterator it = elements.begin(); it != elements.end(); ++it)
			it->second->GetMemoryStatistics(pSizer);

		for(size_t i = 0; i != deadNodes.size(); ++i)
			deadNodes[i]->GetMemoryStatistics(pSizer);
	}

	void GetDeadNodesMemory(ICrySizer *pSizer) const
	{
		for(size_t i = 0; i != deadNodes.size(); ++i)
			deadNodes[i]->GetMemoryStatistics(pSizer);
	}

	SScopeData() : tracker(0) { }
	SScopeData(SNodeLocator _locator, CStatsTracker* _tracker) : locator(_locator), tracker(_tracker) { }
};

//////////////////////////////////////////////////////////////////////////

class CScriptBind_GameStatistics;
class CMasterSrvSender;

typedef std::vector<SGameScopeDesc> TScopeRegistry;
typedef std::map<string, const SGameScopeDesc*> TScopeMap;
typedef std::vector<SScopeData> TScopeStack;

//////////////////////////////////////////////////////////////////////////

class CGameScopes
{
public:
	CGameScopes();

	bool RegisterGameScopes(const SGameScopeDesc scopeDescs[], size_t numScopes);

	CStatsTracker* PushGameScope(size_t scopeID, uint32 timestamp, CGameStatistics* gameStatistics);
	void PopGameScope(size_t checkScopeID);

	size_t FindScopePos(size_t scopeID) const;
	size_t Count() const;
	size_t GetStackSize() const;

	const SScopeData& GetLastScopeDataInStack() const;

private:
	bool ValidateScopes(const SGameScopeDesc scopeDescs[], size_t numScopes);
	void GrowRegistry( size_t numScopes );
	void InsertScopesInRegistry(const SGameScopeDesc * scopeDescs, size_t numScopes);

public:
	TScopeRegistry m_scopeRegistry;
	TScopeMap		m_scopeMap;
	TScopeStack m_scopeStack;
};

//////////////////////////////////////////////////////////////////////////

class CStatRegistry
{
	typedef std::vector<SGameStatDesc> TStatRegistry;
	typedef std::map<string, const SGameStatDesc*> TStatMap;

public:
	bool Register(const SGameStatDesc statDescs[], size_t numStats);
	size_t Register(const char* scriptName, const char* serializeName);
	size_t Count() const;
	size_t GetID(const char* scriptName) const;
	const SGameStatDesc* GetDesc(size_t statID) const;
	void GetMemoryStatistics(ICrySizer *pSizer)const;

private:
	bool ValidateRegistration(const SGameStatDesc *statDescs, size_t numStats);

	TStatRegistry	m_statRegistry;
	TStatMap			m_statMap;
};

//////////////////////////////////////////////////////////////////////////

class CElemRegistry
{
	typedef std::vector<SGameElementDesc> TElemRegistry;
	typedef std::map<string, const SGameElementDesc*> TElemMap;

public:
	bool Register(const SGameElementDesc elemDescs[], size_t numElems);
	size_t Count() const;
	size_t GetID(const char* scriptName) const;
	const SGameElementDesc* GetDesc(size_t statID) const;
	void GetMemoryStatistics(ICrySizer *pSizer)const;

private:
	bool ValidateRegistration(const SGameElementDesc *elemDescs, size_t numElems);

	TElemRegistry m_elemRegistry;
	TElemMap m_elemMap;
};

//////////////////////////////////////////////////////////////////////////

class CGameStatistics : public IGameStatistics
{
	typedef std::vector<IStatsSerializer*> TSerializers;

public:
	CGameStatistics();
	~CGameStatistics();

	virtual bool						RegisterGameEvents(const SGameStatDesc *eventDescs, size_t numEvents);
	virtual size_t					RegisterGameEvent(const char* scriptName, const char* serializeName);
	virtual size_t					GetEventCount() const;
	virtual size_t					GetEventID(const char* scriptName) const;
	virtual size_t          GetEventIDBySerializeName(const char* serializeName) const;
	virtual const SGameStatDesc* GetEventDesc(size_t eventID) const;

	virtual bool						RegisterGameStates(const SGameStatDesc *stateDescs, size_t numStates);
	virtual size_t					RegisterGameState(const char* scriptName, const char* serializeName);
	virtual size_t					GetStateCount() const;
	virtual size_t					GetStateID(const char* scriptName) const;
	virtual const SGameStatDesc* GetStateDesc(size_t stateID) const;

	virtual bool						RegisterGameScopes(const SGameScopeDesc *scopeDescs, size_t numScopes);
	virtual IStatsTracker*	PushGameScope(size_t scopeID);
	virtual void						PopGameScope(size_t scopeID);
	virtual size_t					GetScopeStackSize() const;
	virtual size_t					GetScopeID(size_t depth = 0) const;
	virtual size_t					GetScopeCount() const;
	virtual size_t					GetScopeID(const char* scriptName) const;
	virtual const SGameScopeDesc* GetScopeDesc(size_t scopeID) const;

	virtual bool						RegisterGameElements(const SGameElementDesc *elemDescs, size_t numElems);
	virtual IStatsTracker* 	AddGameElement(const SNodeLocator& locator, IScriptTable* pTable);
	virtual void						RemoveElement(const SNodeLocator& locator);
	virtual size_t					GetElementCount() const;
	virtual size_t					GetElementID(const char* scriptName) const;
	virtual const SGameElementDesc*	GetElementDesc(size_t elemID) const;

	virtual IStatsTracker*	GetTracker(const SNodeLocator& locator) const;
	virtual SNodeLocator		GetTrackedNode(IStatsTracker* tracker) const;

	virtual XmlNodeRef CreateStatXMLNode(const char* tag = "root");
	virtual IXMLSerializable* WrapXMLNode(const XmlNodeRef& node);

	virtual void PreprocessScriptedEventParameter(size_t eventID, SStatAnyValue& value);
	virtual void PreprocessScriptedStateParameter(size_t stateID, SStatAnyValue& value);

	virtual void SetStatisticsCallback(IGameStatisticsCallback* pCallback);
	virtual IGameStatisticsCallback* GetStatisticsCallback() const;

	virtual void RegisterSerializer(IStatsSerializer* serializer);
	virtual void SetMemoryLimit(size_t kb);

	virtual void GetMemoryStatistics(ICrySizer *pSizer)const;

	void OnTrackedEvent(const SNodeLocator& locator, size_t eventID, const CTimeValue& timeVal, const SStatAnyValue& value);
	void OnTrackedState(const SNodeLocator& locator, size_t stateID, const SStatAnyValue& value);

private:
	void DoRemoveElement(size_t scopePos, const SNodeLocator& locator, CStatsTracker* tracker);
	void RemoveAllElements(size_t scopeID);
	size_t FindScopePos(size_t scopeID) const;
	void CheckMemoryOverflow();
	void GrowCachedMemSize(const SStatAnyValue& value);
	size_t GetDeadNodesMemory();
	void SaveDeadNodesRec(SDeadStatNode* node);
	void SaveScopesRec(size_t scopePos = 0);
	const char* GetSerializeName(const SNodeLocator& locator) const;
	void NotifyNodeAdded(const SNodeLocator& locator, const char* serializeName, IStatsContainer& container, EStatNodeState state);
	void NotifyNodeRemoved(const SNodeLocator& locator, const char* serializeName, IStatsContainer& container, EStatNodeState state);
	static IScriptTable* GetGameRulesTable();

	std::auto_ptr<CScriptBind_GameStatistics>	m_scriptBind;
	IGameStatisticsCallback* m_gsCallback;

	CStatRegistry m_eventRegistry;
	CStatRegistry m_stateRegistry;
	CElemRegistry m_elemRegistry;
	CGameScopes	m_gameScopes;

	TSerializers m_serializers;
	size_t m_memoryLimit;
	size_t m_cachedMemUsage;
	uint32 m_currTimeStamp;
};

//////////////////////////////////////////////////////////////////////////

#endif //__GAMESTATISTICS_H__
