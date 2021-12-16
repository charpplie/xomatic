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

#include "StdAfx.h"
#include "Scriptbind_AI_G02.h"


#undef GET_ENTITY
#define GET_ENTITY( i )		\
	ScriptHandle	hdl;			\
	pH->GetParam( i, hdl ); \
	EntityId nID = static_cast<EntityId>(hdl.n);	\
	IEntity*	pEntity = (IEntity*)gEnv->pEntitySystem->GetEntity( nID );


//////////////////////////////////////////////////////////////////////////
// The helper functions for
// int CScriptBind_AI_G02::CanJumpToPoint(IFunctionHandler * pH)
//////////////////////////////////////////////////////////////////////////
float	GetTofPointOnParabola(const Vec3& A, const Vec3& V, const Vec3& P, float g);
Vec3	GetPointOnParabola(const Vec3& A, const Vec3& V, float t, float g);
bool	IntersectSweptSphereWrapper(IAIActorProxy* pAIActorProxy, Vec3 *hitPos, float& hitDist, const Lineseg& lineseg, float radius,IPhysicalEntity** pSkipEnts=0, int nSkipEnts=0, int additionalFilter = 0);
bool	CanJumpToPointHelper(IAIActorProxy* pAIActorProxy, IEntity* pEntity, Vec3& dest, float theta, float maxheight, int flags, Vec3& retVelocity, float& tDest, IPhysicalEntity* pSkipEnt, Vec3* pStartPos);


//====================================================================

CScriptBind_AI_G02::CScriptBind_AI_G02() 
{
	Init( gEnv->pScriptSystem, gEnv->pSystem );
	SetGlobalName( "AI_G02" );

	RegisterGlobals();
	RegisterFunctions();
}

//====================================================================

void CScriptBind_AI_G02::RegisterGlobals()
{

}

//====================================================================

void CScriptBind_AI_G02::RegisterFunctions()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME	& CScriptBind_AI_G02::

#ifndef SP_DEMO
	SCRIPT_REG_FUNC(SetRefpointToAlienHidespot);
	SCRIPT_REG_FUNC(MarkAlienHideSpotUnreachable);
#endif

	SCRIPT_REG_FUNC(GetAlienApproachParams);
	SCRIPT_REG_FUNC(GetHunterApproachParams);
	SCRIPT_REG_FUNC(VerifyAlienTarget);

	SCRIPT_REG_FUNC(CanJumpToPoint);
}

//====================================================================
// Fetch entity pointer from script parameter
//====================================================================
IEntity* CScriptBind_AI_G02::GetEntityFromParam( IFunctionHandler* pH, int i )
{
	EntityId nID = GetEntityIdFromParam( pH, i );
	return gEnv->pEntitySystem->GetEntity(nID);		
}

//====================================================================
// Fetch entity ID from script parameter
//====================================================================
EntityId CScriptBind_AI_G02::GetEntityIdFromParam( IFunctionHandler* pH, int i )
{
	assert(pH);
	assert(i > 0);
	ScriptHandle hdl(0);
	EntityId nID = pH->GetParam(i,hdl);
	return static_cast<EntityId>(hdl.n);
}

//
//-------------------------------------------------------------------------------------------------------------------------
int CScriptBind_AI_G02::GetAlienApproachParams(IFunctionHandler * pH)
{
	// First param is the entity that is attacking, and the second param is the target location (script vector3).

	if(pH->GetParamCount() < 5)
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: Too few parameters." );
		return pH->EndFunction( 0 );
	}

	GET_ENTITY(1);

	if( !pEntity || !pEntity->GetAI() )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: No Entity or AI." );
		return pH->EndFunction( 0 );
	}

	//retrieve the entity
	Vec3 targetPos( 0, 0, 0 );
	Vec3 targetDir( 0, 0, 0 );
	SmartScriptTable	pNewTgtPos;
	SmartScriptTable	pNewTgtDir;
	int	type = 0;

	if( !pH->GetParam( 2, type ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: No type param." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 3, targetPos ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: No target pos." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 4, targetDir ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: No target dir." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 5, pNewTgtPos ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: No out target pos." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 6, pNewTgtDir ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: No out target dir." );
		return pH->EndFunction( 0 );
	}

	IAIObject*	pAIObj = pEntity->GetAI();
	IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
	if(!pAIActor)
		return pH->EndFunction(0);

	Vec3	entPos = pEntity->GetPos();
	Vec3	diff = entPos - targetPos;

	// Use target direction if available
	if( targetDir.GetLength() > 0.0001f )
		diff = -targetDir;

	float	angle = atan2f( diff.y, diff.x );

	Vec3	attackPos[6];
	Vec3	attackDir[6];
	bool	bAttackSight[6] = { false, false, false, false, false, false };
	float	attackPosScore[6] = { 4, 8, 0, 8, 4, 0 };

	Vec3	bbox[2];
	bbox[0].Set( -3, -3, -3 );
	bbox[1].Set( 3, 3, 3 );

	AgentParameters	agentParams = pAIActor->GetParameters();

	float	attackHeight = agentParams.m_fAttackZoneHeight * 2.0f;
	float	attackRad = agentParams.m_fAttackRange * 0.25f;

	// Randomize the location a little.
	angle += (((cry_rand() * 10) / RAND_MAX) * 0.2f - 1.0f) * (gf_PI * 2.0f / 15.0f);
	attackHeight = (float)(cry_rand() / (float)RAND_MAX) * agentParams.m_fAttackZoneHeight * 0.25f;

	int	sightScore = 10;	// Is the target visible.
	int	heightScore = 2;	// Is the location near the attack height.
	int	randomScore = 4;	// Random offset.

	if( type == 0 )
	{
		// Attack position.
		attackPosScore[0] = 0;	// Enter direction.
		attackPosScore[1] = 8;
		attackPosScore[2] = 4;
		attackPosScore[3] = 0;	// Opposite direction.
		attackPosScore[4] = 4;
		attackPosScore[5] = 8;

		attackHeight += agentParams.m_fAttackZoneHeight;

		attackRad = agentParams.m_fAttackRange * (0.1f + 0.1f * (float)((cry_rand() * 4) / RAND_MAX) / 4.0f);

		sightScore = 10;	// Is the target visible.
		heightScore = 2;	// Is the location near the attack height.
		randomScore = 4;	// Random offset.

		attackHeight += (float)(cry_rand() / RAND_MAX) * agentParams.m_fAttackZoneHeight;
	}
	else if( type == 3 )
	{
		// Attack position.
		attackPosScore[0] = 8;	// Enter direction.
		attackPosScore[1] = 4;
		attackPosScore[2] = 2;
		attackPosScore[3] = 0;	// Opposite direction.
		attackPosScore[4] = 2;
		attackPosScore[5] = 4;

		attackHeight += agentParams.m_fAttackZoneHeight;

		attackRad = agentParams.m_fAttackRange * 0.1f; //agentParams.m_fAttackRange * (0.1f + 0.1f * (float)((cry_rand() * 4) / RAND_MAX) / 4.0f);

		sightScore = 10;	// Is the target visible.
		heightScore = 2;	// Is the location near the attack height.
		randomScore = 2;	// Random offset.
	}
	else if( type == 1 )
	{
		// Recoil position.
		attackPosScore[0] = 0;	// Enter direction
		attackPosScore[1] = 0;
		attackPosScore[2] = 4;
		attackPosScore[3] = 8;	// Opposite direction.
		attackPosScore[4] = 4;
		attackPosScore[5] = 0;

		attackHeight += agentParams.m_fAttackZoneHeight * 2.0f;
		attackRad = agentParams.m_fAttackRange * 0.15f;

		sightScore = 10;	// Is the target visible.
		heightScore = 2;	// Is the location near the attack height.
		randomScore = 6;	// Random offset.
	}
	else if( type == 2 )
	{
		// Search position.
		attackPosScore[0] = 0;	// Enter direction
		attackPosScore[1] = 0;
		attackPosScore[2] = 1;
		attackPosScore[3] = 10;	// Opposite direction.
		attackPosScore[4] = 1;
		attackPosScore[5] = 0;

		attackHeight += agentParams.m_fAttackZoneHeight;
		attackRad = 10.0f; //agentParams.m_fAttackRange * 0.1f;

		sightScore = 8;	// Is the target visible.
		heightScore = 0;	// Is the location near the attack height.
		randomScore = 4;	// Random offset.
	}
	else if( type == 4 )
	{
		// Attack position.
		attackPosScore[0] = 4;	// Enter direction.
		attackPosScore[1] = 4;
		attackPosScore[2] = 2;
		attackPosScore[3] = 2;	// Opposite direction.
		attackPosScore[4] = 2;
		attackPosScore[5] = 4;

		attackHeight += agentParams.m_fAttackZoneHeight;

		attackRad = agentParams.m_fAttackRange * (0.02f + (float)((cry_rand() * 4) / RAND_MAX) * 0.02f);

		sightScore = 10;	// Is the target visible.
		heightScore = 0;	// Is the location near the attack height.
		randomScore = 6;	// Random offset.

		attackHeight += (float)((cry_rand() * 3) / RAND_MAX) * agentParams.m_fAttackZoneHeight * 0.25f;
	}

	int	best = 0;
	float bestScore = -1.f;
	int	numValidPos = 0;

	// Check the target height.
	/*	bool	bTooHighTargetObstacle = false;
	{
	float	targetRadius = 2.0f;
	// Make sure the point is above the terrain.
	float	terrainHeight = gEnv->p3DEngine->GetTerrainElevation( targetPos.x, targetPos.y );
	float	maxObstHeight = terrainHeight;

	IPhysicalEntity**	ppList = NULL;
	int	numEntites = pPhysWorld->GetEntitiesInBox( targetPos + Vec3( -1, -1, -1 ) * targetRadius, targetPos + Vec3( 1, 1, 1 ) * targetRadius, ppList, ent_static );
	for( int j = 0; j < numEntites; j++ )
	{
	pe_status_pos status;
	// Get bbox (status) from physics engine
	if(ppList[j]->GetType() == PE_LIVING)
	continue;
	ppList[j]->GetStatus( &status );

	float	height = status.pos.z + status.BBox[1].z; 

	if( height > maxObstHeight )
	maxObstHeight = height;
	}

	bTooHighTargetObstacle = (maxObstHeight - terrainHeight) > maxTargetObstacleHeight;
	}

	if( bTooHighTargetObstacle )
	{
	gEnv->pAISystem->LogComment("<CScriptBind_AI> ", "bTooHighTargetObstacle" );
	}*/

	for( int i = 0; i < 6; i++ )
	{
		// Calc new target position.
		angle += (float)i * (gf_PI * 2.0f / 6.0f);
		attackDir[i].Set( cosf( angle ), sinf( angle ), 0 );
		attackPos[i] = targetPos - attackDir[i] * attackRad;
		attackPos[i].z += attackHeight;

		// Make sure the point is above the terrain.
		/*		float	terrainHeight = gEnv->p3DEngine->GetTerrainElevation( attackPos[i].x, attackPos[i].y );
		float	maxObstHeight = terrainHeight;

		IPhysicalEntity**	ppList = NULL;
		int	numEntites = pPhysWorld->GetEntitiesInBox( attackPos[i] + bbox[0], attackPos[i] + bbox[1], ppList, ent_static );
		for( int j = 0; j < numEntites; j++ )
		{
		pe_status_pos status;
		// Get bbox (status) from physics engine
		if(ppList[j]->GetType() == PE_LIVING)
		continue;
		ppList[j]->GetStatus( &status );

		float	height = status.pos.z + status.BBox[1].z; 

		if( height > maxObstHeight )
		{
		maxObstHeight = height;
		attackPosScore[i] += blockingScore;
		}
		}

		bool	bTooHighObstacle = (maxObstHeight - terrainHeight) > maxObstacleHeight;

		maxObstHeight += attackHeight;*/

		/*
		virtual GraphNode *GetEnclosing(const Vec3 &pos, int navCap, GraphNode *pStart = 0 ,bool bOutsideOnly = false,
		float range = 0.0f, Vec3 * closestValid = 0) = 0;
		*/

		Vec3	checkPos( attackPos[i] - Vec3( 0, 0, pAIActor->GetMovementAbility().minFlightHeight ) );
		Vec3	closestValid( attackPos[i] );
		bool	bValidPos = false;

		if( gAIEnv.pGraph->GetEnclosing( checkPos, IAISystem::NAV_FLIGHT, 0.0f, 0, 0.0f, &closestValid ) )
		{
			bValidPos = true;
			attackPos[i] = closestValid + Vec3( 0, 0, pAIActor->GetMovementAbility().minFlightHeight );
		}

		//		if( attackPos[i].z < maxObstHeight )
		//			attackPos[i].z = maxObstHeight;

		// Recalc the attack direction.
		attackDir[i] = targetPos - attackPos[i];

		ray_hit	hit;
		int	numHits = RayWorldIntersectionWrapper( attackPos[i], attackDir[i], ent_static, rwi_stop_at_pierceable, &hit, 1 );
		bAttackSight[i] = numHits > 0 ? false : true;

		// Normalize the direction. the unnormalized version is needed for the ray intersection.
		attackDir[i].NormalizeSafe();

		if( bAttackSight[i] )
			attackPosScore[i] += sightScore;

		// Favor positions closer to the real attack range.
		if( fabs( targetPos.z - attackPos[i].z ) < attackHeight * 1.1f )
			attackPosScore[i] += heightScore;

		// Check if the point is too close to the old ones.
		for( std::list<SLastApproachParam>::iterator ptIt = m_lstLastestApproachParam.begin(); ptIt != m_lstLastestApproachParam.end(); ++ptIt )
		{
			SLastApproachParam&	param = (*ptIt);
			Vec3	diff2 = attackPos[i] - param.pos;
			if( diff2.len2() < 10 * 10 )
				attackPosScore[i] += -30;
		}

		// Add small jitter into the selection.
		attackPosScore[i] += (cry_rand() * randomScore) / RAND_MAX;

		if( attackPosScore[i] > bestScore )
		{
			best = i;
			bestScore = attackPosScore[i];
		}

		//		if( bAttackSight[i] && !bTooHighObstacle && !bTooHighTargetObstacle )
		if( bAttackSight[i] && bValidPos )
			numValidPos++;
	}

	// Keep history of path points.
	SLastApproachParam	params;
	params.pos = attackPos[best];
	params.dir = attackDir[best];
	m_lstLastestApproachParam.push_back( params );

	// Keep the history count sane.
	if( m_lstLastestApproachParam.size() > 12 )
		m_lstLastestApproachParam.pop_front();

	gEnv->pAISystem->LogComment("<CScriptBind_AI> ", "%d valid pos", numValidPos );

	// Returns the calculated target position and direction.
	pNewTgtPos->SetValue( "x", attackPos[best].x );
	pNewTgtPos->SetValue( "y", attackPos[best].y );
	pNewTgtPos->SetValue( "z", attackPos[best].z );

	pNewTgtDir->SetValue( "x", attackDir[best].x );
	pNewTgtDir->SetValue( "y", attackDir[best].y );
	pNewTgtDir->SetValue( "z", attackDir[best].z );

	return pH->EndFunction( numValidPos );
}

//
//-------------------------------------------------------------------------------------------------------------------------
int CScriptBind_AI_G02::GetHunterApproachParams(IFunctionHandler * pH)
{
	// First param is the entity that is attacking, and the second param is the target location (script vector3).

	if(pH->GetParamCount() < 5)
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetHunterApproachParams: Too few parameters." );
		return pH->EndFunction( 0 );
	}

	GET_ENTITY(1);

	if( !pEntity || !pEntity->GetAI() )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetHunterApproachParams: No Entity or AI." );
		return pH->EndFunction( 0 );
	}

	//retrieve the entity
	Vec3 targetPos( 0, 0, 0 );
	Vec3 targetDir( 0, 0, 0 );
	SmartScriptTable	pNewTgtPos;
	SmartScriptTable	pNewTgtDir;
	int	type = 0;

	if( !pH->GetParam( 2, type ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetHunterApproachParams: No type param." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 3, targetPos ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetHunterApproachParams: No target pos." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 4, targetDir ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetHunterApproachParams: No target dir." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 5, pNewTgtPos ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetHunterApproachParams: No out target pos." );
		return pH->EndFunction( 0 );
	}
	if( !pH->GetParam( 6, pNewTgtDir ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetHunterApproachParams: No out target dir." );
		return pH->EndFunction( 0 );
	}

	IAIObject* pAIObj = pEntity->GetAI();
	IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
	if(!pAIActor)
		return pH->EndFunction(0);


	Vec3	entPos = pEntity->GetPos();
	Vec3	diff = entPos - targetPos;

	// Use target direction if available
	if( targetDir.GetLength() > 0.0001f )
		diff = -targetDir;

	float	angle = atan2f( diff.y, diff.x );

	Vec3	attackPos[6];
	Vec3	attackDir[6];
	bool	bAttackSight[6] = { false, false, false, false, false, false };
	float	attackPosScore[6] = { 4, 8, 0, 8, 4, 0 };

	Vec3	bbox[2];
	bbox[0].Set( -3, -3, -3 );
	bbox[1].Set( 3, 3, 3 );

	AgentParameters	agentParams = pAIActor->GetParameters();

	float	attackHeight = agentParams.m_fAttackZoneHeight * 2.0f;
	float	attackRad = agentParams.m_fAttackRange * 0.25f;

	// Randomize the location a little.
	angle += (((cry_rand() * 10) / RAND_MAX) * 0.2f - 1.0f) * (gf_PI * 2.0f / 15.0f);
	attackHeight = (float)(cry_rand() / (float)RAND_MAX) * agentParams.m_fAttackZoneHeight * 0.25f;

	int	sightScore = 10;	// Is the target visible.
	int	heightScore = 2;	// Is the location near the attack height.
	int	randomScore = 4;	// Random offset.

	if( type == 0 )
	{
		// Attack position.
		attackPosScore[0] = 0;	// Enter direction.
		attackPosScore[1] = 8;
		attackPosScore[2] = 4;
		attackPosScore[3] = 0;	// Opposite direction.
		attackPosScore[4] = 4;
		attackPosScore[5] = 8;

		attackHeight += agentParams.m_fAttackZoneHeight * 2.5f;

		attackRad = agentParams.m_fAttackRange * (0.1f + 0.2f * (float)((cry_rand() * 4) / RAND_MAX) / 4.0f);

		sightScore = 10;	// Is the target visible.
		heightScore = 2;	// Is the location near the attack height.
		randomScore = 4;	// Random offset.
	}
	else if( type == 1 )
	{
		// Attack position.
		attackPosScore[0] = 16;	// Enter direction.
		attackPosScore[1] = 4;
		attackPosScore[2] = 2;
		attackPosScore[3] = 0;	// Opposite direction.
		attackPosScore[4] = 2;
		attackPosScore[5] = 4;

		attackHeight += agentParams.m_fAttackZoneHeight * 1.5f;

		attackRad = agentParams.m_fAttackRange * 0.05f;

		sightScore = 10;	// Is the target visible.
		heightScore = 2;	// Is the location near the attack height.
		randomScore = 2;	// Random offset.
	}

	int	best = 0;
	float bestScore = -1.f;
	int	numValidPos = 0;

	// Check the target height.
	/*	bool	bTooHighTargetObstacle = false;
	{
	float	targetRadius = 2.0f;
	// Make sure the point is above the terrain.
	float	terrainHeight = gEnv->p3DEngine->GetTerrainElevation( targetPos.x, targetPos.y );
	float	maxObstHeight = terrainHeight;

	IPhysicalEntity**	ppList = NULL;
	int	numEntites = pPhysWorld->GetEntitiesInBox( targetPos + Vec3( -1, -1, -1 ) * targetRadius, targetPos + Vec3( 1, 1, 1 ) * targetRadius, ppList, ent_static );
	for( int j = 0; j < numEntites; j++ )
	{
	pe_status_pos status;
	// Get bbox (status) from physics engine
	if(ppList[j]->GetType() == PE_LIVING)
	continue;
	ppList[j]->GetStatus( &status );

	float	height = status.pos.z + status.BBox[1].z; 

	if( height > maxObstHeight )
	maxObstHeight = height;
	}

	bTooHighTargetObstacle = (maxObstHeight - terrainHeight) > maxTargetObstacleHeight;
	}

	if( bTooHighTargetObstacle )
	{
	gEnv->pAISystem->LogComment("<CScriptBind_AI> ", "bTooHighTargetObstacle" );
	}*/

	for( int i = 0; i < 6; i++ )
	{
		// Calc new target position.
		angle += (float)i * (gf_PI * 2.0f / 6.0f);
		attackDir[i].Set( cosf( angle ), sinf( angle ), 0 );
		attackPos[i] = targetPos - attackDir[i] * attackRad;
		attackPos[i].z += attackHeight;

		// Make sure the point is above the terrain.
		/*		float	terrainHeight = gEnv->p3DEngine->GetTerrainElevation( attackPos[i].x, attackPos[i].y );
		float	maxObstHeight = terrainHeight;

		IPhysicalEntity**	ppList = NULL;
		int	numEntites = pPhysWorld->GetEntitiesInBox( attackPos[i] + bbox[0], attackPos[i] + bbox[1], ppList, ent_static );
		for( int j = 0; j < numEntites; j++ )
		{
		pe_status_pos status;
		// Get bbox (status) from physics engine
		if(ppList[j]->GetType() == PE_LIVING)
		continue;
		ppList[j]->GetStatus( &status );

		float	height = status.pos.z + status.BBox[1].z; 

		if( height > maxObstHeight )
		{
		maxObstHeight = height;
		attackPosScore[i] += blockingScore;
		}
		}

		bool	bTooHighObstacle = (maxObstHeight - terrainHeight) > maxObstacleHeight;

		maxObstHeight += attackHeight;*/

		/*
		virtual GraphNode *GetEnclosing(const Vec3 &pos, int navCap, GraphNode *pStart = 0 ,bool bOutsideOnly = false,
		float range = 0.0f, Vec3 * closestValid = 0) = 0;
		*/

		Vec3	closestValid( attackPos[i] );
		bool	bValidPos = false;

		if( gAIEnv.pGraph->GetEnclosing( attackPos[i], IAISystem::NAV_FLIGHT, 0.0f, 0, 0.0f, &closestValid ) )
		{
			bValidPos = true;
			if( closestValid != attackPos[i] )
			{
				attackPos[i] = closestValid;
				attackPos[i].x += pAIActor->GetMovementAbility().minFlightHeight;
			}
		}

		//		if( attackPos[i].z < maxObstHeight )
		//			attackPos[i].z = maxObstHeight;

		// Recalc the attack direction.
		attackDir[i] = targetPos - attackPos[i];

		// Check if the target point can be seen from the new target pos.

		ray_hit	hit;
		int	numHits = RayWorldIntersectionWrapper( attackPos[i], attackDir[i], ent_static, rwi_stop_at_pierceable, &hit, 1 );
		bAttackSight[i] = numHits > 0 ? false : true;

		// Normalize the direction. the unnormalized version is needed for the ray intersection.
		attackDir[i].NormalizeSafe();

		if( bAttackSight[i] )
			attackPosScore[i] += sightScore;

		// Favor positions closer to the real attack range.
		if( fabs( targetPos.z - attackPos[i].z ) < attackHeight * 1.1f )
			attackPosScore[i] += heightScore;

		// Check if the point is too close to the old ones.
		for( std::list<SLastApproachParam>::iterator ptIt = m_lstLastestApproachParam.begin(); ptIt != m_lstLastestApproachParam.end(); ++ptIt )
		{
			SLastApproachParam&	param = (*ptIt);
			Vec3	diff2 = attackPos[i] - param.pos;
			if( diff2.len2() < 10 * 10 )
				attackPosScore[i] += -30;
		}

		// Add small jitter into the selection.
		attackPosScore[i] += (cry_rand() * randomScore) / RAND_MAX;

		if( attackPosScore[i] > bestScore )
		{
			best = i;
			bestScore = attackPosScore[i];
		}

		//		if( bAttackSight[i] && !bTooHighObstacle && !bTooHighTargetObstacle )
		if( bAttackSight[i] && bValidPos )
			numValidPos++;
	}

	// Keep history of path points.
	SLastApproachParam	params;
	params.pos = attackPos[best];
	params.dir = attackDir[best];
	m_lstLastestApproachParam.push_back( params );

	// Keep the history count sane.
	if( m_lstLastestApproachParam.size() > 12 )
		m_lstLastestApproachParam.pop_front();

	gEnv->pAISystem->LogComment("<CScriptBind_AI> ", "%d valid pos", numValidPos );

	// Returns the calculated target position and direction.
	pNewTgtPos->SetValue( "x", attackPos[best].x );
	pNewTgtPos->SetValue( "y", attackPos[best].y );
	pNewTgtPos->SetValue( "z", attackPos[best].z );

	pNewTgtDir->SetValue( "x", attackDir[best].x );
	pNewTgtDir->SetValue( "y", attackDir[best].y );
	pNewTgtDir->SetValue( "z", attackDir[best].z );

	return pH->EndFunction( numValidPos );
}

//
//-------------------------------------------------------------------------------------------------------------------------
int CScriptBind_AI_G02::VerifyAlienTarget(IFunctionHandler * pH)
{
	// First param is the entity that is attacking, and the second param is the target location (script vector3).

	if(pH->GetParamCount() < 1)
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "GetAlienApproachParams: Too few parameters." );
		return pH->EndFunction( 0 );
	}

	//retrieve the entity
	Vec3 targetPos( 0, 0, 0 );

	if( !pH->GetParam( 1, targetPos ) )
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "VerifyAlienTarget: No target pos." );
		return pH->EndFunction( 0 );
	}

	// Check the target height.

	float	targetRadius = 3.0f;
	// Make sure the point is above the terrain.
	float	terrainHeight = gEnv->p3DEngine->GetTerrainElevation( targetPos.x, targetPos.y );
	float	maxObstHeight = terrainHeight;

	IPhysicalEntity**	ppList = NULL;
	const float	checkRadius = 4.0f;
	int	numEntites = gAIEnv.pWorld->GetEntitiesInBox( targetPos + Vec3( -checkRadius, -checkRadius, -checkRadius ) * targetRadius, targetPos + Vec3( checkRadius, checkRadius, checkRadius ) * targetRadius, ppList, ent_static );
	for( int j = 0; j < numEntites; j++ )
	{
		pe_status_pos status;
		// Get bbox (status) from physics engine
		if(ppList[j]->GetType() == PE_LIVING)
			continue;
		ppList[j]->GetStatus( &status );

		float	height = status.pos.z + status.BBox[1].z; 

		if( height > maxObstHeight )
			maxObstHeight = height;
	}

	if( (maxObstHeight - terrainHeight) > 3.0f )
		return pH->EndFunction( 0 );

	return pH->EndFunction( 1 );
}

//
//-------------------------------------------------------------------------------------------------------------------------
int CScriptBind_AI_G02::SetRefpointToAlienHidespot(IFunctionHandler * pH)
{
#ifndef SP_DEMO
	SCRIPT_CHECK_PARAMETERS(3);
	GET_ENTITY(1);
	if(!pEntity)
		return pH->EndFunction(0);	
	IAIObject* pAIObject = pEntity->GetAI();
	if(!pAIObject)
		return pH->EndFunction(0);	
	IPipeUser* pPipeUser = pAIObject->CastToIPipeUser();
	if (!pPipeUser)
		return pH->EndFunction(0);

	float rangeMin = 0.0f;
	float rangeMax = 1.0f;
	pH->GetParam(2,rangeMin);
	pH->GetParam(3,rangeMax);

	const int ALIEN_HIDESPOT = 504;
	const int COMBAT_TERRITORY = 342;

	Vec3 nearestPos(0,0,0);
	float nearestDist = FLT_MAX;

	const Vec3& searchPos = pAIObject->GetPos();
	Vec3 targetPos(searchPos);
	Vec3 targetDir(0,0,0);
	bool targetDirValid = false;
	if (IAIObject* pTarget = pPipeUser->GetAttentionTarget())
	{
		targetPos = pTarget->GetPos();
		if (pTarget->IsAgent())
		{
			targetDir = pTarget->GetViewDir();
			targetDirValid = true;
		}
	}

	// update unavailable hidespots
	CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
	for (unsigned i = 0; i < m_unreachableAlienHideSpots.size(); )
	{
		float dt = (curTime - m_unreachableAlienHideSpots[i].t).GetSeconds();
		// If the unreachable pos is older than 15s or in the future (can happen because of QL/QS), remove it.
		if (dt > 15.0f || dt < -0.00001f)
		{
			m_unreachableAlienHideSpots[i] = m_unreachableAlienHideSpots.back();
			m_unreachableAlienHideSpots.pop_back();
		}
		else
			++i;
	}

	std::vector<Vec3> occupiedPos;

	AutoAIObjectIter itBuddy(GetAISystem()->GetFirstAIObjectInRange(IAISystem::OBJFILTER_TYPE, AIOBJECT_PUPPET, searchPos, rangeMax + 1.0f, false));
	for (; itBuddy->GetObject(); itBuddy->Next())
	{
		IAIObject* pBuddyAI = itBuddy->GetObject();
		if (pBuddyAI == pAIObject) continue;
		if (!pBuddyAI->IsEnabled()) continue;
		if (!pBuddyAI->GetEntity()) continue;
		IPipeUser* pBuddyPipeUser = pBuddyAI->CastToIPipeUser();
		if (!pBuddyPipeUser)
			continue;

		occupiedPos.push_back(pBuddyAI->GetPos());

		if (!pBuddyAI->IsHostile(pAIObject))
			occupiedPos.push_back(pBuddyPipeUser->GetRefPoint()->GetPos());
	}

	int nBuildingID = -1;
	IVisArea* pGoalArea; 
	gAIEnv.pNavigation->CheckNavigationType(pAIObject->GetPos(), nBuildingID, pGoalArea, IAISystem::NAV_VOLUME);

	//	AutoAIObjectIter it(GetAISystem()->GetFirstAIObjectInRange(OBJFILTER_TYPE, ALIEN_HIDESPOT, searchPos, rangeMax, false));

	IAIObjectIter* it = 0;

	const char* szShapeName = GetAISystem()->GetEnclosingGenericShapeOfType(pAIObject->GetPos(), COMBAT_TERRITORY, true);
	if (szShapeName)
		it = GetAISystem()->GetFirstAIObjectInShape(IAISystem::OBJFILTER_TYPE, ALIEN_HIDESPOT, szShapeName, true);
	else
		it = GetAISystem()->GetFirstAIObjectInRange(IAISystem::OBJFILTER_TYPE, ALIEN_HIDESPOT, searchPos, rangeMax, false);

	if (!it)
		return pH->EndFunction(0);

	EntityId userId = pEntity->GetId();
	Vec3 refPointPos = pPipeUser->GetRefPoint()->GetPos();

	for (; it->GetObject(); it->Next())
	{
		IAIObject*	obj = it->GetObject();
		if (!obj->IsEnabled()) continue;

		const Vec3& pos = obj->GetPos();

		float d = Distance::Point_PointSq(searchPos, pos);
		if (d < sqr(rangeMin))
			continue;

		// Dont use same point twice in a row.
		if (Distance::Point_PointSq(refPointPos, pos) < 0.5f)
			continue;

		if (nBuildingID != -1)
		{
			if (!gAIEnv.pNavigation->IsPointInBuilding(pos, nBuildingID))
				continue;
		}

		bool unreachable = false;
		for (unsigned i = 0, ni = m_unreachableAlienHideSpots.size(); i < ni; ++i)
		{
			if (m_unreachableAlienHideSpots[i].entId != userId) continue;
			if (Distance::Point_PointSq(m_unreachableAlienHideSpots[i].pos, pos) < sqr(0.5f))
			{
				unreachable = true;
				break;
			}
		}
		if (unreachable)
			continue;

		bool occupied = false;
		for (unsigned i = 0, ni = occupiedPos.size(); i < ni; ++i)
		{
			if (Distance::Point_PointSq(occupiedPos[i], pos) < sqr(2.0f))
			{
				occupied = true;
				break;
			}
		}
		if (occupied)
			continue;

		if (targetDirValid)
		{
			Vec3 delta = pos - targetPos;
			delta.Normalize();
			float dot = targetDir.Dot(delta);
			const float thr = cosf(DEG2RAD(15));
			if (dot > thr)
			{
				d += sqr(500.0f);
				gEnv->pAISystem->AddDebugLine(pos, (pos+searchPos)*0.5f, 255,196,0, 5.0f);
			}
		}

		if (d < nearestDist)
		{
			ray_hit	hit;
			if (RayWorldIntersectionWrapper(pos, targetPos-pos, ent_static|ent_terrain, rwi_stop_at_pierceable, &hit, 1))
			{
				//				gEnv->pAISystem->AddDebugLine(searchPos, hit.pt, 255,0,0, 10.0f);
				nearestDist = d;
				nearestPos = pos;
			}
		}
	}
	SAFE_RELEASE(it);

	if (!nearestPos.IsZero())
	{
		gEnv->pAISystem->AddDebugLine(searchPos, nearestPos, 255,255,255, 10.0f);
		pPipeUser->SetRefPointPos(nearestPos);
		return pH->EndFunction(1);
	}
#endif
	return pH->EndFunction(0);
}

//
//-------------------------------------------------------------------------------------------------------------------------
int CScriptBind_AI_G02::MarkAlienHideSpotUnreachable(IFunctionHandler * pH)
{
#ifndef SP_DEMO
	SCRIPT_CHECK_PARAMETERS(1);
	GET_ENTITY(1);
	if(!pEntity)
		return pH->EndFunction(0);	
	IAIObject* pAIObject = pEntity->GetAI();
	if(!pAIObject)
		return pH->EndFunction(0);	
	IPipeUser* pPipeUser = pAIObject->CastToIPipeUser();
	if (!pPipeUser)
		return pH->EndFunction(0);

	Vec3 pos = pPipeUser->GetRefPoint()->GetPos();
	EntityId entId = pEntity->GetId();
	CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();

	// If the spot already exists, just update time.
	for (unsigned i = 0, ni = m_unreachableAlienHideSpots.size(); i < ni; ++i)
	{
		if (m_unreachableAlienHideSpots[i].entId == entId && Distance::Point_PointSq(pos, m_unreachableAlienHideSpots[i].pos) < sqr(0.1f))
		{
			m_unreachableAlienHideSpots[i].t = curTime;
			return pH->EndFunction(0);
		}
	}

	// Add new spot
	m_unreachableAlienHideSpots.push_back(SUnreachableAlienHideSpot(curTime, pos, entId));
#endif

	return pH->EndFunction(0);
}

//-----------------------------------------------------------------------------------------------------------


//
// wrapper, to make it use tick-counter
// Copy/pasted from CScriptbind_AI
//-----------------------------------------------------------------------------------------------------------
int CScriptBind_AI_G02::RayWorldIntersectionWrapper(Vec3 org,Vec3 dir, int objtypes, unsigned int flags, ray_hit *hits,int nMaxHits,
																								IPhysicalEntity **pSkipEnts,int nSkipEnts, void *pForeignData,int iForeignData, int iCaller)
{
	int rwiResult(0);
	rwiResult = gAIEnv.pWorld->RayWorldIntersection( org, dir, objtypes, flags, hits,nMaxHits,
		pSkipEnts, nSkipEnts, pForeignData,iForeignData, "RayWorldIntersectionWrapper(AI)", 0, iCaller);
	return rwiResult;
}


// 
//====================================================================
// checks if an AI can jump to a point
// dest = destination point
// theta = input angle (in degrees)
// maxheight = max allowed jump height (in meters)
// flags (AI_JUMP_ON_GROUND -> the point will be projected on ground - AI_JUMP_CHECK_COLLISION -> collision will be checked on the jump parabula)
// retVelocity = if jump can be performed, this is the returned initial velocity
// tDest = if jump can be performed, this is the returned time to reach destination
// pSkipEnt = optional, skip this entity in collision check
// startPos = optional, compute the jump start from this position instead of AI's position
// Returns: true if the jump can be performed
//====================================================================
int CScriptBind_AI_G02::CanJumpToPoint(IFunctionHandler * pH)
{
	if (pH->GetParamCount() < 6)
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "CanJumpToPoint: Not enough parameters." );
		return pH->EndFunction();
	}
	GET_ENTITY(1);
	// Get bbox (status) from physics engine
	if(!pEntity)
		return pH->EndFunction();
	IEntity* pEntity2 = NULL;

	//	IPhysicalEntity* pPhysEntity = pEntity->GetPhysics();
	//	if(pPhysEntity)
	{
		//		float radius;
		//		pe_status_pos status;

		float theta=0; // initial angle
		float tB=0; // returned time at which entity will reach B
		float maxheight; // maximum height allowed
		Vec3 B(ZERO); // final position (parameter)
		Vec3 V(ZERO); // returned initial velocity
		IAIObject* pAI = pEntity->GetAI(); 
		if(!pAI)
		{
			gEnv->pAISystem->Warning("<CScriptBind_AI> ", "CanJumpToPoint: Not enough parameters." );
			return pH->EndFunction();
		}

		IAIActorProxy *pAIActorProxy = pAI->GetProxy();
		if (!pAIActorProxy)
		{
			gEnv->pAISystem->Warning("<CScriptBind_AI> ", "CanJumpToPoint(): Entity '%s' does not have puppet AI proxy.", pEntity->GetName());
			return pH->EndFunction();
		}

		SmartScriptTable	retVelocity;

		ScriptVarType type = pH->GetParamType(2);
		switch(type)
		{
		case svtString:
			if(pAI)
			{
				IPipeUser* pPipeUser = pAI->CastToIPipeUser();
				if(pPipeUser)
				{
					const char* name;
					if(pH->GetParam(2,name))
					{
						IAIObject* pAItarget = pPipeUser->GetSpecialAIObject(name);
						if(pAItarget)
							B = pAItarget->GetPos();
					}
				}
			}
			break;
		case svtPointer:
			{
				pEntity2 = GetEntityFromParam( pH, 2 );
				if(pEntity2)
					B = pEntity2->GetWorldPos();
			}
			break;
		case svtObject:
			{
				SmartScriptTable vecTable;
				if(pH->GetParam(2,vecTable))
				{	
					float x,y,z;
					if(vecTable->GetValue("x",x) && vecTable->GetValue("y",y) && vecTable->GetValue("z",z))
						B.Set(x,y,z);
				}
			}
			break;
		default:
			break;

		}

		if(B.IsZero())
		{
			gEnv->pAISystem->Warning("<CScriptBind_AI> ", "CanJumpToPoint: wrong parameter 2 format" );
			return pH->EndFunction();
		}
		else if(_isnan(B.x+B.y+B.z))
		{
			gEnv->pAISystem->Warning("<CScriptBind_AI> ", "CanJumpToPoint: NAN in destination position" );
			return pH->EndFunction();
		}


		int flags;
		if( !pH->GetParam( 6, retVelocity )) 
			return pH->EndFunction();

		pH->GetParam(5,flags);

		pH->GetParam(3,theta);

		pH->GetParam(4,maxheight);


		IPhysicalEntity* pSkipEnt = NULL;

		if (pH->GetParamCount() >6)
		{
			if(pH->GetParamType(7)==svtPointer)
			{
				pEntity2 = GetEntityFromParam( pH, 7 );
				if(pEntity2)
					pSkipEnt = pEntity2->GetPhysics();
			}
		}
		Vec3 altStartPos;
		Vec3* pAltStartPos = NULL;
		if (pH->GetParamCount() >7 && pH->GetParamType(8)!=svtNull)
		{
			pH->GetParam(8,altStartPos);
			pAltStartPos = &altStartPos;
		}

		if (CanJumpToPointHelper(pAIActorProxy, pEntity, B, theta, maxheight, flags,V, tB, pSkipEnt, pAltStartPos))
		{

			retVelocity->SetValue( "x", V.x );
			retVelocity->SetValue( "y", V.y );
			retVelocity->SetValue( "z", V.z );


			CCCPOINT(CScriptbind_AI_CanJumpToPoint);

			return pH->EndFunction(tB);
		}
	}
	return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
// The helper functions for
// int CScriptBind_AI::CanJumpToPoint(IFunctionHandler * pH)
//////////////////////////////////////////////////////////////////////////

//
//----------------------------------------------------------------------------------------------------------
float GetTofPointOnParabola(const Vec3& A, const Vec3& V, const Vec3& P, float g)
{
	float a = -g/2;
	float b = V.z;
	float c = A.z - P.z;
	float d = b*b - 4*a*c;
	if(d<0)
		return -1.f;
	float sqrtd = sqrtf(d);
	float tP = (-b + sqrtd)/ (2*a);
	float tP1 = (-b - sqrtd)/ (2*a);
	float y = V.y*tP + A.y;
	float y1 = V.y*tP1 + A.y;
	if(fabs(P.y - y) > fabs(P.y - y1) )
		return tP1;
	return tP;
}

//
//----------------------------------------------------------------------------------------------------------
Vec3 GetPointOnParabola(const Vec3& A, const Vec3& V, float t, float g)
{
	return Vec3(V.x * t + A.x, V.y * t + A.y, -g/2*t*t + V.z*t + A.z);
}

//====================================================================
// IntersectSweptSphere
// hitPos is optional - may be faster if 0
//====================================================================
bool IntersectSweptSphereWrapper(IAIActorProxy* pAIActorProxy, Vec3 *hitPos, float& hitDist, const Lineseg& lineseg, float radius,IPhysicalEntity** pSkipEnts, int nSkipEnts, int additionalFilter)
{
	IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
	primitives::sphere spherePrim;
	spherePrim.center = lineseg.start;
	spherePrim.r = radius;
	Vec3 dir = lineseg.end - lineseg.start;

	geom_contact *pContact = 0;
	geom_contact **ppContact = hitPos ? &pContact : 0;
	int geomFlagsAll=0;
	int geomFlagsAny=rwi_stop_at_pierceable|(geom_colltype_player<<rwi_colltype_bit);

	float d = pPhysics->PrimitiveWorldIntersection(spherePrim.type, &spherePrim, dir, 
		ent_static | ent_terrain | ent_ignore_noncolliding | additionalFilter, ppContact, 
		geomFlagsAll, geomFlagsAny, 0,0,0, pSkipEnts, nSkipEnts);

	if (d > 0.0f)
	{
		hitDist = d;
		if (pContact && hitPos)
			*hitPos = pContact->pt;
		return true;
	}
	else
	{
		return false;
	}
}

// 
//====================================================================
bool CanJumpToPointHelper(IAIActorProxy* pAIActorProxy, IEntity* pEntity, Vec3& dest, float theta, float maxheight, int flags, Vec3& retVelocity, float& tDest, IPhysicalEntity* pSkipEnt, Vec3* pStartPos)
{
	//IEntity* pEntity = m_pGameObject->GetEntity();
	if(!pEntity)
	{
		gEnv->pAISystem->Warning(">> ","CAIProxy::OnShoot null entity");
		return false;
	}

	Vec3 hitPos;
	float hitdist;


	SAIBodyInfo bodyInfo;
	pAIActorProxy->QueryBodyInfo(bodyInfo);
	float radius = bodyInfo.colliderSize.GetSize().x;
	float radius2 = bodyInfo.colliderSize.GetSize().y;
	if(radius < radius2)
		radius = radius2;
	float myHeight = bodyInfo.colliderSize.GetSize().z;
	radius = sqrtf(radius*radius + myHeight*myHeight);
	radius = radius*1.2f/2;

	float tB=0; // time at which entity will reach B
	Vec3 B(dest);
	Vec3 A(pStartPos? *pStartPos : pEntity->GetWorldPos());//initial position

	theta = DEG2RAD(theta);

	bool bCheckCollision = (flags & AI_JUMP_CHECK_COLLISION)!=0;

	IPhysicalEntity* pSkipMyEnt = pEntity->GetPhysics();
	IPhysicalEntity* SkipEnt[2];
	std::vector<IPhysicalEntity*> VSkipEnt;

	SkipEnt[0] = pSkipMyEnt;

	int nSkipMyEnts = pSkipMyEnt ? 1 : 0;
	if(pSkipEnt)
		SkipEnt[nSkipMyEnts] = pSkipEnt;

	int nSkipEnts = pSkipEnt ? nSkipMyEnts + 1 : nSkipMyEnts;


	if(flags & AI_JUMP_ON_GROUND)
	{
		// project B on terrain		
		ray_hit	hit;
		Vec3 rayDir(0,0,-30);
		float heightTerrain = gEnv->p3DEngine->GetTerrainElevation( B.x, B.y );
		if(B.z < heightTerrain)
			B.z = heightTerrain;

		if (gEnv->pPhysicalWorld->RayWorldIntersection(B, rayDir, ent_static|ent_terrain, rwi_stop_at_pierceable|(geom_colltype_player<<rwi_colltype_bit), &hit, 1,SkipEnt, nSkipEnts))
			B = hit.pt;
	}



	Vec3 AB = B-A;

	if(AB.z > maxheight)
	{
		return false;
	}

	float cosTheta = cosf(theta);

	Vec3 dir(AB);
	if(fabs(cosTheta)< 0.01f)
	{ // almost vertical, let's approximate to it to avoid lua imprecisions
		cosTheta = 0;
		theta = gf_PI/2;
		dir.x = dir.y = 0;
	}

	dir.z=0;
	float x = dir.GetLength();
	dir.NormalizeSafe();

	float tanTheta = cosTheta != 0 ? tanf(theta) : 0;

	// check if the initial angle is too low for destination
	float cosDestAngle = dir.Dot(AB.GetNormalizedSafe());

	// adding a threshold to avoid super velocities when destination angle is just slightly below requested angle
	float thr = 0.05f;
	// (assuming negative gravity)
	if(B.z > A.z) // destination above
	{
		if( theta<=0 || (cosTheta > cosDestAngle - thr) && cosTheta!=0)
			return false;
	}
	else if(theta<=0 && (cosTheta < cosDestAngle + thr) && cosTheta!=0)
		return false;

	float g = 9.81f;  

	IPhysicalEntity *phys = pEntity->GetPhysics();
	Vec3 vAddVelocity(ZERO);

	if(phys)
	{
		pe_player_dynamics	dyn;
		phys->GetParams(&dyn);

		// use current actual gravity	of the object
		g = -dyn.gravity.z;

		if(flags & AI_JUMP_RELATIVE)
		{
			// jump in the entity's reference frame
			pe_status_dynamics	dyns;
			phys->GetStatus(&dyns);
			vAddVelocity = dyns.v;
		}

	}

	float z0 = AB.z;
	float v; //initial velocity (module), the incognita

	if(cosTheta!=0)
	{
		float t0 = sqrtf(fabs(2/g *(z0 - tanTheta *x)));
		v = x/(cosTheta*t0);
	}
	else
	{
		if(x == 0) // jumping vertically
		{	
			// use the given height as parameter, v = v(AB.z)
			if(z0<0)
				return false;
			v = sqrtf(2* z0 *g);
		}
		else 
			return false;
	}

	// check max height
	float Xc; // max distance on origin's plane
	float ZMax; // max height with v
	Vec3 dir90;//(dir.y,-dir.x,0);
	Vec3 VelN;//(dir.GetRotated(dir90,theta));

	if(x !=0) // not vertical
	{
		dir90.Set(dir.y,-dir.x,0);
		VelN = dir.GetRotated(dir90,theta);
		if(flags & AI_JUMP_RELATIVE)
		{
			VelN = (VelN*v + vAddVelocity);
			v = VelN.GetLength();
			if(v>0)
				VelN /= v;
		}
		Xc = sinf(2*theta)*v*v /g; // max distance on origin's plane
		ZMax = Xc * tanTheta / 4; // max height with v
		if(ZMax > maxheight)
			return false;
	}
	else
	{
		VelN.Set(0,0,1);
		if(flags & AI_JUMP_RELATIVE)
		{
			VelN = (VelN*v + vAddVelocity);
			v = VelN.GetLength();
			if(v>0)
				VelN /= v;
		}
		Xc = 0;
		ZMax = maxheight;
	}


	Vec3 V(VelN * v); // initial velocity vector;

	if(bCheckCollision)
	{

		//---------------------------------------------------
		//check obstacles in trajectory
		//---------------------------------------------------
		Vec3 P(A + VelN*radius);

		if(x< radius)
		{
			// (almost) vertical jump
			Lineseg T(P,B);
			gEnv->pAISystem->AddDebugLine(P,B,128,40,255,5);

			if(IntersectSweptSphereWrapper(pAIActorProxy, &hitPos, hitdist, T, radius*1.2f, nSkipEnts ? SkipEnt : NULL, nSkipEnts))
				return false;
		}
		else if(theta>0)
		{

			//float halfXc = Xc/2;

			float Xm = Xc/2;
			Vec3 Zm(A.x + dir.x*Xm, A.y + dir.y*Xm, A.z + ZMax);

			/*
			x = Vx*t + Ax
			y = Vy*t + Ay
			z = -g/2*t^2 + Vz*t + Az
			*/
			// Zm is such that t = ...
			//float vx = v*cosTheta;
			// x=vt
			//float tZm = Xm/vx;

			// B is such that t = ...
			tB = GetTofPointOnParabola(A,V,B,g); // time to reach destination
			if(tB<0) // no solution
				return false;

			float radiusd = 2*radius;
			float t = 0;
			P = Vec3(A + VelN * radius); // start point for collision check
			Vec3 Vb(V.x,V.y, -g*tB + V.z); // velocity in end point B
			float speedB = Vb.GetLength();
			if(speedB==0)
				return false;

			float tB1 = tB - radiusd/speedB; // time to reach approximate point to end collision check

			float tStep = tB/4;

			//Vec3 Q(B - Vb.GetNormalizedSafe() * radiusd); // end point for collision check

			float zOffset = bodyInfo.colliderSize.GetCenter().z;
			P.z += zOffset;

			static ICVar* aiDeBugDraw = gEnv->pConsole->GetCVar("ai_DebugDraw");
			if(aiDeBugDraw && aiDeBugDraw)
			{
				float step = tB/40;
				for(float t2= 0;t2<tB;t2+=step)
				{
					Vec3 P1 = GetPointOnParabola(A,V,t2,g);
					Vec3 P2 = GetPointOnParabola(A,V,t2+step,g);
					gEnv->pAISystem->AddDebugLine(P1,P2,255,255,0,6);
				}
			}

			ray_hit hit;

			for(int i=0;i<4;i++)
			{
				float t1 = t + tStep;
				//					Vec3 P1 = getPointOnParabula(A,V,t1,g);
				if(tB1 < t1 || i==3 && tB1>=t1) // next point will be end point
				{
					i = 4; // last loop
					t1 = tB1;
					//P1 = Q;
				}
				Vec3 P1 = GetPointOnParabola(A,V,t1,g);
				Vec3 Pr(P.x,P.y, P.z - zOffset); 
				if( gEnv->pPhysicalWorld->RayWorldIntersection( Pr, P1 - Pr, ent_static|ent_terrain|ent_living, rwi_stop_at_pierceable|(geom_colltype_player<<rwi_colltype_bit), &hit, 1,SkipEnt,nSkipEnts ) )
					return false;
				P1.z += zOffset;
				Lineseg T(P ,P1 );
				gEnv->pAISystem->AddDebugLine(P,P1,40,40,255,8);
				if(IntersectSweptSphereWrapper(pAIActorProxy, &hitPos, hitdist, T, radius, nSkipEnts ? SkipEnt:NULL, nSkipEnts,ent_sleeping_rigid))
					return false;
				P = P1;	
				t = t1;
			}

		}
		else //theta <=0
		{
			// TO DO not supported yet
			// ZMax calculations above won't be valid here
			return false;
		}

	}

	gEnv->pAISystem->AddDebugLine(A,A+VelN*4,255,255,255,5);

	// compute duration
	if(tB==0)
	{
		float a = -g/2;
		float b = V.z;
		float c = A.z - B.z;
		float d = b*b - 4*a*c;
		float sqrtd = (x==0? 0:sqrtf(d));
		tB = (-b + sqrtd)/ (2*a);
		float tB1 = (-b - sqrtd)/ (2*a);
		float y = V.y*tB + A.y;
		float y1 = V.y*tB1 + A.y;
		if(fabs(B.y - y) > fabs(B.y - y1) )
			tB = tB1;
	}

	retVelocity = V;
	tDest = tB;

	return true;
}
