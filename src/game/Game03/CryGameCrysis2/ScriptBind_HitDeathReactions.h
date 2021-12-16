/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description: 

-------------------------------------------------------------------------
History:
- 13:11:2009	16:25 : Created by David Ramos
*************************************************************************/
#ifndef __SCRIPT_BIND_HIT_DEATH_REACTIONS_H
#define __SCRIPT_BIND_HIT_DEATH_REACTIONS_H

class CHitDeathReactions;
class CPlayer;
struct HitInfo;

#include <CryExtension/CrySharedPtr.h>
typedef cryshared_ptr<CHitDeathReactions> CHitDeathReactionsPtr;
typedef cryshared_ptr<CHitDeathReactions> CHitDeathReactionsConstPtr;

class CScriptBind_HitDeathReactions : public CScriptableBase
{
public:
	CScriptBind_HitDeathReactions(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_HitDeathReactions();

	// Executes a hit reaction using the default C++ execution code
	// Params:
	// reactionParams = script table with the reaction parameters
	int										ExecuteHitReaction (IFunctionHandler *pH, SmartScriptTable reactionParams);

	// Executes a death reaction using the default C++ execution code
	// Params:
	// reactionParams = script table with the reaction parameters
	int										ExecuteDeathReaction (IFunctionHandler *pH, SmartScriptTable reactionParams);

	// Ends the current reaction
	int										EndCurrentReaction (IFunctionHandler *pH);

	// Run the default C++ validation code and returns its result
	// Params:
	//	reactionParams = script table with the reaction parameters
	//	scriptHitInfo = script table with the hit info
	// Return:
	//	TRUE is the validation was successful, FALSE otherwise
	int										IsValidReaction (IFunctionHandler *pH, SmartScriptTable reactionParams, SmartScriptTable scriptHitInfo);

	// Starts an animation through the HitDeathReactions. Pauses the animation graph while playing it
	// and resumes automatically when the animation ends
	// Params:
	// (sAnimName, bool bLoop = false, float fBlendTime = 0.2f, int iSlot = 0, int iLayer = 0, float fAniSpeed = 1.0f)
	int										StartReactionAnim(IFunctionHandler *pH);

	// Ends the current reaction anim, if any
	int										EndReactionAnim(IFunctionHandler *pH);

	// Sets the value of the last played destructible parts event on the associated HitDeathReaction
	// Params:
	//	szLastEventName = name of the last destructible event
	int										SetDestructiblesLastEvent(IFunctionHandler *pH, const char* szLastEventName);

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

private:
	CPlayer*							GetAssociatedActor(IFunctionHandler *pH) const;
	CHitDeathReactionsPtr GetHitDeathReactions(IFunctionHandler *pH) const;
	void									CreateHitInfoFromScript(SmartScriptTable scriptHitInfo, HitInfo &hitInfo) const;

	SmartScriptTable	m_pParams;

	ISystem*					m_pSystem;
	IGameFramework*		m_pGameFW;
};

#endif // __SCRIPT_BIND_HIT_DEATH_REACTIONS_H
