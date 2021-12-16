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

#include "StdAfx.h"
#include "Scriptbind_AI_G04.h"

#include "Puppet.h"

#undef GET_ENTITY
#define GET_ENTITY( i )		\
	ScriptHandle	hdl;			\
	pH->GetParam( i, hdl ); \
	EntityId nID = static_cast<EntityId>(hdl.n);	\
	IEntity*	pEntity = gEnv->pEntitySystem->GetEntity( nID );

//====================================================================

CScriptBind_AI_G04::CScriptBind_AI_G04() 
{
	Init( gEnv->pScriptSystem, gEnv->pSystem );
	SetGlobalName( "AI_G04" );

	RegisterGlobals();
	RegisterFunctions();
}

//====================================================================

void CScriptBind_AI_G04::RegisterGlobals()
{

}

//====================================================================

void CScriptBind_AI_G04::RegisterFunctions()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME	& CScriptBind_AI_G04::

	SCRIPT_REG_FUNC(SetStrafeParams);
	SCRIPT_REG_FUNC(GetSpeed);
	SCRIPT_REG_FUNC(SetSpeed);
	SCRIPT_REG_FUNC(PlayCustomReadabilitySound);
}

//====================================================================
// SetStrafeParams
//====================================================================
int CScriptBind_AI_G04::SetStrafeParams(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(4);
	GET_ENTITY(1);

	IAIObject* pAIObject = pEntity->GetAI();
	if( pAIObject != NULL )
	{
		CPuppet* pPuppet = pAIObject->CastToCPuppet();
		if( pPuppet != NULL )
		{
			float fStart = 0.0f;
			pH->GetParam( 2, fStart );

			float fEnd = 0.0f;
			pH->GetParam( 3, fEnd);

			bool bWhileMoving = false;						
			pH->GetParam( 4, bWhileMoving );

			pPuppet->SetAllowedStrafeDistances( fStart, fEnd, bWhileMoving );
		}
	}

	return pH->EndFunction();
}

//====================================================================
// GetSpeed
//====================================================================
int CScriptBind_AI_G04::GetSpeed(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	GET_ENTITY(1);

	IAIObject* pAIObject = pEntity->GetAI();
	if (!pAIObject)
	{
		GetAISystem()->Warning("<CScriptBind_AI> ", "GetSpeed(): entity is not an AIObject");
		return pH->EndFunction();
	}

	IAIActor *pAIActor = pAIObject->CastToIAIActor();
	if (!pAIActor)
	{
		GetAISystem()->Warning("<CScriptBind_AI> ", "GetSpeed(): entity is not an AIActor");
		return pH->EndFunction();
	}

	SOBJECTSTATE* pState = pAIActor->GetState();
	if (!pState)
	{
		GetAISystem()->Warning("<CScriptBind_AI> ", "GetSpeed(): missing state object in AIActor");
		return pH->EndFunction();
	}

	return pH->EndFunction(pState->fMovementUrgency);
}

//====================================================================
// SetSpeed
//====================================================================
int CScriptBind_AI_G04::SetSpeed(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(2);
	GET_ENTITY(1);


	IAIObject* pAIObject = pEntity->GetAI();
	if (!pAIObject)
	{
		GetAISystem()->Warning("<CScriptBind_AI> ", "SetSpeed(): entity is not an AIObject");
		return pH->EndFunction();
	}

	IAIActor *pAIActor = pAIObject->CastToIAIActor();
	if (!pAIActor)
	{
		GetAISystem()->Warning("<CScriptBind_AI> ", "SetSpeed(): entity is not an AIActor");
		return pH->EndFunction();
	}

	SOBJECTSTATE* pState = pAIActor->GetState();
	if (!pState)
	{
		GetAISystem()->Warning("<CScriptBind_AI> ", "SetSpeed(): missing state object in AIActor");
		return pH->EndFunction();
	}
	
	int iSpeed = 0;
	if (!pH->GetParam(2, iSpeed))
	{
		GetAISystem()->Warning("<CScriptBind_AI> ", "SetSpeed(): invalid movement urgency speed specified");
		return pH->EndFunction();
	}

	// There is code/configuration elsewhere that means we won't even move at all if we select 
	// a "pseudo-speed" value that is too low (but still non-zero). So we're forced to hard-code these 
	// numbers in the hope that stuff elsewhere won't change because the interface makes us use a floating point number 
	// ("pseudoSpeed") to represent discrete states than a finite set of values.
	if (iSpeed == -4)
		pState->fMovementUrgency = -AISPEED_SPRINT;
	if (iSpeed == -3)
		pState->fMovementUrgency = -AISPEED_RUN;
	if (iSpeed == -2)
		pState->fMovementUrgency = -AISPEED_WALK;
	if (iSpeed == -1)
		pState->fMovementUrgency = -AISPEED_SLOW;
	if (iSpeed == 0)
		pState->fMovementUrgency = AISPEED_SLOW;
	else if (iSpeed == 1)
		pState->fMovementUrgency = AISPEED_WALK;
	else if (iSpeed == 2)
		pState->fMovementUrgency = AISPEED_RUN;
	else if (iSpeed >= 3)
		pState->fMovementUrgency = AISPEED_SPRINT;

	return pH->EndFunction();
}

//====================================================================
// PlayCustomReadabilitySound
//====================================================================
int CScriptBind_AI_G04::PlayCustomReadabilitySound(IFunctionHandler *pH)
{//(CAIHandler* pActor, ReadabilitySoundEntry* curSoundEntry, bool playSoundAtActorTarget, bool stopPreviousSound)

	SCRIPT_CHECK_PARAMETERS(2);
	GET_ENTITY(1);

	const char *sFileName = NULL;
	if ( !pH->GetParam(2,sFileName) )
	{
		GetAISystem()->Error("<CScriptBind_AI> ", "PlayCustomReadabilitySound(): no sound file name given.");
		return pH->EndFunction();
	}

	ISoundSystem *pSoundSystem = gEnv->pSoundSystem;

	IAIObject* pAI = pEntity->GetAI();
	if(!pAI )//|| !pAI->IsEnabled())
		return false;

	IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*) pEntity->GetProxy( ENTITY_PROXY_SOUND );
	if(!pSoundProxy)
		if (pEntity->CreateProxy(ENTITY_PROXY_SOUND ))
			pSoundProxy = (IEntitySoundProxy*)pEntity->GetProxy(ENTITY_PROXY_SOUND);

	if (pSoundProxy)
	{
		//// Stop the previously playing sound.
		//if (stopPreviousSound && pActor->m_ReadibilitySoundID != INVALID_SOUNDID)
		//{
		//	pSoundProxy->StopSound(pActor->m_ReadibilitySoundID);
		//	pActor->m_ReadibilitySoundID = INVALID_SOUNDID;
		//}

		// sound proxy uses head pos on dialog sounds
		ISound *pSound = gEnv->pSoundSystem->CreateSound( sFileName, FLAG_SOUND_DEFAULT_3D | FLAG_SOUND_VOICE );

		if (pSound)
		{
//			pActor->m_ReadibilitySoundID = pSound->GetId();
//			pSound->AddEventListener( pActor, "AIReadibilityManager" );
			pSound->SetSemantic(eSoundSemantic_AI_Readability);
//			pActor->m_bSoundFinished = false;
			pSoundProxy->PlaySound(pSound, Vec3(ZERO), FORWARD_DIRECTION);
		}
		else
		{
			// failed to play the sound.
			//gEnv->pAISystem->Warning("<CAIReadabilityManager> ", "Entity:'%s' Pack:'%s' -  Cannot play sound:'%s'", pEntity->GetName(), GetSoundPackName(pActor->GetSoundPack()), curSoundEntry->m_fileName.c_str());
//			pActor->m_bSoundFinished = true;
		}
	}

	return pH->EndFunction();
}