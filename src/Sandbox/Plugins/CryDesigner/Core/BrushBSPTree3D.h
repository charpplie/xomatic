////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   BrushTree3D.h
//  Version:     v1.00
//  Created:     8/17/2011 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History: 
////////////////////////////////////////////////////////////////////////////

#ifndef _BSPTREE3DTREE_H_
#define _BSPTREE3DTREE_H_

#include "BrushDesigner.h"

class CBrushBSPTree3DNode;

class CBrushBSPTree3D : public CRefCountBase
{
public:

	typedef std::vector<CBrushRegion::RegionPtr> RegionList;

	CBrushBSPTree3D( RegionList& regionList );
	~CBrushBSPTree3D();

	struct SOutputRegions
	{
		RegionList posList;
		RegionList negList;
		RegionList coSameList;
		RegionList coDiffList;
	};

	void GetPartitions( CBrushRegion::RegionPtr& pRegion, SOutputRegions& outRegions ) const;
	bool IsInside( const BrushVec3& vPos ) const;
	bool IsValidTree() const { return m_bValidTree; }

private:

	CBrushBSPTree3DNode* m_pRootNode;
	bool m_bValidTree;
	CBrushBSPTree3DNode* BuildBSP( RegionList& regionList );

};

#endif //_BSPTREE3DTREE_H_