#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSeparateTool.h
//  Created:     Feb/6/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CDesignerBrushObject;

class CBrushDesignerSeparateTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	static CDesignerBrushObject* Separate( BUtil::SMainContext& mc );
};
