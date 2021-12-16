#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerMergeTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerMergeTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	static void MergeRegions( BUtil::SMainContext& mc );

private:
	void MergeObjects();
	void MergeRegions();
};
