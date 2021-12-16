#include "stdafx.h"																								// precompiled header
#include "polybumpworkerthread.h"
#include <mmsystem.h>																							// timeGetTime()
#include "tga.h"																									// PIX_LoadTGA32()
#include "PbTri.h"																								// CPbTri
#include "SimpleTriangleRasterizer.h"															// CSimpleTriangleRasterizer
#include "PolyBump.h"																							// CPbMesh

static const char *g_szWindowClass = "POLYBUMPTHREADERWND";

const uint32 g_dwPreviewSizeX=256;
const uint32 g_dwPreviewSizeY=256;


LRESULT static CALLBACK ThreadderWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CPolyBumpWorkerThread *pThis = (CPolyBumpWorkerThread *)::GetWindowLongPtr(hWnd,GWLP_USERDATA);

	switch(uMsg)
	{
		case WM_PAINT:											//
		{
			PAINTSTRUCT paint;
			HDC hdc = BeginPaint(hWnd, &paint);
			pThis->m_PreviewProgress.Clear();
			pThis->Paint(hdc); 
			EndPaint(hWnd, &paint);
			break;
		}

		case WM_CLOSE:
		{
			pThis->SendStop();
			return 1;
		}
	}

  return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

void CPolyBumpWorkerThread::FreeData()
{
	m_HighMeshData.FreeData();
}



bool CPolyBumpWorkerThread::PushHighPolyMesh( const CSimpleIndexedMesh &rMesh )
{
	assert(m_bSetupPhase);

	DebugLog("build high poly 1");

	// build high poly data structure + accelerator structure

	try
	{
    m_HighMeshData.GetNormalsAndClearBaseVectors(rMesh);

		DebugLog("build high poly 2");

		char str[256];

		sprintf(str,"HighRes triangle count: %d (%d bytes)",m_HighMeshData.m_nNumTris,m_HighMeshData.m_nNumTris*sizeof(CPbHighTri));

		DebugLog("build high poly 3 (%s)",str);
  }
	catch (CMemoryException* )
	{
	  FreeData();

		MessageBox(0,"can't allocate memory","High-Poly mesh ",MB_OK);
		return false;
	}

	return true;
}


UINT CPolyBumpWorkerThread::ThreadMain()
{
	m_bComputationFinished = false;

	SetStartTime(timeGetTime());


	// calls ThreadPollMessages() from time to time
	CreateBumpMap(this);
/*
	{
		char str[256];
		DWORD time=timeGetTime()-GetStartTime();

		sprintf(str,"Processing time: %dms\n",time);
		OutputDebugString(str);

		m_Properties.m_bSummariesString+=str;
	}

	{
		char str[256];

		sprintf(str,"FP status :0x%.4x\n", _control87( 0, 0 ));
		OutputDebugString(str);

		m_Properties.m_bSummariesString+=str;
	}
*/
	m_bComputationFinished = true;
	DestroyThreadderHWND();

/*
	if(!m_bUserHasStopped)
	{
		if(m_Properties.m_bPrintSummary)
		{
			MessageBox(0,m_Properties.m_bSummariesString.c_str(),"PolyBumpPlugin Summary",MB_OK);
		}
	}
*/

	return 0;
}



void CPolyBumpWorkerThread::SetProgressState( const uint32 dwId, const CPolybumpProgress::EPolybumpProgressState state )
{
	m_CurrentProgress.SetState(dwId,state);
	UpdateProgress();
}




// constructor
CPolyBumpWorkerThread::CPolyBumpWorkerThread()
{
	m_bComputationFinished = true;
	m_hThreadderWndParent=0;
	m_hThreadderWnd=0;

	m_bSetupPhase=false;
	
	m_dwStartTime=0;m_dwComputationTime=0;
	m_bUserHasStopped=false;
	m_nImageSizeX=0;
	m_nImageSizeY=0;

	SetThreadPriority(m_hThread,THREAD_PRIORITY_BELOW_NORMAL);

	// Register the window class
  WNDCLASS wndClass = { 0, ThreadderWndProc, 0, 0,GetModuleHandle(0),NULL,LoadCursor( NULL, IDC_ARROW ), ::GetSysColorBrush(COLOR_BTNFACE), NULL, g_szWindowClass };

	RegisterClass(&wndClass);

	m_PreviewBuffer.resize(g_dwPreviewSizeX*g_dwPreviewSizeY);
}

void CPolyBumpWorkerThread::UpdateProgress()
{
	HDC hdc = GetDC(m_hThreadderWnd);

	Paint(hdc);

	ReleaseDC(m_hThreadderWnd,hdc);
}



CPolyBumpWorkerThread::~CPolyBumpWorkerThread()
{ 
	UnregisterClass(g_szWindowClass,GetModuleHandle(0));
}
 





// /return 
bool CPolyBumpWorkerThread::ThreadPollMessages( float infPercent, const char *inszPassName )
{
	char str[256];
	DWORD dwTimeSoFar=timeGetTime()-m_dwStartTime;		// in ms
	DWORD dwSecToGo=0;
	
	if(infPercent>=1.0f)
	{
		dwSecToGo=(DWORD)(((float)dwTimeSoFar/infPercent)*(100.0f-infPercent)/1000.0f);

		DWORD dwMinToGo=dwSecToGo/60;					dwSecToGo-=dwMinToGo*60;
		DWORD dwHToGo=dwMinToGo/60;						dwMinToGo-=dwHToGo*60;

		sprintf(str,"%.0f%% %s (%d:%d:%.2d to go)",infPercent,inszPassName,dwHToGo,dwMinToGo,dwSecToGo);
	}
	else sprintf(str,"%s %.0f %%",inszPassName,infPercent);


	SetWindowText(m_hThreadderWnd,str);


	MSG msg;

	while(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(ShouldDie())
		return false;

	m_dwComputationTime = timeGetTime()-m_dwStartTime;		// in ms

	return true;
}

uint32 CPolyBumpWorkerThread::GetComputationTime() const
{
	return m_dwComputationTime;
}






//
void CPolyBumpWorkerThread::CreateThreadderHWND()
{
	assert(!m_hThreadderWnd);		// don't call twice

	if(m_hThreadderWnd)
		return;										// to get better behavior if called twice


	uint32 dwHeight = m_Properties.GetOutputHeight();

	// each line form a progress unit
	m_CurrentProgress.Init(dwHeight);m_PreviewProgress.Init(dwHeight);

	uint32 dwPreviewSizeX, dwPreviewSizeY;

	ComputePreviewExtend(dwPreviewSizeX,dwPreviewSizeY);

	// Create the window
	m_hThreadderWnd = CreateWindowEx( WS_EX_TOOLWINDOW,g_szWindowClass,"Polybump Progress",WS_BORDER|WS_CAPTION|WS_VISIBLE|WS_SYSMENU, 
		0,0,
		dwPreviewSizeX+2*GetSystemMetrics(SM_CYFIXEDFRAME),
		dwPreviewSizeY+2*GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYSMCAPTION),
		m_hThreadderWndParent,NULL,GetModuleHandle(0),NULL);

	SetWindowLongPtr(m_hThreadderWnd, GWLP_USERDATA, (LONG_PTR)this);

	// set thread priority
	{
		int iPrio = m_Properties.m_iThreadPrio;

		if(iPrio<-2) iPrio=-2;
		if(iPrio>2)	 iPrio=2;

		SetThreadPriority(m_hThread,iPrio);
	}
}

void CPolyBumpWorkerThread::DestroyThreadderHWND()
{
	::ShowWindow(m_hThreadderWnd,SW_HIDE);
	DestroyWindow(m_hThreadderWnd);
	m_hThreadderWnd=0;

	m_CurrentProgress.DeInit();m_PreviewProgress.DeInit();

	PostMessage(m_hThreadderWndParent,WM_POLYBUMPTHREADFINISHED,0,0);				// parent application should update the window

	m_hThreadderWndParent=0;

	SetThreadPriority(m_hThread,THREAD_PRIORITY_BELOW_NORMAL);		// reset thread priority
}












//draw triangle in software
//////////////////////////////////////////////////////////////////////
void CPolyBumpWorkerThread::DrawTriPointer( uint16 *pTriIndices, CPbLowTri &rTri, int color, const bool inbConservative )
{ 
	float fU[3],fV[3];

	for(int i=0;i<3;i++)
	{
		fU[i]=(float)(rTri.m_fS[i]*(float)m_nImageSizeX);
		fV[i]=(float)(rTri.m_fT[i]*(float)m_nImageSizeY);
	}

	CSimpleTriangleRasterizer Raster(m_nImageSizeX,m_nImageSizeY);

	assert(color<0xffff);
	Raster.DTFlatFill<uint16>(pTriIndices,m_nImageSizeX,fU,fV,(uint16)color,inbConservative);
}


// draw low res tris pointers
void CPolyBumpWorkerThread::DrawTrisPointers( uint16 *pTriIndices, CPbLowMesh *pLowMesh, const int iMaterialId )
{
	bool bDebug = m_Properties.m_bOutputDebugInfo;
	uint32 k;

	// conservative
  for(k=0;k<pLowMesh->m_nNumTris;k++)  
    if(iMaterialId==-1 || pLowMesh->m_pMesh[k].m_TriMaterialID==(unsigned char)iMaterialId)
			DrawTriPointer(pTriIndices,pLowMesh->m_pMesh[k],k,true);

//	if(bDebug)
//		PIX_SaveTGA32("DrawTrisPointersCon.tga",(unsigned char *)m_pnTriPointer,m_nImageSizeX,m_nImageSizeY,false,false);

	// non conservative
	for(k=0;k<pLowMesh->m_nNumTris;k++)  
		if(iMaterialId==-1 || pLowMesh->m_pMesh[k].m_TriMaterialID==(unsigned char)iMaterialId)
			DrawTriPointer(pTriIndices,pLowMesh->m_pMesh[k],k,false);

//	if(bDebug)
//		PIX_SaveTGA32("DrawTrisPointersBoth.tga",(unsigned char *)m_pnTriPointer,m_nImageSizeX,m_nImageSizeY,false,false);
}


// set attribute
void CPolyBumpWorkerThread::SetStartTime( DWORD indwStartTime )
{
	m_dwStartTime=indwStartTime;
	m_dwComputationTime=0;
}


// get attribute
DWORD CPolyBumpWorkerThread::GetStartTime()
{
	return m_dwStartTime;
}



void CPolyBumpWorkerThread::DebugLog( const char *sFormat, ... )
{
	va_list vl;
	static char sTraceString[1024];

	va_start(vl, sFormat);
	vsprintf(sTraceString, sFormat, vl);
	va_end(vl);

	OutputDebugString(sTraceString);
	OutputDebugString("\n");

	if(!m_Properties.m_bOutputDebugInfo)
		return;

	FILE *out=fopen("PolyBumpDebug.txt","ab");

	if(!out)
		return;

	OutputDebugString(sTraceString);	
	OutputDebugString("\n");	

	fprintf(out,"%s\r\n",sTraceString);

	m_Properties.m_bSummariesString+=sTraceString;
	m_Properties.m_bSummariesString+="\n";

	fclose(out);
}




bool CPolyBumpWorkerThread::StartComputationSetup( HWND hThreadderParent )
{
	if(GetThreadStatus())
		return false;

	m_bSetupPhase=true;
	m_hThreadderWndParent=hThreadderParent;

	m_HighMeshData.FreeData();


	uint32 dwWidth = m_Properties.GetOutputWidth();
	uint32 dwHeight = m_Properties.GetOutputHeight();

	m_SRFData.Init(dwWidth,dwHeight);

	DebugLog("Width: %d, Height: %d",dwWidth,dwHeight);

	CreateThreadderHWND();

	return true;
}

void CPolyBumpWorkerThread::CancelComputationSetup()
{
	m_LowMesh.FreeData();
	m_HighMeshData.FreeData();
	m_CageMesh.FreeData();
	m_bSetupPhase=false;
	DestroyThreadderHWND();
}


void CPolyBumpWorkerThread::StartComputation()
{
	StartThread();
}


CSRFData *CPolyBumpWorkerThread::GetSRFData() 
{
	if(!m_bComputationFinished)
		return 0;

	return &m_SRFData; 
}

void CPolyBumpWorkerThread::Paint( HDC hdc )
{
	assert(m_CurrentProgress.IsValid());
	assert(m_PreviewProgress.IsValid());

	CSRFData *pSRFData = &m_SRFData;

	for(;;)
	{
		uint32 dwY = m_CurrentProgress.PropagateTo(m_PreviewProgress);

		if(dwY==0xffffffff)
			break;

		CPolybumpProgress::EPolybumpProgressState newstate = m_PreviewProgress.GetState(dwY);

		uint32 dwPreviewSizeX, dwPreviewSizeY;
		
		ComputePreviewExtend(dwPreviewSizeX,dwPreviewSizeY);

		uint32 dwIdCount = m_PreviewProgress.GetIdCount();
		uint32 dwStartPreviewY = (dwY*dwPreviewSizeY)/dwIdCount;
		uint32 dwEndPreviewY = ((dwY+1)*dwPreviewSizeY)/dwIdCount;

		assert(dwStartPreviewY<=dwPreviewSizeY);
		assert(dwEndPreviewY<=dwPreviewSizeY);

		if(dwStartPreviewY!=dwEndPreviewY)
		{
			if(newstate==CPolybumpProgress::EPPS_DoneFinal || newstate==CPolybumpProgress::EPPS_DoneP1)
				pSRFData->RequestPreview(dwPreviewSizeX,dwPreviewSizeY,dwStartPreviewY,dwEndPreviewY,&m_PreviewBuffer[0]);
			else
			{
				// currently in progress is shown as yellow line
				uint32 *p = &m_PreviewBuffer[dwPreviewSizeX*dwStartPreviewY];
			
				for(uint32 dwX=0;dwX<dwPreviewSizeX;++dwX)
					*p++ = 0xffff00;
			}

//			pSRFData->RequestPreview(dwPreviewSizeX,dwPreviewSizeY,0,dwPreviewSizeY,&m_PreviewBuffer[0]);

			DWORD mem[sizeof(BITMAPINFOHEADER)/4+3];	//3 DWORDS for the RGB-Masks
			BITMAPINFO *lpbmi=(BITMAPINFO *)mem; 	

			lpbmi->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
			lpbmi->bmiHeader.biWidth  = dwPreviewSizeX;
			lpbmi->bmiHeader.biHeight = (int)dwEndPreviewY-(int)dwStartPreviewY;
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

			SetDIBitsToDevice(hdc,0,dwPreviewSizeY-dwStartPreviewY,lpbmi->bmiHeader.biWidth,lpbmi->bmiHeader.biHeight,0,0,0,-lpbmi->bmiHeader.biHeight,
				&m_PreviewBuffer[dwStartPreviewY*lpbmi->bmiHeader.biWidth],lpbmi,DIB_RGB_COLORS);
		}
	}
}


void CPolyBumpWorkerThread::ComputePreviewExtend( uint32 &dwWidth, uint32 &dwHeight )
{
	dwWidth = m_SRFData.GetWidth();
	dwHeight = m_SRFData.GetHeight();

	while(dwWidth>g_dwPreviewSizeX || dwHeight>g_dwPreviewSizeY)
	{
		dwWidth/=2;
		dwHeight/=2;
	}
}
