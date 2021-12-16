#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerLoopSelectionTool.h
//  Created:     Feb/13/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerLoopSelectionTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	static void LoopSelection( BUtil::SMainContext& mc );

private:

	static bool SelectLoop( BUtil::SMainContext& mc, const BrushEdge3D& initialEdge );
	static bool SelectBorderInOneRegion( BUtil::SMainContext& mc, const BrushEdge3D& edge );
	static bool SelectBorder( BUtil::SMainContext& mc, const BrushEdge3D& edge, CBrushDesignerElementManager& outElementInfos );
	static int GetRegionCountSharingEdge( BUtil::SMainContext& mc, const BrushEdge3D& edge, const BrushPlane* pPlane = NULL );
	static int CountAllEdges( BUtil::SMainContext& mc );	
};