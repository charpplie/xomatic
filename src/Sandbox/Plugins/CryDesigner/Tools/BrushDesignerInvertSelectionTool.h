#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerInvertSelectionTool.h
//  Created:     Feb/28/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerInvertSelectionTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	static void InvertSelection( BUtil::SMainContext& mc );
};
