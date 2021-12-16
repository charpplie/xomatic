#include "StdAfx.h"
#include "BrushDesignerSmoothingGroup.h"
#include "BrushDesigner.h"
#include "BrushDesignerDB.h"

CBrushDesignerSmoothingGroup::CBrushDesignerSmoothingGroup( const std::vector<CBrushRegion::RegionPtr>& regions ) : m_pDesigner(new CBrushDesigner)
{
	Invalidate();
	m_pDesigner->AddRef();	
	SetRegions(regions);
}

void CBrushDesignerSmoothingGroup::SetRegions( const std::vector<CBrushRegion::RegionPtr>& regions )
{
	m_pDesigner->Clear();

	for( int i = 0, iRegionCount(regions.size()); i < iRegionCount; ++i )
	{
		m_RegionSet.insert(regions[i]);
		m_pDesigner->AddRegion(regions[i],CBrushDesigner::eOpType_Add);
	}

	Invalidate();
}

void CBrushDesignerSmoothingGroup::AddRegion( CBrushRegion::RegionPtr pRegion )
{
	if( m_RegionSet.find(pRegion) != m_RegionSet.end() )
		return;
	m_RegionSet.insert(pRegion);
	m_pDesigner->AddRegion(pRegion,CBrushDesigner::eOpType_Add);
	Invalidate();
}

bool CBrushDesignerSmoothingGroup::HasRegion( CBrushRegion::RegionPtr pRegion ) const
{
	return m_RegionSet.find(pRegion) != m_RegionSet.end();
}

bool CBrushDesignerSmoothingGroup::CalculateNormal( const BrushVec3& vPos, BrushVec3& vOutNormal ) const
{
	BrushVec3 vNormal(0,0,0);
	CBrushDesignerDB::QueryResult qResult;
	if( m_pDesigner->GetDB()->QueryAsVertex(vPos,qResult) && qResult.size() == 1 && !qResult[0].m_MarkList.empty() )
	{
		for( int i = 0, iCount(qResult[0].m_MarkList.size()); i < iCount; ++i )
			vNormal += qResult[0].m_MarkList[i].m_pRegion->GetPlane().Normal();
		vOutNormal = vNormal.GetNormalized();
		return true;
	}
	return false;
}

int CBrushDesignerSmoothingGroup::GetRegionCount() const
{
	return m_pDesigner->GetRegionSize();
}

CBrushRegion::RegionPtr CBrushDesignerSmoothingGroup::GetRegion( int nIndex ) const
{ 
	return m_pDesigner->GetRegion(nIndex);
}

void CBrushDesignerSmoothingGroup::UpdateMeshInfo( bool bGenerateBackFaces )
{
	m_pDesigner->ResetDB(BUtil::eDBRF_Vertex);
	m_MeshInfo.Clear();

	for( int i = 0, iRegionCount(GetRegionCount()); i < iRegionCount; ++i )
	{
		int nVertexOffset = m_MeshInfo.vertexList.size();
		if( GetRegion(i)->CheckFlags(CBrushRegion::eRF_Hidden) )
			continue;
		BUtil::CreateMeshFacesFromRegion(GetRegion(i),m_MeshInfo,bGenerateBackFaces);
		for( int k = nVertexOffset, iVertexCount(m_MeshInfo.vertexList.size()); k < iVertexCount; ++k )
			CalculateNormal(m_MeshInfo.vertexList[k],m_MeshInfo.normalList[k]);
	}
}

void CBrushDesignerSmoothingGroup::RemoveRegion( CBrushRegion::RegionPtr pRegion )
{
	m_RegionSet.erase(pRegion);
	m_pDesigner->RemoveRegion(pRegion);
	Invalidate();
}
