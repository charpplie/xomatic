#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSmoothingGroup.h
//  Created:     July/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushRegion.h"

class CBrushDesigner;

class CBrushDesignerSmoothingGroup : public CRefCountBase
{
public:

	CBrushDesignerSmoothingGroup( const std::vector<CBrushRegion::RegionPtr>& regions );

	void SetRegions( const std::vector<CBrushRegion::RegionPtr>& regions );
	void AddRegion( CBrushRegion::RegionPtr pRegion );
	bool HasRegion( CBrushRegion::RegionPtr pRegion ) const;

	int GetRegionCount() const;
	CBrushRegion::RegionPtr GetRegion( int nIndex ) const;
	void RemoveRegion( CBrushRegion::RegionPtr pRegion );
	void Invalidate() { m_bValidMeshInfo[0] = m_bValidMeshInfo [1]= false; }

	const BUtil::SMeshInfo& GetMeshInfo( bool bGenerateBackFaces = false )
	{ 
		if( !bGenerateBackFaces && !m_bValidMeshInfo[0] || bGenerateBackFaces && !m_bValidMeshInfo[1] )
		{
			if( bGenerateBackFaces ) 
				m_bValidMeshInfo[1] = true;
			else
				m_bValidMeshInfo[0] = true;
			UpdateMeshInfo(bGenerateBackFaces);
		}
		return m_MeshInfo;
	}

private:
	
	bool CalculateNormal( const BrushVec3& nPos, BrushVec3& vOutNormal ) const;	
	void UpdateMeshInfo( bool bGenerateBackFaces = false );

private:	
	
	std::unique_ptr<CBrushDesigner> m_pDesigner;
	std::set<CBrushRegion::RegionPtr> m_RegionSet;
	BUtil::SMeshInfo m_MeshInfo;
	bool m_bValidMeshInfo[2]; // 0 - Front Faces, 1 - Back Faces

};

typedef _smart_ptr<CBrushDesignerSmoothingGroup> DesignerSmoothingGroupPtr;