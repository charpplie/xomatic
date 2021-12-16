// PolyBumpApplicationView.cpp : implementation of the CPolyBumpApplicationView class
//

#include "stdafx.h"
#include "PolyBumpApplication.h"

#include "PolyBumpApplicationDoc.h"
#include "PolyBumpApplicationView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPolyBumpApplicationView

IMPLEMENT_DYNCREATE(CPolyBumpApplicationView, CView)

BEGIN_MESSAGE_MAP(CPolyBumpApplicationView, CView)
END_MESSAGE_MAP()

// CPolyBumpApplicationView construction/destruction

CPolyBumpApplicationView::CPolyBumpApplicationView()
{
}

CPolyBumpApplicationView::~CPolyBumpApplicationView()
{
}


BOOL CPolyBumpApplicationView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}


/*
DWORD Vec3ToColor( const Vec3 &v )
{
	DWORD r = (DWORD)(v.x*127.5+128.0f);
	DWORD g = (DWORD)(v.y*127.5+128.0f);
	DWORD b = (DWORD)(v.z*127.5+128.0f);
 
	return (r<<16) | (g<<8) | b;
}
*/


void AddLine( CDC *pDC, uint32 &dwLineY, const int iLineCY, const char *szLine )
{
	pDC->TextOut(0,dwLineY,szLine,(int)strlen(szLine));

	dwLineY += iLineCY;
}


// CPolyBumpApplicationView drawing
void CPolyBumpApplicationView::OnDraw( CDC *pDC )
{
	CPolyBumpApplicationDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	uint32 dwLineY=0;

	CSRFData *pSRFData = pDoc->m_WorkerThreadData.GetSRFData();

	if(!pSRFData)
	{
		pDC->TextOut(0,dwLineY,"Calculation in progress ...");
		return;				// is processing
	}

	if(pSRFData->GetWidth())
	{	
		uint32 dwMaxPreviewSize = 256;
		uint32 dwPreviewSizeY = pSRFData->GetHeight();				

		if(dwPreviewSizeY>dwMaxPreviewSize)
			dwPreviewSizeY=dwMaxPreviewSize;

		std::vector<uint32>	Mem;

		uint32 dwPreviewSizeX = (dwPreviewSizeY*pSRFData->GetWidth())/pSRFData->GetHeight();

		if(dwPreviewSizeX>dwMaxPreviewSize)
		{
			dwPreviewSizeX=dwMaxPreviewSize;
			dwPreviewSizeY=(dwPreviewSizeX*pSRFData->GetHeight())/pSRFData->GetWidth();
		}

		Mem.resize(dwPreviewSizeX*dwPreviewSizeY);

		uint32 dwPreviewId=0;

		for(;;++dwPreviewId)
		{
			if(!pSRFData->RequestPreview(dwPreviewSizeX,dwPreviewSizeY,0,dwPreviewSizeY,&Mem[0],dwPreviewId))
				break;

			DWORD mem[sizeof(BITMAPINFOHEADER)/4+3];	//3 DWORDS for the RGB-Masks
			BITMAPINFO *lpbmi=(BITMAPINFO *)mem; 	

			lpbmi->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
			lpbmi->bmiHeader.biWidth  = dwPreviewSizeX;
			lpbmi->bmiHeader.biHeight = (int)dwPreviewSizeY;
			lpbmi->bmiHeader.biPlanes = 1;
			lpbmi->bmiHeader.biBitCount=32;
			lpbmi->bmiHeader.biCompression=BI_BITFIELDS;
			lpbmi->bmiHeader.biSizeImage=0;
			lpbmi->bmiHeader.biXPelsPerMeter=1;
			lpbmi->bmiHeader.biYPelsPerMeter=1;
			lpbmi->bmiHeader.biClrUsed=0;
			lpbmi->bmiHeader.biClrImportant=0;

			mem[sizeof(BITMAPINFOHEADER)/4+0]=0x00ff0000;
			mem[sizeof(BITMAPINFOHEADER)/4+1]=0x0000ff00;
			mem[sizeof(BITMAPINFOHEADER)/4+2]=0x000000ff;

			SetDIBitsToDevice(pDC->GetSafeHdc(),dwPreviewId*(dwPreviewSizeX+1),0,lpbmi->bmiHeader.biWidth,lpbmi->bmiHeader.biHeight,0,0,0,-lpbmi->bmiHeader.biHeight,&Mem[0],lpbmi,DIB_RGB_COLORS);

		}			
		
		if(dwPreviewId!=0)
			dwLineY+=dwPreviewSizeY+2;
	}

	int iLineCY = pDC->GetTextExtent("My",2).cy;

	for(uint32 dwI=0;;++dwI)
	{
		const char *szLine = pSRFData->GetInfoString(dwI);			if(!szLine)break;

		AddLine(pDC,dwLineY,iLineCY,szLine);
	}

	uint32 dwComputationTime = (pDoc->m_WorkerThreadData.GetComputationTime()+999)/1000;

	if(dwComputationTime)
	{
		AddLine(pDC,dwLineY,iLineCY,"");

		{
			char str[256];

			sprintf(str,"ComputationTime: %d min %d sec",dwComputationTime/60,dwComputationTime%60);
			AddLine(pDC,dwLineY,iLineCY,str);
		}
	}
}




// CPolyBumpApplicationView diagnostics

#ifdef _DEBUG
void CPolyBumpApplicationView::AssertValid() const
{
	CView::AssertValid();
}

void CPolyBumpApplicationView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPolyBumpApplicationDoc* CPolyBumpApplicationView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPolyBumpApplicationDoc)));
	return (CPolyBumpApplicationDoc*)m_pDocument;
}
#endif //_DEBUG


// CPolyBumpApplicationView message handlers
