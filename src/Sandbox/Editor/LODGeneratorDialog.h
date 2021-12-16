/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#ifndef __LODGENERATORDIALOG_H__
#define __LODGENERATORDIALOG_H__

#pragma once

#include "IEditor.h"
#include "Dialogs/BaseFrameWnd.h"
#include "Util/LODGenerator.h"
#include "LODUtilities.h"
#include "MaterialLODGeneratorDialog.h"
#include <smartptr.h>

class CLodGeneratorFilePanel;
class CGeometryLodGeneratorOptionsPanel;
class CGeometryLodGeneratorTaskPanel;
class CGeometryLodGeneratorPreviewPanel;
class CMaterialLODGeneratorLodItemOptionsPanel;
class CMaterialLODGeneratorTaskPanel;
class CMaterial;


//////////////////////////////////////////////////////////////////////////
// dialog
//////////////////////////////////////////////////////////////////////////

class CGeometryLodGeneratorDialog : public CBaseFrameWnd
{
	DECLARE_DYNCREATE(CGeometryLodGeneratorDialog)

public:
	CGeometryLodGeneratorDialog();
	virtual ~CGeometryLodGeneratorDialog();

	static void RegisterViewClass();

	// Dialog Data
	enum { IDD = IDD_LOD_GENERATOR_DIALOG };
		
protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();

public:

	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnFileOpened(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGenerateLodChain(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCancel(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGenerateLods(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLodRemoved(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLodChainGenerationFinished(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGenerateMaterial(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTextureSizeChanged(WPARAM nWidth, LPARAM nHeight);

	void OnGeometryVarBlockChanged(IVariable* var);
	void OnSave();
	bool OnMaterialGeneratePrepare();
	void ClearLodPanels();
	void GenerateLodPanels();

	void Reset(bool bUpdateOptionPanels);
	
private:

	CRollupCtrl m_oRollupControl;

	CLodGeneratorFilePanel*	m_pFilePanel;
	CGeometryLodGeneratorOptionsPanel* m_pOptionsPanel;
	CGeometryLodGeneratorTaskPanel* m_pTaskPanel;
	CGeometryLodGeneratorPreviewPanel* m_pGeoGenPanel;

	CMaterialLODGeneratorOptionsPanel* m_pMatOptionsPanel;
	std::vector<int> m_vLodPanelsIdx;
	std::vector<CMaterialLODGeneratorLodItemOptionsPanel*> m_vLodPanels;
	CMaterialLODGeneratorTaskPanel*	m_pMatTaskPanel;

	//////////////////////////////////////////////////////////////////////////
	class CGeometryLodGeneratorDialogViewClass : public TRefCountBase<IViewPaneClass>
	{
		//////////////////////////////////////////////////////////////////////////
		// IClassDesc
		//////////////////////////////////////////////////////////////////////////
		virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; }
		virtual REFGUID ClassID()
		{
			// {F6DB7988-A8B4-4cb7-ADAB-23B0E32EC2C5}
			static const GUID guid = { 0xf6db7988, 0xa8b4, 0x4cb7, { 0xad, 0xab, 0x23, 0xb0, 0xe3, 0x2e, 0xc2, 0xc5 } };
			return guid;
		}
		virtual const char* ClassName() { return "LOD Generator"; }
		virtual const char* Category() { return "LOD Tools"; }
		//////////////////////////////////////////////////////////////////////////
		virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CGeometryLodGeneratorDialog); }
		virtual const char* GetPaneTitle() { return _T("LOD Generator"); }
		virtual EDockingDirection GetDockingDirection() { return DOCK_RIGHT; }
		virtual CRect GetPaneRect() { return CRect(100,100,515,800); }
		virtual bool SinglePane() { return false; }
		virtual bool WantIdleUpdate() { return true; }
	};
};



//////////////////////////////////////////////////////////////////////////
// properties panel
//////////////////////////////////////////////////////////////////////////

class CPropertiesPanel;

class CGeometryLodGeneratorOptionsPanel : public CDialog
{
public:
	CGeometryLodGeneratorOptionsPanel(CWnd* pParent = NULL);
	virtual ~CGeometryLodGeneratorOptionsPanel();

	enum { IDD = IDD_PANEL_GEOM_LOD_GEN_OPTIONS };

	const static char * kPanelCaption;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void Update();
	
protected:
	void WireFrameToggles(IVariable * var);
	void SourceLodChanged(IVariable * var);
	void PreviewSourceLod(IVariable * var);

private:
	CPropertiesPanel* m_pVarPanel;
	CToolTipCtrl* m_pToolTip;
};

//////////////////////////////////////////////////////////////////////////
// task panel
//////////////////////////////////////////////////////////////////////////

class CGeometryLodGeneratorTaskPanel : public CDialog
{
public:
	CGeometryLodGeneratorTaskPanel(CWnd* pParent = NULL);
	virtual ~CGeometryLodGeneratorTaskPanel();

	enum { IDD = IDD_PANEL_GEOM_LOD_GEN_TASK };
	
	const static char * kPanelCaption;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedLodGenGenerate();
	afx_msg void OnBnClickedLodGenCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	void TaskStarted();
	void TaskFinished();
	void Reset();

private:
	CTime m_tStartTime;
	UINT_PTR m_nTimer;
	CToolTipCtrl* m_pToolTip;
};

//////////////////////////////////////////////////////////////////////////
// preview window
//////////////////////////////////////////////////////////////////////////

class CGeometryLodGeneratorPreviewPanel : public CDialog
{
public:
	CGeometryLodGeneratorPreviewPanel(CWnd* pParent = NULL);
	virtual ~CGeometryLodGeneratorPreviewPanel();

	enum { IDD = IDD_PANEL_GEOM_LOD_PREVIEW };

	const static char * kPanelCaption;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGenerateLods();

	afx_msg LRESULT OnLodValueChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLodDeleted(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLodAdded(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLodSelected(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSelectionCleared(WPARAM wParam, LPARAM lParam);
	
	void Reset(bool release);
	void CreateLod(float fPercentage);
	void SelectFirst();
	void SetNumLods(int nSourceLod);
	void CreateExistingLodKeys();
	
	void SetWireframe(bool bWireframe);
	void PreviewSource(bool bDisplay, int nSourceLod);

	void EnableExport();
	
	CLODGeneratorErrorGraphRamp* GetRamp() { return m_pRamp; }

private:
	CMeshBakerPopupPreview* m_pPreview;
	CMeshBakerPopupPreview* m_pPreviewPopup;
	CLODGeneratorErrorGraphRamp* m_pRamp;
	CToolTipCtrl* m_pToolTip;
};

#endif // __LODGENERATORDIALOG_H__

