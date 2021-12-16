/********************************************************************
CryGame Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   Navigation.cpp
Version:     v1.00
Description: 

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"

#include "Navigation.h"
#include "PolygonSetOps/Polygon2d.h"
#include "TriangularNavRegion.h"
#include "WaypointHumanNavRegion.h"
#include "Waypoint3DSurfaceNavRegion.h"
#include "VolumeNavRegion.h"
#include "FlightNavRegion.h"
#include "RoadNavRegion.h"
#include "SmartObjectNavRegion.h"
#include "Free2DNavRegion.h"
#include "LayeredNavMesh/LayeredNavMeshRegion.h"
#include "Navigation/CustomNavRegion.h"
#include "DebugDrawContext.h"
#include "Graph.h"
#include "CalculationStopper.h"

static const int maxForbiddenNameLen = 1024;

// flag used for debugging/profiling the improvement obtained from using a
// QuadTree for the forbidden shapes. Currently it's actually faster not
// using quadtree - need to experiment more
// kirill - enabling quadtree - is faster on low-spec
bool useForbiddenQuadTree = true;//false;

CNavigation::CNavigation(CAISystem * pAISystem, ISystem * pSystem) :
		m_navDataState(NDS_UNSET), m_pTriangularNavRegion(0), m_pWaypointHumanNavRegion(0),
		m_pWaypoint3DSurfaceNavRegion(0), m_pVolumeNavRegion(0), m_pFlightNavRegion(0),
		m_pRoadNavRegion(0), m_pFree2DNavRegion(0), m_pSmartObjectNavRegion(0),
		m_nNumBuildings(0), m_pTriangulator(0)
{
	//CAISystem * pAISystem = GetAISystem();
	CGraph * pGraph = gAIEnv.pGraph;

	m_pTriangularNavRegion = new CTriangularNavRegion(pGraph);
	m_pWaypointHumanNavRegion = new CWaypointHumanNavRegion(pGraph);
	m_pWaypoint3DSurfaceNavRegion = new CWaypoint3DSurfaceNavRegion(pGraph);
	m_pFlightNavRegion = new CFlightNavRegion(pSystem->GetIPhysicalWorld(), pGraph);
	m_pVolumeNavRegion = new CVolumeNavRegion(pGraph);
	m_pRoadNavRegion = new CRoadNavRegion(pGraph);
	m_pFree2DNavRegion = new CFree2DNavRegion(pGraph);
	m_pSmartObjectNavRegion = new CSmartObjectNavRegion(pGraph);
	m_pLayeredNavMeshRegion = new CCompositeLayeredNavMeshRegion(pGraph);
	m_pCustomNavRegion = new CCustomNavRegion(pGraph);
}

CNavigation::~CNavigation ()
{
	delete m_pTriangularNavRegion;
	m_pTriangularNavRegion = 0;
	delete m_pWaypointHumanNavRegion;
	m_pWaypointHumanNavRegion = 0;
	delete m_pWaypoint3DSurfaceNavRegion;
	m_pWaypoint3DSurfaceNavRegion = 0;
	delete m_pFlightNavRegion;
	m_pFlightNavRegion = 0;
	delete m_pVolumeNavRegion;
	m_pVolumeNavRegion = 0;
	delete m_pRoadNavRegion;
	m_pRoadNavRegion = 0;
	delete m_pSmartObjectNavRegion;
	m_pSmartObjectNavRegion = 0;
	delete m_pLayeredNavMeshRegion;
	m_pLayeredNavMeshRegion = 0;
	delete m_pCustomNavRegion;
	m_pCustomNavRegion = 0;
}

bool CNavigation::Init()
{
	m_nNumBuildings = 0;
	return true;
}

void CNavigation::ShutDown()
{
	FlushAllAreas();

	if (m_pTriangulator)
	{
		delete m_pTriangulator;
		m_pTriangulator = 0;
	}
}

//====================================================================
// OnMissionLoaded
//====================================================================
void CNavigation::OnMissionLoaded()
{
	m_pWaypoint3DSurfaceNavRegion->OnMissionLoaded();
	m_pWaypointHumanNavRegion->OnMissionLoaded();
	m_pFlightNavRegion->OnMissionLoaded();
	m_pVolumeNavRegion->OnMissionLoaded();
	m_pRoadNavRegion->OnMissionLoaded();
	m_pTriangularNavRegion->OnMissionLoaded();
}

// // loads the triangulation for this level and mission
//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::LoadNavigationData(const char * szLevel, const char * szMission, bool demandLoadLNMs)
{
	LOADING_TIME_PROFILE_SECTION(GetISystem());

	m_nNumBuildings = 0;

	CTimeValue startTime = gEnv->pTimer->GetAsyncCurTime();

	CAISystem * pAISystem = GetAISystem();
	CGraph * pGraph = gAIEnv.pGraph;

	pAISystem->m_VertexList.Clear();

	pGraph->Clear(IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_LAYERED_NAV_MESH);

	m_pTriangularNavRegion->Clear();
	m_pWaypointHumanNavRegion->Clear();
	m_pWaypoint3DSurfaceNavRegion->Clear();
	m_pVolumeNavRegion->Clear();
	m_pFlightNavRegion->Clear();
	m_pRoadNavRegion->Clear();
	m_pSmartObjectNavRegion->Clear();
	m_pFree2DNavRegion->Clear();
	m_pLayeredNavMeshRegion->Clear();
	m_pCustomNavRegion->Clear();

	pGraph->CheckForEmpty(IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_VOLUME | IAISystem::NAV_FLIGHT | IAISystem::NAV_ROAD | IAISystem::NAV_FREE_2D | IAISystem::NAV_LAYERED_NAV_MESH);

	char fileName[1024], fileNameHide[1024], fileNameVolume[1024],
		fileNameFlight[1024], fileNameAreas[1024], fileNameRoads[1024],
		fileNameWaypoint3DSurface[1024], fileNameFree2D[1024], fileNameVerts[1024],
		fileNameLNM[1024];
	sprintf(fileName, "%s/net%s.bai",szLevel,szMission);
	sprintf(fileNameVerts, "%s/verts%s.bai",szLevel,szMission);
	sprintf(fileNameHide, "%s/hide%s.bai",szLevel,szMission);
	sprintf(fileNameVolume, "%s/v3d%s.bai",szLevel,szMission);
	sprintf(fileNameFlight, "%s/fnav%s.bai",szLevel,szMission);
	sprintf(fileNameRoads, "%s/roadnav%s.bai",szLevel,szMission);
	sprintf(fileNameAreas, "%s/areas%s.bai",szLevel,szMission);
	sprintf(fileNameWaypoint3DSurface, "%s/waypt3Dsfc%s.bai",szLevel,szMission);
	sprintf(fileNameFree2D, "%s/free2d%s.bai",szLevel,szMission);
	_snprintf (fileNameLNM, 1024-1, "%s/navmesh%s-", szLevel, szMission);

	m_graphFileName = fileName;

	// basic test - if there's not triangular navigation file then assume there's no navigation/AI
	// in this level
	{
		CCryFile file;;
		if (!file.Open( fileName,"rb"))
		{
			AILogLoading("No navigation data available - disabling AI");
			m_navDataState = NDS_UNSET;
			return;
		}
	}

	// NOTE Aug 22, 2008: <pvl> we consider nav data OK if 1) graph loaded OK and
	// 2) at least one of vertex list and LNM loaded OK 
	m_navDataState = NDS_OK;
	pAISystem->ReadAreasFromFile(fileNameAreas);
	AILogLoading("Reading vertex list");
	bool vertexListOK = pAISystem->m_VertexList.ReadFromFile(fileNameVerts);
	AILogLoading("Reading nav graph.");
	if ( ! pGraph->ReadFromFile(fileName, demandLoadLNMs ? 0 : ~0))
		m_navDataState = NDS_BAD;
	AILogLoading("Reading 3D volumes.");
	m_pVolumeNavRegion->ReadFromFile(fileNameVolume);
	AILogLoading("Reading Flight navigation.");
	m_pFlightNavRegion->ReadFromFile(fileNameFlight);
	AILogLoading("Reading Road navigation");
	m_pRoadNavRegion->ReadFromFile(fileNameRoads);
	AILogLoading("Reading 3D surface navigation");
	m_pWaypoint3DSurfaceNavRegion->ReadFromFile(fileNameWaypoint3DSurface);
	AILogLoading("Reading Layered Mesh navigation");
	bool lnmOK = m_pLayeredNavMeshRegion->ReadFromFile (fileNameLNM, demandLoadLNMs);
	if ( ! vertexListOK && ! lnmOK)
		m_navDataState = NDS_BAD;
	AILogLoading("Building forbidden QuadTrees");
	RebuildForbiddenQuadTrees();
	CTimeValue endTime = gEnv->pTimer->GetAsyncCurTime();
	AILogLoading("Navigation Data Loaded in %5.2f sec", (endTime - startTime).GetSeconds());
}

// NOTE Oct 26, 2009: <pvl> return 'true' if by the time this function exits the
// requested nav mesh is in-core in a ready-to-use state
bool CNavigation::LoadNavMesh(const char * navModifName, const char * agentTypeName)
{
	if (m_graphFileName.empty())
	{
		AIWarning ("Asked to load a nav mesh before system init.");
		return false;
	}

	if (m_pLayeredNavMeshRegion->IsNavMeshLoaded (navModifName, agentTypeName))
		return true;

	CGraph * pGraph = gAIEnv.pGraph;
	unsigned modifIndex = m_pLayeredNavMeshRegion->GetModifierId (navModifName, agentTypeName);
	if (modifIndex == 0)
	{
		AIWarning ("No file to load (%s,%s) nav mesh from", navModifName, agentTypeName);
		return false;
	}

	const int nodeMemBefore = pGraph->GetNodeManager().NodeMemorySize();
	const int linkMemBefore = pGraph->GetLinkManager().MemoryUsed();
	const int linkMemAllocBefore = pGraph->GetLinkManager().MemoryAllocated();

	pGraph->ReadFromFile(m_graphFileName.c_str(), modifIndex);

	const int nodeMemAfter = pGraph->GetNodeManager().NodeMemorySize();
	const int linkMemAfter = pGraph->GetLinkManager().MemoryUsed();
	const int linkMemAllocAfter = pGraph->GetLinkManager().MemoryAllocated();
	AILogLoading ("Navmesh loaded, allocated %d bytes of node memory, "
			"%d bytes of link memory (change in actual allocation %d bytes)",
			nodeMemAfter - nodeMemBefore, linkMemAfter - linkMemBefore,
			linkMemAllocAfter - linkMemAllocBefore);

	return m_pLayeredNavMeshRegion->LoadNavMesh (navModifName, agentTypeName);
}

bool CNavigation::UnloadNavMesh(const char * navModifName, const char * agentTypeName)
{
	CGraph * pGraph = gAIEnv.pGraph;
	const int nodeMemBefore = pGraph->GetNodeManager().NodeMemorySize();
	const int linkMemBefore = pGraph->GetLinkManager().MemoryUsed();
	const int linkMemAllocBefore = pGraph->GetLinkManager().MemoryAllocated();

	const bool success = m_pLayeredNavMeshRegion->UnloadNavMesh (navModifName, agentTypeName);

	if (success)
	{
		pGraph->GetLinkManager().CollectGarbage();

		const int nodeMemAfter = pGraph->GetNodeManager().NodeMemorySize();
		const int linkMemAfter = pGraph->GetLinkManager().MemoryUsed();
		const int linkMemAllocAfter = pGraph->GetLinkManager().MemoryAllocated();
		AILogLoading ("Navmesh unloaded, freed %d bytes of node memory, "
				"%d bytes of link memory (change in actual allocation %d bytes)",
				nodeMemBefore - nodeMemAfter, linkMemBefore - linkMemAfter,
				linkMemAllocBefore - linkMemAllocAfter);
	}

	return success;
}

void CNavigation::Serialize( TSerialize ser, CObjectTracker& objectTracker )
{
	m_pTriangularNavRegion->Serialize(ser, objectTracker);
	m_pWaypointHumanNavRegion->Serialize(ser, objectTracker);
	m_pFlightNavRegion->Serialize(ser, objectTracker);
	m_pVolumeNavRegion->Serialize(ser, objectTracker);
}

void CNavigation::Update(CTimeValue frameStartTime, float frameDeltaTime)
{
	UpdateNavRegions();

	// Update path devalues.
	for(ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
		it->second.devalueTime = max(0.0f, it->second.devalueTime - frameDeltaTime);
}

//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::UpdateNavRegions()
{
    FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

    //return;
	CAISystem * pAISystem = GetAISystem();
	if (gAIEnv.CVars.DynamicWaypointUpdateTime > 0.000001f && m_pWaypointHumanNavRegion)
	{
		CCalculationStopper stopper("DynamicWaypoint", gAIEnv.CVars.DynamicWaypointUpdateTime, 60000);
		m_pWaypointHumanNavRegion->Update(stopper);
	}

	if (gAIEnv.CVars.DynamicVolumeUpdateTime > 0.000001f && m_pVolumeNavRegion)
	{
		CCalculationStopper stopper("DynamicVolume", gAIEnv.CVars.DynamicVolumeUpdateTime, 10000); // guess
		m_pVolumeNavRegion->Update(stopper);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::FlushSystemNavigation(void)
{
	// clear areas id's
	SpecialAreaMap::iterator si = m_mapSpecialAreas.begin();
	while (si!=m_mapSpecialAreas.end())
	{
		m_BuildingIDManager.FreeId((si->second).nBuildingID);
		(si->second).nBuildingID = -1;
		(si->second).bAltered = false;
		++si;
	}
	m_BuildingIDManager.FreeAll();

	ClearForbiddenQuadTrees();

	GetTriangularNavRegion()->Clear();
	GetVolumeNavRegion()->Clear();
	GetWaypoint3DSurfaceNavRegion()->Clear();
	GetWaypointHumanNavRegion()->Clear();
	GetFlightNavRegion()->Clear();
	GetRoadNavRegion()->Clear();
	GetSmartObjectsNavRegion()->Clear();
	GetLayeredNavMeshRegion()->Clear();

	if (gAIEnv.pGraph)
		gAIEnv.pGraph->Clear(IAISystem::NAVMASK_ALL);
}

void CNavigation::Reset(IAISystem::EResetReason reason)
{
	if (reason == IAISystem::RESET_ENTER_GAME)
		m_validationErrorMarkers.clear();

	for (ExtraLinkCostShapeMap::iterator it = m_mapExtraLinkCosts.begin() ; it != m_mapExtraLinkCosts.end() ; ++it)
	{
		const string &name = it->first;
		SExtraLinkCostShape &shape = it->second;
		shape.costFactor = shape.origCostFactor;
	}

	// enable all nav modifiers
	SpecialAreaMap::iterator si = m_mapSpecialAreas.begin();
	while (si!=m_mapSpecialAreas.end())
	{
		(si->second).bAltered = false;
		++si;
	}

	// Reset path devalue stuff.
	for(ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
		it->second.devalueTime = 0;

	GetTriangularNavRegion()->Reset(reason);
	GetVolumeNavRegion()->Reset(reason);
	GetWaypoint3DSurfaceNavRegion()->Reset(reason);
	GetWaypointHumanNavRegion()->Reset(reason);
	GetFlightNavRegion()->Reset(reason);
	GetRoadNavRegion()->Reset(reason);
	GetSmartObjectsNavRegion()->Reset(reason);
	GetLayeredNavMeshRegion()->Reset(reason);
}

// copies a designer path into provided list if a path of such name is found
//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::GetDesignerPath(const char * szName, SShape & path) const
{
	ShapeMap::const_iterator di = m_mapDesignerPaths.find(szName);
	if (di == m_mapDesignerPaths.end())
		return false;
	path = di->second;
	return true;
}

//====================================================================
// ReadArea
//====================================================================
void ReadArea(CCryFile & file, int version, string & name, SpecialArea & sa)
{
	unsigned nameLen;
	file.ReadType(&nameLen);
	char tmpName[1024];
	file.ReadType(&tmpName[0], nameLen);
	tmpName[nameLen] = '\0';
	name = tmpName;

	int64 type = 0;
	file.ReadType(&type);
	sa.type = (SpecialArea::EType) type;

	int64 waypointConnections = 0;
	file.ReadType(&waypointConnections);
	sa.waypointConnections = (EWaypointConnections) waypointConnections;

	char altered = 0;
	file.ReadType(&altered);
	sa.bAltered = altered != 0;
	file.ReadType(&sa.fHeight);
	if (version <= 16)
	{
		float junk;
		file.ReadType(&junk);
	}
	file.ReadType(&sa.fNodeAutoConnectDistance);
	file.ReadType(&sa.fMaxZ);
	file.ReadType(&sa.fMinZ);
	file.ReadType(&sa.nBuildingID);
	if (version >= 18)
	{
		unsigned char lightLevel = 0;
		file.ReadType(&lightLevel);
		sa.lightLevel = (EAILightLevel)lightLevel;
	}

	// now the area itself
	ListPositions pts;
	unsigned ptsSize;
	file.ReadType(&ptsSize);

	for (unsigned iPt = 0 ; iPt < ptsSize ; ++iPt)
	{
		Vec3 pt;
		file.ReadType(&pt);
		pts.push_back(pt);
	}
	sa.SetPolygon(pts);
}

//====================================================================
// ReadForbiddenArea
//====================================================================
bool ReadPolygonArea(CCryFile & file, int version, string & name, ListPositions & pts)
{
	unsigned nameLen = maxForbiddenNameLen;
	file.ReadType(&nameLen);

//Diesel cut
/*
	if (nameLen >= maxForbiddenNameLen)
	{
		AIWarning("Excessive forbidden area name length - AI loading failed");
		return false;
	}
*/
//Diesel ut

	char tmpName[maxForbiddenNameLen];
	file.ReadRaw(&tmpName[0], nameLen);
	tmpName[nameLen] = '\0';
	name = tmpName;

	unsigned ptsSize;
	file.ReadType(&ptsSize);

	for (unsigned iPt = 0 ; iPt < ptsSize ; ++iPt)
	{
		Vec3 pt;
		file.ReadType(&pt);
		pts.push_back(pt);
	}
	return true;
}

//====================================================================
// ReadForbiddenArea
//====================================================================
bool ReadForbiddenArea(CCryFile & file, int version, CAIShape* shape)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "Forbidden areas");

	unsigned nameLen = maxForbiddenNameLen;
	file.ReadType(&nameLen);

//Diesel cut
/*
	if (nameLen >= maxForbiddenNameLen)
	{
		AIWarning("Excessive forbidden area name length - AI loading failed");
		return false;
	}
*/
//Diesel ut


	char tmpName[maxForbiddenNameLen];
	file.ReadRaw(&tmpName[0], nameLen);
	tmpName[nameLen] = '\0';
	shape->SetName(tmpName);

	unsigned ptsSize;
	file.ReadType(&ptsSize);

	ShapePointContainer& pts = shape->GetPoints();
	pts.resize(ptsSize);

	for (unsigned i = 0; i < ptsSize; ++i)
		file.ReadType(&pts[i]);

	// Call build AABB since the point container was filled directly.
	shape->BuildAABB();

	return true;
}

//====================================================================
// ReadExtraLinkCostArea
//====================================================================
void ReadExtraLinkCostArea(CCryFile & file, int version, string & name, SExtraLinkCostShape &shape)
{
	unsigned nameLen;
	file.ReadType(&nameLen);
	char tmpName[1024];
	file.ReadRaw(&tmpName[0], nameLen);
	tmpName[nameLen] = '\0';
	name = tmpName;

	file.ReadType(&shape.origCostFactor);
	shape.costFactor = shape.origCostFactor;
	file.ReadType(&shape.aabb.min);
	file.ReadType(&shape.aabb.max);

	unsigned ptsSize;
	file.ReadType(&ptsSize);

	for (unsigned iPt = 0 ; iPt < ptsSize ; ++iPt)
	{
		Vec3 pt;
		file.ReadType(&pt);
		shape.shape.push_back(pt);
	}
}


//====================================================================
// ReadAreasFromFile
//====================================================================
void CNavigation::ReadAreasFromFile(CCryFile & file, int fileVersion)
{
	FlushAllAreas();

	unsigned numAreas;

	// Read forbidden areas
	{
		file.ReadType(&numAreas);
		// vague sanity check
		AIAssert(numAreas < 1000000);
		for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
		{
			CAIShape* pShape = new CAIShape;
			if (!ReadForbiddenArea(file, fileVersion, pShape))
				return;
			if (m_forbiddenAreas.FindShape(pShape->GetName()) != 0)
			{
				AIError("CAISystem::ReadAreasFromFile: Forbidden area '%s' already exists, please rename the shape and reexport.", pShape->GetName().c_str());
				delete pShape;
			}
			else
				m_forbiddenAreas.InsertShape(pShape);
		}
	}

	// Read navigation modifiers (special areas)
	{
		file.ReadType(&numAreas);
		// vague sanity check
		AIAssert(numAreas < 1000000);
		for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
		{
			SpecialArea sa;
			string name;
			ReadArea(file, fileVersion, name, sa);
			sa.nBuildingID = m_BuildingIDManager.GetId();
			if (m_mapSpecialAreas.find(name) != m_mapSpecialAreas.end())
			{
				AIError("CAISystem::ReadAreasFromFile: Navigation modifier '%s' already exists, please rename the shape and reexport.", name.c_str());
			}
			else
			{
				InsertSpecialArea (name, sa);
			}
		}
	}

	// Read designer forbidden areas
	{
		file.ReadType(&numAreas);
		// vague sanity check
		AIAssert(numAreas < 1000000);
		for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
		{
			CAIShape* pShape = new CAIShape;
			ReadForbiddenArea(file, fileVersion, pShape);
			if (m_designerForbiddenAreas.FindShape(pShape->GetName()) != 0)
			{
				AIError("CAISystem::ReadAreasFromFile: Designer forbidden area '%s' already exists, please rename the shape and reexport.", pShape->GetName().c_str());
				delete pShape;
			}
			else
				m_designerForbiddenAreas.InsertShape(pShape);
		}
	}

	// Read forbidden boundaries
	{
		file.ReadType(&numAreas);
		// vague sanity check
		AIAssert(numAreas < 1000000);
		for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
		{
			CAIShape* pShape = new CAIShape;
			ReadForbiddenArea(file, fileVersion, pShape);
			if (m_forbiddenBoundaries.FindShape(pShape->GetName()) != 0)
			{
				AIError("CAISystem::ReadAreasFromFile: Forbidden boundary '%s' already exists, please rename the shape and reexport.", pShape->GetName().c_str());
				delete pShape;
			}
			else
				m_forbiddenBoundaries.InsertShape(pShape);
		}
	}

	// Read extra link costs
	{
		file.ReadType(&numAreas);
		// vague sanity check
		AIAssert(numAreas < 1000000);
		for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
		{
			SExtraLinkCostShape shape(ListPositions(), 0.0f);
			string name;
			ReadExtraLinkCostArea(file, fileVersion, name, shape);
			if (m_mapExtraLinkCosts.find(name) != m_mapExtraLinkCosts.end())
			{
				AIError("CAISystem::ReadAreasFromFile: Extra link cost modifier '%s' already exists, please rename the shape and reexport.", name.c_str());
			}
			else
			{
				m_mapExtraLinkCosts.insert(ExtraLinkCostShapeMap::iterator::value_type(name,shape));
			}
		}
	}

	// Read designer paths
	{
		file.ReadType(&numAreas);
		// vague sanity check
		AIAssert(numAreas < 1000000);
		for (unsigned iArea = 0 ; iArea < numAreas ; ++iArea)
		{
			ListPositions lp;
			string name;
			ReadPolygonArea(file, fileVersion, name, lp);

			int	navType, type;
			file.ReadType(&navType);
			file.ReadType(&type);

			if (m_mapDesignerPaths.find(name) != m_mapDesignerPaths.end())
				AIError("CAISystem::ReadAreasFromFile: Designer path '%s' already exists, please rename the path and reexport.", name.c_str());
			else
				m_mapDesignerPaths.insert(ShapeMap::iterator::value_type(name, SShape(lp, false, (IAISystem::ENavigationType)navType, type)));
		}
	}
}

//===================================================================
// GetSpecialAreaNearestPos
//===================================================================
const SpecialArea *CNavigation::GetSpecialAreaNearestPos(const Vec3 &pos, SpecialArea::EType areaType)
{
	const SpecialArea *pSA = GetSpecialArea(pos, areaType);
	if (pSA)
		return pSA;

	float bestDist = std::numeric_limits<float>::max();
	pSA = 0;
	for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
	{
		const SpecialArea &sa = si->second;
		if (sa.type == areaType)
		{
			if ( sa.fHeight>0.00001f && (pos.z < sa.fMinZ || pos.z > sa.fMaxZ) )
				continue;
			Vec3 polyPt;
			float dist = Distance::Point_Polygon2DSq(pos, sa.GetPolygon(), polyPt);
			if (dist < bestDist)
			{
				bestDist = dist;
				pSA = &sa;
			}
		}
	}
	return pSA;
}

//====================================================================
// GetPath
//====================================================================
uint32 CNavigation::GetPath(const char *szPathName, Vec3 *points, uint32 maxpoints) const
{
	ListPositions junk1;
	AABB junk2;
	junk2.Reset();
	SShape pathShape(junk1, junk2);
	int count = 0;

	if (GetDesignerPath(szPathName,pathShape))
	{
		for (std::list<Vec3>::iterator it = pathShape.shape.begin(), e = pathShape.shape.end(); it != e && count < maxpoints; ++it)
		{
			points[count] = *it;
			++count;
		}
	}

	return count;
}

//====================================================================
// GetNearestPointOnPath
//====================================================================
float CNavigation::GetNearestPointOnPath(const char * szPathName, const Vec3& vPos, Vec3& vResult, bool& bLoopPath, float& totalLength) const
{
	// calculate a point on a path which has the nearest distance from the given point.
	// Also returns segno it means ( 0.0 start point 100.0 end point always)
	// strict even if t == 0 or t == 1.0

	// return values
	// return value : segno;
	// vResult : the point on path
	// bLoopPath : true if the path is looped

	vResult		= ZERO;
	bLoopPath	= false;

	float result = -1.0;

	SShape pathShape;
	if (!GetDesignerPath(szPathName,pathShape) || pathShape.shape.empty())
		return result;

	float	dist	= FLT_MAX;
	bool	bFound	=false;
	float	segNo	=0.0f;
	float	howmanypath =0.0f;

	Vec3	vPointOnLine;

	ListPositions::const_iterator cur = pathShape.shape.begin();
	ListPositions::const_reverse_iterator last = pathShape.shape.rbegin();
	ListPositions::const_iterator next(cur);
	++next;
	
	float	lengthTmp = 0.0f;
	while( next != pathShape.shape.end())
	{
		Lineseg	seg(*cur, *next);

		lengthTmp += ( *cur - *next ).GetLength();

		float	t;
		float	d = Distance::Point_Lineseg(vPos, seg, t);
		if( d < dist)
		{
			Vec3 vSeg = seg.GetPoint(1.0f) - seg.GetPoint(0.0f);
			Vec3 vTmp = vPos - seg.GetPoint(t);

			vSeg.NormalizeSafe();
			vTmp.NormalizeSafe();

			dist	=d;
			bFound	=true;
			result	=segNo + t;
			vPointOnLine = seg.GetPoint(t);
		}
		cur = next;
		++next;
		segNo +=1.0f;
		howmanypath +=1.0f;
	}
	if ( howmanypath == 0.0f )
		return  result;

	if ( bFound == false )
	{
		segNo = 0.0f;
		cur = pathShape.shape.begin();
		while( cur != pathShape.shape.end() )
		{
			Vec3 vTmp = vPos - *cur;
			float d	= vTmp.GetLength();
			if( d < dist)
			{
				dist	=d;
				bFound	=true;
				result	=segNo;
				vPointOnLine = *cur;
			}
			++cur;
			segNo +=1.0f;
		}
	}

	vResult = vPointOnLine;

	cur		= pathShape.shape.begin();

	Vec3 vTmp	= *cur - *last;
	if ( vTmp.GetLength() < 0.0001f )
		bLoopPath = true;

	totalLength =lengthTmp;

	return result * 100.0f / howmanypath;
}

//====================================================================
// GetPointOnPathBySegNo
//====================================================================
void CNavigation::GetPointOnPathBySegNo(const char *szPathName, Vec3& vResult, float segNo) const
{
	vResult	= ZERO;

	SShape pathShape;
	if ((segNo < 0.f) || (segNo > 100.f) || !GetDesignerPath(szPathName, pathShape))
		return;

	ListPositions& shape = pathShape.shape;
	size_t size = shape.size();
	if (size == 0)
		return;
		
	if (size == 1)
	{
		vResult = *(shape.begin());
		return;
	}

	float totalLength = 0.0f;
	for (ListPositions::const_iterator cur = shape.begin(), next = cur; ++next != shape.end(); cur = next)
	{
		totalLength += ( *next - *cur ).GetLength();
	}

	float segLength = totalLength * segNo / 100.0f; 
	float currentLength = 0.0f;
	float currentSegLength = 0.0f;
	
	ListPositions::const_iterator cur, next;
	for (cur = shape.begin(), next = cur; ++next != shape.end(); cur = next)
	{
		const Vec3& curPoint = *cur;
		Vec3 currentSeg = *next - curPoint;
		currentSegLength = currentSeg.GetLength();
		
		if (currentLength + currentSegLength > segLength)
		{
			vResult = curPoint;
			if (currentSegLength > 0.0003f)
			{
				vResult += ((segLength - currentLength) / currentSegLength) * currentSeg;
			}
			return;
		}
		
		currentLength += currentSegLength;
	}

	vResult = *cur;
}

//====================================================================
// IsSegmentValid
//====================================================================
bool CNavigation::IsSegmentValid(IAISystem::tNavCapMask navCap, float rad, const Vec3& posFrom, Vec3& posTo, IAISystem::ENavigationType& navTypeFrom) const
{
	int nBuildingID = -1;
	IVisArea* pVisArea;

	navTypeFrom = CheckNavigationType(posFrom, nBuildingID, pVisArea, navCap);

	if (navTypeFrom == IAISystem::NAV_TRIANGULAR)
	{
		// Make sure not to enter forbidden area.
		if (IsPathForbidden(posFrom, posTo))
			return false;

		if (IsPointForbidden(posTo, rad))
			return false;
	}

	const SpecialArea* pSpecialArea = (nBuildingID != -1 ? GetSpecialArea(nBuildingID) : 0);
	if (pSpecialArea)
	{
		const ListPositions	& buildingPolygon = pSpecialArea->GetPolygon();
		SCheckWalkabilityState state;
		state.state = SCheckWalkabilityState::CWS_NEW_QUERY;
		state.numIterationsPerCheck = 100000;
		bool res = CheckWalkability(posFrom, posTo, rad + 0.15f - walkabilityRadius, true, buildingPolygon, AICE_ALL, 0, &state);
		if (res)
			posTo = state.toFloor;
		return res;
	}
	else
	{
		ListPositions buildingPolygon;
		SCheckWalkabilityState state;
		state.state = SCheckWalkabilityState::CWS_NEW_QUERY;
		state.numIterationsPerCheck = 100000;
		bool res = CheckWalkability(posFrom, posTo, rad + 0.15f - walkabilityRadius, true, buildingPolygon, AICE_ALL, 0, &state);
		if (res)
			posTo = state.toFloor;
		return res;
	}
}

//====================================================================
// GetSpecialArea
//====================================================================
const SpecialArea *CNavigation::GetSpecialArea(const Vec3 &pos, SpecialArea::EType areaType)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	// make sure each area has a building id
	// Flight/water navigation modifiers are only used to limit the initial navigation 
	// area during preprocessing - give them a building id anyway
	SpecialAreaMap::iterator si = m_mapSpecialAreas.begin();
	SpecialAreaMap::iterator end = m_mapSpecialAreas.end();
	for (; si!=end; ++si)
	{
		SpecialArea &sa = si->second;
		if (sa.nBuildingID<0)
			sa.nBuildingID = m_BuildingIDManager.GetId();
	}  

	si = m_mapSpecialAreas.begin();
	end = m_mapSpecialAreas.end();
	for (; si!=end; ++si)
	{
		const SpecialArea &sa = si->second;
		if (sa.type == areaType)
		{
			if (IsPointInSpecialArea(pos, sa))
			{
				return &sa;
			}
		}
	}

	return 0;
}

//====================================================================
// GetSpecialArea
//====================================================================
const SpecialArea * CNavigation::GetSpecialArea(int buildingID) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (! gEnv->bEditor)
	{
		assert(buildingID < 0 || buildingID < m_vectorSpecialAreas.size());
		if (buildingID >= 0 && buildingID < m_vectorSpecialAreas.size())
			return m_vectorSpecialAreas[buildingID];
	}
	else
	{
		SpecialAreaMap::const_iterator di = m_mapSpecialAreas.begin();
		const SpecialAreaMap::const_iterator end = m_mapSpecialAreas.end();

		for ( ; di != end ; ++di)
		{
			if (di->second.nBuildingID == buildingID)
			{
				return &(di->second);
			}
		}
	}

	return 0;
}

void CNavigation::InsertSpecialArea(const string & name, SpecialArea & sa)
{
	std::pair <SpecialAreaMap::iterator, bool> result =
		m_mapSpecialAreas.insert(SpecialAreaMap::iterator::value_type(name,sa));

	if (! gEnv->IsEditor())
	{
		if ((int)m_vectorSpecialAreas.size() <= sa.nBuildingID)
			m_vectorSpecialAreas.resize (2*m_vectorSpecialAreas.size());
		m_vectorSpecialAreas[sa.nBuildingID] = & result.first->second;
	}
}

void CNavigation::EraseSpecialArea(const string & name)
{
	SpecialAreaMap::iterator si;
	si = m_mapSpecialAreas.find(name);
	if (si== m_mapSpecialAreas.end())
		return;

	if (! gEnv->IsEditor())
	{
		m_vectorSpecialAreas[si->second.nBuildingID] = 0;
	}

	if (si->second.nBuildingID>=0)
		m_BuildingIDManager.FreeId(si->second.nBuildingID);
	m_mapSpecialAreas.erase(si);
}

void CNavigation::FlushSpecialAreas()
{
	m_mapSpecialAreas.clear();
	if (! gEnv->IsEditor())
	{
		m_vectorSpecialAreas.resize(0);
		// NOTE Mai 6, 2007: <pvl> 64 should be enough for most levels I guess. If
		// it's not for some level, InsertSpecialArea() will grow the vector.
		m_vectorSpecialAreas.resize(64,0);
	}
}


//====================================================================
// GetVolumeRegions
//====================================================================
void CNavigation::GetVolumeRegions(VolumeRegions& volumeRegions) const
{
	volumeRegions.resize(0);
	for (SpecialAreaMap::const_iterator di = m_mapSpecialAreas.begin() ; di != m_mapSpecialAreas.end() ; ++di)
	{
		if (di->second.type == SpecialArea::TYPE_VOLUME)
			volumeRegions.push_back(std::make_pair(di->first, &di->second));
	}
}

//====================================================================
// GetBuildingInfo
//====================================================================
bool CNavigation::GetBuildingInfo(int nBuildingID, IAISystem::SBuildingInfo& info) const
{
	const SpecialArea* sa = GetSpecialArea(nBuildingID);
	if (sa)
	{
		info.fNodeAutoConnectDistance = sa->fNodeAutoConnectDistance;
		info.waypointConnections = sa->waypointConnections;
		return true;
	}
	else
	{
		return false;
	}
}

bool CNavigation::IsPointInBuilding(const Vec3& pos, int nBuildingID) const
{
	const SpecialArea* sa = GetSpecialArea(nBuildingID);
	if (!sa) return false;
	return IsPointInSpecialArea(pos, *sa);
}

void CNavigation::FlushAllAreas()
{
	FlushSpecialAreas();

	m_designerForbiddenAreas.Clear();
	m_forbiddenAreas.Clear();
	m_forbiddenBoundaries.Clear();

	m_mapDesignerPaths.clear();
	m_mapExtraLinkCosts.clear();
	m_BuildingIDManager.FreeAll();					// Mikko/Martin - id manager should be reset
}

//====================================================================
// GetNavigationShapeName
//====================================================================
const char *CNavigation::GetNavigationShapeName(int nBuildingID) const
{
	for (SpecialAreaMap::const_iterator di = m_mapSpecialAreas.begin() ; di != m_mapSpecialAreas.end() ; ++di)
	{
		int bID = di->second.nBuildingID;
		const char *name = di->first.c_str();
		if (bID == nBuildingID)
			return name;
	}
	return "Cannot convert building id to name";
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::DoesNavigationShapeExists(const char * szName, EnumAreaType areaType, bool road)
{
	if (areaType == AREATYPE_PATH)
	{
		if (road)
			return m_pRoadNavRegion->DoesRoadExists(szName);
		else
			return m_mapDesignerPaths.find(szName) != m_mapDesignerPaths.end();
	}
	else if (areaType == AREATYPE_FORBIDDEN)
	{
		return m_designerForbiddenAreas.FindShape(szName) != 0;
	}
	else if (areaType == AREATYPE_FORBIDDENBOUNDARY)
	{
		return m_forbiddenBoundaries.FindShape(szName) != 0;
	}
	else if (areaType == AREATYPE_NAVIGATIONMODIFIER)
	{
		return m_mapSpecialAreas.find(szName) != m_mapSpecialAreas.end() ||
			m_mapExtraLinkCosts.find(szName) != m_mapExtraLinkCosts.end();
	}

	return false;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::CreateNavigationShape(const SNavigationShapeParams &params)
{
	std::vector<Vec3> vecPts(params.points, params.points + params.nPoints);

	if (params.areaType==AREATYPE_PATH && params.pathIsRoad == false )
	{
		//designer path need to preserve directions
	}
	else
	{
		if (params.closed)
			EnsureShapeIsWoundAnticlockwise<std::vector<Vec3>, float>(vecPts);
	}

	ListPositions listPts(vecPts.begin(), vecPts.end());

	if (params.areaType==AREATYPE_PATH)
	{
		if (listPts.size() < 2)
			return true; // Do not report too few points as errors.

		if (params.pathIsRoad)
		{
			return m_pRoadNavRegion->CreateRoad(params.szPathName, vecPts, 10.0f, 2.5f);
		}
		else
		{
			if (m_mapDesignerPaths.find(params.szPathName) != m_mapDesignerPaths.end())
			{
				AIError("CAISystem::CreateNavigationShape: Designer path '%s' already exists, please rename the path.", params.szPathName);
				return false;
			}

			if(params.closed)
			{
				if(Distance::Point_Point(listPts.front(), listPts.back()) > 0.1f)
					listPts.push_back(listPts.front());
			}
			m_mapDesignerPaths.insert(ShapeMap::iterator::value_type(params.szPathName, SShape(listPts, false, (IAISystem::ENavigationType)params.nNavType, params.nAuxType)));
		}
	}
	else if (params.areaType==AREATYPE_FORBIDDEN)
	{
		if (listPts.size() < 3)
			return true; // Do not report too few points as errors.

		ClearForbiddenQuadTrees();
		m_forbiddenAreas.Clear();	// The designer forbidden areas will be duplicated into the forbidden areas when regenerating navigation.

		CAIShape* pShape = m_designerForbiddenAreas.FindShape(params.szPathName);
		if (pShape)
		{
			AIError("CAISystem::CreateNavigationShape: Forbidden area '%s' already exists, please rename the shape.", params.szPathName);
			return false;
		}

		pShape = new CAIShape();
		pShape->SetName(params.szPathName);
		pShape->SetPoints(vecPts);
		m_designerForbiddenAreas.InsertShape(pShape);
	}
	else if (params.areaType==AREATYPE_FORBIDDENBOUNDARY)
	{
		if (listPts.size() < 3)
			return true; // Do not report too few points as errors.

		ClearForbiddenQuadTrees();

		CAIShape* pShape = m_forbiddenBoundaries.FindShape(params.szPathName);
		if (pShape)
		{
			AIError("CAISystem::CreateNavigationShape: Forbidden boundary '%s' already exists, please rename the shape.", params.szPathName);
			return false;
		}

		pShape = new CAIShape();
		pShape->SetName(params.szPathName);
		pShape->SetPoints(vecPts);
		m_forbiddenBoundaries.InsertShape(pShape);
	}
	else if (params.areaType == AREATYPE_NAVIGATIONMODIFIER)
	{
		if (listPts.size() < 3)
			return true; // Do not report too few points as errors.

		if (m_mapSpecialAreas.find(params.szPathName) != m_mapSpecialAreas.end())
		{
			AIError("CAISystem::CreateNavigationShape: Navigation modifier '%s' already exists, please rename the modifier.", params.szPathName);
			return false;
		}
		if (m_mapExtraLinkCosts.find(params.szPathName) != m_mapExtraLinkCosts.end())
		{
			AIError("CAISystem::CreateNavigationShape: Navigation modifier '%s' already exists, please rename the modifier.", params.szPathName);
			return false;
		}

		SpecialArea sa;
		sa.SetPolygon(listPts);
		sa.nBuildingID = m_BuildingIDManager.GetId();
		sa.fHeight = params.fHeight;
		sa.lightLevel = params.lightLevel;
		switch (params.nNavType)
		{
		case NMT_WAYPOINTHUMAN: sa.type = SpecialArea::TYPE_WAYPOINT_HUMAN; break;
		case NMT_VOLUME: sa.type = SpecialArea::TYPE_VOLUME; break;
		case NMT_FLIGHT: sa.type = SpecialArea::TYPE_FLIGHT; break;
		case NMT_WATER: sa.type = SpecialArea::TYPE_WATER; break;
		case NMT_WAYPOINT_3DSURFACE: sa.type = SpecialArea::TYPE_WAYPOINT_3DSURFACE; break;
		case NMT_EXTRA_NAV_COST: 
			{
				ExtraLinkCostShapeMap::iterator it = m_mapExtraLinkCosts.insert(ExtraLinkCostShapeMap::iterator::value_type(params.szPathName, SExtraLinkCostShape(listPts, params.extraLinkCostFactor))).first;
				if (params.fHeight < 0.00001f)
				{
					it->second.aabb.min.z = -10000.0f;
					it->second.aabb.max.z =  10000.0f;
				}
				else
				{
					it->second.aabb.max.z += params.fHeight;
				}
				return true;
			}
		case NMT_FREE_2D: sa.type = SpecialArea::TYPE_FREE_2D; break;
		case NMT_TRIANGULATION: sa.type = SpecialArea::TYPE_TRIANGULATION; break;

		default: AIWarning("Unhandled nNavType %d for area type", params.nNavType); return false;
		}
		sa.bCalculate3DNav = params.bCalculate3DNav;
		sa.bCalculateLNM   = params.bCalculateLNM;
		sa.f3DNavVolumeRadius = params.f3DNavVolumeRadius;
		sa.waypointConnections = params.waypointConnections;
		sa.bVehiclesInHumanNav = params.bVehiclesInHumanNav;
		sa.fNodeAutoConnectDistance = params.fNodeAutoConnectDistance;
		sa.fMinZ = 99999.0f;
		sa.fMaxZ = -99999.0f;
		for (ListPositions::const_iterator it = listPts.begin() ; it != listPts.end() ; ++it)
		{
			if (it->z < sa.fMinZ)
				sa.fMinZ = it->z;
			if (it->z > sa.fMaxZ)
				sa.fMaxZ = it->z;
		}
		sa.fMaxZ = sa.fMinZ + sa.fHeight;

		// insert new
		InsertSpecialArea (params.szPathName, sa);
	}

	return true;
}

// deletes designer created path
//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::DeleteNavigationShape(const char * szName)
{
	ShapeMap::iterator di;

	m_pRoadNavRegion->DeleteRoad(szName);

	di = m_mapDesignerPaths.find(szName);
	if (di!= m_mapDesignerPaths.end())
		m_mapDesignerPaths.erase(di);

	if (CAIShape* pShape = m_designerForbiddenAreas.FindShape(szName))
	{
		ClearForbiddenQuadTrees();
		m_forbiddenAreas.Clear();	// The designer forbidden areas will be duplicated into the forbidden areas when regenerating navigation.
		m_designerForbiddenAreas.DeleteShape(pShape);
	}

	if (CAIShape* pShape = m_forbiddenBoundaries.FindShape(szName))
	{
		ClearForbiddenQuadTrees();
		m_forbiddenBoundaries.DeleteShape(pShape);
	}

	EraseSpecialArea(szName);

	ExtraLinkCostShapeMap::iterator it = m_mapExtraLinkCosts.find(szName);
	if (it != m_mapExtraLinkCosts.end())
		m_mapExtraLinkCosts.erase(it);
}

void CNavigation::DisableModifier (const char * name)
{
	SpecialAreaMap::iterator di;
	di = m_mapSpecialAreas.find(name);
	if(di!=m_mapSpecialAreas.end())
	{
		CAISystem * pAISystem = GetAISystem();
		SpecialArea& sa = di->second;
		if(sa.bAltered)	// already disabled
			return;
		pAISystem->InvalidatePathsThroughArea(sa.GetPolygon());
		sa.bAltered = true;
	}
}

//====================================================================
// IsPointInForbiddenRegion
// Note that although forbidden regions aren't allowed to cross they can
// be nested, so we assume that in/out alternates with nesting...
//====================================================================
bool CNavigation::IsPointInForbiddenRegion(const Vec3 & pos, bool checkAutoGenRegions) const
{
	return IsPointInForbiddenRegion(pos, 0, checkAutoGenRegions);
}

//===================================================================
// IsPointInForbiddenRegion
//===================================================================
bool CNavigation::IsPointInForbiddenRegion(const Vec3& pos, const CAIShape** ppShape, bool checkAutoGenRegions) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (checkAutoGenRegions)
		return m_forbiddenAreas.IsPointInside(pos, ppShape);
	return m_designerForbiddenAreas.IsPointInside(pos, ppShape);
}

//====================================================================
// IsPointInWaterAreas
//====================================================================
inline bool IsPointInWaterAreas(const Vec3& pt, const std::vector<ListPositions>& areas)
{
	unsigned nAreas = areas.size();
	for (unsigned i = 0 ; i < nAreas ; ++i)
	{
		const ListPositions& area = areas[i];
		if (Overlap::Point_Polygon2D(pt, area))
			return true;
	}
	return false;
}

//===================================================================
// IsPointInWaterAreas
//===================================================================
bool CNavigation::IsPointInWaterAreas(const Vec3 &pt) const
{
	for (SpecialAreaMap::const_iterator it = m_mapSpecialAreas.begin() ; it != m_mapSpecialAreas.end() ; ++it)
	{
		const string &name = it->first;
		const SpecialArea &sa = it->second;
		if (sa.type == SpecialArea::TYPE_WATER)
		{
			if (Overlap::Point_AABB2D(pt, sa.GetAABB()))
				if (Overlap::Point_Polygon2D(pt, sa.GetPolygon()))
					return true;
		}
	}
	return false;
}

//====================================================================
// IsPointInSpecialArea
//====================================================================
bool CNavigation::IsPointInSpecialArea(const Vec3 &pos, const SpecialArea &sa)
{
	if ( sa.fHeight>0.00001f && (pos.z < sa.fMinZ || pos.z > sa.fMaxZ) )
		return false;
	if (Overlap::Point_Polygon2D(pos, sa.GetPolygon(), &sa.GetAABB()))
		return true;
	else
		return false;
}

//===================================================================
// IsPointInTriangulationAreas
//===================================================================
bool CNavigation::IsPointInTriangulationAreas(const Vec3 &pos) const
{
	AIAssert(gEnv->IsEditor());
	bool foundOne = false;
	for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
	{
		const SpecialArea &sa = si->second;
		if (sa.type == SpecialArea::TYPE_TRIANGULATION)
		{
			if (Overlap::Point_Polygon2D(pos, sa.GetPolygon(), &sa.GetAABB()))
				return true;
			foundOne = true;
		}
	}
	return !foundOne;
}

//
//-----------------------------------------------------------------------------------------------------------
bool CNavigation::IntersectsForbidden(const Vec3& start, const Vec3& end, Vec3& closestPoint, const string* nameToSkip, Vec3* pNormal, 
									  INavigation::EIFMode mode, bool bForceNormalOutwards) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (m_forbiddenAreas.GetShapes().empty() && m_forbiddenBoundaries.GetShapes().empty())
		return false;

	switch(mode)
	{
	case INavigation::IF_AREASBOUNDARIES: 
		if (m_forbiddenAreas.IntersectLineSeg(start, end, closestPoint, pNormal))
			return true;
		if (m_forbiddenBoundaries.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
			return true;
		break;
	case INavigation::IF_AREAS: // areas only
		if (m_forbiddenAreas.IntersectLineSeg(start, end, closestPoint, pNormal))
			return true;
		break;
	case INavigation::IF_BOUNDARIES: // boundaries only
		if (m_forbiddenBoundaries.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
			return true;
		break;
	default:
		AIError("Bad mode %d passed to CAISystem::IntersectsForbidden", mode);
		if (m_forbiddenAreas.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
			return true;
		if (m_forbiddenBoundaries.IntersectLineSeg(start, end, closestPoint, pNormal, bForceNormalOutwards))
			return true;
		break;
	}
	return false;
}

//===================================================================
// ClearForbiddenQuadTrees
//===================================================================
void CNavigation::ClearForbiddenQuadTrees()
{
	m_forbiddenAreas.ClearQuadTree();
	m_forbiddenBoundaries.ClearQuadTree();
	m_designerForbiddenAreas.ClearQuadTree();
}

//===================================================================
// RebuildForbiddenQuadTrees
//===================================================================
void CNavigation::RebuildForbiddenQuadTrees()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "Forbidden quad trees");

	ClearForbiddenQuadTrees();

	m_forbiddenAreas.BuildBins();
	m_forbiddenBoundaries.BuildBins();
	m_designerForbiddenAreas.BuildBins();

	if (useForbiddenQuadTree)
	{
		m_forbiddenAreas.BuildQuadTree(8, 20.0f);
		m_forbiddenBoundaries.BuildQuadTree(8, 20.0f);
		m_designerForbiddenAreas.BuildQuadTree(8, 20.0f);
	}
}

//-----------------------------------------------------------------------------------------------------------
bool CNavigation::IsPointOnForbiddenEdge(const Vec3 & pos, float tol, Vec3* pNormal, const CAIShape** ppShape, bool checkAutoGenRegions) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (checkAutoGenRegions)
	{
		if (m_forbiddenAreas.IsPointOnEdge(pos, tol, ppShape))
			return true;
	}
	else
	{
		if (m_designerForbiddenAreas.IsPointOnEdge(pos, tol, ppShape))
			return true;
	}

	if (m_forbiddenBoundaries.IsPointOnEdge(pos, tol, ppShape))
		return true;

	return false;
}

//====================================================================
// IsPointInForbiddenBoundary
//====================================================================
bool CNavigation::IsPointInForbiddenBoundary(const Vec3 & pos, const CAIShape** ppShape) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	const CAIShape* pHitShape = 0;
	unsigned hits = m_forbiddenBoundaries.IsPointInsideCount(pos, &pHitShape);

	if ((hits % 2) != 0)
	{
		if (ppShape)
			*ppShape = pHitShape;
		return true;
	}
	return false;
}

//====================================================================
// IsPointForbidden
//====================================================================
bool CNavigation::IsPointForbidden(const Vec3& pos, float tol, const CAIShape** ppShape, Vec3* pNormal) const
{
	if (IsPointInForbiddenRegion(pos, ppShape, true))
		return true;
	return IsPointOnForbiddenEdge(pos, tol, pNormal, ppShape);
}

//====================================================================
// IsPointForbidden
//====================================================================
bool CNavigation::IsPointForbidden(const Vec3& pos, float tol, Vec3* pNormal) const
{
	return IsPointForbidden(pos, tol, NULL, pNormal);
}

//====================================================================
// GetPointOutsideForbidden
//====================================================================
Vec3 CNavigation::GetPointOutsideForbidden(Vec3& pos, float distance, const Vec3* startPos) const
{
	Vec3 normal;
	Vec3 retPos(pos);
	const CAIShape* pRegion;
	if(startPos && IntersectsForbidden(pos,*startPos, retPos, NULL, &normal, INavigation::IF_BOUNDARIES, true))
	{
		Vec3 delta = *startPos - retPos;
		float len = delta.NormalizeSafe();
		retPos += delta * min(distance, len);
	}
	else if( startPos && IntersectsForbidden(*startPos,pos, retPos,NULL,&normal,INavigation::IF_AREAS))
	{
		Vec3 delta = *startPos - retPos;
		float len = delta.NormalizeSafe();
		retPos += delta * min(distance, len);
	}
	else if (IsPointInForbiddenRegion(pos, &pRegion, true))// ||IsPointInForbiddenBoundary(pos,pRegion))
	{
		// get the closest point in the area's edge
		Distance::Point_Polygon2DSq(pos, pRegion->GetPoints(), retPos, &normal);
		retPos -= normal*distance; // normal is going towards the pos -> inwards the polygon
	}

	return retPos;
}

const char*	CNavigation::GetNearestPathOfTypeInRange(IAIObject* requester, const Vec3& reqPos, float range, int type, float devalue, bool useStartNode)
{
	AIAssert(requester);
	float	rangeSq(sqr(range));

	ShapeMap::iterator	closestShapeIt(m_mapDesignerPaths.end());
	float closestShapeDist(rangeSq);
	CAIActor* pReqActor = requester->CastToCAIActor();

	for(ShapeMap::iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
	{
		const SShape&	path = it->second;

		// Skip paths which we cannot travel
		if((path.navType & pReqActor->GetMovementAbility().pathfindingProperties.navCapMask) == 0)
			continue;
		// Skip wrong type
		if(path.type != type)
			continue;
		// Skip locked paths
		if(path.devalueTime > 0.01f)
			continue;

		// Skip paths too far away
		Vec3	tmp;
		if(Distance::Point_AABBSq(reqPos, path.aabb, tmp) > rangeSq)
			continue;

		float	d;
		if(useStartNode)
		{
			// Disntance to start node.
			d = Distance::Point_PointSq(reqPos, path.shape.front());
		}
		else
		{
			// Distance to nearest point on path.
			ListPositions::const_iterator	nearest = path.NearestPointOnPath(reqPos, d, tmp);
		}

		if(d < closestShapeDist)
		{
			closestShapeIt = it;
			closestShapeDist = d;
		}
	}

	if(closestShapeIt != m_mapDesignerPaths.end())
	{
		closestShapeIt->second.devalueTime = devalue;
		return closestShapeIt->first.c_str();
	}

	return 0;
}

//====================================================================
// IsPathForbidden
//====================================================================
bool CNavigation::IsPathForbidden(const Vec3 & start, const Vec3 & end) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	// do areas and boundaries separately 

	// for boundaries get a list of each of the boundaries the start and end
	// are in. Path is forbidden if the lists are different.
	std::set<const void *> startBoundaries;
	std::set<const void *> endBoundaries;

	for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
	{
		const CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
		if (pShape->IsPointInside(start))
			startBoundaries.insert(pShape);
	}

	for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
	{
		const CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
		if (pShape->IsPointInside(end))
			endBoundaries.insert(pShape);
	}

	if (startBoundaries != endBoundaries)
		return true;

	// boundaries are not a problem. Areas are trikier because we can 
	// always go from a forbidden area into a not-forbidden area, but not 
	// the other way. Simplify things by assuming they are only nested a bit
	// - i.e. there are no more than two forbidden areas outside a point. 
	std::set<const void *> startAreas;
	std::set<const void *> endAreas;

	for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
	{
		const CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
		if (pShape->IsPointInside(start))
			startAreas.insert(pShape);
	}

	for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
	{
		const CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
		if (pShape->IsPointInside(end))
			endAreas.insert(pShape);
	}

	// start and end completely clear.
	if (startAreas.empty() && endAreas.empty())
		return false;

	// start and end in the same 
	if (startAreas == endAreas)
		return false;

	// going from nested-but-valid to not-nested-but-valid is forbidden
	if (startAreas.size() > endAreas.size()+1)
		return true;

	// check for transition from valid to forbidden
	bool startInForbidden = (startAreas.size() % 2) != 0;
	bool endInForbidden = (endAreas.size() % 2) != 0;
	if (!startInForbidden && endInForbidden)
		return true;

	// This isn't foolproof but it is OK in most sensible situations!
	return false;
}

//====================================================================
// OverlapExtraCostAreas
//====================================================================
bool CNavigation::OverlapExtraCostAreas(const Lineseg & lineseg) const
{
	if (m_mapExtraLinkCosts.empty())
		return false;

	AABB linkAABB(AABB::RESET);
	linkAABB.Add(lineseg.start);
	linkAABB.Add(lineseg.end);

	for (ExtraLinkCostShapeMap::const_iterator it = m_mapExtraLinkCosts.begin() ; it != m_mapExtraLinkCosts.end() ; ++it)
	{
		const SExtraLinkCostShape &shape = it->second;
		if (Overlap::AABB_AABB(shape.aabb, linkAABB))
		{
			float cost = GetExtraLinkCost(lineseg.start, lineseg.end, linkAABB, shape);
			if (cost < 0.0f || cost > 0.0001f)
				return true;
		}
	}
	return false;
}

//====================================================================
// GetExtraLinkCost
//====================================================================
float CNavigation::GetExtraLinkCost(const Vec3 &pos1, const Vec3 &pos2, const AABB &linkAABB, const SExtraLinkCostShape &shape) const
{
	// Danny todo implement this properly. For now just return the full factor if there's any intersection - really
	// we should return just a fraction of the factor, depending on the extent of intersection.

	if (Overlap::Point_Polygon2D(pos1, shape.shape, &shape.aabb) || 
		Overlap::Point_Polygon2D(pos2, shape.shape, &shape.aabb) || 
		Overlap::Lineseg_Polygon2D(Lineseg(pos1, pos2), shape.shape, &shape.aabb))
	{
		return shape.costFactor;
	}
	else
	{
		return 0.0f;
	}
}

//====================================================================
// GetExtraLinkCost
//====================================================================
float CNavigation::GetExtraLinkCost(const Vec3 &pos1, const Vec3 &pos2) const
{
	if (m_mapExtraLinkCosts.empty())
		return 0.0f;

	AABB linkAABB(AABB::RESET);
	linkAABB.Add(pos1);
	linkAABB.Add(pos2);

	float factor = 0.0f;
	for (ExtraLinkCostShapeMap::const_iterator it = m_mapExtraLinkCosts.begin() ; it != m_mapExtraLinkCosts.end() ; ++it)
	{
		const SExtraLinkCostShape &shape = it->second;
		if (Overlap::AABB_AABB(shape.aabb, linkAABB))
		{
			float cost = GetExtraLinkCost(pos1, pos2, linkAABB, shape);
			if (cost < 0.0f)
				return -1.0f;
			factor += cost;
		}
	}
	return factor;
}


//====================================================================
// DisableNavigationInBrokenRegion
//====================================================================
void CNavigation::DisableNavigationInBrokenRegion(std::list<Vec3> & outline)
{
	if (!gAIEnv.pGraph)
	{
		AIWarning("Being asked to disable navigation in broken region, yet no graph");
		return;
	}
	AABB outlineAABB(AABB::RESET);
	for (std::list<Vec3>::const_iterator it = outline.begin() ; it != outline.end() ; ++it)
		outlineAABB.Add(*it);
	outlineAABB.min.z = -std::numeric_limits<float>::max();
	outlineAABB.max.z = std::numeric_limits<float>::max();

	AILogEvent("Disabling navigation in broken region (%5.2f, %5.2f, %5.2f) to (%5.2f, %5.2f, %5.2f)",
		outlineAABB.min.x, outlineAABB.min.y, outlineAABB.min.z, 
		outlineAABB.max.x, outlineAABB.max.y, outlineAABB.max.z);
}


//===================================================================
// ModifyNavCostFactor
//===================================================================
void CNavigation::ModifyNavCostFactor(const char *navModifierName, float factor)
{
	size_t reqNameLen = strlen(navModifierName);

	for (ExtraLinkCostShapeMap::iterator it = m_mapExtraLinkCosts.begin() ; it != m_mapExtraLinkCosts.end() ; ++it)
	{
		const string &name = it->first;
		size_t nameLen = strlen(name.c_str());
		if (nameLen < reqNameLen)
			continue;

		const char *realName = name.c_str() + nameLen - reqNameLen;
		if (strncmp(realName, navModifierName, reqNameLen) == 0)
		{
			SExtraLinkCostShape &shape = it->second;
			shape.costFactor = factor;
			return;
		}
	}
	AIWarning("CAISystem::ModifyNavCostFactor Unable to find extra cost nav modifier called %s", navModifierName);
}

//====================================================================
// GetVolumeRegionFiles
//====================================================================
void CNavigation::GetVolumeRegionFiles(const char * szLevel, const char * szMission, DynArray<CryStringT<char> > & filenames) const
{
	filenames.clear();

	VolumeRegions volumeRegions;
	GetVolumeRegions(volumeRegions);

	string name ;
	unsigned nRegions = volumeRegions.size();
	for (unsigned i = 0 ; i < nRegions ; ++i)
	{
		const string& regionName = volumeRegions[i].first;
		/// match what's done in CVolumeNavRegion
		if (regionName.length() > 17)
		{
			name = string(regionName.c_str() + 17);
		}
		else
		{
			AIWarning("CAISystem::GetVolumeRegionFiles region name is too short %s", regionName.c_str());
			continue;
		}

		CryStringT<char> fileName;
		fileName.Format("%s/v3d%s-region-%s.bai",szLevel,szMission, name.c_str());

		filenames.push_back(fileName);
	}
}

//====================================================================
// GetNavRegion
//====================================================================
CNavRegion *CNavigation::GetNavRegion(IAISystem::ENavigationType type, const class CGraph *pGraph) const
{
	switch (type)
	{
	case IAISystem::NAV_TRIANGULAR: return m_pTriangularNavRegion;
	case IAISystem::NAV_WAYPOINT_HUMAN: return m_pWaypointHumanNavRegion;
	case IAISystem::NAV_WAYPOINT_3DSURFACE: return m_pWaypoint3DSurfaceNavRegion;
	case IAISystem::NAV_FLIGHT: return m_pFlightNavRegion;
	case IAISystem::NAV_VOLUME: return m_pVolumeNavRegion;
	case IAISystem::NAV_ROAD: return m_pRoadNavRegion;
	case IAISystem::NAV_SMARTOBJECT: return m_pSmartObjectNavRegion;
	case IAISystem::NAV_CUSTOM_NAVIGATION: return m_pCustomNavRegion;
	case IAISystem::NAV_FREE_2D: return m_pFree2DNavRegion;
	case IAISystem::NAV_LAYERED_NAV_MESH: return m_pLayeredNavMeshRegion;
	case IAISystem::NAV_UNSET: return 0;
	}
	return 0;
}

//====================================================================
// IsFlightSpaceVoid
// check if the space is void inside of the box which is made by these 8 point
//====================================================================
Vec3 CNavigation::IsFlightSpaceVoid(const Vec3 &vPos, const Vec3 &vFwd, const Vec3 &vWng, const Vec3 &vUp ) const
{
	if ( m_pFlightNavRegion )
		return m_pFlightNavRegion->IsSpaceVoid(vPos,vFwd,vWng,vUp);

	return Vec3(ZERO);
}

//====================================================================
// IsFlightSpaceVoidByRadius
//====================================================================
Vec3 CNavigation::IsFlightSpaceVoidByRadius(const Vec3 &vPosInput, const Vec3 &vFwdInput, float radius ) const
{

	Vec3 vUp(0.0,0.0,1.0f);
	Vec3 vWng;
	Vec3 vFwd = vFwdInput;
	Vec3 vPos = vPosInput;

	float fwdLen = vFwd.GetLength();
	vFwd.NormalizeSafe();

	if ( fabs(vUp.Dot(vFwd)) > cosf( DEG2RAD(0.5f) ) )
	{
		vUp = Vec3( 0.0f, 0.0f, vFwd.z * fwdLen );
		vWng = Vec3( 1.0f, 0.0f, 0.0f );
		vFwd = Vec3( 0.0f, 1.0f, 0.0f );

		vPos -= ( vWng * radius );
		vPos -= ( vFwd * radius  );
		vWng *= radius * 2.0f;
		vFwd *= radius * 2.0f;
	}
	else
	{
		vFwd.NormalizeSafe();
		vWng =vFwd.Cross( vUp );
		vWng.NormalizeSafe();
		vUp = vFwd.Cross( vWng );
		vUp.NormalizeSafe();

		vPos -= ( vWng * radius );
		vPos -= ( vUp * radius  );

		vWng *= radius * 2.0f;
		vUp  *= radius * 2.0f;
		vFwd *= fwdLen;
	}
	if ( m_pFlightNavRegion )
		return m_pFlightNavRegion->IsSpaceVoid(vPos,vFwd,vWng,vUp);

	return Vec3(ZERO);
}

//====================================================================
// CheckNavigationType
// When areas are nested there is an ordering - make this explicit by
// ordering the search over the navigation types
//====================================================================
IAISystem::ENavigationType CNavigation::CheckNavigationType(const Vec3 & pos, int & nBuildingID, IVisArea *&pAreaID, IAISystem::tNavCapMask navCapMask) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

#ifdef _DEBUG
	// make sure each area has a building id
	// Flight/water navigation modifiers are only used to limit the initial navigation 
	// area during preprocessing - give them a building id anyway
	for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
	{
		const SpecialArea &sa = si->second;
		// FIXME: If we hit this assert, then this needs to be handled elsewhere
		assert(sa.nBuildingID >= 0);
		/*if (sa.nBuildingID<0)
			sa.nBuildingID = m_BuildingIDManager.GetId();*/
	}
#endif //_DEBUG

	// LNM takes priority
	if (navCapMask & IAISystem::NAV_LAYERED_NAV_MESH)
	{
		// If LNM is loaded (allows fall-back to old systems so both can safely co-exist)
		if (m_pLayeredNavMeshRegion->IsAnyNavMeshLoaded())
		{
			for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
			{
				const SpecialArea &sa = si->second;
				if (sa.type == SpecialArea::TYPE_LAYERED_NAV_MESH)
				{
					if (IsPointInSpecialArea(pos, sa))
					{
						nBuildingID = sa.nBuildingID;
						I3DEngine *p3dEngine=gEnv->p3DEngine;
						pAreaID = p3dEngine->GetVisAreaFromPos(pos);
						return IAISystem::NAV_LAYERED_NAV_MESH;
					}
				}
			}
		}
	}

	// Note: Marcio tried changing order in the C2 branch, so volume navigation should be prefered in case of overlapping	
	if (navCapMask & IAISystem::NAV_WAYPOINT_HUMAN)
	{
		for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
		{
			const SpecialArea &sa = si->second;
			if (sa.type == SpecialArea::TYPE_WAYPOINT_HUMAN)
			{
				if (IsPointInSpecialArea(pos, sa))
				{
					nBuildingID = sa.nBuildingID;
					I3DEngine *p3dEngine=gEnv->p3DEngine;
					pAreaID = p3dEngine->GetVisAreaFromPos(pos);
					return IAISystem::NAV_WAYPOINT_HUMAN;
				}
			}
		}
	}

	if (navCapMask & IAISystem::NAV_WAYPOINT_3DSURFACE)
	{
		for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
		{
			const SpecialArea &sa = si->second;
			if (sa.type == SpecialArea::TYPE_WAYPOINT_3DSURFACE)
			{
				if (IsPointInSpecialArea(pos, sa))
				{
					// 3D surface is slightly different since it's likely to be overlapped with another
					// type - we only return it if we're in the area AND near a suitable node
					if (0 == m_pWaypoint3DSurfaceNavRegion->GetBestNodeForPosInInRegion(pos, sa))
						continue;

					nBuildingID = sa.nBuildingID;
					I3DEngine *p3dEngine=gEnv->p3DEngine;
					pAreaID = p3dEngine->GetVisAreaFromPos(pos);
					return IAISystem::NAV_WAYPOINT_3DSURFACE;
				}
			}
		}
	}

	if (navCapMask & IAISystem::NAV_VOLUME)
	{
		for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
		{
			const SpecialArea &sa = si->second;
			if (sa.type == SpecialArea::TYPE_VOLUME)
			{
				if (IsPointInSpecialArea(pos, sa))
				{
					nBuildingID = sa.nBuildingID;
					I3DEngine *p3dEngine=gEnv->p3DEngine;
					pAreaID = p3dEngine->GetVisAreaFromPos(pos);
					return IAISystem::NAV_VOLUME;
				}
			}
		}
	}

	if( navCapMask & IAISystem::NAV_FLIGHT )
	{
		for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
		{
			const SpecialArea &sa = si->second;
			if (sa.type == SpecialArea::TYPE_FLIGHT)
			{
				if (IsPointInSpecialArea(pos, sa))
					return IAISystem::NAV_FLIGHT;
			}
		}
		// if (navCapMask != IAISystem::NAVMASK_ALL)
		//   AIWarning("No flight AI nav modifier around point (%5.2f, %5.2f, %5.2f)", pos.x, pos.y, pos.z);
	}

	if (navCapMask & IAISystem::NAV_FREE_2D)
	{
		for (SpecialAreaMap::const_iterator si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();++si)
		{
			const SpecialArea &sa = si->second;
			if (sa.type == SpecialArea::TYPE_FREE_2D)
			{
				if (IsPointInSpecialArea(pos, sa))
				{
					nBuildingID = sa.nBuildingID;
					I3DEngine *p3dEngine=gEnv->p3DEngine;
					pAreaID = p3dEngine->GetVisAreaFromPos(pos);
					return IAISystem::NAV_FREE_2D;
				}
			}
		}
	}

	if (navCapMask & IAISystem::NAV_TRIANGULAR)
		return IAISystem::NAV_TRIANGULAR;

	if (navCapMask & IAISystem::NAV_FREE_2D)
		return IAISystem::NAV_FREE_2D;

	return IAISystem::NAV_UNSET;
}

void CNavigation::GetMemoryStatistics(ICrySizer *pSizer)
{
	{
		SIZER_SUBCOMPONENT_NAME(pSizer,"NavRegions");
		if(m_pTriangularNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"Triangular");
			pSizer->AddObject( m_pTriangularNavRegion, m_pTriangularNavRegion->MemStats());
		}
		if(m_pWaypointHumanNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"WaypointHuman");
			pSizer->AddObject( m_pWaypointHumanNavRegion, m_pWaypointHumanNavRegion->MemStats());
		}
		if(m_pWaypoint3DSurfaceNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"Waypoint3DSurface");
			pSizer->AddObject( m_pWaypoint3DSurfaceNavRegion, m_pWaypoint3DSurfaceNavRegion->MemStats());
		}
		if(m_pVolumeNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"Volume");
			pSizer->AddObject( m_pVolumeNavRegion, m_pVolumeNavRegion->MemStats());
		}
		if(m_pFlightNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"Flight");
			pSizer->AddObject( m_pFlightNavRegion, m_pFlightNavRegion->MemStats());
		}
		if(m_pRoadNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"Road");
			pSizer->AddObject( m_pRoadNavRegion, m_pRoadNavRegion->MemStats());
		}
		if(m_pFree2DNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"Free2D");
			pSizer->AddObject( m_pFree2DNavRegion, m_pFree2DNavRegion->MemStats());
		}
		if(m_pSmartObjectNavRegion)
		{
			SIZER_SUBCOMPONENT_NAME(pSizer,"Smart");
			pSizer->AddObject( m_pSmartObjectNavRegion, m_pSmartObjectNavRegion->MemStats());
		}
	}

	size_t size=0;
	for(ShapeMap::iterator itr=m_mapDesignerPaths.begin(); itr!=m_mapDesignerPaths.end(); itr++)
	{
		size += (itr->first).capacity();
		size += itr->second.MemStats();
	}
	pSizer->AddObject( &m_mapDesignerPaths, size);

	pSizer->AddObject( &m_forbiddenAreas, m_forbiddenAreas.MemStats());
	pSizer->AddObject( &m_designerForbiddenAreas, m_designerForbiddenAreas.MemStats());
	pSizer->AddObject( &m_forbiddenBoundaries, m_forbiddenBoundaries.MemStats());

	size=0;
	for(SpecialAreaMap::iterator sit=m_mapSpecialAreas.begin(); sit!=m_mapSpecialAreas.end(); sit++)
	{
		size += (sit->first).capacity();
		size += sizeof(SpecialArea);
	}
	pSizer->AddObject( &m_mapSpecialAreas, size);

	{
		// FIXME Feb 1, 2008: <pvl> look into moving m_VertexList out of AISystem as well
		CAISystem * pAISystem = GetAISystem();

		SIZER_SUBCOMPONENT_NAME(pSizer,"Triangle vertex data");
		size = pAISystem->m_VertexList.GetCapacity()*sizeof(ObstacleData);
		pSizer->AddObject( &pAISystem->m_VertexList, size );
		size = m_vTriangles.capacity() * sizeof(Tri*) + m_vTriangles.size() * sizeof(Tri);
		pSizer->AddObject( &pAISystem->m_VertexList, size );
		size = 0;
		for(std::vector<Vtx>::iterator vtx=m_vVertices.begin(); vtx!=m_vVertices.end(); vtx++)
			size += sizeof(Vtx) + sizeof(int)*vtx->m_lstTris.size();
		pSizer->AddObject( &m_vVertices, size);
	}
}

//-----------------------------------------------------------------------------------------------------------
void CNavigation::DebugDraw() const
{
	CDebugDrawContext dc;
	
	if (!m_validationErrorMarkers.empty())
	{
		for (unsigned i = 0; i < m_validationErrorMarkers.size(); ++i)
		{
			const SValidationErrorMarker& marker = m_validationErrorMarkers[i];
			dc->DrawOBB(marker.obb, marker.pos, false, marker.col, eBBD_Faceted);

			const float s = 0.01f;
			dc->DrawLine(marker.pos + Vec3(-s,  0,  0), marker.col, marker.pos + Vec3(s, 0, 0), marker.col);
			dc->DrawLine(marker.pos + Vec3( 0, -s,  0), marker.col, marker.pos + Vec3(0, s, 0), marker.col);
			dc->DrawLine(marker.pos + Vec3( 0,  0, -s), marker.col, marker.pos + Vec3(0, 0, s), marker.col);

			dc->Draw3dLabelEx(marker.pos, 1.1f, marker.col, true, true, "%s", marker.msg.c_str());
		}
	}

	CAISystem * pAISystem = GetAISystem();
	int	debugDrawVal = gAIEnv.CVars.DebugDraw;

	if(debugDrawVal==71)
		DebugDrawForbidden();
	else if(debugDrawVal==77)
		m_pVolumeNavRegion->DebugDrawConnections();
	else if(debugDrawVal==78)
		m_pVolumeNavRegion->DebugDrawNavProblems();
	else if(debugDrawVal>=500)
		m_pVolumeNavRegion->DebugDrawConnections(debugDrawVal - 500, true);

	static bool recalcHideSpots = true;
	if (debugDrawVal == 81)
	{
		if (recalcHideSpots)
		{
			m_pVolumeNavRegion->CalculateHideSpots();
			recalcHideSpots = false;
		}
	}
	else
	{
		recalcHideSpots = true;
	}
	if (debugDrawVal == 82)
		m_pVolumeNavRegion->DebugDrawHideSpots();

	if (debugDrawVal == 90)
		m_pFlightNavRegion->DebugDraw(dc->GetCameraPos(), 200.0f);

	if (gAIEnv.CVars.DebugDrawVolumeVoxels != 0)
		m_pVolumeNavRegion->DebugDrawVoxels();

	m_pVolumeNavRegion->DebugDraw();

	{
		// check all entities called CheckVoxelSomething
		std::vector<const IEntity*> checkEntities;
		IEntityItPtr entIt = gEnv->pEntitySystem->GetEntityIterator();
		const char* navName = "CheckVoxel";
		size_t navNameLen = strlen(navName);
		if (entIt)
		{
			entIt->MoveFirst();
			while (!entIt->IsEnd())
			{
				IEntity* ent = entIt->Next();
				if (ent)
				{
					const char* name = ent->GetName();
					if (strnicmp(name, navName, navNameLen) == 0)
					{
						checkEntities.push_back(ent);
					}
				}
			}
		}

		std::vector<const IEntity*> navEntities;
		m_pVolumeNavRegion->GetNavigableSpaceEntities(navEntities);

		for (unsigned iEnt = 0 ; iEnt < checkEntities.size() ; ++iEnt)
		{
			const IEntity* ent = checkEntities[iEnt];
			const Vec3 & pos = ent->GetPos();
			Vec3 definitelyValidPt;
			if (m_pVolumeNavRegion->GetNavigableSpacePoint(pos, definitelyValidPt, navEntities))
			{
				static float range = 30.0f;
				Vec3 regionMin = pos - Vec3(range, range, range);
				Vec3 regionMax = pos + Vec3(range, range, range);
				CVolumeNavRegion::ECheckVoxelResult result = m_pVolumeNavRegion->CheckVoxel(pos, definitelyValidPt);
				ColorF col;
				switch (result)
				{
				case CVolumeNavRegion::VOXEL_VALID:
					col.Set(1, 0, 0);
					break;
				case CVolumeNavRegion::VOXEL_INVALID:
					col.Set(0, 1, 0);
					break;
				}
				dc->DrawSphere(pos, 0.7f, col);
			}
		}
	}

	{
		IEntity* entFrom = gEnv->pEntitySystem->FindEntityByName("CheckCapsuleFrom");
		IEntity* entTo = gEnv->pEntitySystem->FindEntityByName("CheckCapsuleTo");
		if (entFrom && entTo)
		{
			static float radius = 3.0f;

			const Vec3 & posFrom = entFrom->GetPos();
			const Vec3 & posTo = entTo->GetPos();

			bool result = m_pVolumeNavRegion->PathSegmentWorldIntersection(posFrom, posTo, radius, true, true, AICE_ALL);
			ColorF col;
			if (result)
				col.Set(1, 0, 0);
			else
				col.Set(0, 1, 0);
			dc->DrawSphere(posFrom, radius, col);
			dc->DrawSphere(posTo, radius, col);
		}
	}

	{
		IEntity* ent = gEnv->pEntitySystem->FindEntityByName("CheckBody");
		if (ent)
		{
			const Vec3 & pos = ent->GetPos();
			m_pWaypointHumanNavRegion->CheckAndDrawBody(pos);
		}
	}

	// Draw occupied designer paths
	for(ShapeMap::const_iterator it = m_mapDesignerPaths.begin(); it != m_mapDesignerPaths.end(); ++it)
	{
		const SShape&	path = it->second;
		if(path.devalueTime < 0.01f)
			continue;
		dc->Draw3dLabel(path.shape.front(), 1, "%.1f", path.devalueTime);
	}

	if (gAIEnv.CVars.lnmDebugDraw)
		DebugDrawLayeredNavMesh();
}

//===================================================================
// DebugDrawForbidden
//===================================================================
void CNavigation::DebugDrawForbidden() const
{
	CDebugDrawContext dc;

	// draw forbidden areas
	if (!m_forbiddenAreas.GetShapes().empty())
	{
		ColorF col(1, 0, 0);
		for (unsigned i = 0, ni = m_forbiddenAreas.GetShapes().size(); i < ni; ++i)
		{
			const CAIShape* pShape = m_forbiddenAreas.GetShapes()[i];
			const ShapePointContainer& pts = pShape->GetPoints();
			for (unsigned j = 0, nj = pts.size(); j < nj; ++j)
			{
				Vec3 v0 = pts[j];
				Vec3 v1 = pts[(j+1)%nj];
				v0.z += 0.01f;
				v1.z += 0.01f;
				//				v0.z = 0.01f + GetDrawZ(v0, true); // avoid clash with triangles
				//				v1.z = 0.01f + GetDrawZ(v1, true);
				dc->DrawLine(v0, col, v1, col);
			}
		}
	}
	//	else
	{
		// If the forbidden areas are empty, try drawing the designer forbidden areas.
		ColorF col(1, 0, 0.75f, 0.5f);
		for (unsigned i = 0, ni = m_designerForbiddenAreas.GetShapes().size(); i < ni; ++i)
		{
			const CAIShape* pShape = m_designerForbiddenAreas.GetShapes()[i];
			const ShapePointContainer& pts = pShape->GetPoints();
			for (unsigned j = 0, nj = pts.size(); j < nj; ++j)
			{
				Vec3 v0 = pts[j];
				Vec3 v1 = pts[(j+1)%nj];
				v0.z += 0.1f;
				v1.z += 0.1f;
				//				v0.z = 0.1f + GetDrawZ(v0, true); // avoid clash with triangles
				//				v1.z = 0.1f + GetDrawZ(v1, true);
				dc->DrawLine(v0, col, v1, col);
			}
		}
	}

	// draw forbidden boundaries
	if (!m_forbiddenBoundaries.GetShapes().empty())
	{
		ColorF col(1, 1, 0);
		for (unsigned i = 0, ni = m_forbiddenBoundaries.GetShapes().size(); i < ni; ++i)
		{
			const CAIShape* pShape = m_forbiddenBoundaries.GetShapes()[i];
			const ShapePointContainer& pts = pShape->GetPoints();
			for (unsigned j = 0, nj = pts.size(); j < nj; ++j)
			{
				Vec3 v0 = pts[j];
				Vec3 v1 = pts[(j+1)%nj];
				v0.z = 0.01f + dc->GetDebugDrawZ(v0, true);
				v1.z = 0.01f + dc->GetDebugDrawZ(v1, true);
				dc->DrawLine(v0, col, v1, col);
			}
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::DebugDrawNavModifiers() const
{
	CDebugDrawContext dc;

	if (!gAIEnv.CVars.DrawModifiers)
		return;
	Vec3	offset(0.0f,0.0f,0.1f);
	for(SpecialAreaMap::const_iterator si = m_mapSpecialAreas.begin(); si!=m_mapSpecialAreas.end(); ++si)
	{
		const SpecialArea& sa = si->second;
		if(sa.GetPolygon().size()<3)
			continue;
		ColorF col(0.1f, 0.5f, 0.9f); // match editor shape colour
		if(sa.bAltered)
			col.Set(0.95f, 0.8f, 0.25f);

		ListPositions::const_iterator	itrPos=sa.GetPolygon().begin();
		Vec3 prevPoint = *itrPos;

		for(++itrPos; itrPos!=sa.GetPolygon().end(); ++itrPos)
		{
			dc->DrawLine(prevPoint + offset, col, (*itrPos) + offset, col);
			prevPoint = *itrPos;
		}
		dc->DrawLine(prevPoint + offset, col, sa.GetPolygon().front() + offset, col);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CNavigation::DebugDrawLayeredNavMesh () const
{
	m_pLayeredNavMeshRegion->DebugDraw ();
}
