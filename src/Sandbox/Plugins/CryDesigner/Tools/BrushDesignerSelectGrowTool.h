#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSelectGrowTool.h
//  Created:     Feb/11/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerElementManager;

class CBrushDesignerSelectGrowTool : public CBrushDesignerBaseTool
{
public:
	virtual void Enter() override;

	static void GrowSelection( BUtil::SMainContext& mc );

protected:

	static void SelectAdjacentRegionsFromEdgeVertex( BUtil::SMainContext& mc, std::set<CBrushRegion::RegionPtr>& selectedSet, bool bAddNewSelections );
	static bool SelectAdjacentRegions( BUtil::SMainContext& mc, std::set<CBrushRegion::RegionPtr>& selectedSet, bool bAddNewSelections );
	static std::set<CBrushRegion::RegionPtr> MakeInitialSelectedSet( BUtil::SMainContext& mc );
};