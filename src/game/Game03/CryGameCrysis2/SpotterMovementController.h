#ifndef __SPOTTER_MOVEMENTCONTROLLER_H__
#define __SPOTTER_MOVEMENTCONTROLLER_H__

#pragma once

#include "IMovementController.h"
#include "Actor.h"

class CSpotter;

class CSpotterMovementController : public IActorMovementController
{
public:

	CSpotterMovementController(CSpotter* pSpotter);
	~CSpotterMovementController();

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

	CSpotter*	m_pSpotter;

	CMovementRequest m_currentMovementRequest;
	SMovementState m_currentMovementState;
};

#endif