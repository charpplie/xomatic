#ifndef __CoordinationRole_h__
#define __CoordinationRole_h__

#pragma once


#include "Coordination.h"


class CoordinationRole
{
public:
	CoordinationRole();
	~CoordinationRole();

	bool LoadFromXML(const XmlNodeRef& rootNode);

	const char* GetName() const;

	int GetMinActors() const;
	int GetMaxActors() const;

	float GetActorScore(const CoordinationActor& actor) const;
	bool CanActorPlay(const CoordinationActor& actor) const;
	bool IsRequired() const;

private:
	SmartScriptFunction m_condition;
	SmartScriptFunction m_score;

	int m_minActors;
	int m_maxActors;
	bool m_required;

	string m_name;
};


class CoordinationRoles :
	public std::vector<CoordinationRole>
{
};


#endif //__CoordinationRole_h__