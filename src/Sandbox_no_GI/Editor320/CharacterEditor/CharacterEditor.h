#pragma once

#include "Dialogs\BaseFrameWnd.h"

#include "PropertiesPanel.h"
#include "ToolbarDialog.h"
#include "Controls\AnimSequences.h"
#include "Controls\RollupCtrl.h"

#include "CharPanel_Animation.h"
#include "CharPanel_BAttach.h"
#include "CharPanel_Attachments.h"
#include "CharPanel_Morphing.h"
#include "CharPanel_AnimationControl.h"
#include "CharPanel_Character.h"
#include "AnimationBrowser.h"
#include "Include\IFrameWindowExtensionPane.h"
#include "MotionAdaptorDlg.h"

//////////////////////////////////////////////////////////////////////////
class ICharacterChangeListener
{
public:
	virtual void OnCharacterChanged() = 0;
};

class SANDBOX_API CCharacterEditor : public CBaseFrameWnd, public IEditorNotifyListener, public ICharacterChangeListener
{
	DECLARE_DYNCREATE(CCharacterEditor)

public:
	static void RegisterViewClass();

	CCharacterEditor();
	virtual ~CCharacterEditor();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnCharacterChanged();

	//////////////////////////////////////////////////////////////////////////
	// CBaseFrameWnd implementation
	//////////////////////////////////////////////////////////////////////////
	virtual LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
	//////////////////////////////////////////////////////////////////////////

  //
	string m_strCharDefPathName;
  string m_strCharPartsPathName;

	void LoadCharacter(const CString& file, float scale = 1.0f);

	afx_msg void OnFileNew();
	afx_msg void OnFileLoad();
	afx_msg void OnFileSmartOpen();
	afx_msg void OnFileSave();
	afx_msg void OnUpdateUIFileSave( CCmdUI* pCmdUI );
	afx_msg void OnFileSaveAs();
	afx_msg void OnUpdateUIFileSaveAs( CCmdUI* pCmdUI );

  void SceneUpdate();
  CAnimationControlDlg* GetAnimationControlDlg();
	CModelViewportCE*     GetModelViewport() const;

	void PlayAnimations( std::vector<CString> &anims );
	void OnAnimForcePlay();
	void SetViewPaneTitle(const char* name)
	{
		m_wndCaption.UpdateCaption( name,0 );
	}

	// make it possible to access the CCE_Panel_Animation later
	CAnimationBrowser* GetAnimationPanel(){return  &m_animationBrowser;}

	// make it possible to access the Attachment Dlg in the Animation panel
	CAttachmentsDlg* GetAttachmentDlg(){return &m_AttachmentsDlg;}

	CMotionAdaptorDlg* GetMotionAdaptorDlg() {return m_pMotionAdaptorDlg;}
	void ReadUserOptions();


protected:
	void OpenFile(bool bSmart);
	void InitFrameExtensions();
	void InitializeCaptionBar();
	void ResetCharEditor(ICharacterInstance* pCharacter); 
	void DoIdleUpdate();
	void SaveUserOptions();
	
	void SaveLayouts();
	void ReadLayouts();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedExit();
	afx_msg void OnBnClickedBrowseFile();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMoveMode();
	afx_msg void OnMoveModeUpdateUI( CCmdUI *pCmdUI );
	afx_msg void OnRotateMode();
	afx_msg void OnRotateModeUpdateUI( CCmdUI *pCmdUI );

	DECLARE_MESSAGE_MAP()

//CTabCtrl

	CXTPMenuBar *m_pWndMenuBar;
	CWnd m_pane1;
	CWnd m_pane2;
	class CModelViewportCE *m_pModelViewportCE;
	class CharPanel_Preset * m_pCharPanel_Preset;
	CRollupCtrl m_rollupCtrl;
	CEdit m_fileEdit;
	CDlgToolBar m_toolbar;

	// Animations dialog
	CAnimationBrowser	m_animationBrowser;

	//// Caption window.
	CXTCaption m_wndCaption;
	CFont m_fontCaption;

	class CLocalSplitterWnd : public CXTSplitterWnd
	{
	public:
		CLocalSplitterWnd()
		{
			m_cyBorder = m_cxBorder = 0;
			m_cxSplitter = m_cySplitter = 2;
			m_cySplitterGap = m_cxSplitterGap = 2;
		}
	};
	CLocalSplitterWnd m_wndSplitterCaption;

	CAttachmentsDlg m_AttachmentsDlg;
	CMorphingDlg m_MorphingDlg;
	CAnimationControlDlg m_AnimationControlDlg;
	CCharacterPropertiesDlg m_CharacterPropertiesDlg;

	CXTResizeDialog* m_pResizePane;
	CPropertiesPanel* s_varsPanel;

	struct SFrameExtension
	{
		_smart_ptr<IFrameWindowExtensionPane> pIFrameExtension;
		CWnd *pPaneWnd;
	};
	std::vector<SFrameExtension> m_frameExtensions;

  // Motion adaptor dialog
  CMotionAdaptorDlg* m_pMotionAdaptorDlg;
	string m_currExtension;

//string sDefaultFileName

public:
	afx_msg void OnUpdateViewAnimations(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewCharacterparts(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewRollup(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewMotionadaptor(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewSetMaterial(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewShapeDeformation(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewAnimationControl(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewAttachments(CCmdUI *pCmdUI);

	afx_msg void OnViewAnimations();
	afx_msg void OnViewCharacterparts();
	afx_msg void OnViewRollup();
	afx_msg void OnViewMotionadaptor();
	afx_msg void OnViewSetMaterial();
	afx_msg void OnViewShapeDeformation();
	afx_msg void OnViewAnimationControl();
	afx_msg void OnViewAttachments();
	
};
