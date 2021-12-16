// FaceAnimView.cpp : implementation of the CFaceAnimView class
//

#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"
#include "FaceAnimView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFaceAnimView

IMPLEMENT_DYNCREATE(CFaceAnimView, CView)

BEGIN_MESSAGE_MAP(CFaceAnimView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

// CFaceAnimView construction/destruction

CFaceAnimView::CFaceAnimView()
{
	// TODO: add construction code here

}

CFaceAnimView::~CFaceAnimView()
{
}

BOOL CFaceAnimView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CFaceAnimView drawing

void CFaceAnimView::OnDraw(CDC* /*pDC*/)
{
	CFaceAnimDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CFaceAnimView printing

BOOL CFaceAnimView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFaceAnimView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFaceAnimView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CFaceAnimView diagnostics

#ifdef _DEBUG
void CFaceAnimView::AssertValid() const
{
	CView::AssertValid();
}

void CFaceAnimView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFaceAnimDoc* CFaceAnimView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFaceAnimDoc)));
	return (CFaceAnimDoc*)m_pDocument;
}
#endif //_DEBUG


// CFaceAnimView message handlers
