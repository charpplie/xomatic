/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#ifndef __MATERIALLODGENDIALOG_H__
#define __MATERIALLODGENDIALOG_H__

#include "LODUtilities.h"

class CLodGeneratorFilePanel;
class CPropertiesPanel;

class CMaterialLODGeneratorOptionsPanel : public CDialog
{
public:
	CMaterialLODGeneratorOptionsPanel(CWnd* pParent = NULL);
	virtual ~CMaterialLODGeneratorOptionsPanel();

	enum { IDD = IDD_PANEL_MAT_LOD_GEN_OPTIONS };

	const static char * kPanelCaption;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void Update();

private:
	CPropertiesPanel*		m_pVarPanel;
	CToolTipCtrl*			m_pToolTip;
};

class CMaterialLODGeneratorLodItemOptionsPanel : public CDialog
{
public:
	CMaterialLODGeneratorLodItemOptionsPanel(int nLodId, int nSubmatId, bool bAllowControl, CWnd* pParent = NULL);
	virtual ~CMaterialLODGeneratorLodItemOptionsPanel();

	enum { IDD = IDD_PANEL_MAT_LOD_GEN_LOD_ITEM_OPTIONS };

	const static char * kPanelCaption;

protected:
	DECLARE_MESSAGE_MAP()

	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollbar);
	afx_msg void OnEnabledChanged();

	bool IsBakingEnabled();
	int Width();
	int Height();
	int LodId();
	int SubMatId();

	void UpdateSizeControl(bool bUsrMsg);
	void SetTextures();
	void SetTextureSize(int nWidth, int nHeight);
	void Serialize(XmlNodeRef xml, bool load);
	void Reset();

protected:
	void SetTexture(ITexture* pTex, int type);

private:

	int m_nLodId;
	int m_nSubMaterialId;
	int m_bEnabled;
	int m_bAllowControl;

	CStatic m_cagePreviewDummy;
	CMeshBakerPopupPreview * m_pCagePreview;

	CMeshBakerTextureCtrl m_colour;
	CMeshBakerTextureCtrl m_normal;
	CMeshBakerTextureCtrl m_spec;

	CToolTipCtrl*			m_pToolTip;
};


class CMaterialLODGeneratorTaskPanel : public CDialog
{
public:
	CMaterialLODGeneratorTaskPanel(CWnd * pParent = NULL);
	virtual ~CMaterialLODGeneratorTaskPanel();

	enum { IDD = IDD_PANEL_MAT_LOG_GEN_TASK };

	const static char * kPanelCaption;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonGenerate();

	void Reset();

private:

	CToolTipCtrl*			m_pToolTip;
};

#endif