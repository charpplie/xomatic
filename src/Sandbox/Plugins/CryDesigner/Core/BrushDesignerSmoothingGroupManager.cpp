#include "StdAfx.h"
#include "BrushDesignerSmoothingGroupManager.h"
#include "BrushDesigner.h"

bool CBrushDesignerSmoothingGroupManager::AddSmoothingGroup( int nID, DesignerSmoothingGroupPtr pSmoothingGroup )
{
	for( int i = 0, iRegionCount(pSmoothingGroup->GetRegionCount()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pSmoothingGroup->GetRegion(i);
		std::map<CBrushRegion::RegionPtr,int>::iterator ii = m_MapRegion2GropuId.find(pRegion);
		if( ii == m_MapRegion2GropuId.end() )
			continue;
		m_SmoothingGroups[ii->second]->RemoveRegion(pRegion);
		if( m_SmoothingGroups[ii->second]->GetRegionCount() == 0 )
			m_SmoothingGroups.erase(ii->second);
	}

	std::map<int,DesignerSmoothingGroupPtr>::iterator iSmoothingGroup = m_SmoothingGroups.find(nID);
	if( iSmoothingGroup != m_SmoothingGroups.end() )
	{
		for( int i = 0, iCount(pSmoothingGroup->GetRegionCount()); i < iCount; ++i )
			iSmoothingGroup->second->AddRegion(pSmoothingGroup->GetRegion(i));
	}
	else
	{
		m_SmoothingGroups[nID] = pSmoothingGroup;
	}

	for( int i = 0, iRegionCount(pSmoothingGroup->GetRegionCount()); i < iRegionCount; ++i )
		m_MapRegion2GropuId[pSmoothingGroup->GetRegion(i)] = nID;

	return true;
}

void CBrushDesignerSmoothingGroupManager::RemoveSmoothingGroup( int nID )
{
	std::map<int,DesignerSmoothingGroupPtr>::iterator iter = m_SmoothingGroups.find(nID);
	if( iter == m_SmoothingGroups.end() )
		return;
	m_SmoothingGroups.erase(nID);
}

DesignerSmoothingGroupPtr CBrushDesignerSmoothingGroupManager::GetSmoothingGroup( int nID )
{
	std::map<int,DesignerSmoothingGroupPtr>::iterator iter = m_SmoothingGroups.find(nID);
	if( iter == m_SmoothingGroups.end() )
		return NULL;
	return iter->second;
}

int CBrushDesignerSmoothingGroupManager::GetSmoothingGroupID( CBrushRegion::RegionPtr pRegion ) const
{
	std::map<CBrushRegion::RegionPtr,int>::const_iterator ii = m_MapRegion2GropuId.find(pRegion);
	if( ii == m_MapRegion2GropuId.end() )
		return -1;
	return ii->second;
}

int CBrushDesignerSmoothingGroupManager::GetSmoothingGroupID( DesignerSmoothingGroupPtr pSmoothingGroup ) const
{
	std::map<int,DesignerSmoothingGroupPtr>::const_iterator ii = m_SmoothingGroups.begin();

	for( ; ii != m_SmoothingGroups.end(); ++ii )
	{
		if( pSmoothingGroup == ii->second )
			return ii->first;
	}

	return -1;
}

void CBrushDesignerSmoothingGroupManager::Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo, CBrushDesigner* pDesigner )
{
	if( bLoading )
	{
		m_SmoothingGroups.clear();
		m_MapRegion2GropuId.clear();

		int nCount = xmlNode->getChildCount();
		for( int i = 0; i < nCount; ++i )
		{
			XmlNodeRef pSmoothingGroupNode = xmlNode->getChild(i);
			int nID = 0;
			if( pSmoothingGroupNode->getAttr("ID",nID) )
			{
				int nCount = 0;
				std::vector<CBrushRegion::RegionPtr> regions;
				while(1)
				{
					CString attrStr;
					attrStr.Format("Region%d",nCount++);
					GUID guid;
					if( !pSmoothingGroupNode->getAttr(attrStr,guid) )
						break;
					CBrushRegion::RegionPtr pRegion = pDesigner->QueryRegion(guid);
					DESIGNER_ASSERT(pRegion);
					if( !pRegion )
						continue;
					regions.push_back(pRegion);
				}
				AddSmoothingGroup( nID, new CBrushDesignerSmoothingGroup(regions) );
			}
		}
	}
	else
	{
		std::map<int,DesignerSmoothingGroupPtr>::iterator ii = m_SmoothingGroups.begin();
		for( ; ii != m_SmoothingGroups.end(); ++ii )
		{
			DesignerSmoothingGroupPtr pSmoothingGroupPtr = ii->second;
			if( pSmoothingGroupPtr->GetRegionCount() == 0 )
				continue;
			XmlNodeRef pSmoothingGroupNode(xmlNode->newChild("SmoothingGroup"));
			pSmoothingGroupNode->setAttr("ID",ii->first);
			for( int i = 0, iRegionCount(pSmoothingGroupPtr->GetRegionCount()); i < iRegionCount; ++i )
			{
				CBrushRegion::RegionPtr pRegion = pSmoothingGroupPtr->GetRegion(i);
				CString attrStr;
				attrStr.Format("Region%d",i);
				pSmoothingGroupNode->setAttr(attrStr,pRegion->GetGUID());
			}
		}
	}
}

void CBrushDesignerSmoothingGroupManager::Clear()
{
	m_SmoothingGroups.clear();
	m_MapRegion2GropuId.clear();
}

std::vector<DesignerSmoothingGroupPtr> CBrushDesignerSmoothingGroupManager::GetSmoothingGroupList() const
{
	std::vector<DesignerSmoothingGroupPtr> smoothingGroupList;
	std::map<int,DesignerSmoothingGroupPtr>::const_iterator ii = m_SmoothingGroups.begin();
	for( ; ii != m_SmoothingGroups.end(); ++ii )
		smoothingGroupList.push_back(ii->second);
	return smoothingGroupList;
}

void CBrushDesignerSmoothingGroupManager::RemoveRegion( CBrushRegion::RegionPtr pRegion )
{
	std::map<CBrushRegion::RegionPtr,int>::iterator ii = m_MapRegion2GropuId.find(pRegion);
	if( ii == m_MapRegion2GropuId.end() )
		return;

	int nID = ii->second;
	m_MapRegion2GropuId.erase(ii);

	std::map<int,DesignerSmoothingGroupPtr>::iterator iGroup = m_SmoothingGroups.find(nID);	
	if( iGroup == m_SmoothingGroups.end() )
	{
		DESIGNER_ASSERT(0);
		return;
	}

	DesignerSmoothingGroupPtr pSmoothingGroup = iGroup->second;
	pSmoothingGroup->RemoveRegion(pRegion);

	if( pSmoothingGroup->GetRegionCount() == 0 )
		RemoveSmoothingGroup(iGroup->first);
}

int CBrushDesignerSmoothingGroupManager::GetEmptyGroupID() const
{
	for( int i = 1; i <= BUtil::kSmoothingGroupIDNumber; ++i )
	{
		std::map<int,DesignerSmoothingGroupPtr>::const_iterator iter = m_SmoothingGroups.find(i);
		if( iter == m_SmoothingGroups.end() || iter->second->GetRegionCount() == 0 )
			return i;
	}
	return -1;
}

void CBrushDesignerSmoothingGroupManager::CopyFromDesigner( CBrushDesigner* pDesigner, const CBrushDesigner* pSourceDesigner )
{
	Clear();

	CBrushDesignerSmoothingGroupManager* pSourceSmoothingGroupMgr = pSourceDesigner->GetSmoothingGroupMgr();
	std::vector<DesignerSmoothingGroupPtr> sourceSmoothingGroupList = pSourceSmoothingGroupMgr->GetSmoothingGroupList();

	for( int i = 0, iSmoothingGroupCount(sourceSmoothingGroupList.size()); i < iSmoothingGroupCount; ++i )
	{
		int nGroupID = pSourceSmoothingGroupMgr->GetSmoothingGroupID(sourceSmoothingGroupList[i]);
		std::vector<CBrushRegion::RegionPtr> regions;
		for( int k = 0, iRegionCount(sourceSmoothingGroupList[i]->GetRegionCount()); k < iRegionCount; ++k )
		{
			CBrushRegion::RegionPtr pRegion = sourceSmoothingGroupList[i]->GetRegion(k);
			int nRegionIndex = -1;
			int nShelfID = -1;
			for( int a = 0; a < BUtil::kMaxShelfCount; ++a )
			{
				pSourceDesigner->SetShelf(a);
				nRegionIndex = pSourceDesigner->GetRegionIndex(pRegion);
				if( nRegionIndex != -1 )
				{
					nShelfID = a;
					break;
				}
			}
			if( nShelfID == -1 || nRegionIndex == -1 )
				continue;
			pDesigner->SetShelf(nShelfID);
			regions.push_back(pDesigner->GetRegion(nRegionIndex));
		}
		AddSmoothingGroup(nGroupID,new CBrushDesignerSmoothingGroup(regions));
	}
}

void CBrushDesignerSmoothingGroupManager::InvalidateAll()
{
	std::map<int,DesignerSmoothingGroupPtr>::iterator ii = m_SmoothingGroups.begin();
	for( ; ii != m_SmoothingGroups.end(); ++ii )
		ii->second->Invalidate();
}

void CBrushDesignerSmoothingGroupManager::InvalidateSmoothingGroup( CBrushRegion::RegionPtr pRegion )
{
	int nSmoothingGroupID = GetSmoothingGroupID(pRegion);
	if( nSmoothingGroupID != -1 )
	{
		DesignerSmoothingGroupPtr pSmoothingGroupPtr = GetSmoothingGroup(nSmoothingGroupID);
		pSmoothingGroupPtr->Invalidate();
	}
}