/******************************************************************** 
StalTech Source File.
Copyright (C), StalTech Studios, 2008.
-------------------------------------------------------------------------
File name:   ScriptBind_AI.h
Description: Game 02 scriptbinds
These should move into GameDLL when interfaces allow!
-------------------------------------------------------------------------
History:
-20:02:2008   - Created by Matthew
*********************************************************************/

#ifndef __ScriptBind_AI_G02_H__
#define __ScriptBind_AI_G02_H__

#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IAISystem;
struct ISystem;
struct AgentMovementAbility;

class CScriptBind_AI_G02 : public CScriptableBase
{
public:
	//--------------------------------------------------------------------------------------
	// Basics
	//--------------------------------------------------------------------------------------
	CScriptBind_AI_G02();
	virtual ~CScriptBind_AI_G02() {};
	// <title Release>
	// Syntax: AI_G02.Release()
	void	Release() { delete this; };

	//--------------------------------------------------------------------------------------
	// Scriptbind functions
	//--------------------------------------------------------------------------------------

	// <title GetAlienApproachParams>
	// Syntax: AI_G02.GetAlienApproachParams(entityId,type,targetPos,targetDir,pNewTgtPos,pNewTgtDir)
	// Arguments:
	//		entityId	- Alien entity ID.
	//		type		- Alien position type.
	//		targetPos	- Target position.
	//		targetDir	- Target direction.
	//		pNewTgtPos	- New target position.
	//		pNewTgtDir	- New target direction.
	// Description:
	//		Gets the parameters of the alien approach.
	int	GetAlienApproachParams(IFunctionHandler * pH);
	// <title GetHunterApproachParams>
	// Syntax: AI_G02.GetHunterApproachParams(entityId,type,targetPos,targetDir,pNewTgtPos,pNewTgtDir)
	// Arguments:
	//		entityId	- Hunter entity ID.
	//		type		- Alien position type.
	//		targetPos	- Target position.
	//		targetDir	- Target direction.
	//		pNewTgtPos	- New target position.
	//		pNewTgtDir	- New target direction.
	// Description:
	//		Gets the parameters of the hunter approach.
	int	GetHunterApproachParams(IFunctionHandler * pH);
	// <title VerifyAlienTarget>
	// Syntax: AI_G02.VerifyAlienTarget(targetPos)
	// Arguments:
	//		targetPos - Target position.
	// Description:
	//		Checks if the alien target is valid.
	int	VerifyAlienTarget(IFunctionHandler * pH);
	// <title SetRefpointToAlienHidespot>
	// Syntax: AI_G02.SetRefpointToAlienHidespot(entityId,rangeMin,rangeMax)
	// Arguments:
	//		entityId - Alien entity ID.
	//		rangeMin - Minimum range value.
	//		rangeMax - Maximum range value.
	// Description:
	//		Sets the reference point to the alien hide spot.
	int SetRefpointToAlienHidespot(IFunctionHandler *pH);
	// <title MarkAlienHideSpotUnreachable>
	// Syntax: AI_G02.MarkAlienHideSpotUnreachable(entityId)
	// Arguments:
	//		entityId - Alien entity ID.
	// Description:
	//		Makes the alien hide spot unreachable.
	int MarkAlienHideSpotUnreachable(IFunctionHandler *pH);

	// <title CanJumpToPoint>
	// Syntax: AI_G02.CanJumpToPoint(dest,theta,maxheight,flags,retVelocity,tDest,pSkipEnt,startPos )
	// Arguments:
	//		dest		- Destination point.
	//		theta		- Input angle (in degrees).
	//		maxheight	- Max allowed jump height (in meters).
	//		flags		- AI_JUMP_ON_GROUND -> the point will be projected on ground - AI_JUMP_CHECK_COLLISION -> collision will be checked on the jump parabola.
	//		retVelocity - If jump can be performed, this is the returned initial velocity.
	//		tDest		- If jump can be performed, this is the returned time to reach destination.
	//		pSkipEnt	- Optional, skip this entity in collision check.
	//		startPos	- Optional, compute the jump start from this position instead of AI's position.
	// Description:
	//		Checks if an AI can jump to a point.
	// Return Value:
	//		True if the jump can be performed
	int CanJumpToPoint(IFunctionHandler * pH);

protected:
	void	RegisterGlobals();
	void	RegisterFunctions();

	//====================================================================
	// Fetch entity pointer from script parameter
	//====================================================================
	IEntity * GetEntityFromParam( IFunctionHandler *pH, int i );

	//====================================================================
	// Fetch entity ID from script parameter
	//====================================================================
	EntityId GetEntityIdFromParam( IFunctionHandler *pH, int i );

protected:

	// Big cheat! Should be shared with main scriptbind!
	int	RayWorldIntersectionWrapper(Vec3 org,Vec3 dir, int objtypes, unsigned int flags, ray_hit *hits,int nMaxHits,
		IPhysicalEntity **pSkipEnts=0,int nSkipEnts=0, void *pForeignData=0,int iForeignData=0, int iCaller=1);

	struct SUnreachableAlienHideSpot
	{
		SUnreachableAlienHideSpot(CTimeValue t, const Vec3& pos, EntityId entId) : t(t), pos(pos), entId(entId) {}
		CTimeValue t;
		Vec3 pos;
		EntityId entId;
	};
	std::vector<SUnreachableAlienHideSpot>	m_unreachableAlienHideSpots;

	struct SLastApproachParam
	{
		Vec3	pos;
		Vec3	dir;
	};
	std::list<SLastApproachParam>	m_lstLastestApproachParam;

};

#endif //__ScriptBind_AI_G02_H__