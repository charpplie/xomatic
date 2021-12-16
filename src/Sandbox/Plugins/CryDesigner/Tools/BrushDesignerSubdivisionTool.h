#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSubdivision.h
//  Created:     Sep/3/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSelectTool.h"

class CBrushDesignerSubdivisionTool : public CBrushDesignerBaseTool
{
public:

	CBrushDesignerSubdivisionTool() 
	{
	}

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void HighlightEdgeGroup( const char* edgeGroupName );
	void AddNewEdgeTag();
	void DeleteEdgeTag( const char* name );
	void InvalideSelectedEdges() { m_SelectedEdgesAsEnter.clear(); }

	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override {}

	static void Subdivide( int nLevel, bool bUpdateBrush );

	void Display( DisplayContext &dc ) override;

private:

	std::vector<BrushEdge3D> m_HighlightedSharpEdges;
	std::vector<BrushEdge3D> m_SelectedEdgesAsEnter;
};