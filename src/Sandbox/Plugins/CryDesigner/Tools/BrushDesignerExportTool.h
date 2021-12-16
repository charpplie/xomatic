#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerExportTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerExportTool : public CBrushDesignerBaseTool
{
public:

	void BeginEditParams() override;
	void EndEditParams() override;

	void ExportToCgf();
	void ExportToGrp();
	void ExportToObj();
};
