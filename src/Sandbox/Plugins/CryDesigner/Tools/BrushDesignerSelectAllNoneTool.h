#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSelectAllNoneTool.h
//  Created:     Feb/11/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesigner;

class CBrushDesignerSelectAllNoneTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;

	static void SelectAllVertices( CBaseObject* pObject, CBrushDesigner* pDesigner );
	static void SelectAllEdges( CBaseObject* pObject, CBrushDesigner* pDesigner );
	static void SelectAllFaces( CBaseObject* pObject, CBrushDesigner* pDesigner );

	static void DeselectAllVertices();
	static void DeselectAllEdges();
	static void DeselectAllFaces();
};
