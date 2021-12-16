#pragma once

////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerMirrorTool.h
//  Created:     Sep/12/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSliceTool.h"

class CBrushDesignerMirrorTool : public CBrushDesignerSliceTool
{
public:

	void OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo ) override;
	void Display( DisplayContext &dc ) override;

	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void ApplyMirror();
	void FreezeDesigner();

	void UpdateGizmo() override;

	static void ReleaseMirrorMode( CBrushDesigner* pDesigner );
	static void RemoveEdgesOnMirrorPlane( CBrushDesigner* pDesigner );

private:

	bool UpdateManipulatorInMirrorMode( const BrushMatrix34& offsetTM ) override;

};