#ifndef __Coordination_h__
#define __Coordination_h__

#pragma once


#include "CoordinationActor.h"
#include <ICoordinationManager.h>

typedef ScriptAnyValue CoordinationInputValue;

class CoordinationScriptEnvironment;


typedef enum ECoordinationState
{
	CoordinationNotRunning = 0,
	CoordinationFailed,
	CoordinationRunning,
	CoordinationWaiting,
	CoordinationFinished,
};


inline const ColorF GetStateColor(ECoordinationState state, const ColorF& running = Col_White)
{
	if (state == CoordinationFailed)
		return Col_VioletRed;
	if (state == CoordinationFinished)
		return Col_LimeGreen;
	if (state == CoordinationWaiting)
		return Col_Gold;
	return running;
}

inline const ColorF GetStateColor(ECoordinationState state, 
																	const ColorF& running,
																	const ColorF& finished,
																	const ColorF& waiting,
																	const ColorF& failed)
{
	if (state == CoordinationFailed)
		return failed;
	else if (state == CoordinationFinished)
		return finished;
	else if (state == CoordinationWaiting)
		return waiting;
	return running;
}

struct CoordinationInternalStartParams
{
	CoordinationInputs inputs;
	CoordinationActors actors;
};


struct CoordinationTaskMonitor
{
	virtual ECoordinationState GetTaskState(const CoordinationID& coordinationID, int roleId, int taskId) const = 0;
	virtual ECoordinationState GetRoleState(const CoordinationID& coordinationID, int roleId) const = 0;
	virtual int GetCurrentTask(const CoordinationID& coordinationID, int roleId) const = 0;
	virtual const char* GetTaskName(int roleId, int taskId) const = 0;
};

class CoordinationSetup;
class CoordinationRoles;
class CoordinationEnvironmentBinders;


//--------------------------------------------------------------------------------

class CDebugDrawContext;
struct IAIDebugRenderer;

struct CoordinationDebugDrawContext
{
	CoordinationDebugDrawContext(float left, float top, float _fontSize, float _lineHeight, float _identWidth, 
		CDebugDrawContext& _dc);

	float x;
	float y;
	float ident;

	const float identWidth;
	const float fontSize;
	const float lineHeight;

	IAIDebugRenderer* operator->() const;
private:
	CDebugDrawContext& dc;
};


#endif