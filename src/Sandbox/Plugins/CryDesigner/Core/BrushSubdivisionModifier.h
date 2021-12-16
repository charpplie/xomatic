#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushSubdivisionModifier.h
//  Created:     Sep/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerHalfEdgeMesh.h"

namespace BUtil
{
	struct SSubdivisionContext
	{
		_smart_ptr<CBrushDesignerHalfEdgeMesh> fullPatches;
		_smart_ptr<CBrushDesignerHalfEdgeMesh> transitionPatches;
	};
};

class CBrushSubdivisionModifier : public CRefCountBase
{
public:		

	BUtil::SSubdivisionContext CreateSubdividedMesh( CBrushDesigner* pDesigner, int nSubdivisionLevel, int nTessFactor );

private:

	BUtil::SSubdivisionContext CreateSubdividedMesh( BUtil::SSubdivisionContext& sc );

	struct SPos
	{
		enum EVertexType
		{
			eVT_Valid = BIT(0),
			eVT_Corner = BIT(1)
		};
		SPos( int _pos_index, char _flag = eVT_Valid ) : pos_index(_pos_index), flag(_flag) {} 
		int pos_index;
		char flag;
		bool IsValid() const { return flag & eVT_Valid; }
		bool IsCorner() const { return flag & eVT_Corner; }
	};
	
	void CalculateNextLocations( 
		CBrushDesignerHalfEdgeMesh* pHalfEdgeMesh,
		std::vector<HE_Position>& outNextPosList,		
		std::vector<SPos>& outNextFaceLocations, 
		std::vector<SPos>& outNextEdgeLocations, 
		std::vector<SPos>& outNextVertexLocations );

};