/******************************************************************** 
StalTech Source File.
Copyright (C), StalTech Studios, 2008.
-------------------------------------------------------------------------
File name:   ScriptBind_AI.h
Description: Game 04 scriptbinds
             These should move into GameDLL when interfaces allow!
-------------------------------------------------------------------------
History:
-20:02:2008   - Created by Matthew
*********************************************************************/

#ifndef __ScriptBind_AI_G04_H__
#define __ScriptBind_AI_G04_H__

#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IAISystem;
struct ISystem;
struct AgentMovementAbility;

class CScriptBind_AI_G04 : public CScriptableBase
{
public:
	//--------------------------------------------------------------------------------------
	// Basics
	//--------------------------------------------------------------------------------------
	CScriptBind_AI_G04();
	virtual ~CScriptBind_AI_G04() {};
	void Release() { delete this; };

	//--------------------------------------------------------------------------------------
	// Scriptbind functions
	//--------------------------------------------------------------------------------------

	// <title SetStrafe>
	// Syntax: AI.SetStrafe(entityId, allowedStrafeDistanceStart, allowedStrafeDistanceEnd, allowStrafeLookWhileMoving)
	// Description: Sets strafe distance parameters, i.e. at what distance it's allowed
	// Arguments: 
	// entityId - AI's entity
	// allowedStrafeDistanceStart  
	// allowedStrafeDistanceEnd  
	// allowStrafeLookWhileMoving  	
	// See:
	// CPuppet::SetAllowedStrafeDistances
	int SetStrafeParams(IFunctionHandler *pH);

	// <title GetSpeed>
	// Syntax: AI.GetSpeed(entityId)
	// Description: Gets the movement urgency
	// Arguments: 
	// entityId - AI's entity
	// Notes:
	// Possible candidate for being global!
	int GetSpeed(IFunctionHandler *pH);

	// <title SetSpeed>
	// Syntax: AI.SetSpeed(entityId, speed)
	// Description: Sets the movement urgency
	// Arguments: 
	// entityId - AI's entity
	// speed - Desired movement urgency	
	// Notes:
	// Possible candidate for being global!
	int SetSpeed(IFunctionHandler *pH);

	// <title PlayCustomReadabilitySound>
	// Syntax: AI.PlayCustomReadabilitySound(entityId, speed)
	// Description: Sets the movement urgency
	// Arguments: 
	// entityId - AI's entity
	// speed - Desired movement urgency	
	// Notes:
	// Possible candidate for being global!
	int PlayCustomReadabilitySound(IFunctionHandler *pH);	

protected:
	void	RegisterGlobals();
	void	RegisterFunctions();
};

#endif //__ScriptBind_AI_G04_H__