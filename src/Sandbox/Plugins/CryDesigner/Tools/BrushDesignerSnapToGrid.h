#pragma once

#include "BrushDesignerBaseTool.h"

class CBrushDesignerSnapToGridTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	BrushVec3 SnapVertexToGrid( const BrushVec3& vPos );
};