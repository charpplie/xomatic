#include "stdafx.h"
#include <mmsystem.h>
#include "../Polybump.h"							// CreateBumpMap()
#include "PolyBumpApplication.h"
#include "PolyBumpApplicationDoc.h"
#include "MainPanel.h"								// CMainPanel
#include "ResourceCompilerHelper.h"		// CResourceCompilerHelper
#include "OBJFileFormat.h"						// COBJFileFormat


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



inline void Trim(std::string& str, const std::string & ChrsToTrim = " \t\n\r", int TrimDir = 0)
{
  size_t startIndex = str.find_first_not_of(ChrsToTrim);
  if (startIndex == std::string::npos){str.erase(); return;}
  if (TrimDir < 2) str = str.substr(startIndex, str.size()-startIndex);
  if (TrimDir!=1) str = str.substr(0, str.find_last_not_of(ChrsToTrim) + 1);
}

/*
inline void TrimRight(std::string& str, const std::string & ChrsToTrim = " \t\n\r")
{
  Trim(str, ChrsToTrim, 2);
}

inline void TrimLeft(std::string& str, const std::string & ChrsToTrim = " \t\n\r")
{
  Trim(str, ChrsToTrim, 1);
}
*/

// CPolyBumpApplicationDoc

IMPLEMENT_DYNCREATE(CPolyBumpApplicationDoc, CDocument)

BEGIN_MESSAGE_MAP(CPolyBumpApplicationDoc, CDocument)
	ON_COMMAND(ID_DOCUMENT_COMPUTATION, OnDocumentComputation)
	ON_COMMAND(ID_DOCUMENT_EXPORTDATA, OnDocumentExportdata)
	ON_COMMAND(ID_DOCUMENT_STARTCOMPUTATION, OnDocumentStartcomputation)
END_MESSAGE_MAP()


// CPolyBumpApplicationDoc construction/destruction

CPolyBumpApplicationDoc::CPolyBumpApplicationDoc() :m_bUpdateOnIdle(false)
{
	m_WorkerThreadData.m_Properties.m_bLoadHighPolyWithSmoothing=false;		// default for polybumpapp - better is using zBrush models
}

CPolyBumpApplicationDoc::~CPolyBumpApplicationDoc()
{
}

BOOL CPolyBumpApplicationDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	CSRFData *pSRFData = m_WorkerThreadData.GetSRFData();				assert(pSRFData); if(!pSRFData)return FALSE;

	m_WorkerThreadData.m_Properties.SetToSRF(*pSRFData);

	SetModifiedFlag(); 

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CPolyBumpApplicationDoc serialization

void CPolyBumpApplicationDoc::Serialize(CArchive& ar)
{
	assert(0);		// should not be called as we override OnSaveDocument
}


// CPolyBumpApplicationDoc diagnostics

#ifdef _DEBUG
void CPolyBumpApplicationDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPolyBumpApplicationDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CPolyBumpApplicationDoc commands

void CPolyBumpApplicationDoc::OnDocumentComputation()
{
	CSRFData *pSRFData = m_WorkerThreadData.GetSRFData();

	if(!pSRFData)
		MessageBox(0,"Computation in progress","PolyBump",MB_OK);
	else
	{
		CMainPanel m(m_WorkerThreadData.m_Properties);

		m.DoModal();

		m_WorkerThreadData.m_Properties.SetToSRF(*pSRFData);

		UpdateAllViews(0);
	}
}

void CPolyBumpApplicationDoc::OnDocumentExportdata()
{
	if(IsModified())
//	if(GetPathName()=="")
	{
		MessageBox(AfxGetMainWnd()->GetSafeHwnd(),"Document need to be saved first","PolyBump Application",MB_ICONHAND|MB_OK);
		return;
	}


	// start RC
	CResourceCompilerHelper rc;

	if(rc.CallResourceCompiler(GetPathName(),"/refresh") == CResourceCompilerHelper::eRcCallResult_notFound)
		rc.ResourceCompilerUI(AfxGetMainWnd()->GetSafeHwnd());
}






// \param rIn \r\n separated list
std::string ExtractName( std::string &rIn )
{
	std::string InCpy = rIn;
	std::string ret;

	const char *p = InCpy.c_str();

	while(*p!=0 && *p!='\r')
		ret += *p++;

	if(*p!='\r')
	{
		rIn="";
		return ret;
	}

	++p;

	assert(*p=='\n');			if(*p!='\n'){ rIn=""; return ret; }		// input data is corrupted

	++p;

	rIn=p;

	return ret;
}




void CPolyBumpApplicationDoc::OnDocumentStartcomputation()
{
	if(!m_WorkerThreadData.StartComputationSetup(AfxGetMainWnd()->GetSafeHwnd()))
	{
		MessageBox(0,"Computation in progress","PolyBump",MB_OK);
		return;
	}

	SetModifiedFlag();  // mark dirty

//	bool bOk1 = Convert("D:/DATA/Programming/Crytek/new_ts_generator.OBJ",m_WorkerThreadData.GetHighMesh(),false,true);
//	bool bOk2 = Convert("D:/DATA/Programming/Crytek/new_ts_generator.OBJ",m_WorkerThreadData.GetLowMesh(),true,true);

//	bool bOk1 = Convert(m_Properties.m_HighPolyMeshFileName.c_str(),m_WorkerThreadData.GetHighMesh(),false,true);

	{
		COBJFileFormat obj(true);

		if(!obj.LoadOBJ(m_WorkerThreadData.m_Properties.m_LowPolyMeshFileName.c_str(),m_WorkerThreadData.GetLowMesh()))
		{
			m_WorkerThreadData.CancelComputationSetup();
			std::string sError = std::string("Error: low poly mesh '")+m_WorkerThreadData.m_Properties.m_LowPolyMeshFileName+"' cannot be found!";
			MessageBox(0,sError.c_str(),"PolyBump",MB_OK|MB_ICONHAND);
			return;
		}

		if(!m_WorkerThreadData.GetLowMesh().GetUVCount())		// needs UV assignment)
		{
			m_WorkerThreadData.CancelComputationSetup();
			std::string sError = std::string("low poly mesh '")+m_WorkerThreadData.m_Properties.m_LowPolyMeshFileName+"' needs UV mapping!";
			MessageBox(0,sError.c_str(),"PolyBump",MB_OK|MB_ICONHAND);
			return;
		}

//		m_WorkerThreadData.GetLowMesh().ExportAsOBJ("test_low.obj");
	}

	std::string sCageMeshFileName = m_WorkerThreadData.m_Properties.m_CageMeshFileName;

	Trim(sCageMeshFileName);

	if(!sCageMeshFileName.empty())
	{
		COBJFileFormat obj(true);

		if(!obj.LoadOBJ(m_WorkerThreadData.m_Properties.m_CageMeshFileName.c_str(),m_WorkerThreadData.GetCageMesh()))
		{
			m_WorkerThreadData.CancelComputationSetup();
			std::string sError = std::string("Error: cage mesh '")+m_WorkerThreadData.m_Properties.m_CageMeshFileName+"' cannot be found!";
			MessageBox(0,sError.c_str(),"PolyBump",MB_OK|MB_ICONHAND);
			return;
		}

//		m_WorkerThreadData.GetLowMesh().ExportAsOBJ("test_cage.obj");
	}

//	bool bOk2 = Convert(m_Properties.m_LowPolyMeshFileName.c_str(),m_WorkerThreadData.GetLowMesh(),true,true);

	{
		COBJFileFormat obj(m_WorkerThreadData.m_Properties.m_bLoadHighPolyWithSmoothing);

		std::string fullname = m_WorkerThreadData.m_Properties.m_HighPolyMeshFileName;

		for(;;)
		{
			std::string filename = ExtractName(fullname);

			if(filename.empty())
				break;

			CSimpleIndexedMesh mesh;

			if(!obj.LoadOBJ(filename.c_str(),mesh))
			{
				m_WorkerThreadData.CancelComputationSetup();
				std::string sError = std::string("Error: high poly mesh '")+filename+"' cannot be found!";
				MessageBox(0,sError.c_str(),"PolyBump",MB_OK|MB_ICONHAND);
				return;
			}
			else m_WorkerThreadData.PushHighPolyMesh(mesh);
		}
	}



/*
	if(!Convert(m_ip,m_WorkerThreadData.GetHighMesh(),m_sHighPolySceneNames,1,false,true))							// specify channel #1, but its not used
	{
		MessageBox(0,"PickHighPolyMesh failed","PolyBump",MB_OK);
		m_sHighPolySceneNames="";

		SetHighPolyDialogElement();		// update dialog element
		return(false);
	}
*/

	m_bUpdateOnIdle=true;
	m_WorkerThreadData.StartComputation();
	UpdateAllViews(0);
}







BOOL CPolyBumpApplicationDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	CSRFData *pSRFData = m_WorkerThreadData.GetSRFData();

	if(!pSRFData)
	{
		MessageBox(0,"Calculation is progress","Polybump Appliction",MB_OK);
		return FALSE;
	}

	CFileException fe;
	CFile* pFile = NULL;
	pFile = GetFile(lpszPathName, CFile::modeCreate |
		CFile::modeReadWrite | CFile::shareExclusive, &fe);

	if (pFile == NULL)
	{
		ReportSaveLoadException(lpszPathName, &fe,
			TRUE, AFX_IDP_INVALID_FILENAME);
		return FALSE;
	}

	ReleaseFile(pFile, FALSE);

	CWaitCursor wait;
	
	if(!pSRFData->Save(lpszPathName))
	{
		MessageBox(0,"failed to save the document","Polybump Appliction",MB_OK);
		return FALSE;
	}

	m_WorkerThreadData.m_Properties.m_szOutputFilename = lpszPathName;


	SetModifiedFlag(FALSE);     // back to unmodified

	return TRUE;        // success
}


BOOL CPolyBumpApplicationDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	CSRFData *pSRFData = m_WorkerThreadData.GetSRFData();

	if(!pSRFData)
	{
		MessageBox(0,"Calculation is progress","Polybump Appliction",MB_OK);
		return FALSE;
	}

#ifdef _DEBUG
	if (IsModified())
		TRACE(traceAppMsg, 0, "Warning: OnOpenDocument replaces an unsaved document.\n");
#endif

	CFileException fe;
	CFile* pFile = GetFile(lpszPathName,
		CFile::modeRead|CFile::shareDenyWrite, &fe);

	if (pFile == NULL)
	{
		ReportSaveLoadException(lpszPathName, &fe,
			FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		return FALSE;
	}

	ReleaseFile(pFile, FALSE);

	DeleteContents();
	SetModifiedFlag();  // dirty during de-serialize

	CWaitCursor wait;

	if(!pSRFData->Load(lpszPathName))
	{
		MessageBox(0,"failed to load the document","Polybump Appliction",MB_OK);
		return FALSE;
	}

	m_WorkerThreadData.m_Properties.GetFromSRF(*pSRFData);

	m_WorkerThreadData.m_Properties.m_szOutputFilename = lpszPathName;

	SetModifiedFlag(FALSE);     // start off with unmodified

	return TRUE;
}

/*
void CPolyBumpApplicationDoc::OnUserIdle()
{
//	if(!m_bUpdateOnIdle)
//		return;

	if(m_WorkerThreadData.GetSRFData())
	{
		UpdateAllViews(0);
		m_bUpdateOnIdle=false;
	}
}
*/