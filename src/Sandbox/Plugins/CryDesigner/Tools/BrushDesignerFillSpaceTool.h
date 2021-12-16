#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerFillSpaceTool.h
//  Created:     July/28/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerFillSpaceTool : public CBrushDesignerBaseTool
{
public:

	void Enter() override;
	void Leave() override;

	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;

	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

private:

	void CompileHoles();

	bool FillHoleBasedOnSelectedElements();

	static bool ContainRegion( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& regionList );
	static CBrushRegion::RegionPtr QueryAdjacentRegion( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& regionList );

	_smart_ptr<CBrushDesigner> m_pHoleContainer;
	CBrushRegion::RegionPtr m_PickedHoleRegion;

};