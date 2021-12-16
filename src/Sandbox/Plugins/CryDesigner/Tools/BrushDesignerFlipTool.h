#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerFlipTool.h
//  Created:     Jan/29/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSelectTool.h"

class CBrushDesignerFlipTool : public CBrushDesignerBaseTool
{
public:

	void Enter() override;
	void Leave() override;

	static void FlipRegions( BUtil::SMainContext& mc, CBrushDesignerElementManager& outFlipedElements );

private:

	void FlipRegions();

	CBrushDesignerElementManager m_FlipedSelectedElements;
};