

#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10View::CreateRenderingContext(HDC hdc)
{
	m_hGLContext = wglCreateContext(hdc);
	if(m_hGLContext==NULL)
		return false;
	if(wglMakeCurrent(hdc,m_hGLContext)==FALSE)
		return false;
	return true;
}

// COpenGLView message handlers
//////////////////////////////////////////////////////////////////////////
int CPhotoBump10View::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
	return -1;

	HWND hWnd = GetSafeHwnd();
	HDC hDC = ::GetDC(hWnd);

	if (!SetPixelFrmt(hDC))
		return -1;
	
	if (!CreateRenderingContext(hDC))
		return -1;

	////////////////////////////////////////////////////
	//wglMakeCurrent(this->GetWindowDC()->GetSafeHdc(),GetRC());
	SetGLCurrent();

	CPhotoBump10Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->m_pGLView=this;

	m_pVideo=pDoc->m_pVideo;
	m_pUtils=pDoc->m_pUtils;

	GetClientRect(&m_ClientRect);
	
  int nWidth = m_ClientRect.Width();
  int nHeight = m_ClientRect.Height();

	m_Camera.Init(nWidth,nHeight,DEFAULT_FOV,200.0,0.15f);
	m_Camera.SetPos(DEFAULT_CAMERA_POS);
	m_Camera.SetAngles(DEFAULT_CAMERA_ANGLES);
	m_Camera.m_fSpeed=DEFAULT_CAMERA_SPEED;
	
	m_pVideo->InitGL();

	wglMakeCurrent(NULL,NULL);
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////
HGLRC CPhotoBump10View::GetRC()
{
	return  m_hGLContext;
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10View::OnDestroy()
{
	if(wglGetCurrentContext()!=NULL)
		wglMakeCurrent(NULL,NULL);

	if (m_hGLContext!=NULL)
	{
		wglDeleteContext(m_hGLContext);
		m_hGLContext=NULL;
	}
	CView::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10View::SetPixelFrmt(HDC hdc)
{
	PIXELFORMATDESCRIPTOR pixelDesc;
	pixelDesc.nSize		= sizeof(PIXELFORMATDESCRIPTOR);
	pixelDesc.nVersion	= 1;
	
	pixelDesc.dwFlags	=	PFD_DRAW_TO_WINDOW | 
		PFD_SUPPORT_OPENGL | 
		PFD_DOUBLEBUFFER | 
		PFD_STEREO_DONTCARE;  
	
	pixelDesc.iPixelType		= PFD_TYPE_RGBA;
	pixelDesc.cColorBits		= 32;
	pixelDesc.cRedBits			= 8;
	pixelDesc.cRedShift			= 16;
	pixelDesc.cGreenBits		= 8;
	pixelDesc.cGreenShift		= 8;
	pixelDesc.cBlueBits			= 8;
	pixelDesc.cBlueShift		= 0;
	pixelDesc.cAlphaBits		= 0;
	pixelDesc.cAlphaShift		= 0;
	pixelDesc.cAccumBits		= 64;	
	pixelDesc.cAccumRedBits		= 16;
	pixelDesc.cAccumGreenBits	= 16;
	pixelDesc.cAccumBlueBits	= 16;
	pixelDesc.cAccumAlphaBits	= 0;
	pixelDesc.cDepthBits		= 24;
	pixelDesc.cStencilBits		= 8;
	pixelDesc.cAuxBuffers		= 0;
	pixelDesc.iLayerType		= PFD_MAIN_PLANE;
	pixelDesc.bReserved			= 0;
	pixelDesc.dwLayerMask		= 0;
	pixelDesc.dwVisibleMask		= 0;
	pixelDesc.dwDamageMask		= 0;
	
	m_GLPixelIndex = ChoosePixelFormat( hdc, &pixelDesc);
	if (m_GLPixelIndex==0) // Let's choose a default index.
	{
		m_GLPixelIndex = 1;	
		if (DescribePixelFormat(hdc, 
			m_GLPixelIndex, 
			sizeof(PIXELFORMATDESCRIPTOR), 
			&pixelDesc)==0)
		{
			return false;
		}
	}
	
	if (::SetPixelFormat( hdc, 
		m_GLPixelIndex, 
		&pixelDesc)==FALSE)
	{
		return false;
	}
	return true;
}
