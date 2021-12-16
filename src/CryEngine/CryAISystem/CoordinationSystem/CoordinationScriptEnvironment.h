#ifndef __CoordinationScriptEnvironment_h__
#define __CoordinationScriptEnvironment_h__

#pragma once


#include "Coordination.h"


class CoordinationScriptEnvironment
{
public:
	CoordinationScriptEnvironment()
	{
	};

	CoordinationScriptEnvironment(const CoordinationID& coordinationID, const char* name, const SmartScriptTable& inputs);
	~CoordinationScriptEnvironment();

	SmartScriptTable GetEnvironment() const;

	void SetInputValue(const char* name, const CoordinationInputValue& value) const;
	bool GetInputValue(const char* name, CoordinationInputValue& value) const;

	void SetActor(const CoordinationActor& actor) const;
	void SetActors(const CoordinationActors& actors) const;

	void Apply() const;
private:
	SmartScriptTable m_environment;
	SmartScriptTable m_inputs;
	SmartScriptTable m_actors;
};

#endif //__CoordinationScriptEnvironment_h__