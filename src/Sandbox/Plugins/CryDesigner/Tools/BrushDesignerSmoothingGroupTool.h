#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSmoothingGroupTool.h
//  Created:     June/27/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSelectTool.h"

class CBrushDesignerSmoothingGroupTool : public CBrushDesignerSelectTool
{
public:

	CBrushDesignerSmoothingGroupTool() : CBrushDesignerSelectTool(BUtil::ePF_Face) {}

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;

	void SetSmoothingGroup( int nSmoothingGroupID );

	void RemoveRegionsFromSmoothingGroups();
	void ApplyAutoSmooth( int nAngle );
	void SelectRegionsInSmoothingGroup( int nID );
	void ClearSelectedElements();

private:

	void HideNumbersFromSelectElements();

};