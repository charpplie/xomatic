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

#include "StdAfx.h"
#include "AIMoveSimulation.h"
#include "Objects\EntityObject.h"
#include "GameEngine.h"
#include "IAIObject.h"
#include "Viewport.h"

//////////////////////////////////////////////////////////////////////////
CAIMoveSimulation::CAIMoveSimulation()
: m_vGotoPoint(ZERO)
, m_vLastRefPoint(ZERO)
, m_selectedAI(GUID_NULL)
, m_iGoalPipeId(0)
{

}

//////////////////////////////////////////////////////////////////////////
CAIMoveSimulation::~CAIMoveSimulation()
{

}

//////////////////////////////////////////////////////////////////////////
void CAIMoveSimulation::OnGoalPipeEvent(IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId)
{
	CRY_ASSERT(goalPipeId == m_iGoalPipeId);

	if (event == ePN_Resumed)
	{
		pPipeUser->SetRefPointPos(m_vGotoPoint);
	}
	else if (event == ePN_Deselected || event == ePN_Finished)
	{
		pPipeUser->SetRefPointPos(m_vLastRefPoint);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAIMoveSimulation::OnSelectionChanged()
{
	IEditor *pEditor = GetIEditor();
	CRY_ASSERT(pEditor);

	CBaseObject *pSelected = pEditor->GetSelectedObject();
	if (!pSelected || pSelected->GetId() != m_selectedAI)
	{
		CancelMove();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAIMoveSimulation::CancelMove()
{
	IEditor *pEditor = GetIEditor();
	CRY_ASSERT(pEditor);

	IObjectManager *pObjectManager = pEditor ? pEditor->GetObjectManager() : NULL;
	CRY_ASSERT(pObjectManager);

	// Deselected, cancel the command
	CBaseObject *pLastObject = pObjectManager ? pObjectManager->FindObject(m_selectedAI) : NULL;
	if (pLastObject && pLastObject->IsKindOf(RUNTIME_CLASS(CEntityObject)))
	{
		IEntity *pEntity = ((CEntityObject*)pLastObject)->GetIEntity();
		IAIObject *pAI = pEntity ? pEntity->GetAI() : NULL;
		IPipeUser *pPipeUser = CastToIPipeUserSafe(pAI);
		if (pPipeUser && pPipeUser->IsUsingPipe(m_iGoalPipeId))
		{
			pPipeUser->CancelSubPipe(m_iGoalPipeId);
		}
	}

	m_iGoalPipeId = 0;
	m_vLastRefPoint.zero();
}

//////////////////////////////////////////////////////////////////////////
bool CAIMoveSimulation::UpdateAIMoveSimulation(CViewport *pView, const CPoint& point)
{
	bool bResult = false;

	IEditor *pEditor = GetIEditor();
	CRY_ASSERT(pEditor);

	CGameEngine *pGameEngine = pEditor ? pEditor->GetGameEngine() : NULL;
	CRY_ASSERT(pGameEngine);

	// Cancel the current move order
	CancelMove();

	m_selectedAI = GUID_NULL;
	m_vGotoPoint.zero();

	CBaseObject *pSelected = pEditor->GetSelectedObject();
	if (pSelected && pGameEngine && pGameEngine->GetSimulationMode())
	{
		// Get AI object of the selected entity
		IAIObject *pSelectedAI = NULL;
		if (pSelected->IsKindOf(RUNTIME_CLASS(CEntityObject)))
		{
			IEntity *pEntity = ((CEntityObject*)pSelected)->GetIEntity();
			pSelectedAI = pEntity ? pEntity->GetAI() : NULL;
		}
		
		Vec3 vGotoPoint(ZERO);
		if (pSelectedAI && GetAIMoveSimulationDestination(pView, point, vGotoPoint))
		{
			if (SendAIMoveSimulation(pSelectedAI, vGotoPoint))
			{
				m_selectedAI = pSelected->GetId();
				m_vGotoPoint = vGotoPoint;
				bResult = true;
			}
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CAIMoveSimulation::GetAIMoveSimulationDestination(CViewport *pView, const CPoint& point, Vec3& outGotoPoint) const
{
	HitContext hitInfo;
	pView->HitTest(point, hitInfo);

	// TODO Get point or projected point on hit object's bounds
	CBaseObject *pHitObj = hitInfo.object;
	if (pHitObj)
	{
		AABB bbox;
		pHitObj->GetBoundBox(bbox);

		// TODO Get closest approachable point to bounds
		outGotoPoint = pView->SnapToGrid(pView->ViewToWorld(point));
	}
	else
	{
		outGotoPoint = pView->SnapToGrid(pView->ViewToWorld(point));
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CAIMoveSimulation::SendAIMoveSimulation(IAIObject *pAI, const Vec3& vGotoPoint)
{
	assert(pAI);

	bool bResult = false;

	IEntity *pEntity = pAI ? pAI->GetEntity() : NULL;

	IScriptSystem *pSS = gEnv->pScriptSystem;
	SmartScriptTable pAITable = NULL;
	if (pEntity && pSS->GetGlobalValue("AI", pAITable) && pSS->BeginCall(pAITable, "OnEditorMoveSimulation"))
	{
		CScriptVector posTbl(pSS);
		posTbl.Set(vGotoPoint);

		pSS->PushFuncParam(pAITable);
		pSS->PushFuncParam(pEntity->GetScriptTable());
		pSS->PushFuncParam(m_iGoalPipeId);
		pSS->PushFuncParam(posTbl);
		bResult = pSS->EndCall();
	}

	return bResult;
}

