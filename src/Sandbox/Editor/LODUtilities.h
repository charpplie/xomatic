/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#ifndef __LODUTILITIES_H__
#define __LODUTILITIES_H__

#include "Controls/PreviewModelCtrl.h"
#include "Controls/RampControl.h"

#define LOD_GENERATOR_NAME "LOD Generator"
#define LOD_GENERATOR_VER  "0.01"
#define LOD_GENERATOR_LAYOUT_SECTION _T("LODTools")

static UINT NEAR WM_GEOM_LOD_FILE_OPENED = RegisterWindowMessage("GEOMLODFILEOPENED");
static UINT NEAR WM_GEOM_LOD_CHAIN_GENERATE = RegisterWindowMessage("GEOMLODCHAINGEN");
static UINT NEAR WM_GEOM_LOD_CHAIN_CANCEL = RegisterWindowMessage("GEOMLODCHAINCANCEL");
static UINT NEAR WM_GEOM_LOD_CHAIN_GENERATION_FINISHED = RegisterWindowMessage("GEOMLODCHAINFINISHED");
static UINT NEAR WM_GEOM_LOD_GENERATE_LODS = RegisterWindowMessage("GEOMLODGENLODS");
static UINT NEAR WM_GEOM_LOD_REMOVED = RegisterWindowMessage("GEOMLODREMOVED");
static UINT NEAR WM_MAT_LOD_GENERATE = RegisterWindowMessage("MATLODGENERATE");
static UINT NEAR WM_MAT_LOD_TEXTURESIZE_CHANGED = RegisterWindowMessage("MATLODTEXSZCHNG");

//////////////////////////////////////////////////////////////////////////
// texture control
//////////////////////////////////////////////////////////////////////////

class CMeshBakerTextureCtrl : public CDialog
{
public:
	CMeshBakerTextureCtrl(bool bTooltip=true);
	virtual ~CMeshBakerTextureCtrl();
	void SetTexture(ITexture *pTex, bool bShowAlpha=false);
	ITexture* GetTexture() { return m_pCurrentTex; }
	BOOL Create( CWnd *pWndParent,const CRect &rc,DWORD dwStyle );
protected:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	void UpdateBitmap();
	_smart_ptr<ITexture> m_pCurrentTex;
	bool m_bShowAlpha;
	bool m_bHovering;
	bool m_bTooltip;
	CBitmap m_bitmap;
	CMeshBakerTextureCtrl *m_pToolTip;
};

//////////////////////////////////////////////////////////////////////////
// preview viewport
//////////////////////////////////////////////////////////////////////////

class CMeshBakerPopupPreview : public CDialog
{
	DECLARE_DYNAMIC(CMeshBakerPopupPreview)

public:
	CMeshBakerPopupPreview();
	virtual ~CMeshBakerPopupPreview(){}

protected:
	DECLARE_MESSAGE_MAP()

public:
	BOOL Create(CWnd *pWndParent,const CRect &rc,DWORD dwStyle);

	afx_msg void OnSize(UINT nType, int cx, int cy);

	void SetModel(IStatObj * pObj);
	void SetMaterial(CMaterial* pMat);
	void SetRotate(bool rotate);
	void SetWireframe(bool wireframe);
	void SetGrid(bool grid);
	void Reset();

	CPreviewModelCtrl* GetModelCtrl() { return &m_modelCtrl; } 
	
private:
	CString m_modelPath;
	CPreviewModelCtrl m_modelCtrl;
};

//////////////////////////////////////////////////////////////////////////
// error graph ramp
//////////////////////////////////////////////////////////////////////////

class CLODGeneratorErrorGraphRamp : public CRampControl
{
public:
	CLODGeneratorErrorGraphRamp();
	virtual ~CLODGeneratorErrorGraphRamp(){}

	void SetStats(IStatObj::SStatistics stats);
	void DrawBackground(CDC &dc);
	void DrawForeground(CDC &dc);
	void AddCustomMenuOptions(CMenu * menu);
	void OnMenuCustom(UINT nID);

private:

	float m_totalError;
	float m_selectedError;
	IStatObj::SStatistics m_currentStats;
};


#endif