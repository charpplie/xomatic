#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerBooleanTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerBooleanTool : public CBrushDesignerBaseTool
{

public:

	void Enter() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void BooleanOperation( BUtil::EBooleanOperationEnum booleanType );
};
