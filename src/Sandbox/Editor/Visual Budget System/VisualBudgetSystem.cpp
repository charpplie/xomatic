////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   VisualBudgetSystem.h
//  Created:     4/08/2009 by Paulo Zaffari.
//  Description: This class implements the data gathering and rendering of
//							 the visual budget system.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include <stdafx.h>
#include "VisualBudgetSystem.h"
#include "VisualBudgetTool.h"

#include "IRenderAuxGeom.h"
#include "..\Asset Browser/AssetBrowserDialog.h"
#include "..\ViewManager.h"


//////////////////////////////////////////////////////////////////////////
#include "IRenderMesh.h"
#include "IStatObj.h"
//////////////////////////////////////////////////////////////////////////

static const float BASE_CELL_SIZE = 128.0f;

//////////////////////////////////////////////////////////////////////////
CVisualBudgetSystem& CVisualBudgetSystem::GetVisualBudgetSystem()
{
	static CVisualBudgetSystem oVisualBudgetSystem;
	return oVisualBudgetSystem;
}
//////////////////////////////////////////////////////////////////////////
CVisualBudgetSystem::CVisualBudgetSystem()
{
	GetIEditor()->RegisterNotifyListener(this);

	m_fMinimumCellSize=16.0f;				// Default minimum cell size.

	SetCellSize(BASE_CELL_SIZE); // Default size of the cell.

	m_fHalfDrawingHeight=25;

	m_boIsInDetailAnalysisMode=true;

	m_boAccountTerrain=true;
	m_boAccountMiscelaneous=true;
	m_boAccountBrushes=true;
	m_boAccountVegetation=true;

	m_boAccountRoads=true;
	m_boAccountClouds=true;

	m_boAccoundEntities=true;
	m_boAccountDecals=true;

	m_displayType=1;

	m_pAssetBrowserDialog = 0;

	m_pCrySizerGeometry = gEnv->pSystem->CreateSizer();
	m_pCrySizerGeometry->SetResourceCollector(&m_ResourceCollector);
	m_pCrySizerTexture = gEnv->pSystem->CreateSizer();
	m_pCrySizerTexture->SetResourceCollector(&m_ResourceCollector);

	m_e_StreamCgfPoolSize = gEnv->pConsole->GetCVar("e_StreamCgfPoolSize");
	m_r_TexturesStreamPoolSize = gEnv->pConsole->GetCVar("r_TexturesStreamPoolSize");

	REGISTER_CVAR2("ed_budgetGradingScale", &m_budgetGradingScale, 10.0f, VF_DUMPTODISK, 
		"A scaling factor used for leveling overall grading of the visual budget system to that of the streaming system. Bigger this, then more reddish result.");

	m_resourceType = RSRC_BOTH;
	m_currentRegion.Reset();
}
//////////////////////////////////////////////////////////////////////////
CVisualBudgetSystem::~CVisualBudgetSystem()
{
	FreeData();
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::FreeData()
{
	GetIEditor()->UnregisterNotifyListener(this);
	m_pCrySizerGeometry->Release();
	m_pCrySizerTexture->Release();
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetDetailedAnalysisStatus() const
{
	return m_boIsInDetailAnalysisMode;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetDetailedAnalysisStatus(const bool bDetailedAnalysis)
{
	m_boIsInDetailAnalysisMode=bDetailedAnalysis;

	if (m_boIsInDetailAnalysisMode)
	{
		GetIEditor()->SetEditTool(new CVisualBudgetMode(),true);
	}
	else
	{
		if (GetIEditor()->GetEditTool()->IsKindOf(RUNTIME_CLASS(CVisualBudgetMode)))
		{
			GetIEditor()->SetEditTool(NULL,true);
		}		
		ClearDetailedAnalysisRegions();
	}
}

//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetCellSize(float fCellSize)
{
	m_fCellSize=fCellSize;

	if (m_fCellSize<m_fMinimumCellSize)
	{
		m_fCellSize=m_fMinimumCellSize;
	}
}

//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountTerrainStatus(const bool bAccountTerrain)
{
	m_boAccountTerrain=bAccountTerrain;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetAccountMiscelaneousStatus() const
{
	return m_boAccountMiscelaneous;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountMiscelaneousStatus(const bool bAccountMiscelaneous)
{
	m_boAccountMiscelaneous=bAccountMiscelaneous;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetAccountBrushStatus() const
{
	return m_boAccountBrushes;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountBrushStatus(const bool bAccountBrush)
{
	m_boAccountBrushes=bAccountBrush;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetAccountVegetationStatus() const
{
	return m_boAccountVegetation;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountVegetationStatus(const bool bAccountVegetation)
{
	m_boAccountVegetation=bAccountVegetation;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetAccountCloudsStatus() const
{
	return m_boAccountClouds;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountCloudsStatus(const bool bAccountClouds)
{
	m_boAccountClouds=bAccountClouds;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetAccountRoadsStatus() const
{
	return m_boAccountRoads;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountRoadsStatus(const bool bAccountRoads)
{
	m_boAccountRoads=bAccountRoads;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetAccountEntitiesStatus() const
{
	return m_boAccoundEntities;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountEntitiesStatus(const bool bAccountEntities)
{
	m_boAccoundEntities=bAccountEntities;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetAccountDecalsStatus() const
{
	return m_boAccountDecals;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetAccountDecalsStatus(const bool bAccountDecals)
{
	m_boAccountDecals=bAccountDecals;
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::ClearDetailedAnalysisRegions()
{
	m_detailedAnalysisRegions.clear();
	m_detailedAnalysisObjects.clear();
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::AddDetailedAnalysisRegion(const AABB& aabb)
{
	for(int i=0; i<m_detailedAnalysisRegions.size(); ++i)
	{
		if (IsEquivalent(aabb,m_detailedAnalysisRegions[i],0.0f))
			return;
	}
	m_detailedAnalysisRegions.push_back(aabb);
	FindObjectsInRegions();
	SortObjectsByResourceUsage(m_resourceType);
}
//////////////////////////////////////////////////////////////////////////
const AABBList& CVisualBudgetSystem::GetDetailedAnalysisRegions() const
{
	return m_detailedAnalysisRegions;
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::SetCurrentRegion(const AABB& aabb)
{
	m_currentRegion = aabb;
}
//////////////////////////////////////////////////////////////////////////
bool	CVisualBudgetSystem::GetSectorBoundingBoxFromPoint(const Vec3& DetailedAnalysisPoint,AABB& stBoundingBox)
{
	size_t nCurrentAABB(0);
	size_t nTotalNumberOfAABB(0);

	nTotalNumberOfAABB=m_cstQueryBoundingBoxes.size();
	for (nCurrentAABB=0;nCurrentAABB<nTotalNumberOfAABB;++nCurrentAABB)
	{
		AABB& rCurrentBoundingBox(m_cstQueryBoundingBoxes[nCurrentAABB]);
		if (rCurrentBoundingBox.IsContainPoint(DetailedAnalysisPoint))
		{
			stBoundingBox=rCurrentBoundingBox;
			return true;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::GetResourceMemoryUsage(ICrySizer * pSizerGeometry,ICrySizer * pSizerTexture,const AABB& cstAABB)
{
	//////////////////////////////////////////////////////////////////////////
	std::vector<IRenderNode*>	cFoundRenderNodes;
	unsigned int							nFoundObjects(0);
	I3DEngine									*piEngine(gEnv->p3DEngine);
	ITerrain									*piTerrain(piEngine->GetITerrain());

	nFoundObjects=piEngine->GetObjectsInBox(cstAABB);

	cFoundRenderNodes.resize(nFoundObjects,NULL);
	piEngine->GetObjectsInBox(cstAABB,&cFoundRenderNodes.front());

	size_t nCurrentRenderNode(0);
	size_t nTotalNumberOfRenderNodes(0);

	nTotalNumberOfRenderNodes=cFoundRenderNodes.size();
	for (nCurrentRenderNode=0;nCurrentRenderNode<nTotalNumberOfRenderNodes;++nCurrentRenderNode)
	{
		IRenderNode*& piRenderNode=cFoundRenderNodes[nCurrentRenderNode];

		GetResourceUsageOfRenderNode(piRenderNode, pSizerGeometry, pSizerTexture);
	}
	//////////////////////////////////////////////////////////////////////////


	if (m_boAccountTerrain)
	{
		if (piTerrain)
		{
			piTerrain->GetResourceMemoryUsage(pSizerTexture,cstAABB);
		}
	}

	//if (m_pTerrain)
	//{
	//	CTerrainNode	*poTerrainNode=m_pTerrain->FindMinNodeContainingBox(cstAABB);
	//	if (poTerrainNode)
	//	{
	//		poTerrainNode->GetResourceMemoryUsage(pSizer,cstAABB);
	//	}
	//}


	//{
	//	SIZER_COMPONENT_NAME(pSizer, "Particles");
	//	if(m_pPartManager)
	//		m_pPartManager->GetMemoryUsage(pSizer);
	//}

	//pSizer->AddContainer(m_lstDynLights);
	//pSizer->AddContainer(m_lstDynLightsNoLight);
	//pSizer->AddContainer(m_lstStaticLights);
	//pSizer->AddContainer(m_lstAffectingLightsCombinations);
	//pSizer->AddContainer(m_arrLightProjFrustums);
	//pSizer->AddContainer(m_arrEntsInFoliage);

	//pSizer->AddContainer(m_lstVoxelObjectsForUpdate);
	//pSizer->AddContainer(m_lstRoadRenderNodesForUpdate);

	//pSizer->AddContainer(m_lstKilledVegetations);
	//pSizer->AddContainer(arrFPSforSaveLevelStats);
	//pSizer->AddContainer(m_lstAlwaysVisible);
	//pSizer->AddObject(m_pCVars, sizeof(CVars));

	//if(m_pDecalManager)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "DecalManager");
	//	m_pDecalManager->GetMemoryUsage(pSizer);
	//	pSizer->AddObject(m_pDecalManager, sizeof(*m_pDecalManager));
	//}

	//if(m_pObjectsTree)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "OutdoorObjectsTree");
	//	m_pObjectsTree->GetMemoryUsage(pSizer);
	//}

	//if(m_pObjManager)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "ObjManager");
	//	m_pObjManager->GetMemoryUsage(pSizer);
	//}

	//if (m_pTerrain)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "Terrain");
	//	m_pTerrain->GetMemoryUsage(pSizer);
	//}

	//if (m_pVisAreaManager)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "VisAreas");
	//	m_pVisAreaManager->GetMemoryUsage(pSizer);
	//}

	//if(m_pCoverageBuffer)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "ObjManager");
	//	pSizer->AddObject(m_pCoverageBuffer, sizeof(*m_pCoverageBuffer));
	//}

	//if(m_pCoverageBuffer)
	//	m_pCoverageBuffer->GetMemoryUsage(pSizer);

	//{
	//	SIZER_COMPONENT_NAME(pSizer, "RNTmpDataPool");

	//	CRNTmpData * pNext = NULL;

	//	for(CRNTmpData * pElem = m_LTPRootFree.pNext; pElem!=&m_LTPRootFree; pElem = pNext)
	//	{
	//		pSizer->AddObject(pElem, sizeof(*pElem));
	//		pNext = pElem->pNext;
	//	}

	//	for(CRNTmpData * pElem = m_LTPRootUsed.pNext; pElem!=&m_LTPRootUsed; pElem = pNext)
	//	{
	//		pSizer->AddObject(pElem, sizeof(*pElem));
	//		pNext = pElem->pNext;
	//	}


	//	//    pSizer->AddObject(&m_RNTmpDataPools, m_RNTmpDataPools.GetMemUsage());
	//}

	//if(CProcVegetPoolMan * pMan = CTerrainNode::GetProcObjPoolMan())
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "ProcVegetPool");

	//	if (pMan)
	//		pMan->GetMemoryUsage(pSizer);

	//	SProcObjChunkPool * pPool = CTerrainNode::GetProcObjChunkPool();
	//	if (pPool)
	//		pPool->GetMemoryUsage(pSizer);
	//}

	//if(m_arrBaseTextureData.Count())
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "LayersBaseTextureData");

	//	for(int i=0; i<m_arrBaseTextureData.Count(); i++)
	//		pSizer->AddObject(&m_arrBaseTextureData[i], m_arrBaseTextureData[i].GetMemoryUsage());
	//}

	//if(m_pVoxTerrain)
	//	m_pVoxTerrain->GetMemoryUsage(pSizer);
}


//////////////////////////////////////////////////////////////////////////
CResourceCollector * CVisualBudgetSystem::CalculateMapResourceMemoryUsage(const AABBList& AABBs)
{
	m_pCrySizerGeometry->Reset();
	m_pCrySizerTexture->Reset();
	for (size_t i=0; i<AABBs.size(); ++i)
		GetResourceMemoryUsage(m_pCrySizerGeometry, m_pCrySizerTexture, AABBs[i]);
	return &m_ResourceCollector;
}


//////////////////////////////////////////////////////////////////////////
bool CVisualBudgetSystem::CalculateMapResourceMemoryUsage()
{
	int nTerrainSize = gEnv->p3DEngine->GetTerrainSize();

	m_cstDrawingBoundingBoxes.resize(0);
	m_cstQueryBoundingBoxes.resize(0);
	m_cnUsedCellMemory.resize(0);


	AABB box;
	float s = m_fCellSize;
	for(float y=0; y<=nTerrainSize; y+=s)
	for(float x=0; x<=nTerrainSize; x+=s)
	{
		box.Reset();
		box.Add(Vec3(x  , y  , GetIEditor()->GetTerrainElevation(x  , y  )));
		box.Add(Vec3(x+s, y  , GetIEditor()->GetTerrainElevation(x+s, y  )));
		box.Add(Vec3(x  , y+s, GetIEditor()->GetTerrainElevation(x  , y+s)));
		box.Add(Vec3(x+s, y+s, GetIEditor()->GetTerrainElevation(x+s, y+s)));
		m_cstDrawingBoundingBoxes.push_back(box);

		box.min.z=-32000.0f;
		box.max.z= 32000.0f;
		m_cstQueryBoundingBoxes.push_back(box);

		//////////////////////////////////////////////////////////////////////////
		m_pCrySizerGeometry->Reset();
		m_pCrySizerTexture->Reset();
		GetResourceMemoryUsage(m_pCrySizerGeometry,m_pCrySizerTexture,box);
		m_cnUsedCellMemory.push_back(
			std::make_pair(m_pCrySizerGeometry->GetTotalSize(), 
										m_pCrySizerTexture->GetTotalSize())
			);
		//////////////////////////////////////////////////////////////////////////
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnCloseScene:    // Send when the document is about to close.
	case eNotify_OnBeginSceneOpen:// Sent when document is about to be opened.
	case eNotify_OnBeginNewScene: // Sent when the document is begin to be cleared.
		{
			m_boPauseRendering=true;
		}
		break;

	case eNotify_OnEndSceneOpen: // Sent after document have been opened.
	case eNotify_OnEndNewScene:  // Sent after the document have been cleared.
		{
			m_boPauseRendering=false;
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
static bool CheckVirtualKey( int virtualKey )
{
	GetAsyncKeyState(virtualKey);
	if (GetAsyncKeyState(virtualKey) & BIT(15))
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::Render(DisplayContext& rstDisplayContext)
{
	if (m_boPauseRendering)
	{
		return;
	}

	IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
	const CCamera & camera = gEnv->p3DEngine->GetCurrentCamera();

	size_t	nTotalSize(0);
	int			nFrameSize(0);

	static size_t nMax(0);

	
	nMax=m_cstDrawingBoundingBoxes.size();

	float geometryBudget = float(m_e_StreamCgfPoolSize->GetIVal() * 1024 * 1024);
	float textureBudget = float(m_r_TexturesStreamPoolSize->GetIVal() * 1024 * 1024);

	for (size_t nCurrentAABB=0; nCurrentAABB<nMax; ++nCurrentAABB)
	{
		AABB& stDrawingBoundingBox(m_cstDrawingBoundingBoxes[nCurrentAABB]);

		if (!camera.IsAABBVisible_F(stDrawingBoundingBox))
		{
			continue;
		}

		float fRedness, fRednessGeometry, fRednessTexture;
		size_t usage;
		float budgetGradingScale = m_budgetGradingScale * (BASE_CELL_SIZE/m_fCellSize) * (BASE_CELL_SIZE/m_fCellSize);
		fRednessGeometry=(float)m_cnUsedCellMemory[nCurrentAABB].first*m_budgetGradingScale/geometryBudget;
		fRednessTexture=(float)m_cnUsedCellMemory[nCurrentAABB].second*m_budgetGradingScale/textureBudget;
	
		ColorB oColor(0,0,0,192);
		if (GetMeasuredResourceType() == RSRC_BOTH)
		{
			fRedness = std::max(fRednessGeometry, fRednessTexture);
			usage = m_cnUsedCellMemory[nCurrentAABB].first + m_cnUsedCellMemory[nCurrentAABB].second;
		}
		else if (GetMeasuredResourceType() == RSRC_GEOMETRY)
		{
			fRedness = fRednessGeometry;
			usage = m_cnUsedCellMemory[nCurrentAABB].first;
		}
		else //if (GetMeasuredResourceType() == RSRC_TEXTURE)
		{
			fRedness = fRednessTexture;
			usage = m_cnUsedCellMemory[nCurrentAABB].second;
		}

		fRedness=MIN(1,fRedness);
		oColor.r=fRedness*255;
		oColor.g=192*(1-fRedness);
		oColor.b=0;

		if (m_boIsInDetailAnalysisMode)
		{
			if (IsEquivalent(m_currentRegion,m_cstQueryBoundingBoxes[nCurrentAABB],0.0f))
			{
				// For the cursor region, use a purple color.
				oColor.b=255;
				oColor.r=255;
				oColor.g=0;
			}
			else
			{
				bool bDetailedAnalysisRegion = false;
				for (int k=0; k<m_detailedAnalysisRegions.size(); ++k)
				{
					if (IsEquivalent(m_cstQueryBoundingBoxes[nCurrentAABB],m_detailedAnalysisRegions[k],0.0f))
					{
						bDetailedAnalysisRegion = true;
						break;
					}
				}
				if (bDetailedAnalysisRegion)
				{
					// For analyzed regions, use a blue color.
					oColor.b=255;
					oColor.r=0;
					oColor.g=0;
				}
			}

		}
		//////////////////////////////////////////////////////////////////////////

		SAuxGeomRenderFlags				sOriginalRenderFlags=pAuxGeom->GetRenderFlags();
		SAuxGeomRenderFlags				sNormalRenderFlags(e_Def3DPublicRenderflags);

		sNormalRenderFlags.SetAlphaBlendMode(e_AlphaBlended);
		sNormalRenderFlags.SetCullMode(e_CullModeNone);

		pAuxGeom->SetRenderFlags(sNormalRenderFlags);

		if (m_displayType==1)
		{
			pAuxGeom->DrawAABB(stDrawingBoundingBox,true,oColor,eBBD_Faceted);
		}
		else
		{  
			RenderTerrainSquare(rstDisplayContext,stDrawingBoundingBox.min.x,stDrawingBoundingBox.min.y,
													stDrawingBoundingBox.max.x,stDrawingBoundingBox.max.y,
													oColor);
		}

		pAuxGeom->SetRenderFlags(sOriginalRenderFlags);

		if (CheckVirtualKey(VK_SHIFT))
		{
			CString label;
			label.Format("%.3f", float(usage)/float(1024*1024));	// In MBytes
			rstDisplayContext.SetColor(1.0f, 1.0f, 1.0f);
			rstDisplayContext.DrawTextLabel(stDrawingBoundingBox.GetCenter(), 1, label.GetString());
		}

		nFrameSize+=nTotalSize;
		//////////////////////////////////////////////////////////////////////////
	}

	RenderResourceUsagePerObject(rstDisplayContext);

	//string strFrameTotal;
	//strFrameTotal.Format("Frame total: %u\n",nFrameSize);
	//OutputDebugString(strFrameTotal.c_str());
}

//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::RenderTerrainSquare(DisplayContext& dc, float x1,float y1,float x2,float y2,
																							 ColorB& rstColor)
{
	I3DEngine*				m_engine(gEnv->p3DEngine);
	IRenderer*				m_renderer(gEnv->pRenderer);
	IRenderAuxGeom*		pAuxGeom(gEnv->pRenderer->GetIRenderAuxGeom());

	if (!m_engine)
		return;

	float x,y;

	float step = MAX( y2-y1,x2-x1 );
	if (step < 0.1)
		return;

	step = step / 100.0f;

	float offset = 0.01f;

	Vec3 p1,p2;

	for (y = y1; y < y2+step; y += step)
	{
		p1.x = x1;
		p1.y = y;
		p1.z = m_engine->GetTerrainElevation( p1.x,p1.y ) + offset;

		p2.x = x1;
		p2.y = y+step;
		p2.z = m_engine->GetTerrainElevation( p2.x,p2.y ) + offset;
		pAuxGeom->DrawLine(p1,rstColor,p2,rstColor);

		p1.x = x2+step;
		p1.y = y;
		p1.z = m_engine->GetTerrainElevation( p1.x,p1.y ) + offset;

		p2.x = x2+step;
		p2.y = y+step;
		p2.z = m_engine->GetTerrainElevation( p2.x,p2.y ) + offset;
		pAuxGeom->DrawLine(p1,rstColor,p2,rstColor);
	}
	for (x = x1; x < x2+step; x += step)
	{
		p1.x = x;
		p1.y = y1;
		p1.z = m_engine->GetTerrainElevation( p1.x,p1.y ) + offset;

		p2.x = x+step;
		p2.y = y1;
		p2.z = m_engine->GetTerrainElevation( p2.x,p2.y ) + offset;
		pAuxGeom->DrawLine(p1,rstColor,p2,rstColor);

		p1.x = x;
		p1.y = y2+step;
		p1.z = m_engine->GetTerrainElevation( p1.x,p1.y ) + offset;

		p2.x = x+step;
		p2.y = y2+step;
		p2.z = m_engine->GetTerrainElevation( p2.x,p2.y ) + offset;
		pAuxGeom->DrawLine(p1,rstColor,p2,rstColor);
	}
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::Update()
{
	CalculateMapResourceMemoryUsage();
}

//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystem::SetMeasuredResourceType(ResourceType resourceType) 
{ 
	if (m_resourceType != resourceType)
	{
		m_resourceType = resourceType; 
		SortObjectsByResourceUsage(m_resourceType);
	}
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::FindObjectsInRegions()
{
	m_detailedAnalysisObjects.clear();
	if (m_detailedAnalysisRegions.empty())
		return;

	CBaseObjectsCache *pDisplayedViewObjects 
		= GetIEditor()->GetViewManager()->GetGameViewport()->GetVisibleObjectsCache();

	int numObjects = pDisplayedViewObjects->GetObjectCount();
	for (int i=0; i<numObjects; ++i)
	{
		CBaseObject *pObject = pDisplayedViewObjects->GetObject(i);
		
		AABB aabb;
		pObject->GetBoundBox(aabb);
		bool bInRegions = false;
		for (int k=0; k<m_detailedAnalysisRegions.size(); ++k)
		{
			if (aabb.IsIntersectBox(m_detailedAnalysisRegions[k]))
			{
				bInRegions = true;
				break;
			}
		}
		if (bInRegions)
		{
			m_detailedAnalysisObjects.push_back(std::make_pair(pObject, -1.0f));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::SortObjectsByResourceUsage(ResourceType resourceType)
{
	// Update usage values for all objects.
	for (int i=0; i<m_detailedAnalysisObjects.size(); ++i)
	{
		CBaseObject* pObject = m_detailedAnalysisObjects[i].first;
	
		m_pCrySizerGeometry->Reset();
		m_pCrySizerTexture->Reset();
		if (pObject->GetEngineNode())
			GetResourceUsageOfRenderNode(pObject->GetEngineNode(), 
																	m_pCrySizerGeometry, m_pCrySizerTexture);
		size_t usage;
		if (GetMeasuredResourceType() == RSRC_GEOMETRY)
			usage = m_pCrySizerGeometry->GetTotalSize();
		else if (GetMeasuredResourceType() == RSRC_TEXTURE)
			usage = m_pCrySizerTexture->GetTotalSize();
		else
			usage = m_pCrySizerGeometry->GetTotalSize() + m_pCrySizerTexture->GetTotalSize();

		m_detailedAnalysisObjects[i].second = float(usage) / float(1024*1024);	// In MBytes
	}

	// Sort
	struct compare_by_resource_usage : public std::binary_function<bool, CBaseObject*, CBaseObject*>
	{
		bool operator()(const ObjectAndUsage& left, const ObjectAndUsage& right) const
		{
			return left.second > right.second; 
		}
	};

	std::sort(m_detailedAnalysisObjects.begin(), m_detailedAnalysisObjects.end(), 
						compare_by_resource_usage());
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystem::RenderResourceUsagePerObject(DisplayContext& dc)
{
	const int MAX_DRAW_COUNT = 64;
	int count = std::min(MAX_DRAW_COUNT, int(m_detailedAnalysisObjects.size()));
	for (int i=count-1; i>=0; --i)
	{
		float redness = 1.0f - float(i) / float(count);
		ColorB color;
		color.r=redness*255;
		color.g=192*(1-redness);
		color.b=0;
		m_detailedAnalysisObjects[i].first->DrawBudgetUsage(dc, RGB(color.r, color.g, color.b));

		if (CheckVirtualKey(VK_SHIFT))
		{
			dc.SetColor(RGB(color.r, color.g, color.b), 1.0f);
			CString label;
			label.Format("%s : %.3fMB", 
				m_detailedAnalysisObjects[i].first->GetName(),
				m_detailedAnalysisObjects[i].second);
			AABB aabb;
			m_detailedAnalysisObjects[i].first->GetBoundBox(aabb);
			dc.DrawTextLabel(aabb.GetCenter(), 1.0f + redness, label.GetString());
		}
	}
}

void CVisualBudgetSystem::GetResourceUsageOfRenderNode( IRenderNode *pRenderNode, ICrySizer * pSizerGeometry, ICrySizer * pSizerTexture )
{
	switch (pRenderNode->GetRenderNodeType())
	{
	case eERType_Vegetation:
		{
			if (!m_boAccountVegetation)
			{
				return;
			}
		}
		break;

	case eERType_Brush:
		{
			if (!m_boAccountBrushes)
			{
				return;
			}
		}
		break;

	case eERType_Road:
		{
			if (!m_boAccountRoads)
			{
				return;
			}
		}
		break;

	case eERType_DistanceCloud:
	case eERType_Cloud:
		{
			if (!m_boAccountClouds)
			{
				return;
			}
		}
		break;

	case eERType_RenderProxy:
		{
			if (!m_boAccoundEntities)
			{
				return;
			}
		}
		break;

	case eERType_Decal:
		{
			if (!m_boAccountDecals)
			{
				return;
			}
		}
		break;

	default:
		{
			if (!m_boAccountMiscelaneous)
			{
				return;
			}
		}
		break;
	}

	{
		IMaterial*		piMaterial(NULL);
		IMaterial*		piOverrideMaterial(NULL);
		size_t				nCount(0);
		std::vector<IRenderMesh*> meshes;
		IRenderMesh		*piMesh(NULL);

		// In case of a Brush, we should take care of the base LOD not loaded.
		if (pRenderNode->GetRenderNodeType() == eERType_Brush)
		{
			IStatObj *pStatObj = static_cast<IBrush*>(pRenderNode)->GetStatObj();
			if (pStatObj)
			{
				SRendParams dummy;
				nCount = pStatObj->FindNearesLoadedLOD(0, dummy);
			}
		}

		piMesh = pRenderNode->GetRenderMesh(nCount);
		for (;piMesh;++nCount,piMesh=pRenderNode->GetRenderMesh(nCount))
			meshes.push_back(piMesh);

		// In case of a Brush, we should take care of sub-objects also.
		if (pRenderNode->GetRenderNodeType() == eERType_Brush)
		{
			IStatObj *pStatObj = static_cast<IBrush*>(pRenderNode)->GetStatObj();
			if (pStatObj)
			{
				for (int i=0; i<pStatObj->GetSubObjectCount(); ++i)
				{
					IStatObj::SSubObject *pSubObject = pStatObj->GetSubObject(i);
					if (pSubObject->pStatObj == NULL)
						continue;
					if (pSubObject->nType != STATIC_SUB_OBJECT_MESH)
						continue;
					SRendParams dummy;
					nCount = pSubObject->pStatObj->FindNearesLoadedLOD(0, dummy);
					IStatObj *pSubLodObject = pSubObject->pStatObj->GetLodObject(nCount);
					piMesh = pSubLodObject ? pSubLodObject->GetRenderMesh() : NULL;
					while (piMesh)
					{
						meshes.push_back(piMesh);
						++nCount;
						pSubLodObject = pSubObject->pStatObj->GetLodObject(nCount);
						piMesh = pSubLodObject ? pSubLodObject->GetRenderMesh() : NULL;
					}
				}
			}
		}

		for (int i=0; i<meshes.size(); ++i)
		{
			piMesh = meshes[i];
			size_t usage = piMesh->GetMemoryUsage(pSizerGeometry,IRenderMesh::MEM_USAGE_ONLY_SYSTEM);
			pSizerGeometry->GetResourceCollector().AddResource(piMesh->GetSourceName(),usage);

			piMaterial=piMesh->GetMaterial();
			if (piMaterial)
			{
				piMaterial->GetResourceMemoryUsage(pSizerTexture);
			}
		}

		piOverrideMaterial=pRenderNode->GetMaterialOverride();
		if (piOverrideMaterial)
		{
			piOverrideMaterial->GetResourceMemoryUsage(pSizerTexture);
		}

		piMaterial=pRenderNode->GetMaterial();
		if (piMaterial&&(piMaterial!=piOverrideMaterial))
		{
			piMaterial->GetResourceMemoryUsage(pSizerTexture);
		}
	}
}