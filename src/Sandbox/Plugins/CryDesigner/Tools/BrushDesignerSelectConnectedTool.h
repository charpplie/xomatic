#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSelectConnectedTool.h
//  Created:     Feb/11/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSelectGrowTool.h"

class CBrushDesignerSelectConnectedTool : public CBrushDesignerSelectGrowTool
{
public:
	void Enter() override;

	static void SelectConnectedRegions( BUtil::SMainContext& mc );
	
};