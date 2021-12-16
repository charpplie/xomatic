#ifndef __CoordinationPlan_h__
#define __CoordinationPlan_h__

#pragma once


#include "Coordination.h"
#include "CoordinationElement.h"


class CoordinationPlan :
	public CoordinationElement
{
public:
	CoordinationPlan(const CoordinationSetup* setup);
	~CoordinationPlan();

	const char* GetName() const;

	bool LoadFromXML(uint32 planId, const XmlNodeRef& rootNode);

	uint32 GetRunningCount() const;
	const CoordinationID& GetRunningCoordinationID(uint32 index) const;
	uint32 GetRunningElementID(const CoordinationID& coordinationID) const;
	const CoordinationElement& GetRunningElement(const CoordinationID& coordinationID) const;
	uint32 GetElementCount() const;
	const CoordinationElement& GetElement(uint32 index) const;

	void DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const;

	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const;
	virtual void Start(const CoordinationID& coordinationID);
	virtual void Stop(const CoordinationID& coordinationID);
	virtual void Update(float frameTime);

private:
	typedef struct RunningPlan
	{
		RunningPlan()
			: state(CoordinationRunning)
		{
		}

		int elementId;
		ECoordinationState state;
	};

	typedef std::map<CoordinationID, RunningPlan> RunningPlans;
	RunningPlans m_running;

	CoordinationElements m_elements;

	const CoordinationSetup* m_setup;
	string m_name;
};


#endif //__CoordinationPlan_h__