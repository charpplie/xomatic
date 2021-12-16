#include "StdAfx.h"
#include "BrushDesignerSmoothingGroupTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushDesignerSmoothingGroupManager.h"
#include "IBaseToolPanel.h"

namespace 
{
	ISmoothingGroupToolPanel* g_pDesignerSmoothingGroupToolPanel = NULL;
}

void CBrushDesignerSmoothingGroupTool::HideNumbersFromSelectElements()
{
	g_pDesignerSmoothingGroupToolPanel->ShowAllNumbers();
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int  i = 0, iElementCount(pSelected->GetSize()); i < iElementCount; ++i )
	{
		int nGroupID = GetDesigner()->GetSmoothingGroupMgr()->GetSmoothingGroupID((*pSelected)[i].m_pRegion);
		if( nGroupID == -1 )
			continue;
		g_pDesignerSmoothingGroupToolPanel->HideNumber(nGroupID);
	}
}

void CBrushDesignerSmoothingGroupTool::Enter()
{
	__super::Enter();

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Erase(BUtil::ePF_Vertex|BUtil::ePF_Edge);

	HideNumbersFromSelectElements();
}

void CBrushDesignerSmoothingGroupTool::Leave()
{
	__super::Leave();
}

void CBrushDesignerSmoothingGroupTool::BeginEditParams()
{
	if( !g_pDesignerSmoothingGroupToolPanel )
		g_pDesignerSmoothingGroupToolPanel = CreateSmoothingGroupToolPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerSmoothingGroupTool::EndEditParams()
{
	if( g_pDesignerSmoothingGroupToolPanel )
	{
		g_pDesignerSmoothingGroupToolPanel->DestroyPanel();
		g_pDesignerSmoothingGroupToolPanel = NULL;
	}
}

void CBrushDesignerSmoothingGroupTool::SetSmoothingGroup( int nSmoothingGroupID )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
		return;	

	std::vector<CBrushRegion::RegionPtr> regions;
	for( int i = 0, iElementCount(pSelected->GetSize()) ; i < iElementCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() || !(*pSelected)[i].m_pRegion )
			continue;
		regions.push_back((*pSelected)[i].m_pRegion);
	}		

	CBrushDesignerSmoothingGroupManager* pSmoothingGroupMgr = GetDesigner()->GetSmoothingGroupMgr();
	pSmoothingGroupMgr->AddSmoothingGroup(nSmoothingGroupID, new CBrushDesignerSmoothingGroup(regions));

	Sync();
	UpdateBrush();
}

void CBrushDesignerSmoothingGroupTool::RemoveRegionsFromSmoothingGroups()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	CBrushDesignerSmoothingGroupManager* pSmoothingGroupMgr = GetDesigner()->GetSmoothingGroupMgr();
	for( int i = 0, iElementCount(pSelected->GetSize()); i < iElementCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() || !(*pSelected)[i].m_pRegion )
			continue;
		pSmoothingGroupMgr->RemoveRegion((*pSelected)[i].m_pRegion);
	}
	Sync();
	UpdateBrush();
}

bool IsLessThanAngleOfSeedRegions( CBrushRegion::RegionPtr pRegion, const std::set<CBrushRegion::RegionPtr>& seedRegions, BrushFloat fRadian )
{	
	std::set<CBrushRegion::RegionPtr>::const_iterator ii = seedRegions.begin();

	for( ; ii != seedRegions.end(); ++ii )
	{
	 	BrushFloat dot = (*ii)->GetPlane().Normal().Dot(pRegion->GetPlane().Normal());
		if( std::acos(dot) <= fRadian )
			return true;
	}
	return false;
}

bool IsAdjacentWithSeedRegions( CBrushRegion::RegionPtr pRegion, const std::set<CBrushRegion::RegionPtr>& seedRegions )
{
	std::set<CBrushRegion::RegionPtr>::const_iterator ii = seedRegions.begin();
	for( ; ii != seedRegions.end(); ++ii )
	{
		bool bHadCommonEdge = false;
		int nEdgeCount = pRegion->GetEdgeSize();
		for( int k = 0; k < nEdgeCount; ++k )
		{
			BrushEdge3D e = pRegion->GetEdge(k);
			if( (*ii)->HasEdge(e) )
			{
				bHadCommonEdge = true;
				break;
			}
		}
		if( bHadCommonEdge )
			return true;
	}
	return false;
}

void CBrushDesignerSmoothingGroupTool::ApplyAutoSmooth( int nAngle )
{
	BrushFloat fRadian = ((BrushFloat)nAngle/(BrushFloat)180)*BUtil::PI;

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();	
	std::set<CBrushRegion::RegionPtr> usedRegions;

	CBrushDesignerSmoothingGroupManager* pSmoothingGroupMgr = GetDesigner()->GetSmoothingGroupMgr();
	int iSelectedElementCount(pSelected->GetSize());

	std::set<CBrushRegion::RegionPtr> seedRegions;
	std::vector<CBrushRegion::RegionPtr> regionsInGroup;

	while(1)
	{
		CBrushRegion::RegionPtr pStartingSeedRegion = NULL;
		if( seedRegions.empty() )
		{
			for( int i = 0; i < iSelectedElementCount; ++i )
			{
				if( usedRegions.find((*pSelected)[i].m_pRegion) == usedRegions.end() )
				{
					pStartingSeedRegion = (*pSelected)[i].m_pRegion;
					seedRegions.insert(pStartingSeedRegion);
					regionsInGroup.push_back(pStartingSeedRegion);
					usedRegions.insert(pStartingSeedRegion);
					break;
				}
			}
			if( seedRegions.empty() )
				break;
		}

		bool bFinishLoop = false;
		while( !bFinishLoop && !seedRegions.empty() )
		{
			int nOffset = regionsInGroup.size();
			for( int i = 0; i < iSelectedElementCount; ++i )
			{
				CBrushRegion::RegionPtr pRegion = (*pSelected)[i].m_pRegion;
				if( usedRegions.find(pRegion) != usedRegions.end() )
					continue;

				if( !IsLessThanAngleOfSeedRegions(pRegion, seedRegions, fRadian) )
					continue;

				if( !IsAdjacentWithSeedRegions(pRegion, seedRegions) )
					continue;

				regionsInGroup.push_back(pRegion);
				usedRegions.insert(pRegion);
			}

			seedRegions.clear();
			if( nOffset < regionsInGroup.size() )
			{	
				seedRegions.insert(regionsInGroup.begin()+nOffset,regionsInGroup.end());
			}
			else
			{
				if( regionsInGroup.empty() )
				{
					bFinishLoop = true;
					break;
				}
				for( int i = 0, iRegionCount(regionsInGroup.size()); i < iRegionCount; ++i )
					pSmoothingGroupMgr->RemoveRegion(regionsInGroup[i]);
				int nEmptyGroupID = pSmoothingGroupMgr->GetEmptyGroupID();
				if( nEmptyGroupID == -1 )
				{
					bFinishLoop = true;
					break;
				}
				pSmoothingGroupMgr->AddSmoothingGroup(nEmptyGroupID,new CBrushDesignerSmoothingGroup(regionsInGroup));
				regionsInGroup.clear();
				break;
			}
		}
		if( bFinishLoop )
			break;
	}

	Sync();
	UpdateBrush();
}

void CBrushDesignerSmoothingGroupTool::SelectRegionsInSmoothingGroup( int nID )
{
	CBrushDesignerSmoothingGroupManager* pSmoothingGroupMgr = GetDesigner()->GetSmoothingGroupMgr();
	DesignerSmoothingGroupPtr pSmoothingGroup = pSmoothingGroupMgr->GetSmoothingGroup(nID);
	if( pSmoothingGroup == NULL )
		return;	

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iRegionCount(pSmoothingGroup->GetRegionCount()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pSmoothingGroup->GetRegion(i);
		pSelected->Add(SDesignerElement(GetBaseObject(),pRegion));
	}

	UpdateSelectionMeshFromSelectedElementList(GetMainContext());
}

void CBrushDesignerSmoothingGroupTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	__super::OnLButtonUp(view,nFlags,point);
	g_pDesignerSmoothingGroupToolPanel->ClearAllSelectionsOfNumbers();
	HideNumbersFromSelectElements();
}

void CBrushDesignerSmoothingGroupTool::ClearSelectedElements()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Clear();
}