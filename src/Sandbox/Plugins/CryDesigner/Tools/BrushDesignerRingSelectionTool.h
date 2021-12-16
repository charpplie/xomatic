#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerRingSelectionTool.h
//  Created:     Feb/13/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerRingSelectionTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	static void RingSelection( BUtil::SMainContext& mc );

private:
	static void SelectRing( BUtil::SMainContext& mc, const BrushEdge3D& inputEdge );
};
