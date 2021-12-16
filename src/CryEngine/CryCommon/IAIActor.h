#ifndef IAIActor_h
#define IAIActor_h

#include "IAgent.h"
#include "IPathfinder.h"

struct IAIActor : public IAIPathAgent
{
	virtual ~IAIActor() {}

	virtual SOBJECTSTATE * GetState() = 0;
	virtual const SOBJECTSTATE * GetState() const = 0;
	virtual IAIActorProxy* GetProxy() const = 0;
	virtual void SetSignal(int nSignalID, const char * szText, IEntity *pSender=0, IAISignalExtraData *pData=NULL, uint32 crcCode = 0) = 0;
	virtual void NotifySignalReceived(const char* szText, IAISignalExtraData* pData=NULL, uint32 crcCode = 0) = 0;
	virtual const AgentParameters& GetParameters() const = 0;
	virtual void SetParameters(AgentParameters &pParams)=0;
	virtual const AgentMovementAbility& GetMovementAbility() const = 0;
	virtual void SetMovementAbility(AgentMovementAbility &pParams)=0;
	virtual void ParseParameters(const AIObjectParameters &params)=0;
	virtual bool CanAcquireTarget(IAIObject* pOther) const = 0;
	virtual void ResetPerception() = 0;
	virtual bool IsActive() const = 0;

	virtual void EnablePerception(bool enable) = 0;
	virtual bool IsPerceptionEnabled() const = 0;

	virtual void RegisterBehaviorListener(IActorBehaviorListener* listener) = 0;
	virtual void UnregisterBehaviorListener(IActorBehaviorListener* listener) = 0;
	virtual void BehaviorEvent(EBehaviorEvent event) = 0;
	virtual void BehaviorChanged(const char* current, const char* previous) = 0;

	virtual bool SetBehaviorTreeProfile(const char* szProfile) { return false; }
	virtual IBlackBoard* GetBehaviorBlackBoard() { return NULL; }
};


#endif