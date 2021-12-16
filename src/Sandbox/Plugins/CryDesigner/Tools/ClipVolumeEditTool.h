#pragma once

#include "BrushDesignerEditTool.h"

class CClipVolumeEditTool : public CBrushDesignerEditTool
{
public:

	DECLARE_DYNCREATE(CClipVolumeEditTool)

	static void RegisterTool( CRegistrationContext &rc );

	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;
	void OnManipulatorMouseEvent( CViewport *view, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo ) override;

	void BeginEditParams( IEditor *ie,int flags ) override;
	void EndEditParams() override;

	void SetUserData( const char *key,void *userData ) override;
	void Finish() override;

	int GetPanelIndex() const override;
	
	IBrushDesignerEditToolPanel* GetDesignerToolPanel() const override;

private: 	
	static IBrushDesignerEditToolPanel* s_pMenuPanel;
};