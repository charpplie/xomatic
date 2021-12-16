#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerHideFace.h
//  Created:     July/21/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerHideFaceTool : public CBrushDesignerBaseTool 
{
public:
	void Enter() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void UnhideAll();
};