#ifndef __ANIMATIONGRAPHDIALOG2_H__
#define __ANIMATIONGRAPHDIALOG2_H__

#pragma once

#include "Resource.h"
#include "Controls\TemplDef.h" // message map extensions for templates
#include "ToolbarDialog.h"
#include "Controls\PropertyCtrl.h"

#include "FlatFrameCtrl.h"

#include "CharacterEditor\CharacterEditor.h"
#include "AnimationGraphStateEditor_2.h"
#include "AnimationGraphStateQuery_2.h"
#include "AnimationGraphTester_2.h"
#include "AnimationGraph_2.h"
#include "..\HyperGraphView.h"
#include "AnimationGraphPreviewDialog_2.h"
#include "AnimationGraphPreviewManager_2.h"
//#include "AnimationGraph2_AnimationNodeDetails.h"

#include "AnimationGraph2_DefaultsDialog.h"

class CAnimationGraphDialog2;

class CAGHyperGraphView2: public CHyperGraphView
{
protected:
	virtual void ShowContextMenu( CPoint point, CHyperNode* pNode );

	//////////////////////////////////////////////////////////////////////////
	virtual void UpdateTooltip( CHyperNode* pNode, CHyperNodePort* pPort ) {}
};


class CAnimationGraph2;
typedef _smart_ptr<CAnimationGraph2> CAnimationGraph2Ptr;

class CAnimationGraphListCtrl2 : public CTreeCtrl
{
public:
	CAnimationGraphListCtrl2( CAnimationGraphDialog2 * pParent ) : m_pParent(pParent) {}

	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	void ReloadStates();
	void ReloadViews();
	void ReloadInputs();
	void ReloadModifiers();
	void SelectItemByName( const CString& name );
	typedef std::pair< CString, int > TNameIcon;

private:
	CAnimationGraphDialog2 * m_pParent;
	void AddList( const CString& root, std::vector< TNameIcon >& values );

	CImageList m_imageList;
};

class CAnimationGraphStateParamCtrl2 : public CDialog, public _reference_target_t
{
	DECLARE_DYNAMIC( CAnimationGraphStateParamCtrl2 );
public:
	CAnimationGraphStateParamCtrl2( const char* name ): m_Name(name) {}
	~CAnimationGraphStateParamCtrl2() {}

	virtual BOOL DestroyWindow();
	void AdjustHeight();

	enum { IDD = IDD_PANEL_AG_PARAM };

	CString m_Name;
	CString m_SelectedValue;
	CEdit m_ParamNameEdit;
	CListBox m_ValuesList;

private:
	int m_ItemHeight;

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnListBoxDblClk();
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos );
	afx_msg void OnParamNameKillFocus();

public:
	afx_msg void OnListBoxSelChange();

protected:
	virtual void OnOK() { OnParamNameKillFocus(); }
	virtual void OnCancel() { m_ParamNameEdit.SetWindowText( m_Name ); }
};
typedef _smart_ptr< CAnimationGraphStateParamCtrl2 > CAnimationGraphStateParamCtrl2Ptr;

class CAnimationGraphStateParamsDlg2 : public CDialog
{
	DECLARE_DYNCREATE( CAnimationGraphStateParamsDlg2 );
public:
	CAnimationGraphStateParamsDlg2() {}
	~CAnimationGraphStateParamsDlg2() {}

	enum { IDD = IDD_PANEL_AG_STATEPARAMS };

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnAddParam();
	afx_msg void OnCheckBoxChanged();

protected:
	virtual void OnOK() {}
	virtual void OnCancel() {}
};

class CAnimationGraphStateParamsPanel2 : public CDialog
{
	DECLARE_DYNCREATE( CAnimationGraphStateParamsPanel2 );

	friend class CAnimationGraphStateParamCtrl2;

public:
	CAnimationGraphStateParamsPanel2() : m_bIgnoreVScroll(false), m_pParamsDeclaration(NULL) {}
	~CAnimationGraphStateParamsPanel2() {}

	enum { IDD = IDD_PANEL_PROPERTIES };

	void Init();

	bool OnAddParam( const char* name );
	bool AddParamValue( const char* param, const char* value );
	bool DeleteParamValue( const char* param, const char* value );
	bool RenameParamValue( const char* param, const char* oldValue, const char* newValue );

	void OnSelChanged();

	void SetParamsDeclaration( CParamsDeclaration2* pParamsDcl );

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

protected:
	virtual void OnOK() {}
	virtual void OnCancel() {}

private:
	int	m_MiniPanelWidth;
	int	m_StateParamsDlgHeight;
	bool m_bIgnoreVScroll;

	CParamsDeclaration2* m_pParamsDeclaration;

	void ArrangeMiniPanels();

	bool DeleteParam( CAnimationGraphStateParamCtrl2* pParamCtrl );
	bool RenameParam( CAnimationGraphStateParamCtrl2* pParamCtrl, const CString& newName );

public:
	CAnimationGraphStateParamsDlg2 m_StateParamsDlg;

	typedef std::list< CAnimationGraphStateParamCtrl2Ptr > TListParamCtrls;
	TListParamCtrls m_ParamCtrls;

	TParameterizationId2 m_CurrentSelection;
};

class CAnimationGraph2_GenericDetails : public CDialog
{
	//DECLARE_DYNCREATE( CAnimationGraph2_GenericDetails );

public:
	CAnimationGraph2_GenericDetails() {}
	~CAnimationGraph2_GenericDetails() {}

	enum { IDD = IDD_AG2_DETAILS_PANE };

	void Init() {}

protected:
	//DECLARE_MESSAGE_MAP()
 
// 	afx_msg void OnSize(UINT nType, int cx, int cy);
// 	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

protected:
	virtual void OnOK() {}
	virtual void OnCancel() {}


};

class CAnimationGraphDialog2
	: public CXTPFrameWnd
	, public IAnimationGraphListener2
	, public IHyperGraphListener
	, ICharacterChangeListener
{
	friend class CAGHyperGraphView2;

	DECLARE_DYNCREATE(CAnimationGraphDialog2);
public:
	static void RegisterViewClass();

	CAnimationGraphDialog2();
	~CAnimationGraphDialog2();

	BOOL Create( DWORD dwStyle,const RECT& rect,CWnd* pParentWnd );

	void OnHyperGraphEvent( IHyperNode * pNode, EHyperGraphEvent event );

	void OnModifierNodeEvent( CAGNodeBase2* pBaseNode, EHyperGraphEvent event );
	void OnStateNodeEvent( EHyperGraphEvent event );
	void OnLinkEdit( CHyperEdge * pEdge );

	void OnStateEvent( EAGStateEvent2 evt, CAGState2Ptr pState );
	void OnViewEvent( EAGViewEvent2, CAGView2Ptr pView );
	void OnInputEvent( EAGInputEvent2, CAGInput2Ptr pInput );
	void OnModifierEvent( EAG2ModifierEvent evt, CAG2ModifierBasePtr pModifier );

	void OnGraphModified();
	void OnGraphIslandReport();
	void OnGraphSpeedReport();
	void OnGraphBadCALReport();
	void OnGraphBadNullNodeReport();
	void OnGraphDeadInputReport();
	void OnGraphTransitionLengthReport();
	void OnGraphMatchSpeeds();
	void OnGraphOrphanNodesReport();

	//! Menu entry Settings->Default Settings
	//! Opens the dialog window and updates the character file afterwards if needed
	void OnGraphDefaultSettingsDialog();

	//! Opens About box in the menu
	void OnGraphVersionInfo();

	// CDialog m_aboutAG;

	CAnimationGraph2Ptr GetAnimationGraph();

	void SetParamsDeclaration( CParamsDeclaration2 * pParamsDeclaration )
	{
		m_stateParamsCtrl.SetParamsDeclaration( pParamsDeclaration );
	}
	void OnStateParamSelChanged();

	bool ResetParameterization();

	bool AddParamValue( const char* param, const char* value ) { return m_stateEditor.AddParamValue( param, value ); }
	bool DeleteParamValue( const char* param, const char* value ) { return m_stateEditor.DeleteParamValue( param, value ); }
	bool RenameParamValue( const char* param, const char* oldValue, const char* newValue ) { return m_stateEditor.RenameParamValue( param, oldValue, newValue ); }

	bool AddParameter( const CString& name ) { return m_stateEditor.AddParameter( name ); }
	bool DeleteParameter( const CString& name ) { return m_stateEditor.DeleteParameter( name ); }
	bool RenameParameter( const CString& oldName, const CString& newName ) { return m_stateEditor.RenameParameter( oldName, newName ); }

	int GetExcludeFromGraph() { return m_vStates.size() != 1 ? -1 : m_vStates[0]->GetExcludeFromGraph(); }
	void SetExcludeFromGraph( bool bExclude ) { if ( m_vStates.size() == 1 ) m_vStates[0]->SetExcludeFromGraph( bExclude ); }
	const CAGView2Ptr GetSelectedView() const { return m_pView; }

	enum { IDD = IDD_TRACKVIEWDIALOG };

	CXTPDockingPaneManager* GetDockingPaneManager() { return &m_paneManager; }
/*
	CXTPDockingPane* GetNodeDetailsPane() 
	{ 
		return m_pNodeDetailsPane; 
	}
*/

	CWnd* GetNodeDetailsParent() 
	{ 
		return &m_genericNodeDetails;
	}

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnGraphAddState();
	afx_msg void OnGraphAddView();
	afx_msg void OnGraphAddInput();
	afx_msg void OnGraphAddModifier( UINT nID );
	afx_msg void OnGraphCreateAnimation();
	afx_msg void OnGraphTrial();
	afx_msg void OnAGLinkMappingChanged();
	afx_msg void OnViewPane( UINT cmdID );
	afx_msg void OnViewPaneStateQuery();
	afx_msg void OnStateListSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnViewListSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnInputListSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnModifierListSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTaskPanelNotify(WPARAM wParam, LPARAM lParam);    

	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnModifierListBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnIconSizeChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateIconSizeUI(CCmdUI* pCmdUI);
	afx_msg void OnDisplayIconsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateDisplayIconsUI(CCmdUI* pCmdUI);
	afx_msg void OnDisplayPreviewChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateDisplayPreviewUI(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos );

private:
	enum EItemType
	{
		eIT_State,
		eIT_View,
		eIT_Input,
		eIT_Root,
		eIT_Error
	};

	void SetGraph( CAnimationGraph2Ptr pGraph );
	void SetActiveView( CAGView2Ptr pView );
	void SetActiveState( CAGState2Ptr pState );
	void SetActiveStates( std::vector< CAGState2Ptr > vStates );
	void ViewReport( const CString& report );

	void UpdateTitle();
	void UpdateGraphMenu();
	void ReadDisplayedIconSize();
	void EnableIcons(bool enableIcons);
	void EnablePreview(bool enablePreview);

	void AnimationStatePopupMenu( CAGState2* pState, CWnd* wnd, CPoint &pos );
	void ModifierStatePopupMenu( CAG2ModifierBasePtr pModifier, CWnd* wnd, CPoint &pos );

	void RegisterConsoleVariables();

	// ICharacterChangeListener
	virtual void OnCharacterChanged();

	int m_nBlockSelChanged;

	CXTPDockingPaneManager m_paneManager;

	CAnimationGraph2Ptr m_pGraph;
	CAGView2Ptr m_pView;
	std::vector< CAGState2Ptr > m_vStates;
	CFlatFramedCtrl2< CAnimationGraphListCtrl2 > m_stateListCtrl;
	CFlatFramedCtrl2< CAnimationGraphListCtrl2 > m_viewListCtrl;
	CFlatFramedCtrl2< CAnimationGraphListCtrl2 > m_inputListCtrl;
	CFlatFramedCtrl2< CAnimationGraphListCtrl2 > m_modifierListCtrl;
	CAnimationGraphStateEditor2 m_stateEditor;
	CFlatFramedCtrl2< CAnimationGraphStateParamsPanel2 > m_stateParamsCtrl;
	CFlatFramedCtrl2< CAnimationGraph2_GenericDetails > m_genericNodeDetails;
//	CFlatFramedCtrl2< CAnimationGraphAnimationNodeDetails > m_AnimationNodeDetails;
	CAnimationGraphTester2 m_tester;
	CAnimationGraphStateQuery2 m_stateQuery;
	_smart_ptr<CHyperGraph> m_pViewHypergraph;
	CFlatFramedCtrl2< CAGHyperGraphView2 > m_view;
	CImageList * m_pDragImage;
	CString m_dragNodeClass;
	uint m_dragModifierId;
	CStatusBar m_statusBar;
	CAnimationImageManager2Ptr m_pCAnimationImageManager;
	CAnimationGraphPreviewDialog2 m_previewDialog;
	CAnimationGraphPreviewManager2 m_previewManager;
	CAnimationGraph2DefaultsDialog m_defaultsDialog;
	CRollupCtrl m_rollupCtrl;
	CXTPToolBar m_wndToolBar;

	// Pane for the various node details to attach their interfaces to display upon
	CXTPDockingPane* m_pNodeDetailsPane;

	typedef std::pair<int, int> IconSize;
	typedef std::vector<IconSize> IconSizeList;
	IconSizeList m_iconSizes;

	ICVar* m_displayedIconSize;
	ICVar* m_enableIcons;
	ICVar* m_preview;
};

#endif
