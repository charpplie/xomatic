#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerCopyTool.h
//  Created:     Feb/6/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerElementManager;

class CBrushDesignerCopyTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	static void Copy( BUtil::SMainContext& mc, CBrushDesignerElementManager* pOutCopiedElements );
};
