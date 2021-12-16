#ifndef __CoordinationSetup_h__
#define __CoordinationSetup_h__

#pragma once


#include "CoordinationRole.h"
#include "CoordinationPlan.h"
#include "CoordinationScriptEnvironment.h"


//--------------------------------------------------------------------------------

class CoordinationRequirement
{
public:
	CoordinationRequirement(const char* name, const char* binderName, const HSCRIPTFUNCTION& cond);
	~CoordinationRequirement();

	bool IsSatisfied() const;

	const char* GetName() const;
	const char* GetBinderName() const;

private:
	SmartScriptFunction m_condition;

	string m_name;
	string m_binderName;
};

//--------------------------------------------------------------------------------

class CoordinationEnvironmentBinder
{
public:
	CoordinationEnvironmentBinder(const HSCRIPTFUNCTION& code);
	~CoordinationEnvironmentBinder();

	bool Execute() const;

private:
	SmartScriptFunction m_code;
};


class CoordinationEnvironmentBinders :
	public std::map<string, CoordinationEnvironmentBinder>
{
};


//--------------------------------------------------------------------------------

class CoordinationInput
{
public:
	CoordinationInput(const char* name)
		: m_name(name)
	{
	}

	const char* GetName() const;

private:
	string m_name;
};

//--------------------------------------------------------------------------------

class CoordinationSetup
{
	struct RunningSetup;
public:
	CoordinationSetup();

	const char* GetName() const;

	bool LoadFromXML(const XmlNodeRef& rootNode, const char* fileName);

	int GetRoleID(const char* roleName) const;
	const CoordinationRoles& GetRoles() const;
	const CoordinationInternalStartParams& GetParams(const CoordinationID& coordinationID) const;
	const CoordinationScriptEnvironment& GetScriptEnvironment(const CoordinationID& coordinationID) const;
	const CoordinationEnvironmentBinders& GetContextBinders(const CoordinationID& coordinationID) const;

	uint32 GetRunningCount() const;
	const CoordinationID& GetRunningCoordinationID(uint32 index) const;
	uint32 GetRunningPlanID(const CoordinationID& coordinationID) const;
	const CoordinationPlan& GetRunningPlan(const CoordinationID& coordinationID) const;
	uint32 GetPlanCount() const;
	const CoordinationPlan& GetPlan(uint32 index) const;

	void DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const;

	virtual ECoordinationState GetState(const CoordinationID& coordinationID) const;
	virtual bool CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const;
	virtual void Start(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params);
	virtual void Stop(const CoordinationID& coordinationID);
	virtual void Update(float frameTime);

	void SetInputValue(const CoordinationID& coordinationID, const char* name, const CoordinationInputValue& value);
	bool GetInputValue(const CoordinationID& coordinationID, const char* name, CoordinationInputValue& value);

private:
	bool LoadRequirements(const XmlNodeRef& rootNode);
	bool LoadBinders(const XmlNodeRef& rootNode);
	bool LoadInputs(const XmlNodeRef& rootNode);
	bool LoadRoles(const XmlNodeRef& rootNode);

	typedef struct RunningSetup
	{
		RunningSetup()
			: planId(0)
			, state(CoordinationRunning)
		{
		}

		int planId;
		ECoordinationState state;

		CoordinationInternalStartParams params;
		CoordinationScriptEnvironment environment;
	};

	typedef std::map<CoordinationID, RunningSetup> RunningSetups;
	RunningSetups m_running;

	typedef std::vector<CoordinationRequirement> Requirements;
	Requirements m_requirements;

	typedef std::vector<CoordinationInput> Inputs;
	Inputs m_inputs;

	typedef std::map<CoordinationID, CoordinationScriptEnvironment> TransientEnvironments;
	mutable TransientEnvironments m_transientEnvironments;

	CoordinationEnvironmentBinders m_binders;
	CoordinationRoles m_roles;

	typedef std::vector<CoordinationPlan> Plans;
	Plans m_plans;

	string m_name;
};


#endif //__CoordinationSetup_h__