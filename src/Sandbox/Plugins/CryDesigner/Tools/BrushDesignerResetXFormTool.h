#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerResetXFormTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerResetXFormTool : public CBrushDesignerBaseTool
{
public:

	void BeginEditParams() override;
	void EndEditParams() override;

	void FreezeXForm(  int nResetFlag );
	static void FreezeXForm( CBrushDesigner* pDesigner, CBaseBrush* pBrush, CBaseObject* pObj, int nResetFlag );

};
