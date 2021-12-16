#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerRemoveTool.h
//  Created:     May/5/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"
#include "Core/BrushDesignerSpotManager.h"
#include "Core/BrushDesigner.h"

class CBrushDesignerRemoveTool : public CBrushDesignerBaseTool, public CBrushDesignerSpotManager
{
public:

	void Enter();

	static bool RemoveSelectedElements( BUtil::SMainContext& mc, bool bEraseMirrored );

private:

	bool RemoveSelectedElements();
};
