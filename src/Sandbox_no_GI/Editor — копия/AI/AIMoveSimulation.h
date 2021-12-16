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

#include <IAgent.h>

class CAIMoveSimulation : public IGoalPipeListener
{
public:
	CAIMoveSimulation();
	virtual ~CAIMoveSimulation();

	// IGoalPipeListener
	virtual void OnGoalPipeEvent(IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId);
	//~IGoalPipeListener

	void OnSelectionChanged();
	void CancelMove();
	bool UpdateAIMoveSimulation(CViewport *pView, const CPoint& point);

private:
	bool GetAIMoveSimulationDestination(CViewport *pView, const CPoint& point, Vec3& outGotoPoint) const;
	bool SendAIMoveSimulation(IAIObject *pAI, const Vec3& vGotoPoint);

	Vec3 m_vGotoPoint;
	Vec3 m_vLastRefPoint;
	GUID m_selectedAI;
	int m_iGoalPipeId;
};

#endif //__AI_MOVE_SIMULATION_H__
