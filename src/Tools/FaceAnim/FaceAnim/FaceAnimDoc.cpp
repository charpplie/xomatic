// FaceAnimDoc.cpp : implementation of the CFaceAnimDoc class
//

#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"
#include "CalibrationAVI4.h"
#include "FolderDlg.h"
#include <io.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
 

// CFaceAnimDoc

IMPLEMENT_DYNCREATE(CFaceAnimDoc, CDocument)

BEGIN_MESSAGE_MAP(CFaceAnimDoc, CDocument)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_CREATECALIBRATIONFILE, OnFileCreatecalibrationfile)
	ON_COMMAND(ID_FILE_LOADCALIBRATIONFILE, OnFileLoadcalibrationfile)
	ON_COMMAND(ID_FILE_CREATEFACE, OnFileCreateFace)
	ON_COMMAND(ID_FILE_BATCHPROCESSFOLDER, OnFileBatchProcessFolder)
END_MESSAGE_MAP()


// CFaceAnimDoc construction/destruction

CFaceAnimDoc::CFaceAnimDoc()
{
	// TODO: add one-time construction code here
	m_bEditMode=false;
	m_nMarkerSelected=-1;
	m_bEditOperation=false;

	m_bBatchMode=false;
	m_bCreateCalibrationData=false;
	m_bMouse3=m_bMouse1=m_bMouse2=false;
}

CFaceAnimDoc::~CFaceAnimDoc()
{
}

BOOL CFaceAnimDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

void CFaceAnimDoc::OnFileOpen()
{	
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"AVI Files (*.avi)|*.avi|All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, "avi", NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Change the window's title to the saved file's title.
		//CString fileName = fileDlg.GetFileTitle();   
		//SetWindowText(wnd->GetSafeHwnd(),fileName);		

		char szFilename[256];		
		strcpy(szFilename,fileDlg.GetPathName());
		//CalibrationAVI3(szFilename);
		m_bCreateCalibrationData=false;
		SetCurrentDirectory(theApp.m_szWorkingFolder);		
		CalibrationAVI3_2(szFilename);

	}		
}

void CFaceAnimDoc::OnFileLoadcalibrationfile()
{
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"ini Files (*.ini)|*.ini|All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, "ini", NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Change the window's title to the saved file's title.
		//CString fileName = fileDlg.GetFileTitle();   
		//SetWindowText(wnd->GetSafeHwnd(),fileName);		

		char szFilename[256];		
		char szText[256];
		strcpy(szFilename,fileDlg.GetPathName());

		strcpy(theApp.m_szCalibrationFile,GetFilename(szFilename));
		ReplaceExtension(theApp.m_szCalibrationFile,"ini");
		sprintf(szText,"FaceAnim Video Tool - calibration file \"%s\"",theApp.m_szCalibrationFile);
		CWnd *wnd=AfxGetMainWnd();
		// Change the window's title to the saved file's title.	
		SetWindowText(wnd->GetSafeHwnd(),szText);		

		SetCurrentDirectory(theApp.m_szWorkingFolder);		

		m_bCreateCalibrationData=false;		
	}		
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::OnFileCreatecalibrationfile()
{
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	char szFilters[]=
		"AVI Files (*.avi)|*.avi|All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, "avi", NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Change the window's title to the saved file's title.
		//CString fileName = fileDlg.GetFileTitle();   
		//SetWindowText(wnd->GetSafeHwnd(),fileName);		

		char szFilename[256];		
		strcpy(szFilename,fileDlg.GetPathName());
		
		strcpy(theApp.m_szCalibrationFile,GetFilename(szFilename));
		ReplaceExtension(theApp.m_szCalibrationFile,"ini");
				
		m_bCreateCalibrationData=true;
		
		SetCurrentDirectory(theApp.m_szWorkingFolder);		
		CalibrationAVI3_2(szFilename);

	}		
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::ProcessFolder(const char *szFilename)
{
	MyOutputDebugString("Processing Folder %s\n",szFilename);

	struct _finddata_t c_file;
	intptr_t hFile;

	char szFile[256];
	char szFolder[256];
	sprintf(szFolder,"%s/*.avi",szFilename);

	// Find first .c file in current directory 
	if( (hFile = _findfirst( szFolder, &c_file )) == -1L )
	{

	}
	else
	{		
		do {
			sprintf(szFile,"%s/%s",szFilename,c_file.name);
			MyOutputDebugString("Processing file %s\n",szFile);
			CalibrationAVI3_2(szFile);			
		} while( _findnext( hFile, &c_file ) == 0 );
		_findclose( hFile );
	}

	sprintf(szFolder,"%s/*.*",szFilename);

	if( (hFile = _findfirst( szFolder, &c_file )) == -1L )
		return;

	do {				
		if (strlen(c_file.name)<3)	
			continue;

		if (!(c_file.attrib & _A_SUBDIR))
			continue;
		
		sprintf(szFile,"%s/%s",szFilename,c_file.name);
		ProcessFolder(szFile);

	} while( _findnext( hFile, &c_file ) == 0 );
	_findclose( hFile );
	
	
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::OnFileBatchProcessFolder()
{
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	//CFolderDialog dlg(  _T( "Dialog Title" ), m_strFolderPath, this );
	CFolderDialog dlg(  _T( "Dialog Title" ), NULL, wnd );
	if( dlg.DoModal() == IDOK  )
	{    
		CString strFolderPath  = dlg.GetFolderPath();
		//CString strDisplayName = dlg.GetFolderDisplayName();
		// Use folder path and display name here ...		
		SetCurrentDirectory(theApp.m_szWorkingFolder);
		ProcessFolder(strFolderPath);
	}    
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::OnFileCreateFace()
{
	// TODO: Add your command handler code here
	CWnd *wnd=AfxGetMainWnd();

	//char szFilters[]=
	//	"Bitmap Image Files (*.bmp)|*.bmp|Jpeg Image Files (*.jpg)|*.jpg|Targa Image Files (*.tga)|*.tga|All Files (*.*)|*.*||";

	char szFilters[]=
		"TXT Files (*.txt)|*.txt|All Files (*.*)|*.*||";

	CFileDialog fileDlg (TRUE, "txt", NULL,0, szFilters, wnd);

	if (fileDlg.DoModal()==IDOK)
	{
		CString pathName = fileDlg.GetPathName();

		// Change the window's title to the saved file's title.
		//CString fileName = fileDlg.GetFileTitle();   
		//SetWindowText(wnd->GetSafeHwnd(),fileName);		

		char szFilename[256];		
		strcpy(szFilename,fileDlg.GetPathName());

		SetCurrentDirectory(theApp.m_szWorkingFolder);		
		//CreateFace(szFilename);
		CreateFace2(szFilename);

	}		
}

// CFaceAnimDoc serialization

void CFaceAnimDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
		char szFilename[256];
		strcpy(szFilename,ar.GetFile()->GetFilePath());
		//CalibrationAVI3(szFilename);
		m_bBatchMode=true;
		m_bCreateCalibrationData=false;
		CalibrationAVI3_2(szFilename);
		exit(0);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::LoadCalibrationFile(const char *szFilename)
{
	FILE *fp=fopen(szFilename,"rt");
	if (!fp)
		return (false);

	int nMarkers;
	char szText[256],szDummy[256];

	fgets(szText,256,fp);
	sscanf(szText,"Markers=%d\n",&nMarkers);
	if (nMarkers!=FA_NUM_MARKERS)
		return (false);

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		fgets(szText,256,fp);
		float fX,fY;
		sscanf(szText,"Joystick=%s Scale=(%f,%f)\n",szDummy,&fX,&fY);
		m_pFace->lst_ref_Markers[k].fScaleX=fX;
		m_pFace->lst_ref_Markers[k].fScaleY=fY;
		assert(stricmp(m_pFace->lst_ref_Markers[k].szName,szDummy)==0);
	} //k

	fclose(fp);
	
	sprintf(szText,"FaceAnim Video Tool - calibration file \"%s\"",szFilename);
	CWnd *wnd=AfxGetMainWnd();
	// Change the window's title to the saved file's title.	
	SetWindowText(wnd->GetSafeHwnd(),szText);		

	return (true);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::EditMarkers(int x,int y,int event)
{
	tFeatureProcessing *tFrame=m_pFace->pCurrFrame;
	cvConvertImage(tFrame->pDrawingFrame, tFrame->pDrawingFrameEdit, 0);

	int nBestMarker=m_nMarkerSelected;	

	if (nBestMarker<0)
	{			
		ftype fBestDist=99999;	

		ftype	fX1=x;
		ftype fY1=y;

		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			if (m_pFace->lst_ref_Markers[k].bExclude)
				continue;
			
			ftype fX2=tFrame->lst_markers[k].x;
			ftype fY2=tFrame->lst_markers[k].y;	

			ftype fDist=sqr(fX1-fX2)+sqr(fY1-fY2);		

			if (fDist<fBestDist)
			{
				fBestDist=fDist;
				nBestMarker=k;
			}
		} //k

		CvPoint p;
		p.x=(int)(tFrame->lst_markers[nBestMarker].x);
		p.y=(int)(tFrame->lst_markers[nBestMarker].y);
		
		cvCircle(tFrame->pDrawingFrameEdit,p,7,CV_RGB(0,255,0), 0);

		char szText[256];
		sprintf(szText,"%s",tFrame->pFace->lst_ref_Markers[nBestMarker].szName);
		cvPutText( tFrame->pDrawingFrameEdit, szText,cvPoint((int)(tFrame->lst_markers[nBestMarker].x),(int)(tFrame->lst_markers[nBestMarker].y)), &tFrame->pFace->font1, CV_RGB(255,255,0) );
	}
	else
	{
		CvPoint p;
		p.x=(int)(x);
		p.y=(int)(y);

		cvCircle(tFrame->pDrawingFrameEdit,p,7,CV_RGB(0,0,0), 0);

	}

	switch( event )
	{
		case CV_EVENT_LBUTTONDOWN:
		{
			if (m_nMarkerSelected<0)
				m_nMarkerSelected=nBestMarker;
			else
			{
				tFrame->lst_markers[nBestMarker].x=x;
				tFrame->lst_markers[nBestMarker].y=y;
				m_bEditOperation=true;

				m_nMarkerSelected=-1;
			}
		}
		break;

		case CV_EVENT_RBUTTONDOWN:
		{
			// abort
			m_nMarkerSelected=-1;
		}
		break;
	}

	cvShowImage(m_szWindowFilename, tFrame->pDrawingFrameEdit);
	cvWaitKey(1);

}

//////////////////////////////////////////////////////////////////////////

// CFaceAnimDoc diagnostics

#ifdef _DEBUG
void CFaceAnimDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFaceAnimDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CFaceAnimDoc commands
