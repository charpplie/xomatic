#pragma once

#ifndef GameAIInstanceBase_h
#define GameAIInstanceBase_h

class CGameAIInstanceBase
{
public:
	CGameAIInstanceBase(const EntityId entityID);
	void SendSignal(const char* signal);
	EntityId GetEntityID() const { return m_entityID; }
	const string& GetDebugEntityName() const { return m_debugEntityName; }

private:
	EntityId m_entityID;
	string m_debugEntityName;
};

#endif // GameAIInstanceBase_h
