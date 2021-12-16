/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Player nav path agent, to hook in to AI path finder

-------------------------------------------------------------------------
History:
- 02:11:2009: Created by Kevin Kirst
- 18:11:2009: Made Nav Path code based on Kevin's Ruler code

*************************************************************************/

#include "StdAfx.h"
#include "PlayerNavPathAgent.h"
#include "PlayerNavPathPoint.h"
#include "PlayerNavPathTagPointCloud.h"
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "INavigation.h"
#include "ISplines.h"

static const float g_fPathHeightAdjust = 0.2f; // Z height to add to each point to keep it above terrain


//////////////////////////////////////////////////////////////////////////
// Small helper class to allow use of Bezier Spline (needed because SerializeSpline was abstract)
class CBSplineVec3 : public spline::CBaseSplineInterpolator<Vec3,spline::BezierSpline<Vec3> >
{
public:
	virtual int GetNumDimensions() { return 3; };
	virtual ESplineType GetSplineType() { return ESPLINE_BEZIER; }

	void SerializeSpline( XmlNodeRef &node,bool bLoading )
	{		
	}
};


//////////////////////////////////////////////////////////////////////////
CPlayerNavPathAgent::CPlayerNavPathAgent()
: m_bPathQueued(false)
, m_bLastPathSuccess(false)
, m_LastNavNode(0)
, m_fLastPathDist(0.0f)
, m_vStartPoint(ZERO)
, m_pPlayerEntity(NULL)
{
	m_AgentMovementAbility.pathfindingProperties.navCapMask = IAISystem::NAV_CUSTOM_NAVIGATION; 
	//m_AgentMovementAbility.pathfindingProperties.navCapMask = IAISystem::NAV_TRIANGULAR + IAISystem::NAV_WAYPOINT_HUMAN + IAISystem::NAV_SMARTOBJECT + IAISystem::NAV_LAYERED_NAV_MESH + IAISystem::NAV_ROAD;
	m_AgentMovementAbility.pathfindingProperties.radius = 1.0f;
	m_AgentMovementAbility.movementSpeeds.SetBasicSpeeds(1.0f, 2.0f, 4.0f, 8.0f, 8.0f);
}

//////////////////////////////////////////////////////////////////////////
CPlayerNavPathAgent::~CPlayerNavPathAgent()
{
}
//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::Reset()
{
	ClearLastRequest();
	m_path.clear();
	m_smoothedPath.clear();
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::SetPlayerEntity(IEntity* pPlayerEntity)
{
	m_pPlayerEntity = pPlayerEntity;
}



//////////////////////////////////////////////////////////////////////////
bool CPlayerNavPathAgent::RequestPath(const CPlayerNavPathPoint &startPoint, const CPlayerNavPathPoint &endPoint)
{
	if(m_bPathQueued)
		return false;

	bool bResult = false;

	if (startPoint.IsEmpty() || endPoint.IsEmpty())
	{
		CRY_ASSERT_MESSAGE(false, "CPlayerNavPathAgent::RequestPath but the path points aren't set!");
		return false;
	}

	// Request the path
	IAIPathFinder *pPathFinder = gEnv->pAISystem->GetIAIPathFinder();
	if (pPathFinder)
	{
		ClearLastRequest();

		// Request new path
		m_bPathQueued = true;
		m_vStartPoint = startPoint.GetPos();
		pPathFinder->RequestPathTo(m_vStartPoint, endPoint.GetPos(), Vec3Constants<float>::fVec3_OneY, this, false, -1, 20.0f, 5.0f);

		bResult = true;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CPlayerNavPathAgent::RequestPath(const CPlayerNavPathTagPointCloud::SNavTagPoint& startNavNode, const CPlayerNavPathTagPointCloud::SNavTagPoint& endNavNode)
{
	if(m_bPathQueued)
		return false;

	bool bResult = false;

	if (!startNavNode.m_navNodeId || !endNavNode.m_navNodeId)
	{
		CRY_ASSERT_MESSAGE(false, "CPlayerNavPathAgent::RequestPath but the path points aren't set!");
		return false;
	}

	// Request the path
	IAIPathFinder *pPathFinder = gEnv->pAISystem->GetIAIPathFinder();
	if (pPathFinder)
	{
		ClearLastRequest();

		// Request new path
		m_bPathQueued = true;
		m_vStartPoint = startNavNode.m_pos;
		pPathFinder->RequestPathTo(startNavNode.m_navNodeId, endNavNode.m_navNodeId, Vec3Constants<float>::fVec3_OneY, this, false, -1, 20.0f, 5.0f);

		bResult = true;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CPlayerNavPathAgent::RequestPath(const char* designerPathName)
{
	if(m_bPathQueued)
		return false;

	if(!designerPathName || !designerPathName[0])
		return false;

	static Vec3 points[64];
	uint32 numpoints = gEnv->pAISystem->GetINavigation()->GetPath(designerPathName, points, 64);

	m_purePath.reserve(numpoints);
	for(uint32 i=0; i<numpoints; ++i)
	{
		m_purePath.push_back(points[i]);
	}

	SShapePath path(points, numpoints);

	m_path.clear();
	m_bPathQueued = false;

	m_bLastPathSuccess = !path.Empty();
	if (m_bLastPathSuccess)
	{
		m_fLastPathDist = path.GetPathLength();

		GeneratePath(path);
	}


	return true;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::ClearLastRequest()
{
	// Reset last path info
	IAIPathFinder *pPathFinder = gEnv->pAISystem->GetIAIPathFinder();
	if (pPathFinder)
	{
		pPathFinder->CancelAnyPathsFor(this);
	}

	m_bLastPathSuccess = false;
	m_fLastPathDist = 0.0f;
	
	m_bPathQueued = false;
}

//////////////////////////////////////////////////////////////////////////
IEntity *CPlayerNavPathAgent::GetPathAgentEntity() const
{
	return m_pPlayerEntity;
}

//////////////////////////////////////////////////////////////////////////
const char *CPlayerNavPathAgent::GetPathAgentName() const
{
	return "PlayerNavPathAgent";
}

//////////////////////////////////////////////////////////////////////////
unsigned short CPlayerNavPathAgent::GetPathAgentType() const
{
	return AIOBJECT_PLAYER;
}

//////////////////////////////////////////////////////////////////////////
float CPlayerNavPathAgent::GetPathAgentPassRadius() const
{
	return 1.0f; // TODO Parameterize?
}

//////////////////////////////////////////////////////////////////////////
Vec3 CPlayerNavPathAgent::GetPathAgentPos() const
{
	return m_vStartPoint;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CPlayerNavPathAgent::GetPathAgentVelocity() const
{
	return Vec3Constants<float>::fVec3_Zero; // TODO ??
}

//////////////////////////////////////////////////////////////////////////
const AgentMovementAbility &CPlayerNavPathAgent::GetPathAgentMovementAbility() const
{
	return m_AgentMovementAbility;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::PathEvent(SAIEVENT *pEvent)
{
	m_path.clear();
	m_purePath.clear();
	m_bPathQueued = false;

	m_bLastPathSuccess = pEvent->bPathFound;
	if (pEvent->bPathFound)
	{
		IAIPathFinder *pPathFinder = gEnv->pAISystem->GetIAIPathFinder();
		const INavPath *pPath = pPathFinder->GetCurrentPath();
		m_fLastPathDist = pPath->GetPathLength(false);

		GeneratePath(pPath);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::GeneratePath(const INavPath *pPath)
{
	CRY_ASSERT(pPath && !pPath->Empty());
	if (!pPath || pPath->Empty())
		return;

	const float fPathDistance = pPath->GetPathLength(false);
	const int iMaxRefinementSteps = clamp_tpl(g_pGame->GetHUD()->GetCVars()->hud_navPath_maxPoints, 1, 200);
	const float fStepIncrease = clamp_tpl(g_pGame->GetHUD()->GetCVars()->hud_navPath_segmentLengthIncrease, 1.0f, 2.0f);
	float fInitialStepSize = clamp_tpl(g_pGame->GetHUD()->GetCVars()->hud_navPath_minSegmentLength, 0.5f, 100.0f);
	int iSteps = CalcNumSteps(fPathDistance, iMaxRefinementSteps, fStepIncrease, fInitialStepSize);

	PATHPOINTVECTOR pathPoints;
	pPath->GetPath(pathPoints);
	int purePathSize = pathPoints.size();
	
	m_purePath.reserve(purePathSize+1);
	m_purePath.push_back(m_vStartPoint);
	for(int i=purePathSize-1; i>=0; --i)
	{
		m_purePath.push_back(pathPoints[i].vPos);
	}

	m_path.reserve(iSteps+m_purePath.size());
	m_path.push_back(m_vStartPoint);

	Vec3 vNextPoint = Vec3(ZERO);
	IAISystem::ENavigationType nextPointType = IAISystem::NAV_UNSET;
	float fStepSize = fInitialStepSize; // make each subsequent step fStepIncrease larger
	float fDist = 0.0f;

	int purePathIndex = 0;
	float checkNextPurePointDistance = 0.0f;

	TNavPath::const_iterator nextPP = m_purePath.begin();
	TNavPath::const_iterator currentPP = m_purePath.begin();
	TNavPath::const_iterator endPP = m_purePath.end();
	for (int i=0; i<iSteps; ++i)
	{
		fDist = min(fDist + fStepSize, fPathDistance);

		if(checkNextPurePointDistance<=fDist)
		{
			++nextPP;
			if(nextPP != endPP)
			{
				checkNextPurePointDistance += (*nextPP - *currentPP).GetLength();
			}
			else
			{
				checkNextPurePointDistance = fPathDistance;
			}
			m_path.push_back(*currentPP);
			currentPP = nextPP;
		}

		pPath->GetPosAlongPath(vNextPoint, fDist, false, false/*, &nextPointType*/);
		AdjustPathPointHeight(vNextPoint, nextPointType);

		m_path.push_back(vNextPoint);

		fStepSize *= fStepIncrease;	
	} 

	m_path.push_back(m_purePath.back());
	MakeSmoothedPath();
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::GeneratePath(const SShapePath& path)
{
	if (path.Empty())
		return;

	m_path.push_back(m_vStartPoint);

	const float fPathDistance = path.GetPathLength();
	const int iMaxRefinementSteps = clamp_tpl(g_pGame->GetHUD()->GetCVars()->hud_navPath_maxPoints, 1, 100);
	const float fStepIncrease = clamp_tpl(g_pGame->GetHUD()->GetCVars()->hud_navPath_segmentLengthIncrease, 1.0f, 2.0f);
	float fInitialStepSize = clamp_tpl(g_pGame->GetHUD()->GetCVars()->hud_navPath_minSegmentLength, 0.5f, 100.0f);
	int iSteps = CalcNumSteps(fPathDistance, iMaxRefinementSteps, fStepIncrease, fInitialStepSize);

	Vec3 vNextPoint = Vec3(ZERO);
	IAISystem::ENavigationType nextPointType = IAISystem::NAV_UNSET;
	float fStepSize = fInitialStepSize; // make each subsequent step fStepIncrease larger
	float fDist = 0.0f;

	int indexCount = 0;
	float checkPurePointDistance = 0.0f;

	for (int i=0; i<iSteps; ++i)
	{
		fDist = min(fDist + fStepSize, fPathDistance);

		if(checkPurePointDistance<=fDist)
		{
			m_path.push_back(path.GetPoint(indexCount));
			++indexCount;
			if(indexCount < path.Size())
				checkPurePointDistance += ( path.GetPoint(indexCount) - path.GetPoint(indexCount-1) ).GetLength();
			else
				checkPurePointDistance = fPathDistance;
		}

		vNextPoint = path.GetPosAlongPath(fDist);
		AdjustPathPointHeight(vNextPoint, nextPointType);

		m_path.push_back(vNextPoint);

		fStepSize *= fStepIncrease;	
	} 

	m_path.push_back(path.GetPoint(path.Size()-1));

	MakeSmoothedPath();
}

//////////////////////////////////////////////////////////////////////////
int CPlayerNavPathAgent::CalcNumSteps(float fPathDistance, int iMaxSteps, float fStepIncrease, float &fInitialLength) const
{
	// Using Sum of Geometric Series formula: S = a * (1 - r^n) / (1 - r)

	fStepIncrease = max(fStepIncrease, 1.0001f); // geometric sum doesn't work if step increase is exactly 1
	const float fTerm = 1.0f - fPathDistance * (1.0f - fStepIncrease) / fInitialLength;
	int iNumSteps = static_cast<int>( cry_ceilf( cry_logf( fTerm ) / cry_logf(fStepIncrease) ) );
	if (iNumSteps > iMaxSteps)
	{
		// resize initial length to achieve path distance within max steps
		iNumSteps = iMaxSteps;
		fInitialLength = fPathDistance * (1 - fStepIncrease) / (1 - cry_powf(fStepIncrease, (float)iMaxSteps));
	}
	
	return iNumSteps;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::AdjustPathPointHeight(Vec3 &vPoint, IAISystem::ENavigationType nextPointType) const
{
	if (nextPointType != IAISystem::NAV_TRIANGULAR)
	{
		// Don't do height checks on waypoints and other non-triangular nav points
		vPoint.z += g_fPathHeightAdjust;
	}
	else
	{
		// TODO handle raycasts

		const float fTerrainHeight = gEnv->p3DEngine->GetTerrainElevation(vPoint.x, vPoint.y);
		vPoint.z = fTerrainHeight + g_fPathHeightAdjust;
	}	
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::GetPathAgentNavigationBlockers(NavigationBlockers &blockers, const PathfindRequest *pRequest)
{
	// TODO ?
}

//////////////////////////////////////////////////////////////////////////
unsigned int CPlayerNavPathAgent::GetPathAgentLastNavNode() const
{
	return m_LastNavNode;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::SetPathAgentLastNavNode(unsigned int lastNavNode)
{
	m_LastNavNode = lastNavNode;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CPlayerNavPathAgent::GetForcedStartPos() const
{
	return ZERO;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CPlayerNavPathAgent::GetFirstPathPoint() const
{
	return (!m_path.empty()) ? m_path[0] : Vec3(ZERO);
}

//////////////////////////////////////////////////////////////////////////
Vec3 CPlayerNavPathAgent::GetEndPathPoint() const
{
	return (!m_path.empty()) ? m_path[m_path.size()-1] : Vec3(ZERO);
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathAgent::MakeSmoothedPath()
{
	const int iSize = m_path.size();
	const int iSubPoints = clamp_tpl(g_pGame->GetHUD()->GetCVars()->hud_navPath_smoothLevel, 0, 50);
	const int iTotalPoints = iSize * iSubPoints;

	// Create spline
	CBSplineVec3 pathSpline;
	pathSpline.reserve_keys(iSize);
	float fTime = 0.0f;
	for (int i=0; i<iSize; ++i)
	{
		ISplineInterpolator::ValueType vals = {m_path[i].x, m_path[i].y, m_path[i].z};
		pathSpline.InsertKey(fTime, vals);
		fTime += iSubPoints;
	}

	// Interpolate through spline and create refined points
	const float fTotalTime = float(iTotalPoints);
	m_smoothedPath.resize(iTotalPoints);
	float fCurrentTime = 0.0f;
	for (int i=0; i<iTotalPoints; ++i)
	{
		ISplineInterpolator::ValueType vals;
		pathSpline.Interpolate(fCurrentTime, vals);
		m_smoothedPath[i].vPoint = Vec3(vals[0], vals[1], vals[2]);
		fCurrentTime += 1.0f;
	}

	// Calculate horizontal normals for each point
	for (int i=0; i<iTotalPoints-1; ++i)
	{
		const Vec3 &vSeg = m_smoothedPath[i+1].vPoint - m_smoothedPath[i].vPoint;
		if (!vSeg.IsZeroFast())
			m_smoothedPath[i].vNormal = Vec3(vSeg.y, -vSeg.x, 0.0f);
		else if (i>0)
			m_smoothedPath[i].vNormal = m_smoothedPath[i-1].vNormal;
		else
			m_smoothedPath[i].vNormal = Vec3(1.0f, 1.0f, 0.0f); // reasonable value so we at least have a valid normal
		m_smoothedPath[i].vNormal.NormalizeFast();
	}
	m_smoothedPath[iTotalPoints-1].vNormal = m_smoothedPath[iTotalPoints-2].vNormal;
}

