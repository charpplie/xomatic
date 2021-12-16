#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerObjectModeTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CObjectMode;

class CBrushDesignerObjectModeTool : public CBrushDesignerBaseTool
{
public:

	CBrushDesignerObjectModeTool() : m_pObjectMode(NULL), m_bSelectedAnother(false)
	{
	}
	virtual ~CBrushDesignerObjectModeTool(){}

	void Enter() override;
	void Leave() override;

	void OnLButtonDown( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnLButtonUp( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnLButtonDblClk( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnRButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnRButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view, UINT nFlags, CPoint point ) override;
	bool OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags ) override;
	void Display( DisplayContext &dc ) override;
	void OnManipulatorDrag( CViewport *view,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value ) override;

	void OnEditorNotifyEvent( EEditorNotifyEvent event );

	bool EnabledSeamlessSelection() const override { return false; }

private:

	CObjectMode* m_pObjectMode;
	bool m_bSelectedAnother;

};