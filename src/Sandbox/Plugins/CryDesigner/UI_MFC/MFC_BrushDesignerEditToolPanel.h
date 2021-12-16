////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   BrushDesignerToolPanel.h
//  Version:     v1.00
//  Created:     8/12/2011 by Jaesik.
//  Compilers:   Visual Studio 2008
//  Description: .
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "IBaseToolPanel.h"

class CBrushDesignerEditTool;
class CBaseObject;
class CBrushDesigner;
class CBaseBrush;

//#define DesignerPanelDialog CXTResizeDialog
#define DesignerPanelDialog CDialog

////////////////////////////////////////////////////////////////////////
class MFC_BrushDesignerEditToolPanel : public DesignerPanelDialog, public IBrushDesignerEditToolPanel
{
	DECLARE_DYNAMIC(MFC_BrushDesignerEditToolPanel)

public:
	MFC_BrushDesignerEditToolPanel( CWnd* pParent = NULL );
	virtual ~MFC_BrushDesignerEditToolPanel();

	void DestroyPanel() override;
	void SetEditTool( CBrushDesignerEditTool* pTool, BUtil::EDesignerMode designerMode = BUtil::eDesigner_Max ) override;
	void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
	void UpdateBackFaceCheckBox( CBrushDesigner* pDesigner ) override;
	void UpdateCloneArrayButtons() override;
	void SetButtonCheck( int nButtonID, int nCheckID ) override;
	int GetPanelIndex() override;
	void DisableButton( int nButtonID ) override;

	void PostNcDestroy(){ delete this; }

	// Dialog Data
	enum { IDD = IDD_PANEL_BRUSH_DESIGNERTOOL };


protected:
	void OnOK() {};
	void OnCancel() {};
	void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();

	void OnSize( UINT nType, int cx, int cy );

	void UpdateBackFaceFlag( CBaseObject* pBaseObject, CBaseBrush* pBrush, CBrushDesigner* pDesigner );
	void UpdateBackFaceCheckBoxFromContext();

	CBrushDesignerEditTool* m_pEditTool;
	bool m_bExclusiveModeBeforeSave;

	DECLARE_MESSAGE_MAP()

	afx_msg void OnDestroy();
	afx_msg void OnBnClickedDesignerPrimitiveBox();
	afx_msg void OnBnClickedDesignerPrimitiveSphere();
	afx_msg void OnBnClickedDesignerPrimitiveCylinder();
	afx_msg void OnBnClickedDesignerPrimitiveCone();
	afx_msg void OnBnClickedDesignerPrimitiveRectangle();
	afx_msg void OnBnClickedDesignerPrimitiveDisc();
	afx_msg void OnBnClickedDesignerDrawingLine();
	afx_msg void OnBnClickedDesignerDrawingCurve();
	afx_msg void OnBnClickedDesignerWeldTool();
	afx_msg void OnBnClickedDesignerSliceTool();
	afx_msg void OnBnClickedDesignerRemoveTool();
	afx_msg void OnBnClickedDesignerFillTool();
	afx_msg void OnBnClickedDesignerExtrudeTool();
	afx_msg void OnBnClickedDesignerOffsetTool();
	afx_msg void OnBnClickedDesignerSeparateTool();
	afx_msg void OnBnClickedDesignerMergeTool();
	afx_msg void OnBnClickedDesignerCopyTool();
	afx_msg void OnBnClickedDesignerFlipTool();
	afx_msg void OnBnClickedDesignerBevelTool();
	afx_msg void OnBnClickedDesignerStairTool();
	afx_msg void OnBnClickedDesignerStairProfileTool();
	afx_msg void OnBnClickedDesignerCloneTool();
	afx_msg void OnBnClickedDesignerArraycloneTool();
	afx_msg void OnBnClickedDesignerCirclecloneTool();
	afx_msg void OnBnClickedDesignerMirrorTool();
	afx_msg void OnBnClickedDesignerLatheTool();
	afx_msg void OnBnClickedDesignerMappingTool();
	afx_msg void OnBnClickedDesignerDebuggerTool();
	afx_msg void OnBnClickedDesignerResetXFormTool();
	afx_msg void OnBnClickedDesignerExportTool();
	afx_msg void OnBnClickedDesignerSetuppivotTool();
	afx_msg void OnBnClickedDesignerBooleanUnion();
	afx_msg void OnBnClickedDesignerBooleanSubtract();
	afx_msg void OnBnClickedDesignerBooleanIntersection();
	afx_msg void OnBnClickedDesignerDisplayBackfaces();
	afx_msg void OnBnClickedDesignerExclusivemode();
	afx_msg void OnBnClickedDesignerObjectmode();
	afx_msg void OnBnClickedDesignerBooleantool();
	afx_msg void OnBnClickedDesignerSelectVertex();
	afx_msg void OnBnClickedDesignerSelectEdge();
	afx_msg void OnBnClickedDesignerSelectFace();
	afx_msg void OnBnClickedDesignerPivot2Bottom();
	afx_msg void OnBnClickedDesignerSelectAllnone();
	afx_msg void OnBnClickedDesignerSelectConnected();
	afx_msg void OnBnClickedDesignerSelectGrow();
	afx_msg void OnBnClickedDesignerSelectLoop();
	afx_msg void OnBnClickedDesignerSelectionRing();
	afx_msg void OnBnClickedDesignerSelectInvert();
	afx_msg void OnBnClickedDesignerSnapToGrid();
	afx_msg void OnBnClickedDesignerSmoothinggroup();
	afx_msg void OnBnClickedDesignerMagnetTool();
	afx_msg void OnBnClickedDesignerRemoveDoubles();
	afx_msg void OnBnClickedDesignerHideFace();
	afx_msg void OnBnClickedDesignerCubeeditor();
	afx_msg void OnBnClickedDesignerSubdivisionTool();
	afx_msg void OnBnClickedDesignerSeamlessedit();
};