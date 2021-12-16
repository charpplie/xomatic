#include "StdAfx.h"
#include "CoordinationRole.h"


CoordinationRole::CoordinationRole()
: m_minActors(0)
, m_maxActors(INT_MAX)
, m_required(false)
{
}

CoordinationRole::~CoordinationRole()
{
}


const char* CoordinationRole::GetName() const
{
	return m_name.c_str();
}


bool CoordinationRole::LoadFromXML(const XmlNodeRef& rootNode)
{
	const char* name;
	if (!rootNode->getAttr("name", &name))
	{
		AIWarning("Missing 'name' attribute for 'Role' tag at line %d...", rootNode->getLine());

		return false;
	}

	m_name = name;

	if (m_name.SpanExcluding(":, ") != m_name)
	{
		AIWarning("'name' attribute for 'Role' tag at line %d contains invalid characters... ':', ',' and ' ' are not allowed...", rootNode->getLine());

		return false;
	}

	rootNode->getAttr("min", m_minActors);
	rootNode->getAttr("max", m_maxActors);

	XMLUtils::BoolType isRequired = XMLUtils::GetBoolType(rootNode, "required", XMLUtils::True);

	if (isRequired == XMLUtils::Invalid)
	{
		AIWarning("Invalid 'required' attribute for 'Role' tag at line %d...", 
			rootNode->getLine());

		return false;
	}

	m_required = (isRequired == XMLUtils::True);

	const char* scoreString = 0;
	if (!rootNode->getAttr("score", &scoreString))
	{
		AIWarning("Missing 'score' attribute for 'Role' tag at line %d...", 
			rootNode->getLine());

		return false;
	}

	HSCRIPTFUNCTION score = gEnv->pScriptSystem->CompileBuffer(scoreString, strlen(scoreString),
		"RoleScore");

	m_score = SmartScriptFunction(gEnv->pScriptSystem, score);

	const char* conditionString = 0;
	if (rootNode->getAttr("condition", &conditionString))
	{
		HSCRIPTFUNCTION condition = gEnv->pScriptSystem->CompileBuffer(conditionString, strlen(conditionString),
			"RoleCondition");

		m_condition = SmartScriptFunction(gEnv->pScriptSystem, condition);
	}

	return true;
}


int CoordinationRole::GetMinActors() const
{
	return m_minActors;
}

int CoordinationRole::GetMaxActors() const
{
	return m_maxActors;
}

float CoordinationRole::GetActorScore(const CoordinationActor& actor) const
{
	if (m_score)
	{
		float score = 0.0f;
		// TODO: Setup function environment
		Script::CallReturn(gEnv->pScriptSystem, m_score, score);

		return score;
	}

	return 0.0f;
}

bool CoordinationRole::CanActorPlay(const CoordinationActor& actor) const
{
	if (m_condition)
	{
		bool canPlay = false;
		// TODO: Setup function environment
		Script::CallReturn(gEnv->pScriptSystem, m_condition, canPlay);

		return canPlay;
	}

	return true;
}

bool CoordinationRole::IsRequired() const
{
	return m_required;
}