#include "StdAfx.h"
#include "CoordinationScriptEnvironment.h"


CoordinationScriptEnvironment::CoordinationScriptEnvironment(const CoordinationID& coordinationID, const char* name,
																														 const SmartScriptTable& inputs)
: m_environment(gEnv->pScriptSystem)
, m_inputs(gEnv->pScriptSystem)
, m_actors(gEnv->pScriptSystem)
{
	m_environment->SetValue("inputs", m_inputs);
	m_environment->SetValue("actors", m_actors);

	m_environment->SetValue("id", ScriptHandle(coordinationID));
	m_environment->SetValue("name", name);

	if (inputs)
		m_inputs->Clone(inputs, true);
}

CoordinationScriptEnvironment::~CoordinationScriptEnvironment()
{
}

SmartScriptTable CoordinationScriptEnvironment::GetEnvironment() const
{
	return m_environment;
}

void CoordinationScriptEnvironment::SetInputValue(const char* name, const CoordinationInputValue& value) const
{
	m_inputs->SetValueAny(name, value);
}

bool CoordinationScriptEnvironment::GetInputValue(const char* name, CoordinationInputValue& value) const
{
	return m_inputs->GetValueAny(name, value);
}

void CoordinationScriptEnvironment::SetActor(const CoordinationActor& actor) const
{
	m_environment->SetValue("actor", actor.GetScript());
}

void CoordinationScriptEnvironment::SetActors(const CoordinationActors& actors) const
{
	uint currentCount = m_actors->Count();

	CoordinationActors::const_iterator it = actors.begin();
	CoordinationActors::const_iterator end = actors.end();

	uint k = 1;
	for ( ; it != end; ++it, ++k)
		m_actors->SetAt(k, it->GetScript());

	for ( ; k < currentCount; ++k)
		m_actors->SetNullAt(k);
}

void CoordinationScriptEnvironment::Apply() const
{
	gEnv->pScriptSystem->SetGlobalValue("env", m_environment);
}
