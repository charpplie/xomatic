/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   PerceptionManager.h
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 2008				: Created by Mikko Mononen
- 2 Mar 2009	: Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _PERCEPTIONMANAGER_H_
#define _PERCEPTIONMANAGER_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "ValueHistory.h"

class CPerceptionManager
{
public:
	
	~CPerceptionManager();

	static CPerceptionManager* GetInstance();
	static void DestroyInstance();

	void Reset();
	void Update(float dt);
	void DebugDraw(int mode);
	void DebugDrawPerformance(int mode);
	void Serialize(TSerialize ser);
	void OnObjectRemoved(CAIObject *pObject);

	bool UpdatePerception(CPuppet* pPuppet, std::vector<CAIObject*>& priorityTargets);
	void RemovePuppetLOSQueries(CPuppet* pPuppet);

	void RegisterStimulus(const SAIStimulus& stim);
	void IgnoreStimulusFrom(EntityId sourceId, EAIStimulusType type, float time);
	bool IsPointInRadiusOfStimulus(EAIStimulusType type, const Vec3& pos) const;

	bool RegisterStimulusDesc(EAIStimulusType type, const SAIStimulusTypeDesc& desc);

	void RegisterAIEventListener(IAIEventListener* pListener, const Vec3& pos, float rad, int flags);
	void UnregisterAIEventListener(IAIEventListener* pListener);

protected:
	CPerceptionManager();

private:

	struct SStimulusRecord
	{
		EntityId sourceId;
		EntityId targetId;
		Vec3 pos;
		Vec3 dir;
		float radius;
		float t;
		unsigned char type;
		unsigned char subType;
		unsigned char flags;
		unsigned char dispatched;

		void Serialize(TSerialize ser)
		{
			ser.BeginGroup("Stim");
			ser.Value("sourceId", sourceId);
			ser.Value("targetId", targetId);
			ser.Value("pos", pos);
			ser.Value("dir", dir);
			ser.Value("radius", radius);
			ser.Value("t", t);
			ser.Value("type", type);
			ser.Value("subType", subType);
			ser.Value("flags", flags);
			ser.Value("dispatched", dispatched);
			ser.EndGroup();
		}
	};

	struct SAIEventListener
	{
		IAIEventListener* pListener;
		Vec3 pos;
		float radius;
		int flags;
	};

	void HandleSound(const SStimulusRecord& stim);
	void HandleCollision(const SStimulusRecord& stim);
	void HandleExplosion(const SStimulusRecord& stim);
	void HandleBulletHit(const SStimulusRecord& stim);
	void HandleBulletWhizz(const SStimulusRecord& stim);
	void HandleGrenade(const SStimulusRecord& stim);
	void VisCheckBroadPhase(float dt);
	/// Checks if the sound is occluded.
	bool IsSoundOccluded(CPuppet* pPuppet, const Vec3& soundPos);
	/// Suppresses the sound radius based on sound suppressors.
	float SupressSound(const Vec3& pos, float radius);
	int RayOcclusionPlaneIntersection(const Vec3 &start,const Vec3 &end);
	void NotifyAIEventListeners(const SStimulusRecord& stim, float threat);
	void InitCommonTypeDescs();
	bool FilterStimulus(SAIStimulus* stim);

	/// Records a stimulus event to the AI recorder 
	void RecordStimulusEvent(const SStimulusRecord& stim, float fRadius, IAIObject &receiver) const;


	SAIStimulusTypeDesc m_stimulusTypes[AI_MAX_STIMULI];
	std::vector<SStimulusRecord> m_stimuli[AI_MAX_STIMULI];
	std::vector<SAIStimulus> m_incomingStimuli;
	typedef std::map<EntityId, float> StimulusIgnoreMap;
	StimulusIgnoreMap m_ignoreStimuliFrom[AI_MAX_STIMULI];
	std::vector<SAIEventListener>	m_eventListeners;
	float m_visBroadPhaseDt;
	int m_visCheckRaycastCount;


	static const int PERF_TRACKER_SAMPLE_COUNT = 200;

	class CPerfTracker
	{
		int m_count, m_countMax;
		CValueHistory<int> m_hist;
	public:
		inline CPerfTracker() : m_count(0), m_countMax(0), m_hist(PERF_TRACKER_SAMPLE_COUNT, 0) {}
		inline ~CPerfTracker() {}

		inline void Inc(int n = 1)
		{
			m_count += n;
		}

		inline void Update()
		{
			m_countMax = max(m_count, m_countMax);
			m_hist.Sample(m_count);
			m_count = 0;
		}

		inline void Reset()
		{
			m_count = m_countMax = 0;
			m_hist.Reset();
		}

		inline int GetCount() const { return m_count; }
		inline int GetCountMax() const { return m_countMax; }
		inline const CValueHistory<int>& GetHist() const { return m_hist; }
	};

	enum Trackers
	{
		PERFTRACK_VIS_CHECKS,
		PERFTRACK_VIS_RAYS,
		PERFTRACK_UPDATES,
		PERFTRACK_INCOMING_STIMS,
		PERFTRACK_STIMS,
		COUNT_PERFTRACK // must be last
	};

	struct PerfStats
	{
		inline void Update()
		{
			for (unsigned i = 0; i < COUNT_PERFTRACK; ++i)
				trackers[i].Update();
		}

		CPerfTracker trackers[COUNT_PERFTRACK];
	};

	PerfStats m_stats;


	static CPerceptionManager* Instance;
	//////////////////////////////////////////////////////////////////////////////
	//Physics thread LOS queries//////////////////////////////////////////////////
	struct PuppetToObjectLOS
	{
		PuppetToObjectLOS(CPuppet* pPuppet = NULL, CAIObject* pTarget = NULL)
			:puppet(pPuppet),target(pTarget), Status(QUEUED){};

		bool operator<(const PuppetToObjectLOS& other)const
		{
			int puppetInt = reinterpret_cast<int>(this->puppet);
			int otherPuppetInt = reinterpret_cast<int>(other.puppet);
			int targetInt = reinterpret_cast<int>(this->target);
			int otherTargetInt = reinterpret_cast<int>(other.target);

			if(puppetInt == otherPuppetInt)
				return targetInt < otherTargetInt;

			return puppetInt < otherPuppetInt;
		};

		CPuppet* puppet;
		CAIObject* target;
		enum eStatus{QUEUED, IN_LOS, OUT_LOS} Status;
	};

	typedef std::set<PuppetToObjectLOS> PuppetLOSQueries;
	PuppetLOSQueries m_PuppetLOSQueries;

	static int OnRwiResult(const EventPhys *pEvent)
	{
		EventPhysRWIResult *pRWIResult = (EventPhysRWIResult*)pEvent;
		assert(Instance);

		PuppetLOSQueries::iterator iter = Instance->m_PuppetLOSQueries.find(
			PuppetToObjectLOS(
				reinterpret_cast<CPuppet*>(pRWIResult->iForeignData)
				,reinterpret_cast<CAIObject*>(pRWIResult->pForeignData))
		);

		if(iter != Instance->m_PuppetLOSQueries.end())
		{
			const_cast<PuppetToObjectLOS*>(&*iter)->Status = (pRWIResult->nHits == 0) ? PuppetToObjectLOS::IN_LOS : PuppetToObjectLOS::OUT_LOS;
			return 1;
		}

		return 0;
	}
	//Physics thread LOS queries//////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
};

#endif
