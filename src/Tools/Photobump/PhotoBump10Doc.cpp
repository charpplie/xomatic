// PhotoBump10Doc.cpp : implementation of the CPhotoBump10Doc class
//

#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PVertex.h"
#include "PQuadtree.h"
#include "PhotoImage.h"

#include "opencv/cv.h"
#include ".\photobump10doc.h"
#include "HeightmapDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace Wml;

// CPhotoBump10Doc

IMPLEMENT_DYNCREATE(CPhotoBump10Doc, CDocument)

BEGIN_MESSAGE_MAP(CPhotoBump10Doc, CDocument)
		ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
		ON_COMMAND(ID_FILE_INVERTCHANNELS, OnFileInvertchannels)
		//ON_COMMAND(ID_FILE_CONVERTTOHEIGHTMAP, OnFileConverttoheightmap)
		ON_COMMAND(ID_Menu, OnSwapRGChannelsInvertRGValues)
		ON_COMMAND(ID_FILE_ALIGNTO, OnFileAlignto)
		//ON_COMMAND(ID_FILE_CONVERTTOHEIGHTMAP32775, OnFileConverttoheightmap32775)
		//ON_COMMAND(ID_FILE_AUTOMATICTILING, OnFileAutomatictiling)
END_MESSAGE_MAP()

// CPhotoBump10Doc construction/destruction
//////////////////////////////////////////////////////////////////////////
CPhotoBump10Doc::CPhotoBump10Doc()
{
	// TODO: add one-time construction code here
	m_pGLView=NULL;

	m_pUtils=new CUtils;
	m_pUtils->InitTimer();
	m_pVideo=new CVideo(m_pUtils);	
	m_QTree=NULL;	

	m_Indices=NULL;
	m_Vertices=NULL;	
	m_lstTriangles=NULL;
	m_ConvexHull2D=NULL;	
	
	m_bGenerateNormalMap=true;
	m_bGenerateHeightMap=true;
	m_nNumVertices=0;
	
	RemoveCalibrationData();
}

//////////////////////////////////////////////////////////////////////////
CPhotoBump10Doc::~CPhotoBump10Doc()
{ 
	RemoveCalibrationData();	
	SAFE_DELETE(m_pUtils);
	SAFE_DELETE(m_pVideo);
	//SAFE_DELETE(m_QTree);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::RemoveCalibrationData()
{	
	m_nNumFeatures=0;
	m_bHumans=false;
	m_bDone=false;
	m_nUnreliableIterations=0;
	m_bInitialUpdate=true;
	m_bShowFeaturesOnly=false;
	m_bLoadedFromBojou=false;
	m_bSegErrorsSet=false;
	m_nScale=m_nLastScale=0;
	m_fDetailScale=1.0;
	m_fNormalMapScale=1.0;
	m_x=m_y=0;
	m_bLoadData=false;
	m_nNumIndexes=0;		
	m_nFrame=0;
	SAFE_DELETE(m_ConvexHull2D);
	int k=0;
	for (lstFramesIt i=m_lstFrames.begin();i!=m_lstFrames.end();i++,k++)
	{
		CPhotoFrame *pFrame=(*i);
		delete pFrame;		
	} //i	
	m_lstFrames.clear();

	for (lstPhotoVerticesIt i2=m_lstVertices.begin();i2!=m_lstVertices.end();i2++)
	{
		CPVertex *pVert=(*i2);
		delete pVert;
	} //i
	m_lstVertices.clear();
	m_szDataLoad[0]=0;

	SAFE_DELETE_ARRAY(m_Vertices);
	SAFE_DELETE_ARRAY(m_Indices);
	SAFE_DELETE_ARRAY(m_lstTriangles);		
}

//////////////////////////////////////////////////////////////////////////
int		CPhotoBump10Doc::GetInfo()
{ 
	if (m_lstFrames.empty())
		return(0); 

	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;

	//return(int)(((ftype)(m_nDiffusePixel)/(ftype)(w*h))*100.0);
	return (0);
}

//////////////////////////////////////////////////////////////////////////
BOOL CPhotoBump10Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

// CPhotoBump10Doc serialization
//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here		
	}
	else
	{
		// TODO: add loading code here

		m_pGLView->SetTimer(1,15,NULL); 
		// dump error at least once to set all camera matrices
		// NOTE: must be called from view otherwise GL context etc. is not 
		// valid, thus no gl matrix is returned - the function just sets a bool
		//m_pGLView->DumpError();
		//m_pGLView->SetTimer(1,500,NULL); 			

		RemoveCalibrationData();		
		strcpy(m_szDataLoad,ar.GetFile()->GetFilePath());
		m_bLoadData=true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::OnFileSaveAs()
{
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
      "OBJ Files (*.obj)|*.obj|All Files (*.*)|*.*||";

	char szFile[512];
	strcpy(szFile,m_pUtils->GetFilename(m_szDataLoad));
	m_pUtils->ReplaceExtension(szFile,"obj");

  CFileDialog fileDlg (TRUE, "obj", szFile,0, szFilters, wnd);
  
  if (fileDlg.DoModal()==IDOK)
  {
    CString pathName = fileDlg.GetPathName();
  
    // Implement opening and reading file in here.
		ExportObj(pathName);		
		//CDocument::OnSaveDocument(pathName);
    
    // Change the window's title to the saved file's title.
    CString fileName = fileDlg.GetFileTitle ();   
    SetWindowText(wnd->GetSafeHwnd(),fileName);		
  }		
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::OnFileInvertchannels()
{
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, NULL, NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Implement opening and reading file in here.
		int nW,nH;
		uchar *szRes=InvertChannels(pathName,nW,nH);		

		if (szRes)
		{		
			char szFile[512];
			strcpy(szFile,pathName);
			m_pUtils->ReplaceExtension(szFile,"tga");
			char *szPos=szFile+strlen(szFile)-4; // ".tga"
			strcpy(szPos,"_inverted.tga");
			SaveTga(szRes,3,nW,nH,szFile,true);		
			// Change the window's title to the saved file's title.
			CString fileName = fileDlg.GetFileTitle();   
			SetWindowText(wnd->GetSafeHwnd(),fileName);		
			SAFE_DELETE_ARRAY(szRes);
		}
		else
			AfxMessageBox("Cannot open file");
	}		
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::OnSwapRGChannelsInvertRGValues()
{
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, NULL, NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Implement opening and reading file in here.
		int nW,nH;
		uchar *szRes=SwapRGChannelsInvertRGValues(pathName,nW,nH);		

		if (szRes)
		{		
			char szFile[512];
			strcpy(szFile,pathName);
			m_pUtils->ReplaceExtension(szFile,"tga");
			char *szPos=szFile+strlen(szFile)-4; // ".tga"
			strcpy(szPos,"_inverted.tga");
			SaveTga(szRes,3,nW,nH,szFile,true);		
			// Change the window's title to the saved file's title.
			CString fileName = fileDlg.GetFileTitle();   
			SetWindowText(wnd->GetSafeHwnd(),fileName);		
			SAFE_DELETE_ARRAY(szRes);
		}
		else
			AfxMessageBox("Cannot open file");
	}		
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::OnFileAlignto()
{
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, NULL, NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Implement opening and reading file in here.
		int nW,nH;
		uchar *szRes=AlignNormalMapFromFile(pathName,nW,nH);		

		if (szRes)
		{		
			char szFile[512];
			strcpy(szFile,pathName);
			m_pUtils->ReplaceExtension(szFile,"tga");
			char *szPos=szFile+strlen(szFile)-4; // ".tga"
			strcpy(szPos,"_aligned.tga");
			SaveTga(szRes,3,nW,nH,szFile,true);		
			// Change the window's title to the saved file's title.
			CString fileName = fileDlg.GetFileTitle();   
			SetWindowText(wnd->GetSafeHwnd(),fileName);		
			SAFE_DELETE_ARRAY(szRes);
		}
		else
			AfxMessageBox("Cannot open file");
	}			
}

/*
//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::OnFileConverttoheightmap()
{	
	//return;

	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, NULL, NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		CHeightmapDialog HDialog;	
		if (HDialog.DoModal()!=IDOK)
			return;	
		int nIters=HDialog.m_nIterations;
		// Implement opening and reading file in here.
		int nW,nH;
		uchar *szRes=ConvertToHeightmap(pathName,nW,nH,nIters);				

		if (szRes)
		{		
			char szFile[512];
			strcpy(szFile,pathName);
			m_pUtils->ReplaceExtension(szFile,"tga");
			char *szPos=szFile+strlen(szFile)-4; // ".tga"
			strcpy(szPos,"_heightmap.tga");
			SaveTga(szRes,4,nW,nH,szFile,true);		
			// Change the window's title to the saved file's title.
			//CString fileName = fileDlg.GetFileTitle();   
			SetWindowText(wnd->GetSafeHwnd(),szFile);		
			SAFE_DELETE_ARRAY(szRes);
		}
		else
			AfxMessageBox("Cannot open file");
	}			
}
*/

// CPhotoBump10Doc diagnostics
//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
void CPhotoBump10Doc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPhotoBump10Doc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CPhotoBump10Doc commands


/*
void CPhotoBump10Doc::OnFileAutomatictiling()
{
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, NULL, NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Implement opening and reading file in here.
		int nW,nH;
		uchar *szRes=AutomaticTiling(pathName,nW,nH);				

		if (szRes)
		{		
			char szFile[512];
			strcpy(szFile,pathName);
			m_pUtils->ReplaceExtension(szFile,"tga");
			char *szPos=szFile+strlen(szFile)-4; // ".tga"
			strcpy(szPos,"_tiled.tga");
			SaveTga(szRes,3,nW,nH,szFile,true);		
			// Change the window's title to the saved file's title.
			//CString fileName = fileDlg.GetFileTitle();   
			SetWindowText(wnd->GetSafeHwnd(),szFile);		
			SAFE_DELETE_ARRAY(szRes);
		}
		else
			AfxMessageBox("Cannot open file");
	}			
}
*/