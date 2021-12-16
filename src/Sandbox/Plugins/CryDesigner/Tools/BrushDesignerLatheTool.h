#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerLatheTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerLatheTool : public CBrushDesignerBaseTool
{
public:

	void Enter() override;
	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;

private:

	enum ELatheErrorCode
	{
		eLEC_Success,
		eLEC_NoPath,
		eLEC_InappropriateProfileShape,
		eLEC_ProfileShapeTooBig,
	};

	ELatheErrorCode CreateShapeAlongPath( CBrushRegion::RegionPtr pInitProfileRegion );

	std::vector<BrushPlane> CreatePlanesAtEachPointOfPath( const std::vector<BrushVec3>& vPath, bool bPathClosed );
	std::vector<BrushVec3> ExtractPathFromSelectedElements( bool& bOutClosed );
	bool GlueRegions( const std::vector<CBrushRegion::RegionPtr>& regions );
	
	void AddRegionToDesigner( CBrushDesigner* pDesigner, const std::vector<BrushVec3>& vList, CBrushRegion::RegionPtr pInitRegion, bool bFlip );

private:

	CBrushRegion::RegionPtr m_pPathRegion;

};