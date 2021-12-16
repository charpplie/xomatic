//------------------------------------------
// Movement controller for prototype pinger
//
// 11-08-08: Benito G.R. 
//-------------------------------------------

#ifndef __PINGER_MOVEMENTCONTROLLER_H__
#define __PINGER_MOVEMENTCONTROLLER_H__

#pragma once

#include "IMovementController.h"
#include "Actor.h"

class CPinger;

class CPingerMovementController : public IActorMovementController
{
public:

	CPingerMovementController(CPinger* pPinger);
	~CPingerMovementController();

	//IActorMovementController
	virtual void Reset(){};
	virtual bool Update( float frameTime, SActorFrameMovementParams& params );
	virtual bool GetStats(SStats& stats){ return false; };
	virtual void PostUpdate( float frameTime ){};
	virtual void Release();
	virtual void BindInputs( IAnimationGraphState * ) {};
	virtual void Serialize(TSerialize &ser){};
	//~IActorMovementController

	//IMovementController
	virtual bool RequestMovement( CMovementRequest& request );
	ILINE virtual void GetMovementState( SMovementState& state ) { state = m_currentMovementState; };
	virtual bool GetStanceState( const SStanceStateQuery& query, SStanceState& state );
	//~IMovementController

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const;
private:

	void	UpdateCurMovementState(const SActorFrameMovementParams& params);

	CPinger*	m_pPinger;

	CMovementRequest m_currentMovementRequest;
	SMovementState m_currentMovementState;
};

#endif