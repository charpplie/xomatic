#include "StdAfx.h"
#include "CoordinationSetup.h"

#include "DebugDrawContext.h"


CoordinationRequirement::CoordinationRequirement(const char* name, const char* binderName, const HSCRIPTFUNCTION& cond)
: m_condition(gEnv->pScriptSystem, cond)
, m_name(name)
, m_binderName(binderName)
{
}

CoordinationRequirement::~CoordinationRequirement()
{
}

bool CoordinationRequirement::IsSatisfied() const
{
	if (m_condition)
	{
		bool state = false;

		return Script::CallReturn(gEnv->pScriptSystem, m_condition, state) && state;
	}

	return false;
}


const char* CoordinationRequirement::GetName() const
{
	return m_name.c_str();
}


const char* CoordinationRequirement::GetBinderName() const
{
	return m_binderName.c_str();
}


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

CoordinationEnvironmentBinder::CoordinationEnvironmentBinder(const HSCRIPTFUNCTION& code)
: m_code(gEnv->pScriptSystem, code)
{
}

CoordinationEnvironmentBinder::~CoordinationEnvironmentBinder()
{
}

bool CoordinationEnvironmentBinder::Execute() const
{
	if (m_code)
		return Script::Call(gEnv->pScriptSystem, m_code);

	return false;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------


const char* CoordinationInput::GetName() const
{
	return m_name.c_str();
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------


CoordinationSetup::CoordinationSetup()
{
}

const char* CoordinationSetup::GetName() const
{
	return m_name.c_str();
}


bool CoordinationSetup::LoadFromXML(const XmlNodeRef& rootNode, const char* fileName)
{
	const char* name;
	rootNode->getAttr("name", &name);
	m_name = name;

	if (rootNode->haveAttr("script"))
	{
		const char* script;
		if (rootNode->getAttr("script", &script) && script && script[0])
		{
			stack_string scriptFile(PathUtil::GetPath(fileName).c_str());
			scriptFile.append(script);

			gEnv->pScriptSystem->ExecuteFile(scriptFile.c_str(), true, true);
		}
	}

	int childCount = rootNode->getChildCount();

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef childNode = rootNode->getChild(i);
		const char* tagName = childNode->getTag();

		if (!stricmp(tagName, "Requirements"))
		{
			if (!LoadRequirements(childNode))
				return false;
		}
		else if (!stricmp(tagName, "Environment"))
		{
			if (!LoadBinders(childNode))
				return false;
		}
		else if (!stricmp(tagName, "Inputs"))
		{
			if (!LoadInputs(childNode))
				return false;
		}
		else if (!stricmp(tagName, "Plan"))
		{
			CoordinationPlan plan(this);
			if (!plan.LoadFromXML(m_plans.size(), childNode))
				return false;

			m_plans.push_back(plan);
		}
		else if (!stricmp(tagName, "Roles"))
		{
			if (!LoadRoles(childNode))
				return false;
		}
		else
		{
			AIWarning("Unexpected tag '%s' at line %d...", tagName, childNode->getLine());

			return false;
		}
	}

	return true;
}

int CoordinationSetup::GetRoleID(const char* roleName) const
{
	uint32 roleCount = m_roles.size();
	
	for (uint32 i = 0; i < roleCount; ++i)
	{
		if (!stricmp(m_roles[i].GetName(), roleName))
			return i;
	}

	return -1;
}

const CoordinationRoles& CoordinationSetup::GetRoles() const
{
	return m_roles;
}

const CoordinationInternalStartParams& CoordinationSetup::GetParams(const CoordinationID& coordinationID) const
{
	RunningSetups::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningSetup& setup = it->second;

		return setup.params;
	}

	static CoordinationInternalStartParams empty;

	assert(0);

	return empty;
}

const CoordinationScriptEnvironment& CoordinationSetup::GetScriptEnvironment(const CoordinationID& coordinationID) const
{
	RunningSetups::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningSetup& setup = it->second;

		return setup.environment;
	}

	static CoordinationScriptEnvironment empty(0, 0, CoordinationInputs());

	assert(0);

	return empty;
}

const CoordinationEnvironmentBinders& CoordinationSetup::GetContextBinders(const CoordinationID& coordinationID) const
{
	return m_binders;
}

bool CoordinationSetup::LoadRequirements(const XmlNodeRef& rootNode)
{
	int childCount = rootNode->getChildCount();

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef reqNode = rootNode->getChild(i);

		if (stricmp(reqNode->getTag(), "Requirement"))
			continue;

		const char* name = "<Unnamed>";
		if (reqNode->haveAttr("name"))
			reqNode->getAttr("name", &name);

		const char* binderName = "";
		reqNode->getAttr("binder", &binderName);

		const char* conditionString = 0;
		if (!reqNode->getAttr("condition", &conditionString))
		{
			AIWarning("Missing 'condition' attribute for 'Requirement' tag at line %d...", 
				reqNode->getLine());

			return false;
		}

		HSCRIPTFUNCTION condition = gEnv->pScriptSystem->CompileBuffer(conditionString, strlen(conditionString),
			"RequirementCondition");

		if (!condition)
		{
			AIWarning("Failed to compile condition for Requirement at line %d...", 
				reqNode->getLine());

			return false;
		}
		
		m_requirements.push_back(CoordinationRequirement(name, binderName, condition));
	}

	return true;
}


bool CoordinationSetup::LoadBinders(const XmlNodeRef& rootNode)
{
	int childCount = rootNode->getChildCount();

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef binderNode = rootNode->getChild(i);

		if (stricmp(binderNode->getTag(), "Binder"))
			continue;

		if (!binderNode->haveAttr("name"))
		{
			AIWarning("Missing 'name' attribute for 'Binder' tag at line %d...", 
				binderNode->getLine());
		}

		const char* name;
		binderNode->getAttr("name", &name);

		const char* codeString = 0;
		if (!binderNode->getAttr("code", &codeString))
		{
			AIWarning("Missing 'code' attribute for 'Binder' tag at line %d...", 
				binderNode->getLine());

			return false;
		}

		HSCRIPTFUNCTION code = gEnv->pScriptSystem->CompileBuffer(codeString, strlen(codeString),
			"EnvironmentBinderCode");

		if (!code)
		{
			AIWarning("Failed to compile code for EnvironmentBinder at line %d...", 
				binderNode->getLine());

			return false;
		}

		m_binders.insert(CoordinationEnvironmentBinders::value_type(name, CoordinationEnvironmentBinder(code)));
	}

	return true;
}

bool CoordinationSetup::LoadInputs(const XmlNodeRef& rootNode)
{
	int childCount = rootNode->getChildCount();

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef inputNode = rootNode->getChild(i);

		const char* tagName = inputNode->getTag();

		if (stricmp(tagName, "Input"))
		{
			AIWarning("Unexpected tag '%s' at line %d...", tagName, inputNode->getLine());

			return false;
		}

		const char* name = 0;
		if (!inputNode->getAttr("name", &name))
		{
			AIWarning("Missing 'name' attribute for 'Input' tag at line %d...", 
				inputNode->getLine());

			return false;
		}

		m_inputs.push_back(CoordinationInput(name));
	}

	return true;
}


bool CoordinationSetup::LoadRoles(const XmlNodeRef& rootNode)
{
	int roleCount = rootNode->getChildCount();

	for (int r = 0; r < roleCount; ++r)
	{
		XmlNodeRef roleNode = rootNode->getChild(r);

		const char* roleTagName = roleNode->getTag();

		if (!stricmp(roleTagName, "Role"))
		{
			CoordinationRole role;

			if (!role.LoadFromXML(roleNode))
				return false;

			m_roles.push_back(role);
		}
		else
		{
			AIWarning("Unexpected tag '%s' at line %d...", roleTagName, roleNode->getLine());

			return false;
		}
	}

	return true;
}

uint32 CoordinationSetup::GetRunningCount() const
{
	return m_running.size();
}

const CoordinationID& CoordinationSetup::GetRunningCoordinationID(uint32 index) const
{
	assert(index >= 0 && index < m_running.size());

	RunningSetups::const_iterator it = m_running.begin();
	std::advance(it, index);

	return it->first;
}

uint32 CoordinationSetup::GetRunningPlanID(const CoordinationID& coordinationID) const
{
	RunningSetups::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningSetup& setup = it->second;
	return setup.planId;
}

const CoordinationPlan& CoordinationSetup::GetRunningPlan(const CoordinationID& coordinationID) const
{
	RunningSetups::const_iterator it = m_running.find(coordinationID);
	assert(it != m_running.end());

	const RunningSetup& setup = it->second;
	return m_plans[setup.planId];
}

uint32 CoordinationSetup::GetPlanCount() const
{
	return m_plans.size();
}

const CoordinationPlan& CoordinationSetup::GetPlan(uint32 index) const
{
	assert(index >= 0 && index < m_plans.size());

	return m_plans[index];
}

void CoordinationSetup::DebugDrawStatus(const CoordinationID& coordinationID, CoordinationDebugDrawContext& context) const
{
	stack_string coordName;
	coordName.FormatFast("%s:%d", GetName(), (uint32)coordinationID);

	context->Draw2dLabel(context.x + context.ident, context.y, context.fontSize * 1.25f, Col_SlateBlue, false, "%s", 
		coordName.c_str());
	context.y += context.lineHeight * 1.25f;

	uint32 planCount = GetPlanCount();
	uint32 runningPlanID = GetRunningPlanID(coordinationID);

	context.ident += context.identWidth;

	for (uint32 p = 0; p < planCount; ++p)
	{
		const CoordinationPlan& plan = GetPlan(p);

		if (runningPlanID != p)
		{
			context->Draw2dLabel(context.x + context.ident, context.y, context.fontSize, Col_DarkGray, false, "%s", plan.GetName());
			context.y += context.lineHeight;
		}
		else
		{
			plan.DebugDrawStatus(coordinationID, context);
		}
	}

	context.ident -= context.identWidth;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

ECoordinationState CoordinationSetup::GetState(const CoordinationID& coordinationID) const
{
	RunningSetups::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
	{
		const RunningSetup& running = it->second;

		return running.state;
	}

	return CoordinationNotRunning;
}

bool CoordinationSetup::CanStart(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params) const
{
	if (!m_requirements.empty())
	{
		CoordinationScriptEnvironment environment(coordinationID, m_name.c_str(), params.inputs);
		environment.SetActors(params.actors);
		environment.Apply();

		m_transientEnvironments.insert(TransientEnvironments::value_type(coordinationID, environment));

		Requirements::const_iterator it = m_requirements.begin();
		Requirements::const_iterator end = m_requirements.end();

		for ( ; it != end; ++it)
		{
			const char* binderName = it->GetBinderName();
			if (binderName && binderName[0])
			{
				CoordinationEnvironmentBinders::const_iterator it = m_binders.find(binderName);
				it->second.Execute();
			}

			if (!it->IsSatisfied())
			{
				AIWarning("Failed to start coordination '%s' - Requirement '%s' not satisfied!", m_name.c_str(), it->GetName());
				
				return false;
			}
		}
	}

	return true;
}

void CoordinationSetup::Start(const CoordinationID& coordinationID, const CoordinationInternalStartParams& params)
{
	RunningSetups::const_iterator it = m_running.find(coordinationID);
	if (it != m_running.end())
		return;

	std::pair<RunningSetups::iterator, bool> result = m_running.insert(RunningSetups::value_type(coordinationID, RunningSetup()));
	RunningSetup& running = result.first->second;

	for (int i = 0; i < (int)m_plans.size(); ++i)
	{
		if (m_plans[i].CanStart(coordinationID, params))
		{
			running.planId = i;
			break;
		}
	}

	running.params = params;

	TransientEnvironments::iterator tit = m_transientEnvironments.find(coordinationID);
	if (tit!= m_transientEnvironments.end())
		running.environment = tit->second;
	else
	{
		running.environment = CoordinationScriptEnvironment(coordinationID, m_name.c_str(), params.inputs);
		running.environment.SetActors(params.actors);
	}

	CoordinationPlan& plan = m_plans[running.planId];
	plan.Start(coordinationID);
}

void CoordinationSetup::Stop(const CoordinationID& coordinationID)
{
	RunningSetups::iterator it = m_running.find(coordinationID);
	
	if (it != m_running.end())
	{
		RunningSetup& running = it->second;
		CoordinationPlan& plan = m_plans[running.planId];

		plan.Stop(coordinationID);
		m_running.erase(it);
	}
}

void CoordinationSetup::Update(float frameTime)
{
	m_transientEnvironments.clear();

	if (!m_running.empty())
	{
		Plans::iterator it = m_plans.begin();
		Plans::iterator end = m_plans.end();

		for ( ; it != end; ++it)
			it->Update(frameTime);

		RunningSetups::iterator rit = m_running.begin();
		RunningSetups::iterator rend = m_running.end();

		for ( ; rit != rend; ++rit)
		{
			const CoordinationID& coordinationID = rit->first;
			RunningSetup& running = rit->second;

			CoordinationPlan& plan = m_plans[running.planId];
			ECoordinationState state = plan.GetState(coordinationID);
			running.state = state;
		}
	}
}

void CoordinationSetup::SetInputValue(const CoordinationID& coordinationID, const char* name,
																			const CoordinationInputValue& value)
{
	GetScriptEnvironment(coordinationID).SetInputValue(name, value);
}

bool CoordinationSetup::GetInputValue(const CoordinationID& coordinationID, const char* name, CoordinationInputValue& value)
{
	return GetScriptEnvironment(coordinationID).GetInputValue(name, value);
}
