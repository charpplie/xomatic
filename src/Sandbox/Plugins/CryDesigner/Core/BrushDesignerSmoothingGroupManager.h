#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSmoothingGroupManager.h
//  Created:     July/7/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSmoothingGroup.h"

class CBrushDesigner;

class CBrushDesignerSmoothingGroupManager : public CRefCountBase
{
public:

	void Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo, CBrushDesigner* pDesigner );

	bool AddSmoothingGroup( int nID, DesignerSmoothingGroupPtr pSmoothingGroup );
	void RemoveSmoothingGroup( int nID );
	DesignerSmoothingGroupPtr GetSmoothingGroup( int nID );
	int GetSmoothingGroupID( CBrushRegion::RegionPtr pRegion ) const;
	int GetSmoothingGroupID( DesignerSmoothingGroupPtr pSmoothingGroup ) const;

	void InvalidateSmoothingGroup( CBrushRegion::RegionPtr pRegion );
	void InvalidateAll();

	std::vector<DesignerSmoothingGroupPtr> GetSmoothingGroupList() const;

	void Clear();
	void RemoveRegion( CBrushRegion::RegionPtr pRegion );

	int GetEmptyGroupID() const;

	void CopyFromDesigner( CBrushDesigner* pDesigner, const CBrushDesigner* pSourceDesigner );

private:

	std::map<int,DesignerSmoothingGroupPtr> m_SmoothingGroups;
	std::map<CBrushRegion::RegionPtr,int> m_MapRegion2GropuId;
};