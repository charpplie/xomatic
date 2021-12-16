/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc main frame window.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_MAINFRAMEWND_H__
#define __SCHEMATYC_MAINFRAMEWND_H__

#include <QWidget>
#include <EditorCommon/QPropertyTree/ContextList.h>
#include <Schematyc/Schematyc_IEnvRegistry.h>
#include <Schematyc/Schematyc_IFoundation.h>
#include <Schematyc/Schematyc_SerializationUtils.h>

#include "Schematyc_BrowserCtrl.h"
#include "Schematyc_DocGraphView.h"
#include "Schematyc_EnvBrowserCtrl.h"
#include "Schematyc_RichEditCtrl.h"

// TODO : Wrap detail panel!
// TODO : Clean up log output control implementation!
// TODO : Optimize log stream filtering.

struct	SRenderContext;
struct	IPropertyTree;
class		QParentWndWidget;
class		QViewport;

namespace Schematyc
{
	class CViewportWnd;

	class CViewportMessageHandler : public QObject
	{
		Q_OBJECT

	public:

		CViewportMessageHandler();

		void Connect(CViewportWnd* pViewportWnd, QViewport* pViewport);

	protected slots:

		void OnRender(const SRenderContext& context);

	private:

		CViewportWnd*	m_pViewportWnd;
	};

	class CViewportWnd : public CWnd
	{
		DECLARE_MESSAGE_MAP()

	public:

		CViewportWnd();

		~CViewportWnd();

		void Init();
		void InitLayout();
		void Update();
		void SetPreview(const IDocSchemaConstPtr& pSchema);
		void ResetPreview();

		void OnRender(const SRenderContext& context);
	
	private:

		afx_msg void OnSize(UINT nType, int cx, int cy);

		CViewportMessageHandler					m_messageHandler;
		QParentWndWidget*								m_pParentWndWidget;
		QViewport*											m_pViewport;
		SGUID														m_schemaGUID;
		IFoundationPreviewExtensionPtr	m_pFoundationPreviewExtension;
	};

	class CMainFrameWnd : public CXTPFrameWnd, public IEditorNotifyListener
	{
		DECLARE_DYNCREATE(CMainFrameWnd)	

		public:
	
			CMainFrameWnd();

			~CMainFrameWnd();

			virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
			virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd);

			// IEditorNotifyListener
			virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
			// ~IEditorNotifyListener

			static void RegisterViewClass();

		protected:
	
			DECLARE_MESSAGE_MAP()

			virtual BOOL OnInitDialog();
			virtual void DoDataExchange(CDataExchange* pDataExchange);
			virtual void PostNcDestroy();
			virtual BOOL PreTranslateMessage(MSG* pMsg);

			afx_msg BOOL OnViewMenuCommand(UINT nID);
			afx_msg void OnUpdateViewMenuItem(CCmdUI* pCmdUI);
			afx_msg void OnToolsConfigureLogStreams();
			afx_msg BOOL OnToolsConfigureLogOutput(UINT nID);
			afx_msg BOOL OnWorldDebugMenuCommand(UINT nID);
			afx_msg void OnUpdateWorldDebugMenuItem(CCmdUI* pCmdUI);
			afx_msg void OnDebugSelectedEntity();
			afx_msg void OnUpdateDebugSelectedEntity(CCmdUI* pCmdUI);
			afx_msg BOOL OnEntityDebugMenuCommand(UINT nID);
			afx_msg void OnUpdateEntityDebugMenuItem(CCmdUI* pCmdUI);
			afx_msg void OnSave();
			afx_msg void OnRefreshEnv();
			afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
			afx_msg void OnDestroy();

		private:

			typedef std::vector<SLogStreamName> TLogStreamNameVector;

			struct SLogOutputSettings
			{
				SLogOutputSettings();

				void Serialize(Serialization::IArchive& archive);

				bool									showComments;
				bool									showWarnings;
				bool									showErrors;
				TLogStreamNameVector	streams;
			};

			bool CreateMenuAndToolbar();
			void CreatePanes();
			void LoadLayout();
			void SaveLayout();
			void InitPaneTitles();
			void SelectGraph(IDoc& doc, IDocGraph& docGraph);
			void ClearGraph();
			void UpdateCompilerOutputCtrl(bool recompileGraph);
			void UpdateViewport();
			ILibPtr CompileDoc(IDoc& doc);
			void OnBrowserSelection(const CBrowserCtrlItemPtr& pBrowserItem);
			void OnBrowserDocModified(IDoc& doc);
			void OnBrowserDocGraphRemoved(IDoc& doc, IDocGraph& docGraph);
			void OnDetailPropertyChange();
			void OnGraphViewNodeSelection(const CDocGraphViewNodePtr& pSelectedNode);
			void OnGraphViewLinkSelection(SGraphViewLink* pSelectedLink);
			void OnGraphViewDocModified(IDoc& doc);
			void OnLogMessage(const TLogStreamId& streamId, LogMessageType::EValue messageType, const char* message);
			void UpdateLogOutputCtrl(const SLogOutputSettings& logOutputSettings, CCustomRichEditCtrl& logOutputCtrl, const TLogStreamId& streamId, LogMessageType::EValue messageType, const char* message);
			
			CXTPDockingPaneManager												m_dockingPaneManager;
			CBrowserCtrl																	m_browserCtrl;
			CEnvBrowserCtrl																m_envBrowserCtrl;
			IPropertyTree*																m_pDetailPropertyTree;
			std::unique_ptr<Serialization::CContextList>	m_pDetailContextList;
			IDoc*																					m_pDetailDoc;
			CDocGraphView*																m_pGraphView;
			SLogOutputSettings														m_logOutputSettings[4];
			CCustomRichEditCtrl														m_logOutputCtrlA;
			CCustomRichEditCtrl														m_logOutputCtrlB;
			CCustomRichEditCtrl														m_logOutputCtrlC;
			CCustomRichEditCtrl														m_logOutputCtrlD;
			CCustomRichEditCtrl														m_compilerOutputCtrl;
			CViewportWnd*																	m_pViewportWnd;

			struct 
			{
				CBrowserCtrl::TSelectionSignal::TScope				browserSelection;
				CBrowserCtrl::TDocModifiedSignal::TScope			browserDocModified;
				CBrowserCtrl::TDocGraphRemovedSignal::TScope	browserDocGraphRemoved;
				CDocGraphView::TNodeSelectionSignal::TScope		graphViewNodeSelection;
				CDocGraphView::TLinkSelectionSignal::TScope		graphViewLinkSelection;
				CDocGraphView::TDocModifiedSignal::TScope			graphViewDocModified;
				ILogMessageSignal::TScope											logMessage;
			} m_signalScopes;
	};
}

#endif //__SCHEMATYC_MAINFRAMEWND_H__