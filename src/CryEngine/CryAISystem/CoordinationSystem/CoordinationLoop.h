#ifndef __CoordinationLoop_h__
#define __CoordinationLoop_h__

#pragma once


#include "CoordinationElement.h"


class CoordinationPlan;

class CoordinationLoop :
	public CoordinationElement
{
public:
	explicit CoordinationLoop(const CoordinationSetup* setup);

	uint32 GetRunningCount() const;
	const CoordinationID& GetRunningCoordinationID(uint32 index) const;
	uint32 GetRunningElementID(const CoordinationID& coordinationID) const;
	const CoordinationElement& GetRunningElement(const CoordinationID& coordinationID) const;
	uint32 GetElementCount() const;
	const CoordinationElement& GetElement(uint32 index) const;

	// CoordinationElement
	virtual bool LoadFromXML(uint32 elementId, const XmlNodeRef& rootNode);

	virtual void DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const;

	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const;
	virtual void Start(const CoordinationID& coordinationID);
	virtual void Stop(const CoordinationID& coordinationID);
	virtual void Update(float frameTime);
	// ~CoordinationElement

private:
	typedef struct RunningLoop
	{
		int elementID;
		ECoordinationState state;
	};

	typedef std::map<CoordinationID, RunningLoop> RunningLoops;
	RunningLoops m_running;

	CoordinationElements m_elements;
	const CoordinationSetup* m_setup;
};


#endif //__CoordinationLoop_h__