// PhotoBump10View.cpp : implementation of the CPhotoBump10View class
//

#include "stdafx.h"
#include "PhotoBump10.h"

#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPhotoBump10View

IMPLEMENT_DYNCREATE(CPhotoBump10View, CView)

BEGIN_MESSAGE_MAP(CPhotoBump10View, CView)
	ON_WM_ERASEBKGND()	
	ON_WM_CREATE()
	ON_WM_DESTROY()	
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN() 
	ON_WM_RBUTTONDOWN() 
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()	
	ON_WM_KEYDOWN()
	ON_WM_TIMER() 
	
END_MESSAGE_MAP()

// CPhotoBump10View construction/destruction

CPhotoBump10View::CPhotoBump10View()
{
	// TODO: add construction code here
	m_pVideo=NULL;
	m_pUtils=NULL;
	m_nShowCameraInfo=1;	
	m_bRotateCamera=m_bTranslateCamera=m_bForward=false;
	m_Camera.m_nPivot=0;
	m_nSelectedCamera=0;
	theApp.m_pView=this;	
	m_bDumpError=false;
	m_bDrawing=false;
	m_bUseGLLight=false;
}

//////////////////////////////////////////////////////////////////////////
CPhotoBump10View::~CPhotoBump10View()
{
}

//////////////////////////////////////////////////////////////////////////
BOOL CPhotoBump10View::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;//CView::OnEraseBkgnd(pDC);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	GetClientRect(&m_ClientRect);
	
  int nWidth = m_ClientRect.Width();
  int nHeight = m_ClientRect.Height();

	if (nHeight<=0 || nWidth<=0)
		return;

	//wglMakeCurrent(this->GetWindowDC()->GetSafeHdc(), GetRC());
	SetGLCurrent();

	//m_pVideo->CheckError("P1");	
	m_pVideo->OnSize(m_ClientRect.left,m_ClientRect.top,nWidth,nHeight);
	m_Camera.Init(m_pVideo->m_Width,m_pVideo->m_Height,m_Camera.m_fFov,m_Camera.m_fZMax,m_Camera.m_fZMin);	
	
	m_pVideo->SetCamera(&m_Camera,true,true);

	//m_pVideo->CheckError("P2");
	
	wglMakeCurrent(NULL,NULL);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnPaint() 
{

	//////////////////////////////////////////////////////////////////////////	
	// Draw	

	DrawScene(); 		
}

//////////////////////////////////////////////////////////////////////////
BOOL CPhotoBump10View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CPhotoBump10View drawing
//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnDraw(CDC* /*pDC*/)
{
	CPhotoBump10Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnTimer(UINT nIDEvent) 
{ 
	DrawScene(); 

	CView::OnTimer(nIDEvent); 

	// Eat spurious WM_TIMER messages 
	MSG msg; 
	while(::PeekMessage(&msg, m_hWnd, WM_TIMER, WM_TIMER, PM_REMOVE)); 
} 


// CPhotoBump10View diagnostics

#ifdef _DEBUG
void CPhotoBump10View::AssertValid() const
{
	CView::AssertValid();
}

void CPhotoBump10View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPhotoBump10Doc* CPhotoBump10View::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPhotoBump10Doc)));
	return (CPhotoBump10Doc*)m_pDocument;
}
#endif //_DEBUG


// CPhotoBump10View message handlers
