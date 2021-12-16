/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Simulation mode for AI movement by sending instructions to
				an AI on where to pathfind in simulation mode
  
 -------------------------------------------------------------------------
  History:
  - 15:07:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __AI_MOVE_SIMULATION_H__
#define __AI_MOVE_SIMULATION_H__

#include <MovementRequestID.h>

class CAIMoveSimulation
{
public:
	CAIMoveSimulation();
	virtual ~CAIMoveSimulation();

	void OnSelectionChanged();
	void CancelMove();
	bool UpdateAIMoveSimulation(CViewport *pView, const CPoint& point);

private:
	bool GetAIMoveSimulationDestination(CViewport *pView, const CPoint& point, Vec3& outGotoPoint) const;
	bool SendAIMoveSimulation(IEntity* pEntity, const Vec3& vGotoPoint);

	Vec3 m_vGotoPoint;
	Vec3 m_vLastRefPoint;
	GUID m_selectedAI;
	MovementRequestID m_movementRequestID;
};

#endif //__AI_MOVE_SIMULATION_H__
