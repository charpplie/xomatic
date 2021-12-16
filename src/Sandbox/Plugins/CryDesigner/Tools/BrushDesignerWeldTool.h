#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerWeldTool.h
//  Created:     Feb/12/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerWeldTool : public CBrushDesignerBaseTool
{
public:
	virtual void Enter() override;
	static void Weld( BUtil::SMainContext& mc, const BrushVec3& vSrc, const BrushVec3& vTarget );
};
