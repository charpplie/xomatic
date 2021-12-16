////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MannequinDialog_h__
#define __MannequinDialog_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SequencerSplitter.h"
#include "SequencerDopeSheet.h"

#include "Dialogs\BaseFrameWnd.h"

#include "PropertiesPanel.h"

#include "MannKeyPropertiesDlgFE.h"
#include "MannequinNodes.h"
#include "MannequinBase.h"
#include "MannErrorReportDialog.h"

#include "Controls/FragmentBrowser.h"
#include "Controls/SequenceBrowser.h"
#include "Controls/TransitionBrowser.h"
#include "Controls/FragmentEditorPage.h"
#include "Controls/TransitionEditorPage.h"
#include "Controls/PreviewerPage.h"
#include "FragmentSplitter.h"

class CTagSelectionControl;
class CFragmentTagDefFileSelectionControl;
class CSequencerSplitter;
class CMannequinFileChangeWriter;

class CMannequinDialog : public CBaseFrameWnd, public IEditorNotifyListener
{
	DECLARE_DYNCREATE(CMannequinDialog)

public:

	enum EMannequinDialogPanes
	{
		IDW_FRAGMENT_EDITOR_PANE = AFX_IDW_CONTROLBAR_FIRST+103,
		IDW_PREVIEWER_PANE,
		IDW_TRANSITION_EDITOR_PANE,
		IDW_FRAGMENTS_PANE,
		IDW_SEQUENCES_PANE,
		IDW_TRANSITIONS_PANE,
		IDW_ERROR_REPORT_PANE,
	};

	CMannequinDialog(CWnd* pParent = NULL);
	virtual ~CMannequinDialog();

	enum { IDD = IDD_SEQUENCERDIALOG };

	static void RegisterViewClass();

	static CMannequinDialog * GetCurrentInstance() { return s_pMannequinDialog; }

	SMannequinContexts* Contexts() { return &m_contexts; }
	CFragmentEditorPage* FragmentEditor() { return &m_wndFragmentEditorPage; }
	CTransitionEditorPage* TransitionEditor() { return &m_wndTransitionEditorPage; }
	CPreviewerPage* Previewer() { return &m_wndPreviewerPage; }
	CFragmentBrowser* FragmentBrowser() { return m_wndFragmentBrowser; }
	CTransitionBrowser* TransitionBrowser() { return m_wndTransitionBrowser; }

	bool PreviewFileLoaded() const { return m_bPreviewFileLoaded; }

	void Update();

	void OnLayerUpdate( int event,CObjectLayer *pLayer );

	void LoadNewPreviewFile(const char *previewFile);

	bool EnableContextData(const EMannequinEditorMode editorMode, const int scopeContext);
	void ClearContextData(const EMannequinEditorMode editorMode, const int scopeContextID);

	void EditFragment(FragmentID fragID, const SFragTagState &tagState, int contextID = -1);
	void EditFragmentByKey(const struct CFragmentKey &key, int contextID = -1);
	void StopEditingFragment();

	void FindFragmentReferences(const CString &fragmentName);
	void FindTagReferences(const CString &tagName);
	void FindClipReferences(const struct CClipKey &key);

	void EditTransitionByKey(const struct CFragmentKey &key, int contextID = -1);

	bool CheckChangedData();
	
	void OnMoveLocator(EMannequinEditorMode editorMode, uint32 refID, const char *locatorName, const QuatT &transform);

	void SetKeyProperty( const char* propertyName, const char* propertyValue );

	void SetTagState(const TagState &tagState);
	void UpdateTagList(FragmentID fragID = FRAGMENT_ID_INVALID);
	void SetTagStateOnCtrl(const SFragTagState &tagState);

	void ResavePreviewFile();

protected:
	DECLARE_MESSAGE_MAP()

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	virtual bool CanClose();

	virtual BOOL OnInitDialog();

	bool SavePreviewFile(const char *filename);
	bool LoadPreviewFile(const char *filename, XmlNodeRef &xmlSequenceNode);

	void EnableContextEditor(bool enable);

	bool InitialiseToPreviewFile(const char *previewFile);

	void PopulateTagList(const CTagDefinition *tagDef = NULL);
	SFragTagState GetTagStateFromCtrl() const;
	void OnInternalVariableChange( IVariable *pVar );

	void SaveLayouts();
	void LoadLayouts();

	void SetCursorPosText( float fTime );

public:
	void Validate();
	void Validate(const SScopeContextData &contextDef);

	void OnRender();
	void LaunchTagDefinitionEditor() { OnTagDefinitionEditor(); }

private:
	afx_msg void OnDestroy();

	afx_msg void OnMenuLoadPreviewFile();
	afx_msg void OnContexteditor();
	afx_msg void OnAnimDBEditor();
	afx_msg void OnTagDefinitionEditor();
	afx_msg void OnSave();
	afx_msg void OnReexportAll();
	afx_msg void OnNewSequence();
	afx_msg void OnLoadSequence();
	afx_msg void OnSaveSequence();
	afx_msg void OnViewFragmentEditor();
	afx_msg void OnViewPreviewer();
	afx_msg void OnViewTransitionEditor();
	afx_msg void OnViewErrorReport();

	afx_msg void OnUpdateNewSequence(CCmdUI *pCmdUI);
	afx_msg void OnUpdateLoadSequence(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSaveSequence(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewFragmentEditor(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewPreviewer(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewTransitionEditor(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewErrorReport(CCmdUI *pCmdUI);

	afx_msg void OnToolsListUsedAnimations();
	afx_msg void OnToolsListUsedAnimationsForCurrentPreview();

	void EnableMenuCommand(uint32 commandId, bool enable);

	void OnFragmentBrowserSelectedItemEdit( FragmentID fragmentId, CString &tags );
	void OnFragmentBrowserScopeContextChanged();
	void UpdateAnimationDBEditorMenuItemEnabledState();

	void LoadMannequinFolder();

	void ActivateErrorReport();

	void ClearContextEntities();
	void ClearContextViewData();
	void ReloadContextEntities();

	void UpdateForFragment(FragmentID fragmentID, CString &tags);

public:
	static const int s_minPanelSize;

private:
	CFragmentEditorPage										m_wndFragmentEditorPage;
	CTransitionEditorPage									m_wndTransitionEditorPage;
	CPreviewerPage												m_wndPreviewerPage;

	CPropertiesPanel											*m_wndTagsPanel;
	CFont																	m_wndTagsPanelFont;
	CFragmentSplitter											m_wndFragmentSplitter;
	CFragmentBrowser											*m_wndFragmentBrowser;
	CSequenceBrowser											*m_wndSequenceBrowser;
	CTransitionBrowser										*m_wndTransitionBrowser;
	CRollupCtrl														m_wndFragmentsRollup;
	CMannErrorReportDialog								m_wndErrorReport;

	SMannequinContexts m_contexts;
	bool m_bPreviewFileLoaded;

	std::auto_ptr<CMannequinFileChangeWriter> m_pFileChangeWriter;

	static CMannequinDialog *s_pMannequinDialog;
	const CTagDefinition *m_lastFragTagDef;
	bool m_bRefreshingTagCtrl;
	TSmartPtr<CVarBlock> m_tagVars;
	CTagControl m_tagControls;
	CTagControl m_fragTagControls;

	XmlNodeRef m_historyBackup;
};

enum {
	BITMAP_ERROR=0,
	BITMAP_WARNING=1,
	BITMAP_COMMENT=2,

	COLUMN_MAIL_ICON=0,
	COLUMN_CHECK_ICON=2,

	ID_REPORT_CONTROL=100
};

const CString MANNEQUIN_FOLDER = "Animations/Mannequin/ADB/";
const CString TAGS_FILENAME_ENDING = "tags";
const CString TAGS_FILENAME_EXTENSION = ".xml";

#endif // __MannequinDialog_h__
