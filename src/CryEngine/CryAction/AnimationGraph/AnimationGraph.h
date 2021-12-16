#ifndef __ANIMATIONGRAPH_H__
#define __ANIMATIONGRAPH_H__

#pragma once

#include "StateIndex.h"
#include "RangeMerger.h"
#include <queue>
#include "IAnimationGraph.h"
#include "IAnimationStateNode.h"
#include "MiniQueue.h"
#include "VectorMap.h"
#include "STLPoolAllocator.h"

struct IAnimationStateNodeFactory;
class CAnimationGraphManager;
class CAnimationGraphState;
class CLocomotionManager;
struct IAnimationGraphDeadInputReportCallback;

// update this if binary serialization format changes
const static int AG_VERSION = 54;

namespace AnimGraphHelpers
{
	class CLoadProfiler
	{
	public:
		CLoadProfiler( const char * function, float timeout ) : m_time(gEnv->pTimer->GetAsyncTime()), m_function(function), m_timeout(timeout) {}
		~CLoadProfiler()
		{
			float elapsed = (gEnv->pTimer->GetAsyncTime() - m_time).GetSeconds();
			if (elapsed > m_timeout)
				CryLog( "%s: %f seconds", m_function, elapsed );
		}

	private:
		const char * m_function;
		CTimeValue m_time;
		float m_timeout;
	};

}

#define PROFILE_LOADING_FUNCTION AnimGraphHelpers::CLoadProfiler loadProfiler(__FUNC__, 0.1f)




#define ANIMGRAPH_BIT_VAR(type,name,bits) type name : bits


class CAnimationGraph : public IAnimationGraph
{
	friend class CAnimationGraphState;
	friend class CAnimationGraphExistanceQuery;
	friend class CExactPositioning;

public:
	typedef CStateIndex::StateID StateID;
	static const uint16 INVALID_STATE = ~0;
	static const size_t MAX_INPUTS = 32;
	static const size_t MAX_STATELIST_ENTRIES = 24;
	typedef MiniQueue<StateID, MAX_STATELIST_ENTRIES> StateList;

	static CAnimationGraph* CURRENT_ANIMGRAPH_DEBUG;

private:
	// the constructor is private to make sure no instance are created as local (or member) variables
	// use CreateInstance method instead and store it in a smart pointer - IAnimationGraphPtr or _smart_ptr< CAnimationGraph >
	CAnimationGraph( CAnimationGraphManager * pManager, string name );

public:
	~CAnimationGraph();

	static _smart_ptr< CAnimationGraph > CreateInstance( CAnimationGraphManager * pManager, string name )
	{
		return new CAnimationGraph( pManager, name );
	}

	bool Init( const XmlNodeRef& nodeRef );
	void AddRef();
	void Release();
	bool IsLastRef();

	void GetMemoryStatistics(ICrySizer * s);

	// IAnimationGraph
	VIRTUAL IAnimationGraphState * CreateState();
	VIRTUAL IAnimationGraph::InputID LookupInputId( const char * name );
	VIRTUAL const char * GetName();
	VIRTUAL uint8 GetBlendValueID( const char * name );
	VIRTUAL bool SerializeAsFile(const char* fileName, bool reading = false);
	VIRTUAL const char* RegisterVariationInputs( const char* animationName );
	VIRTUAL TimeAlignmentMask CalculateTimeAlignMask(const string &name);
	// ~IAnimationGraph

	void SwapAndIncrementVersion( _smart_ptr<CAnimationGraph> pGraph );

	CCryName StateIDToName( CStateIndex::StateID id )
	{
		return m_states[id].id;
	}
	CStateIndex::StateID StateNameToID( const CCryName& name )
	{
		return stl::find_in_map( m_stateNameToID, name, INVALID_STATE );
	}

	const VectorMap<CCryName, CStateIndex::StateID>& GetAllStates() const
	{
		return m_stateNameToID;
	}

	int DeclareOutput( const char * name, const char * value );

	void FindDeadInputValues( IAnimationGraphDeadInputReportCallback * pCallback );

	bool IsDirectlyLinked(CStateIndex::StateID fromID, CStateIndex::StateID toID) const;

	ILINE bool UseCrysis1LocoStateSelectionLogic() const { return m_useCrysis1LocoStateSelectionLogic; }

	struct SLinkInfo
	{
		//Remember : if you add members, you have to update SerializeAsFile
		StateID from, to;
		uint16 forceFollowChance;
		uint16 cost;
		float overrideTransitionTime;

		bool operator<( const SLinkInfo& rhs ) const
		{
			return from < rhs.from || (from == rhs.from && to < rhs.to);
		}
	};
	const SLinkInfo* FindLink(CStateIndex::StateID fromID, CStateIndex::StateID toID) const;

	const SAnimationDesc* GetAnimationDesc(IEntity* pEntity, CStateIndex::StateID stateID, const CAnimationGraphState* pState);
	bool ConstructAnimationDescForAllStates(IEntity* pEntity, const CAnimationGraphState* pState);
	SAnimationDesc ConstructAnimationDesc(IEntity * pEntity, CStateIndex::StateID stateID, const CAnimationGraphState* pState);

  bool IsStateLooped( StateID state );

private:
	struct PathFindStats
	{
		uint32 maxCost;
		uint32 finalCost;
		size_t largestOpenQueue;
		size_t nodesTouched;
		size_t linksFollowed;
		size_t cumLinksInQueue;
		size_t guardEvaluationsPerformed;
		size_t guardEvaluationsFailed;
		size_t aiWalkabilityQueriesPerformed;
		size_t animationMovementQueriesPerformed;
		size_t maybeTargets;
		size_t replacementsMade;
		size_t expensiveGuardEvaluationsPerformed;
	};

	struct IInputValue
	{		//Remember : if you add members, you have to update SerializeAsFile
		InputID id;
		union
		{
			EAnimationGraphInputType type;
			int typeIntVal;
		};
		bool signalled;
		uint8 priority;
		CStateIndex::InputValue defaultValue;
		string name;
		uint32 offset;
		static const uint32 UNBOUND_OFFSET = ~uint32(0);

		IInputValue()	//this is used to simplify file serialization (a lot)
		{}

		IInputValue( EAnimationGraphInputType type_, const string& name_ ) : type(type_), name(name_), offset(UNBOUND_OFFSET), signalled(false), defaultValue(CStateIndex::INPUT_VALUE_DONT_CARE) {}

		virtual void Release() = 0;
		virtual CStateIndex::InputValue EncodeInput( const char * value ) = 0;
		virtual CStateIndex::InputValue EncodeInput( float value ) = 0;
		virtual CStateIndex::InputValue EncodeInput( int value ) = 0;
		virtual void DecodeInput( char * buf, CStateIndex::InputValue input ) = 0;
		virtual void DebugText( char * buf, CStateIndex::InputValue encoded, const float* floats ) = 0;
		virtual float AsFloat( const void * basePtr ) = 0;
		virtual void DeclInputs( CStateIndex::Builder * pBuilder, CStateIndex::InputID& curID ) = 0;
		virtual bool GetRestrictions( const XmlNodeRef& node, std::vector<CStateIndex::InputValue>& ) = 0;
		virtual void Serialize( TSerialize ser, CStateIndex::InputValue* pInputValue, float* pInputFloat ) = 0;
		virtual void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(name);
		}
		virtual void SerializeAsFile( bool reading, AG_FILE* pAGI )
		{
				FileSerializationHelper h(reading, pAGI);
				h.Value(&typeIntVal);
				h.StringValue(&name);
				h.Value(&id);
				h.Value(&signalled);
				h.Value(&defaultValue);
				h.Value(&offset);
				h.Value(&priority);
		}

	protected:
		template <class T>
		ILINE const T * GetValue( const void * basePtr )
		{
			CRY_ASSERT( AnimGraph_GetTypeIdForType<T>::value == type );
			if (offset == UNBOUND_OFFSET)
				return 0;
			return *(const T* const *)(((const char*)basePtr)+offset);
		}
	};

	class CStateLoader;
	class CDiscreteValue;
	class CKeyedInputValue;
	class CIntegerInputValue;
	class CFloatInputValue;
	class CVec3InputValue;

	struct SGuard
	{
		SGuard();
		uint16 /*InputID*/ input : 5;
		uint16 opCode : 10;
	};

	static const int LOG2_MAX_GUARDS = 3;
	static const int MAX_GUARDS = 1<<LOG2_MAX_GUARDS;

	struct SStateInfo
	{
		//Remember : if you add members, you have to update SerializeAsFile
		CCryName id;
		StateID parentState;
		ANIMGRAPH_BIT_VAR(uint8, allowSelect, 1);
		ANIMGRAPH_BIT_VAR(uint8, hasForceFollows, 1);
		ANIMGRAPH_BIT_VAR(uint8, skipFirstPerson, 1);
		ANIMGRAPH_BIT_VAR(uint8, animationControlledView, 1);
		ANIMGRAPH_BIT_VAR(uint8, noPhysicalCollider, 1);
		ANIMGRAPH_BIT_VAR(uint8, hurryable, 1);
		ANIMGRAPH_BIT_VAR(uint8, matchesSignalledInputs, 1);
		ANIMGRAPH_BIT_VAR(uint8, hasExpensiveGuards, 1); // No need to serialize, because it's generated everytime the graph is reloaded.
		ANIMGRAPH_BIT_VAR(uint8, evaluateGuardsOnExit, 1);
		ANIMGRAPH_BIT_VAR(uint8, hasAnyLinksFrom, 1);
		ANIMGRAPH_BIT_VAR(uint8, hasAnyLinksTo, 1);
		ANIMGRAPH_BIT_VAR(uint8, canMix, 1);
		//ANIMGRAPH_BIT_VAR(uint8, hasTrivialGuards, 1);
		uint8 MovementControlMethodH;
		uint8 MovementControlMethodV;
		uint8 factoryLength;
		uint8 numGuards;
		uint16 cost;
		StateID prevState; // pathfinding
		SGuard guards[MAX_GUARDS*2]; // pathfinding
		uint32 factoryStart;
		uint32 linkOffset;
		float additionalTurnMultiplier;
		// pathfinding:
		SAnimationDesc animDesc;

		bool EvaluateGuards( const CStateIndex::InputValue * pInputs ) const;
		void SerializeAsFile(bool reading, AG_FILE *pAGI, CAnimationGraphManager* pManager, const std::map<int, IAnimationStateNodeFactory*>& idToFactory, const std::map<IAnimationStateNodeFactory*, int>& factoryToId);
	};

	/*
	 * REMEMBER
	 * --------
	 *
	 * whenever adding state to this class, you must also update SwapAndIncrementVersion
	 * whenever adding ANY MEMBER you have to update SerializeAsFile as well
	 */
	uint32 m_serial;
	string m_name;
	CAnimationGraphManager * m_pManager;
	typedef std::vector<IInputValue*> InputValues;
	InputValues m_inputValues; // all input values
	InputValues m_blendWeightInputValues; // input values determining blend weights
	float m_blendWeightMoveSpeeds[9];
	CStateIndex::InputID m_numInputIDs;
	typedef std::vector<SStateInfo> StateInfoVec;
	StateInfoVec m_states;
	typedef std::vector<SLinkInfo> LinkInfoVec;
	LinkInfoVec m_links;
	VectorMap<CCryName, StateID> m_stateNameToID;
	CStateIndex m_stateIndex;
	StateID m_replaceMeAnimation;

	//! If this flag is false, the SelectLocomotionState function will not be used
	bool m_useCrysis1LocoStateSelectionLogic;

	struct STimeAlignmentGroup
	{
		string name;
		int ID;
	};
	std::vector<STimeAlignmentGroup> m_timeAlignmentGroups;
	int RegisterGroup(const string &groupName);

	bool m_hasAnimDesc;
	std::vector<uint8> m_factorySlotIndices;
	std::vector<IAnimationStateNodeFactory*> m_factories;
	int m_nRefs;

	VectorMap<string, int> m_stringToOutputHigh;
	VectorMap<string, int> m_stringToOutputLow;
	int m_nextOutputHigh;
	int m_nextOutputLow;
	VectorMap< int, std::pair<string,string> > m_outputs;
	int RegisterOutput( string& value, VectorMap<string, int>& sto, int& next );

	uint8 m_nextBlendValueID;
	VectorMap< string, uint8 > m_blendValueIDs;

	typedef VectorMap< string, unsigned char, stl::less_stricmp<string> > MapVariationInputIDs;
	MapVariationInputIDs m_variationInputIDs;
	char RegisterVariationInput( const string& variationInputName );
	int GetVariationInputID( const char* variationInputName ) const;

	struct OpenItem
	{
		uint32  cost;
		uint16	stateID;

		bool operator >(const OpenItem &item) const
		{
			return cost > item.cost;
		}
	};
	typedef std::vector<OpenItem> OpenList;

	// path-finder temporary state
	// shared to save lots of allocations
	struct SStatePathfindState
	{
		void Clear()
		{
			cost = ~0;
			guardsEvaluated = false;
			guardsSucceed = false;
			onList = false;
			nextTouched = ~0;
		}
		uint32 cost;
		SAnimationMovement movement;
		bool guardsEvaluated;
		bool guardsSucceed;
		bool onList;
		StateID nextTouched;
	};
	StateID m_lastTouched;
	bool m_pathFindInitialized;
	std::vector<SStatePathfindState> m_statePathfindState;
	ILINE void AddTouched( StateID state )
	{
		SStatePathfindState& s = m_statePathfindState[state];
		if (s.nextTouched == StateID(~0))
		{
			s.nextTouched = m_lastTouched;
			m_lastTouched = state;
		}
	}

	// this is cache memory for AnimationGraphState
	std::vector< std::pair<int,StateID> > m_destinationsCache;
    
	bool HaveState( const CCryName& state ) const { return m_stateNameToID.find(state) != m_stateNameToID.end(); }
	typedef std::map< CCryName, XmlNodeRef > LinkTermAndXmlMap;
	typedef std::map< CCryName, LinkTermAndXmlMap > LinkMap;
	bool AddLinkToMap( LinkMap& m, const CCryName& from, const CCryName& to, const XmlNodeRef& node );

	struct SPathFindParams
	{
		SPathFindParams()
		: pGraphState(NULL)
		, destinationStateID(INVALID_STATE)
		, pEntity(NULL)
		, radius(0.0f)
		, time(0.0f)
		, pTransitions(NULL)
		, pStats(NULL)
		, pMovement(NULL)
		, pCurInputValues(NULL)
		, randomNumber(1337)
		, pTargetPointRequest(NULL)
		{}

		CAnimationGraphState* pGraphState; // in
		StateID destinationStateID; // in
		IEntity * pEntity; // in
		float radius; // in
		float time; // in
		StateList* pTransitions; // in/out
		PathFindStats* pStats; // in/out
		SAnimationMovement* pMovement; // out
		const CStateIndex::InputValue * pCurInputValues; // in
		int randomNumber; // in
		CTargetPointRequest * pTargetPointRequest; // out
	};
	OpenItem *m_openList;
	uint32		m_openListSize;

	static uint32 CalculateCost( const SLinkInfo& link, const SStateInfo& state );
	bool PreprocessTemplates( XmlNodeRef root );
	bool LoadInputs( const XmlNodeRef& root, CStateIndex::Builder * pStateIndexBuilder );
	bool LoadStates( const XmlNodeRef& root, CStateIndex::Builder * pStateIndexBuilder, CStateLoader& loader );
	bool LoadTransitions( const XmlNodeRef& root, const CStateLoader& loader );
	// pre-calculate things that we'll need.
	bool CachePass();
	bool PushLinksFromState( StateID fromStateID, SPathFindParams& params );
	ETriState PathFindBetweenStates( StateID fromStateID, SPathFindParams& params );
	IInputValue * FindInputValue( const string& name, CStateIndex::InputID * pID = 0 );
	Vec2 GetStateMinMaxSpeed( IEntity * pEntity, StateID state );
	void CalculateStatePositions();
	bool PreprocessParameterization( XmlNodeRef rootGraph );

	// AG 1.5
	bool PreProcessNodes( XmlNodeRef rootGraph );

	IAnimationStateNodeFactory* FindAnimationLayerFactory(CStateIndex::StateID stateID) const;
};

#endif
