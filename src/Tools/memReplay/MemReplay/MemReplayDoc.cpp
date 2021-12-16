// DemoDoc.cpp : implementation of the CMemReplayDoc class
//

#include "stdafx.h"
#include "MemReplay.h"

#include "MemReplayDoc.h"
#include "MemReplayView.h"

#include "ReplayProgressDlg.h"

#include "ReplaySDK/ContextTree.h"
#include "ReplaySDK/ReplayLogProgressTask.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMemReplayDoc

IMPLEMENT_DYNCREATE(CMemReplayDoc, CDocument)

BEGIN_MESSAGE_MAP(CMemReplayDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
END_MESSAGE_MAP()


// CMemReplayDoc construction/destruction

CMemReplayDoc::CMemReplayDoc()
{
	// TODO: add one-time construction code here

}

CMemReplayDoc::~CMemReplayDoc()
{
}

BOOL CMemReplayDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_sdkDoc = SharedPtr<IReplayDatabaseConnection>();

	return TRUE;
}

CMemReplayView* CMemReplayDoc::GetView()
{
	CFrameWnd * pFrame = (CFrameWnd *)(AfxGetApp()->m_pMainWnd);
	CView * pView = pFrame->GetActiveView();
	if ( !pView )
		return NULL;
	if ( ! pView->IsKindOf( RUNTIME_CLASS(CMemReplayView) ) )
		return NULL;
	return (CMemReplayView*)pView;
}

BOOL CMemReplayDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	m_sdkDoc = IReplayDatabaseConnection::Open(lpszPathName);

	return m_sdkDoc.IsValid() ? TRUE : FALSE;
}

BOOL CMemReplayDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnSaveDocument(lpszPathName))
		return FALSE;

	return FALSE;
}


void CMemReplayDoc::OnFileSaveAs()
{
}

// CMemReplayDoc serialization

void CMemReplayDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CMemReplayDoc diagnostics

#ifdef _DEBUG
void CMemReplayDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMemReplayDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMemReplayDoc commands
