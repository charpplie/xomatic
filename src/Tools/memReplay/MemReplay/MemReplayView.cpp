// DemoView.cpp : implementation of the CMemReplayView class
//

#include "stdafx.h"
#include "MemReplay.h"

#include "MemReplayDoc.h"
#include "MemReplayView.h"
#include "MainFrm.h"

#include "CodeTreePage.h"
#include "UsagePage.h"
#include "MemoryMapPage.h"
#include "TypeInfoPage.h"
#include "SizerTreePage.h"
#include "BucketAnalysisPage.h"

#include "ReplayProgressDlg.h"
#include "WaitForFutureReporter.h"
#include "CodeTreeUtility.h"

#include "ReplaySDK/ContextTree.h"
#include "ReplaySDK/ContextTreeFunctors.h"
#include "ReplaySDK/ExcelExport.h"
#include "ReplaySDK/CommonCSVExporters.h"

static void PopulateOffsets(const ContextTreeNode* node, std::vector<ContextStreamOffsetSpan>& offsetsOut)
{
	offsetsOut.insert(offsetsOut.end(), node->GetAllocStreamOffsets().begin(), node->GetAllocStreamOffsets().end());

	for (const ContextTreeNode* child = node->GetChildren(); child; child = child->GetNextSibling())
	{
		PopulateOffsets(child, offsetsOut);
	}
}

// CMemReplayView

IMPLEMENT_DYNCREATE(CMemReplayView, CView)

BEGIN_MESSAGE_MAP(CMemReplayView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_NOTIFY(CTabPageWnd::TPW_PAGECHANGED, ID_TABWND, OnPageChanged)
	ON_NOTIFY(CTabPageWnd::TPW_CLOSECLICKED, ID_TABWND, OnPageCloseClicked)
END_MESSAGE_MAP()

// CMemReplayView construction/destruction

CMemReplayView::CMemReplayView()
{
}

CMemReplayView::~CMemReplayView()
{
}

void CMemReplayView::OpenCodeViewFor(const char* name, const SharedPtr<GenericTree>& hier)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		CCodeTreePage* page = new CCodeTreePage(hier, db->GetSymbolTable(), name);

		AddTab(page, true);
		SelectTab(page);
	}
}

void CMemReplayView::OpenCodeViewFor(const ContextTreeNode& node)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		std::vector<ContextStreamOffsetSpan> offsets;
		PopulateOffsets(&node, offsets);

		CProgressReportDlg progressDlg;
		ReplayQueryFuture<SharedPtr<GenericTree> > future = db->BeginQueryCodeTree(
			offsets, 
			(IReplayDatabaseConnection::CodeTreeFilter) (IReplayDatabaseConnection::CTF_Allocations | IReplayDatabaseConnection::CTF_Frees));
		progressDlg.Run(MakeWaitForFutureReporter(future));

		OpenCodeViewFor(node.GetName(), future.GetResult());
	}
}

void CMemReplayView::OpenCodeViewForFrames(int frameBeginIdx, int frameEndIdx)
{
	using std::swap;

	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		if (frameBeginIdx > frameEndIdx)
			swap(frameBeginIdx, frameEndIdx);

		const FrameUsageTracker& tracker = *db->GetFrameUsageTracker();

		frameBeginIdx = Clamp<int>(0, std::distance(tracker.FrameOffsetsBegin(), tracker.FrameOffsetsEnd()) - 1, frameBeginIdx);
		frameEndIdx = Clamp<int>(0, std::distance(tracker.FrameOffsetsBegin(), tracker.FrameOffsetsEnd()) - 1, frameEndIdx);

		std::vector<std::pair<u64, u64> > spans(tracker.FrameOffsetsBegin() + frameBeginIdx, tracker.FrameOffsetsBegin() + frameEndIdx);

		ReplayQueryFuture<SharedPtr<GenericTree> > future;
		char name[256];

		UINT type = theApp.PopupMenu(IDR_CODEINSPECT);
		switch (type)
		{
		case ID_CODEINSPECT_ALLOCATIONS:
		case ID_CODEINSPECT_DELTA:
		case ID_CODEINSPECT_FREES:
			sprintf_s(name, 256, "Frames %i-%i", frameBeginIdx, frameEndIdx);
			future = db->BeginQueryCodeTree(spans, CodeTreeFilterFromType(type), std::vector<int>());
			break;

		case ID_CODEINSPECT_FREEDSPACE:
			sprintf_s(name, 256, "Freed space up to frame %i", frameEndIdx);
			future = db->BeginQueryFreedSpace(spans.back().second, std::vector<int>());
			break;
		}

		if (future.IsValid())
		{
			CProgressReportDlg progressDlg;
			progressDlg.Run(MakeWaitForFutureReporter(future));

			OpenCodeViewFor(name, future.GetResult());
		}
	}
}

void CMemReplayView::OpenCodeViewForAllocEvRange(s64 begin, s64 end)
{
	using std::swap;

	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		if (begin > end)
			swap(begin, end);

		const FrameUsageTracker& tracker = *db->GetFrameUsageTracker();

		begin = Clamp<s64>(0, std::distance(tracker.AllocEvOffsetsBegin(), tracker.AllocEvOffsetsEnd()) - 1, begin);
		end = Clamp<s64>(0, std::distance(tracker.AllocEvOffsetsBegin(), tracker.AllocEvOffsetsEnd()) - 1, end);

		ReplayQueryFuture<SharedPtr<GenericTree> > future;
		char name[256];
		
		UINT type = theApp.PopupMenu(IDR_CODEINSPECT);
		switch (type)
		{
		case ID_CODEINSPECT_ALLOCATIONS:
		case ID_CODEINSPECT_DELTA:
		case ID_CODEINSPECT_FREES:
			sprintf_s(name, 256, "Alloc events %I64u-%I64u", begin * tracker.GetAllocEventSplit(), end * tracker.GetAllocEventSplit());
			future = db->BeginQueryCodeTree(
				std::vector<std::pair<u64, u64> >(tracker.AllocEvOffsetsBegin() + begin, tracker.AllocEvOffsetsBegin() + end),
				CodeTreeFilterFromType(type), std::vector<int>());
			break;

		case ID_CODEINSPECT_FREEDSPACE:
			sprintf_s(name, 256, "Freed space up to %I64u", end * tracker.GetAllocEventSplit());
			future = db->BeginQueryFreedSpace(end * tracker.GetAllocEventSplit(), std::vector<int>());
			break;
		}

		if (future.IsValid())
		{
			CProgressReportDlg progressDlg;
			progressDlg.Run(MakeWaitForFutureReporter(future));

			CCodeTreePage* page = new CCodeTreePage(future.GetResult(), db->GetSymbolTable(), name);

			AddTab(page, true);
			SelectTab(page);
		}
	}
}

void CMemReplayView::OpenCodeBucketViewForAllocEvRange(s64 begin, s64 end, const std::vector<int>& buckets)
{
	using std::swap;

	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		if (begin > end)
			swap(begin, end);

		const FrameUsageTracker& tracker = *db->GetFrameUsageTracker();

		begin = Clamp<s64>(0, std::distance(tracker.AllocEvOffsetsBegin(), tracker.AllocEvOffsetsEnd()) - 1, begin);
		end = Clamp<s64>(0, std::distance(tracker.AllocEvOffsetsBegin(), tracker.AllocEvOffsetsEnd()) - 1, end);

		std::vector<std::pair<u64, u64> > spans(tracker.AllocEvOffsetsBegin() + begin, tracker.AllocEvOffsetsBegin() + end);

		ReplayQueryFuture<SharedPtr<GenericTree> > future;
		char name[256], sz1[64], sz2[64];

		UINT type = theApp.PopupMenu(IDR_CODEINSPECT);
		switch (type)
		{
		case ID_CODEINSPECT_ALLOCATIONS:
		case ID_CODEINSPECT_DELTA:
		case ID_CODEINSPECT_FREES:
			future = db->BeginQueryCodeTree(spans, CodeTreeFilterFromType(type), buckets);
			sprintf_s(
				name, 256,
				"Alloc events %s-%s",
				FormatThousands<s64>(sz1, 64, begin * tracker.GetAllocEventSplit()),
				FormatThousands<s64>(sz2, 64, end * tracker.GetAllocEventSplit()));
			break;

		case ID_CODEINSPECT_FREEDSPACE:
			future = db->BeginQueryFreedSpace(end * tracker.GetAllocEventSplit(), buckets);
			sprintf_s(
				name, 256, "Freed space up to %s", 
				FormatThousands<s64>(sz1, 64, end * tracker.GetAllocEventSplit()));
			break;
		}

		if (future.IsValid())
		{
			CProgressReportDlg progressDlg;
			progressDlg.Run(MakeWaitForFutureReporter(future));

			CCodeTreePage* page = new CCodeTreePage(future.GetResult(), db->GetSymbolTable(), name);

			AddTab(page, true);
			SelectTab(page);
		}
	}
}

void CMemReplayView::OpenCodeQuery_Sizer(const GenericTree& tree, const GenericTreeNode* root)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		std::vector<const char*> matchAgainst;

		for (const GenericTreeNode* n = root; n; n = n->parent)
			matchAgainst.push_back(n->GetKey<const char*>());
		matchAgainst.pop_back();

		std::reverse(matchAgainst.begin(), matchAgainst.end());

		ReplayQueryFuture<SharedPtr<GenericTree> > future = db->BeginQueryCodeTreeFromSizer(0, matchAgainst);

		CProgressReportDlg dlg;
		dlg.Run(MakeWaitForFutureReporter(future));

		SharedPtr<GenericTree> code = future->GetResult();

		if (code.IsValid())
		{
			CCodeTreePage* page = new CCodeTreePage(code, db->GetSymbolTable(), root->GetKey<const char*>());
			AddTab(page, true);
			SelectTab(page);
		}
	}
}

void CMemReplayView::OpenReverseCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		SharedPtr<GenericTree> reversed = ReverseCode(hier, from);

		if (reversed.IsValid())
		{
			CCodeTreePage* page = new CCodeTreePage(reversed, db->GetSymbolTable(), name);

			AddTab(page, true);
			SelectTab(page);
		}
	}
}

void CMemReplayView::OpenGatherTDCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		SharedPtr<GenericTree> gathered = GatherCodeTopDown(hier, from);

		if (gathered.IsValid())
		{
			CCodeTreePage* page = new CCodeTreePage(gathered, db->GetSymbolTable(), name);

			AddTab(page, true);
			SelectTab(page);
		}
	}
}

void CMemReplayView::OpenGatherBUCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		SharedPtr<GenericTree> gathered = GatherCodeBottomUp(hier, from);

		if (gathered.IsValid())
		{
			CCodeTreePage* page = new CCodeTreePage(gathered, db->GetSymbolTable(), name);

			AddTab(page, true);
			SelectTab(page);
		}
	}
}

void CMemReplayView::OpenExcludeCodeViewFor(const char* name, const GenericTree& hier, const std::vector<TAddress>& from)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		SharedPtr<GenericTree> gathered = ExcludeCode(hier, from);

		if (gathered.IsValid())
		{
			CCodeTreePage* page = new CCodeTreePage(gathered, db->GetSymbolTable(), name);

			AddTab(page, true);
			SelectTab(page);
		}
	}
}
	
void CMemReplayView::OpenTypeInfoViewFor(const s8 *typeName)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		ReplayQueryFuture<SharedPtr<GenericTree> > future = db->BeginQueryTypeStructure(typeName);
		SharedPtr<GenericTree> result = future.GetResult();

		if (result.IsValid())
		{
			CTypeInfoPage* page = new CTypeInfoPage(result, typeName);
			AddTab(page, true);
			SelectTab(page);
		}
	}
}

void CMemReplayView::AddCodeUsagePlot(const char* name, const std::vector<TAddress>& addresses, int memoryType)
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		ReplayQueryFuture<SharedPtr<SizeInfoUsageResult> > future = db->BeginQueryCodeUsage(addresses);

		CProgressReportDlg dlg;
		dlg.Run(MakeWaitForFutureReporter(future));

		SharedPtr<SizeInfoUsageResult> result = future.GetResult();

		if (result.IsValid())
		{
			m_usagePage->AddPlot(name, result, memoryType);
			SelectTab(m_usagePage);
		}
	}
}

void CMemReplayView::OpenBucketAnalysis()
{
	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		ReplayQueryFuture<SharedPtr<BucketUsageResult> > future = db->BeginQueryBucketUsage();

		CProgressReportDlg dlg;
		dlg.Run(MakeWaitForFutureReporter(future));

		CBucketAnalysisPage* page = new CBucketAnalysisPage(db, future.GetResult());
		AddTab(page, true);
		SelectTab(page);
	}
}

void CMemReplayView::ExportToXLS()
{
	CFileDialog dlg(FALSE, "*.xml", NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, "XML Files|*.xml", this);

	if (dlg.DoModal() == IDOK)
	{
		CString filename = dlg.GetFileName();

		ExcelExport xls;
		if (xls.Open(filename.GetString()))
		{
			int style = xls.BeginStyle();
			xls.EndStyle();

			for (std::vector<ITabPage*>::iterator it = m_tabs.begin(); it != m_tabs.end(); ++ it)
			{
				(*it)->Export(xls);
			}

			xls.Close();
		}
	}
}

ITabPage* CMemReplayView::GetActiveTab()
{
	CDialog* activePage = m_tabWnd.GetActivePage();
	if (activePage)
	{
		int index = m_tabWnd.GetPageIndex(activePage);
		return m_tabs[index];
	}

	return NULL;
}

void CMemReplayView::SelectTab(ITabPage* page)
{ 
	assert (page);

	std::vector<ITabPage*>::iterator it = std::find(m_tabs.begin(), m_tabs.end(), page);
	assert (it != m_tabs.end());

	m_tabWnd.SetActivePage(it - m_tabs.begin());
	OnPageChanged(NULL, NULL);
}

BOOL CMemReplayView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CView::PreCreateWindow(cs))
		return FALSE;

	cs.style |= WS_CLIPCHILDREN;

	return TRUE;
}

int CMemReplayView::OnCreate(LPCREATESTRUCT lpcs)
{
	int result = CView::OnCreate(lpcs);

	CRect rcClient;
	GetClientRect(rcClient);

	m_tabWnd.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, rcClient, this, ID_TABWND);
	m_tabWnd.ModifyStyleEx(0, WS_EX_CONTROLPARENT);

	m_usagePage = new CUsagePage(NULL);
	AddTab(m_usagePage, false);

	return result;
}

void CMemReplayView::OnInitialUpdate()
{
	using namespace ContextTreeFunctor;

	CView::OnInitialUpdate();

	for (int i = m_tabWnd.GetPageCount() - 1; i > 0; -- i)
	{
		CDialog* page = m_tabWnd.GetPage(i);
		m_tabWnd.RemovePage(i);
		delete page;
	}

	m_tabs.erase(m_tabs.begin() + 1, m_tabs.end());

	IReplayDatabaseConnection* db = GetConnection();

	if (db)
	{
		m_usagePage = new CUsagePage(db);
		AddTab(m_usagePage, false);

		SharedPtr<ContextTree> assetTree = db->GetRootContextTree();

		if (assetTree.IsValid())
		{
			SharedPtr<ContextTree> physicsTree = db->GetCgfPhysicsTree();
			SharedPtr<ContextTree> textureTree = db->GetTextureTree();
			SharedPtr<ContextTree> renderMeshTree = db->GetRenderMeshTree();
			SharedPtr<ContextTree> terrainTree = db->GetTerrainTree();
			SharedPtr<ContextTree> animationTree = db->GetAnimationTree();
			SharedPtr<ContextTree> navTree = db->GetNavTree();
			SharedPtr<ContextTree> entityTree = db->GetEntityTree();
			SharedPtr<ContextTree> soundProjectTree = db->GetSoundProjectTree();
			SharedPtr<ContextTree> overviewTree = db->GetOverviewTree();

			//AddTab(new CContextPage(overviewTree, &CommonCSVExporters::OverviewExporter, false), false);

			AddTab(new CContextPage(physicsTree, &CommonCSVExporters::CgfPhysExporter, false), false);
			AddTab(new CContextPage(textureTree, &CommonCSVExporters::TextureExporter, false), false);
			AddTab(new CContextPage(renderMeshTree, &CommonCSVExporters::RenderMeshExporter, false), false);
			AddTab(new CContextPage(terrainTree, &CommonCSVExporters::RenderMeshExporter, false), false);
			AddTab(new CContextPage(animationTree, &CommonCSVExporters::AnimationExporter, false), false);
			AddTab(new CContextPage(navTree, &CommonCSVExporters::NavExporter, true), false);
			AddTab(new CContextPage(entityTree, &CommonCSVExporters::EntityExporter, true), false);
			AddTab(new CContextPage(soundProjectTree, &CommonCSVExporters::SoundExporter, true), false);

			AddTab(new CContextPage(assetTree, NULL, true), false);
		}

		if (m_tabs.size() > 1)
		{
			// Delete the first page now that there's guaranteed to be at least one page left.
			DeleteTab(0);
		}
	}

	// The tabctrl font gets reset for some reason. Reload it.
	CFont* pFont = CFont::FromHandle( (HFONT)GetStockObject( DEFAULT_GUI_FONT) );
	m_tabWnd.SetFont(pFont);

	SelectTab(m_usagePage);
}

void CMemReplayView::OnDraw(CDC* pDC)
{
}

void CMemReplayView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (m_tabWnd.GetSafeHwnd())
	{
		CRect rcClient;
		GetClientRect(rcClient);

		//if (rcClient.Width() >= 100 && rcClient.Height() >= 100)
		{
			rcClient.DeflateRect(TabWnd_Margin, TabWnd_Margin);
			m_tabWnd.MoveWindow(rcClient);
		}
	}
}

BOOL CMemReplayView::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);

	const int margin = TabWnd_Margin;

	CRect rcBorders[] = {
		CRect(rcClient.left, rcClient.top, rcClient.top + margin, rcClient.bottom),
		CRect(rcClient.left + margin, rcClient.top, rcClient.right - margin, rcClient.top + margin),
		CRect(rcClient.right - margin, rcClient.top, rcClient.right, rcClient.bottom),
		CRect(rcClient.left + margin, rcClient.bottom - margin, rcClient.right - margin, rcClient.bottom)
	};

	CBrush bkgndBrush;
	bkgndBrush.CreateSysColorBrush(COLOR_3DFACE);
	
	for (int i = 0; i < 4; ++ i)
		pDC->FillRect(rcBorders[i], &bkgndBrush);

	return TRUE;
}

void CMemReplayView::OnPageChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CMainFrame* frame = static_cast<CMainFrame*>(AfxGetApp()->m_pMainWnd);
	frame->UpdateToolbar();
	GetActiveTab()->OnTabActivate();
}

void CMemReplayView::OnPageCloseClicked(NMHDR* pNMHDR, LRESULT* pResult)
{
	CTabPageWnd::CloseClicked* cc = reinterpret_cast<CTabPageWnd::CloseClicked*>(pNMHDR);

	if (cc->index >= 0)
	{
		DeleteTab(cc->index);
		GetActiveTab()->OnTabActivate();
	}
}

IReplayDatabaseConnection::CodeTreeFilter CMemReplayView::PromptForCTF()
{
	UINT type = theApp.PopupMenu(IDR_CODEINSPECT);
	return CodeTreeFilterFromType(type);
}

IReplayDatabaseConnection::CodeTreeFilter CMemReplayView::CodeTreeFilterFromType(UINT type)
{
	IReplayDatabaseConnection::CodeTreeFilter ctf;

	switch (type)
	{
	case ID_CODEINSPECT_ALLOCATIONS:
		ctf = IReplayDatabaseConnection::CTF_Allocations;
		break;

	case ID_CODEINSPECT_FREES:
		ctf = IReplayDatabaseConnection::CTF_Frees;
		break;

	case ID_CODEINSPECT_DELTA:
	default:
		ctf = static_cast<IReplayDatabaseConnection::CodeTreeFilter>(IReplayDatabaseConnection::CTF_Allocations | IReplayDatabaseConnection::CTF_Frees);
		break;
	}

	return ctf;
}
