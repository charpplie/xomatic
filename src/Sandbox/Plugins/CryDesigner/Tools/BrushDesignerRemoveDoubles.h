#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerRemoveDoubles.h
//  Created:     July/21/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerWeldTool.h"

class CBrushDesignerRemoveDoublesTool : public CBrushDesignerWeldTool
{
public:

	void Enter() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	static void RemoveDoubles( BUtil::SMainContext& mc, float fDistance );
};