////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   paneldisplaylayer.h
//  Version:     v1.00
//  Created:     9/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __paneldisplaylayer_h__
#define __paneldisplaylayer_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include <XTToolkitPro.h>
#include "Controls\LayersListBox.h"
#include "ToolbarDialog.h"

class CObjectLayer;
class CObjectLayerManager;
class CPanelDisplayLayer;


class CLayerToolbar : public CDlgToolBar
{
public:
	CLayerToolbar() : CDlgToolBar(), m_pParent(0) {}
	void SetPanelDisplayLayer( CPanelDisplayLayer* pParent ) { m_pParent = pParent; }

protected:
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

public:
	CPanelDisplayLayer* m_pParent;
};


class CPanelDisplayLayer : public CToolbarDialog
{
	DECLARE_DYNAMIC(CPanelDisplayLayer)

public:
	CPanelDisplayLayer(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPanelDisplayLayer();

	void ShowContextMenu( CMenu* pMenu = 0 );

// Dialog Data
	enum { IDD = IDD_PANEL_DISPLAY_LAYERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//! Callback called when layer is updated.
	void OnLayerUpdate( int event,CObjectLayer *pLayer );
	void SelectLayer( CObjectLayer *pLayer );

	void ReloadLayers();
	void OnLayersUpdate();
	CString GetSelectedLayer();

	DECLARE_MESSAGE_MAP()
private:
	// ! ListBox of layers.
	CXTTreeCtrl m_treeCtrl;
	CLayerToolbar m_toolbar;
	CColorCtrl<CLayersListBox> m_layersCtrl;

	HTREEITEM AddLayer( CObjectLayer *pLayer );
	void SetItemState( HTREEITEM hItem );
	void UpdateLayerItem( CObjectLayer *pLayer );
	void ChangeControlSize();

	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL OnInitDialog();
	afx_msg void OnSelChanged();
	afx_msg void OnLayersRButtonUp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedNew();
	afx_msg void OnBnClickedDelete();
	afx_msg void OnBnClickedExport();
	afx_msg void OnBnClickedImport();
	afx_msg void OnBnClickedRename();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	bool m_bLayersValid;

	typedef std::map<CObjectLayer*,HTREEITEM> LayerToItemMap;
	LayerToItemMap m_layerToItemMap;

	CObjectLayer *m_currentLayer;
	CObjectLayerManager *m_pLayerManager;
	bool m_bIgnoreSelectItem;

	HTREEITEM m_hPrevSelected;

	int m_nPrevHeight;
};

#endif // __paneldisplaylayer_h__