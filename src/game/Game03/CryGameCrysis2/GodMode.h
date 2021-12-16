#pragma once

#ifndef GOD_MODE_H
#define GOD_MODE_H

class CActor;

enum EGodModeState
{
	eGMS_None,
	eGMS_GodMode,
	eGMS_TeamGodMode,
	eGMS_DemiGodMode,
	eGMS_LAST
};

class CGodMode
{
public:
	static CGodMode& GetInstance();

	void RegisterConsoleVars(IConsole* pConsole);
	void UnregisterConsoleVars(IConsole* pConsole) const;

	void MoveToNextState();
	void RespawnIfDead(CActor* actor) const;
	void RespawnPlayerIfDead() const;

	EGodModeState GetCurrentState() const;
	const char* GetCurrentStateString() const;

	void Update(float frameTime);
	void DemiGodDeath();

	bool IsGod() const;
	bool IsGodModeActive() const;

private:
	CGodMode();
	CGodMode(const CGodMode&);
	CGodMode& operator=(const CGodMode&);

	static const float m_timeToWaitBeforeRespawn;
	static const char* m_godModeCVarName;

	bool m_respawningFromDemiGodDeath;
	float m_elapsedTime;
	int m_godMode;
};

#endif
