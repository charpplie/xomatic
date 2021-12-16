/***************************************************************************
*                                                                           *
*                     based on  3DS Max Test Plugin                         *
*                                                                           *
 ***************************************************************************
*                                                                           *
* Version : 1.0ß, March 11th, 1998                                          *
*                                                                           *
* Written By Loic Baumann from Fatal Design, specially for Gamasutra        *
* modifed to PolyBumpPlugin by Martin Mittring                              *
*                                                                           *
 ***************************************************************************
*                                                                           *
* The plugin is using MFCs                                                 *
*                                                                           *
 ***************************************************************************/

#include "stdafx.h"																		// precompiled headers

#ifdef STRICT																					// Already defined by stdafx,
#undef STRICT																					// so we avoid warning linking msg
#endif
#ifdef _MBCS																					// The same as above
#undef _MBCS
#endif

#include <Max.h>																			// MAX's main header file
#include <color.h>																		// MAX color
#include <bmmlib.h>																		// Bitmap lib
#include "STDMAT.H"
#include <map>																				// STL map<,>
#include "../SimpleIndexedMesh.h"											// CSimpleIndexedMesh
#include "PolyBumpPlugin.h"
#include "About.h"
#include "../PolyBump.h"															// CreateBumpMap()
#include "LowMeshDataDlg.h"														// CLowMeshDataDlg
#include "VNormal.h"																	// normal calculation max preferes it
#include "../CopyProtection.h"												// GetSerialFromRegistry()
#include "ResourceCompilerHelper.h"										// CResourceCompilerHelper
#include "Cry_Math.h"																	// CalcAngleBetween()

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

#define DLLEXPORT __declspec(dllexport)	// Facilite Function's Exportation Declaration

CPolyBumpPluginApp	theApp;							// Global Object representing the application
CPlugPanel					thePlugPanel;				// The One and only One Plug Panel object
PlugDesc						thePlugDesc;

/////////////////////////////////////////////////////////////////////////////
// CPolyBumpPluginApp

BEGIN_MESSAGE_MAP(CPolyBumpPluginApp, CWinApp)
	//{{AFX_MSG_MAP(CPolyBumpPluginApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPolyBumpPluginApp construction

CPolyBumpPluginApp::CPolyBumpPluginApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// DLL's InitInstance, called when the DLL is being linked to a client
BOOL CPolyBumpPluginApp::InitInstance() 
{
/*
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_DELAY_FREE_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
*/

	//Get the DLL's HINSTANCE
	thePlugPanel.m_hInstance = AfxGetResourceHandle();

	//Init 3DS Max Custom Controls
	InitCustomControls(thePlugPanel.m_hInstance);

	//Init Win32 Controls
	InitCommonControls();

	GetSerialFromRegistry();

	return TRUE;				//CWinApp::InitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// DLL's ExitInstance, called when the DLL is being unlinked from a client
int CPolyBumpPluginApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// 3DS Max Identification Callbacks
DLLEXPORT const TCHAR* LibDescription ()
{ 
	return _T("PolyBump Plugin"); 
}

/////////////////////////////////////////////////////////////////////////////
// Return the Number of Classes present in the DLLs
DLLEXPORT int LibNumberClasses ()
{ 
	return 1;					//Return 1 because this DLL contains 1 plugin
}

/////////////////////////////////////////////////////////////////////////////
//Return the i'th Class Description Object
DLLEXPORT ClassDesc* LibClassDesc(int i)
{
	switch(i) 
	{
    case  0:
			return &thePlugDesc;	//Return the Plugin's Description Object
			break;

    default:
			return 0;
      break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// MAX API Version
DLLEXPORT ULONG LibVersion ()
{ 
	return (VERSION_3DSMAX);
}



/***************************************************************************
*                                                                           *
*                            Plugin Panel Class                             *
*                                                                           *
 ***************************************************************************/
// This class is the main class of the plugin (like the Dialog's class for
// dialog based application)
//
// A unique object of this class (called thePlugPanel) is globally instanced

////////////////////////////////////////////////////////////////////////////////
//Constructor
CPlugPanel::CPlugPanel()
{
	// interface to 3DStudio
	m_iu = NULL;
	m_ip = NULL;

	m_iChannelNo=1;									// uv channel #1
}

////////////////////////////////////////////////////////////////////////////////
//Destructor
CPlugPanel::~CPlugPanel()
{
	thePlugPanel.EndProcess();
}


////////////////////////////////////////////////////////////////////////////////
// CPlugPanel Dialog Procedure to handle User and System Interactions
static INT_PTR CALLBACK PlugPanelProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch (msg)
	{
		case WM_INITDIALOG:							// Panel's extra initialization
			thePlugPanel.Init(hWnd); 
			break;

		case WM_DESTROY:								// Panel's extra destruction
//			thePlugPanel.EndProcess();	// don´t stop the work when the artist hides the panel
			thePlugPanel.Destroy(hWnd);
			break;

		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{ 
				case TTN_NEEDTEXT: 
				{ 
					UINT idButton;
					LPTOOLTIPTEXT lpttt; 

					lpttt = (LPTOOLTIPTEXT) lParam; 
					lpttt->hinst = NULL;
					idButton = lpttt->hdr.idFrom; 

					char ToolTipString[256];

					switch (idButton)
					{ 
						case IDC_PICKHIGHPOLY: 
							sprintf(ToolTipString, "%s", "Button One");
							lpttt->lpszText = ToolTipString; 
							break; 
					} 
				} 
				break;
			}
			break; 


		case WM_POLYBUMPTHREADFINISHED:
			thePlugPanel.OnProcessFinished();
			break;

		case WM_COMMAND :
			if(HIWORD(wParam) == EN_SETFOCUS)
				DisableAccelerators();
			 else if(HIWORD(wParam) == EN_KILLFOCUS)
				EnableAccelerators();

			switch (LOWORD(wParam)) 			// Handle Panel's controls
			{
				case IDC_HITNEAREST:		thePlugPanel.m_Properties.m_nHitMode=0;	break;
				case IDC_HITLATEST:			thePlugPanel.m_Properties.m_nHitMode=1;	break;			

				case IDC_HORIZONMODE:
					{
						HWND dlgitem=GetDlgItem(hWnd,LOWORD(wParam));

						thePlugPanel.m_Properties.m_iHorizonMode = ComboBox_GetCurSel(dlgitem);
					}
					thePlugPanel.SetStates();
					break;

				case IDC_OUTPUTSIZEX:
					{
						HWND dlgitem=GetDlgItem(hWnd,LOWORD(wParam));

						thePlugPanel.m_Properties.m_iTextureSizePX = ComboBox_GetCurSel(dlgitem);
					}
					break;


				case IDC_THREADPRIORITY:
					{
						HWND dlgitem=GetDlgItem(hWnd,LOWORD(wParam));

						thePlugPanel.m_Properties.m_iThreadPrio = ComboBox_GetCurSel(dlgitem)-2;
					}
					break;

				case IDC_OUTPUTSIZEY:
					{
						HWND dlgitem=GetDlgItem(hWnd,LOWORD(wParam));

						thePlugPanel.m_Properties.m_iTextureSizePY = ComboBox_GetCurSel(dlgitem);
					}
					break;
			
				case IDC_ANTIALIASING:
					{
						HWND dlgitem=GetDlgItem(hWnd,LOWORD(wParam));

						thePlugPanel.m_Properties.m_iAntiAliasingMode = ComboBox_GetCurSel(dlgitem);
					}
					break;

				case IDC_RAYLENGTH:
					{
						HWND dlgitem2=::GetDlgItem(hWnd,LOWORD(wParam));

						thePlugPanel.m_Properties.m_fRayLength=0.06f;
						char str[80];
						GetWindowText(dlgitem2,str,80);

						sscanf(str,"%f",&thePlugPanel.m_Properties.m_fRayLength);
						thePlugPanel.m_Properties.m_fRayLength*=0.01f;
						if(thePlugPanel.m_Properties.m_fRayLength<0.0f)				// make positive
							thePlugPanel.m_Properties.m_fRayLength=-thePlugPanel.m_Properties.m_fRayLength;
					}
					break;

				case IDC_ABOUT: 
					{
						CAbout dlg;
						dlg.DoModal();
					}
					break;

				case IDC_EXPORTSELECTION:
					thePlugPanel.ExportSelection();
					break;

				case IDC_PICKHIGHPOLY:
					thePlugPanel.PickHighPolyName();
					break;

				case IDC_PICKCAGE:
					thePlugPanel.PickCageName();
					break;

				case IDC_DEBUGBUTTON:
					thePlugPanel.m_Properties.m_bPrintSummary=true;
					thePlugPanel.m_Properties.m_bOutputDebugInfo=true;
					break;

				case IDC_PICKPROJECTION:
					// todo
					break;

				case IDC_PICKLOWPOLY:
					{
						CLowMeshDataDlg dlg;

						int nRet = dlg.DoModal();

						// Handle the return value from DoModal.
						switch ( nRet )
						{
							case -1: 
								AfxMessageBox("Dialog box could not be created!");
								break;
							case IDOK:	
								{
									thePlugPanel.PickLowPolyName(dlg.m_iChannelNo,dlg.m_iMaterialId);
								}
							break;
							case IDABORT:
							case IDCANCEL:
								break;
							default:
								break;
						};
					}
					break;

				case IDC_OPENEXISTINGSRF:
					thePlugPanel.OpenExistingSRF();
					break;

				case IDC_STARTPROCESS:
						thePlugPanel.StartProcess();
					break;

				case IDC_ENDPROCESS:
					thePlugPanel.EndProcess();
					break;
			}
			break;

		////////////////////////////////////////////////////////////////////////
		//The following lines are not necessary for MAX2 Plugins
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			thePlugPanel.m_ip->RollupMouseMessage(hWnd, msg, wParam, lParam);
			break;
		////////////////////////////////////////////////////////////////////////

		default:
			return FALSE;
	}
	return TRUE;
}



////////////////////////////////////////////////////////////////////////////////
// Display the Plugin's Panel
void CPlugPanel::BeginEditParams( Interface* ip, IUtil* iu )
{
	m_ip = ip;								// Get the Plugin's Interface
	m_iu = iu;								// Get the Utility Plugin's Interface

	//Display the plugin's Rollup Dialog
	m_ip->AddRollupPage(m_hInstance, MAKEINTRESOURCE(IDD),
		PlugPanelProc, _T("PolyBump Plugin"),0);
}

////////////////////////////////////////////////////////////////////////////////
// Free the Plugins' Panel
void CPlugPanel::EndEditParams( Interface *ip, IUtil *iu )
{
	//Remove the Rollup Dialog
	ip->DeleteRollupPage(m_hPanel);
	m_hPanel = NULL;

	m_iu = 0;m_ip = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Panel Initialization callback
void CPlugPanel::Init( HWND hWnd )
{	
	m_hPanel=hWnd;
	SelectionSetChanged(m_ip,m_iu);

	{
		HWND dlgitem=GetDlgItem(m_hPanel,IDC_OUTPUTSIZEX);

		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"8");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"16");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"32");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"64");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"128");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"256");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"512");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"1024");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"2048");

		ComboBox_SetCurSel(dlgitem,m_Properties.m_iTextureSizePX);													// set default
	}

	{
		HWND dlgitem=GetDlgItem(m_hPanel,IDC_THREADPRIORITY);

		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"-2 (minimum)");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"-1");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)" 0 (normal)");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"+1");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"+2 (maximum)");

		ComboBox_SetCurSel(dlgitem,m_Properties.m_iThreadPrio+2);													// set default
	}

	{
		HWND dlgitem=GetDlgItem(m_hPanel,IDC_OUTPUTSIZEY);

		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"8");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"16");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"32");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"64");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"128");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"256");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"512");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"1024");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"2048");

		ComboBox_SetCurSel(dlgitem,m_Properties.m_iTextureSizePY);													// set default
	}

	{
		HWND dlgitem=GetDlgItem(m_hPanel,IDC_ANTIALIASING);

		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"none");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"sel 2x2");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"sel 3x3");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"full 3x3");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"full 4x4");

		ComboBox_SetCurSel(dlgitem,m_Properties.m_iAntiAliasingMode);												// set default
	}

	{
		HWND dlgitem=GetDlgItem(m_hPanel,IDC_HORIZONMODE);

		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"none");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"low");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"med");
		SendMessage(dlgitem,CB_ADDSTRING,0,(LPARAM)"high");

		ComboBox_SetCurSel(dlgitem,m_Properties.m_iHorizonMode);												// set default
	}

	{
		HWND dlgitem=GetDlgItem(m_hPanel,IDC_HITNEAREST);

		Button_SetCheck(dlgitem,m_Properties.m_nHitMode==0 ? 1 : 0); 

		dlgitem=GetDlgItem(m_hPanel,IDC_HITLATEST);

		Button_SetCheck(dlgitem,m_Properties.m_nHitMode==1 ? 1 : 0); 
	}

#define SETCHECKBOX(id,var)																\
	{																												\
		HWND dlgitem=GetDlgItem(m_hPanel,id);									\
		Button_SetCheck(dlgitem,m_Properties.##var?1:0);			\
	}

		{
		HWND dlgitem=::GetDlgItem(m_hPanel,IDC_RAYLENGTH);
		char str[80];

		sprintf(str,"%.2f",m_Properties.m_fRayLength*100.0f);
		SetWindowText(dlgitem,str);						// 10% ray length
	}

	SetHighPolyDialogElement();																				// update high poly dialog element
	SetCageDialogElement();																				// update high poly dialog element

	{
		HWND dlgitem=::GetDlgItem(m_hPanel,IDC_LOWPOLYNAME);

		SetWindowText(dlgitem,m_sLowPolySceneName.c_str());							// update low poly dialog element
	}

	{
		HWND dlgitem=::GetDlgItem(m_hPanel,IDC_PROJECTIONNAME);

		SetWindowText(dlgitem,m_sProjectionPolySceneName.c_str());							// update low poly dialog element
	}

	SetStates();

	ShowWindow(GetDlgItem(m_hPanel,IDC_LINE1),SW_HIDE);
	ShowWindow(GetDlgItem(m_hPanel,IDC_LINE2),SW_HIDE);
	ShowWindow(GetDlgItem(m_hPanel,IDC_LINE3),SW_HIDE);
	ShowWindow(GetDlgItem(m_hPanel,IDC_LINE4),SW_HIDE);
}

////////////////////////////////////////////////////////////////////////////////
// Panel Destruction callback
void CPlugPanel::Destroy( HWND hWnd )
{
}


void CPlugPanel::SelectionSetChanged( Interface *ip, IUtil *iu )
{
	assert(m_hPanel);

	SetStates();		// grey or ungrey some dialog elements
		
	if(ip->GetSelNodeCount()==1)
	{
		::SetDlgItemText(m_hPanel,IDC_SELECTIONNAME,ip->GetSelNode(0)->GetName());
	}
	else if(ip->GetSelNodeCount())
	{
		::SetDlgItemText(m_hPanel,IDC_SELECTIONNAME,"<multiple objects>");
	}
	else 
	{
		::SetDlgItemText(m_hPanel,IDC_SELECTIONNAME,"<none>");
	}	
}


void CPlugPanel::SetStates()
{
	if(m_ip->GetSelNodeCount()==1)
		::EnableWindow(::GetDlgItem(m_hPanel,IDC_PICKLOWPOLY),TRUE);
	 else 
		::EnableWindow(::GetDlgItem(m_hPanel,IDC_PICKLOWPOLY),FALSE);

	if(m_ip->GetSelNodeCount()>=1)
	{
		::EnableWindow(::GetDlgItem(m_hPanel,IDC_EXPORTSELECTION),TRUE);
		::EnableWindow(::GetDlgItem(m_hPanel,IDC_PICKHIGHPOLY),TRUE);
	}
	else 
	{
		::EnableWindow(::GetDlgItem(m_hPanel,IDC_EXPORTSELECTION),FALSE);
		::EnableWindow(::GetDlgItem(m_hPanel,IDC_PICKHIGHPOLY),FALSE);
	}
}


std::string CPlugPanel::GetNameFromMultiSelection()
{
	std::string ret;
	DWORD dwCount=m_ip->GetSelNodeCount();

	for(DWORD dwI=0;dwI<dwCount;dwI++)
	{
		ret += m_ip->GetSelNode(dwI)->GetName();

		if(dwI!=dwCount-1)ret += "\n";
	}

	return(ret);
}


class CTriNormIndex
{
public:
	DWORD p[3];							//!< index in m_BaseVectors
};

// helper to get order for CObjNorm
struct NormalCompare: public std::binary_function< CObjNorm, CObjNorm, bool>
{
	bool operator() ( const CObjNorm &a, const CObjNorm &b ) const
	{
		// first sort by x
		if(a.x<b.x)return true;
		if(a.x>b.x)return false;

		// then by y
		if(a.y<b.y)return true;
		if(a.y>b.y)return false;

		// then by z
		if(a.z<b.z)return true;
		if(a.z>b.z)return false;

		return false;
	}
};



void WeldSimilarNormals( CSimpleIndexedMesh &inoutMesh )
{
	if(!inoutMesh.m_pNorms)return;		// nothing to do

	std::vector<CTriNormIndex>			NormIndx;			//!< normal indices for each triangle
	std::vector<Vec3>								NormVec;

	// remap the normals (weld them)

	// assume more to avoid resize of container
	NormIndx.reserve(inoutMesh.m_FaceCount);
	NormVec.reserve(inoutMesh.m_FaceCount);

	std::map<CObjNorm,DWORD,NormalCompare>	mapNormalsToNumber;
	DWORD dwmapSize=0;

	// for every triangle
	for(DWORD i=0;i<(DWORD)(inoutMesh.m_FaceCount);i++)
	{
		CTriNormIndex idx;

		// for every vertex of the triangle
		for(DWORD e=0;e<3;e++)
		{
			int iNorm=inoutMesh.m_pFaces[i].n[e];

			CObjNorm &vNorm=inoutMesh.m_pNorms[iNorm];

			std::map<CObjNorm,DWORD,NormalCompare>::iterator iFind=mapNormalsToNumber.find(vNorm);

			if(iFind==mapNormalsToNumber.end())				// not found
			{
				NormVec.push_back(Vec3(vNorm.x,vNorm.y,vNorm.z));
				idx.p[e]=dwmapSize;
				mapNormalsToNumber[vNorm]=dwmapSize;
				dwmapSize++;
			}
			else idx.p[e]=(*iFind).second;
		}

		NormIndx.push_back(idx);
	}

	// write data back
	{
		for(DWORD i=0;i<(DWORD)(inoutMesh.m_FaceCount);i++)
		{
			inoutMesh.m_pFaces[i].n[0]=NormIndx[i].p[0];
			inoutMesh.m_pFaces[i].n[1]=NormIndx[i].p[1];
			inoutMesh.m_pFaces[i].n[2]=NormIndx[i].p[2];
		}

		// realloc
		free(inoutMesh.m_pNorms);
		inoutMesh.m_NormCount=NormVec.size();
		inoutMesh.m_pNorms=(CObjNorm *)malloc(inoutMesh.m_NormCount*sizeof(CObjNorm));

		for(DWORD i=0;i<(DWORD)(inoutMesh.m_NormCount);i++)
			inoutMesh.m_pNorms[i]=CObjNorm(NormVec[i].x,NormVec[i].y,NormVec[i].z);
	}
}




void CPlugPanel::ExportSelection()
{
	std::string sSelectionString=GetNameFromMultiSelection();

	CSimpleIndexedMesh simplemesh;

	if(!Convert(m_ip,simplemesh,sSelectionString,1,false,true))							// specify channel #1, but its not used
	{
		AfxMessageBox("ExportSelection failed1");
		return;
	}

	OPENFILENAME ofn;

	memset(&ofn,0,sizeof(OPENFILENAME));
	char filename[1024]="";

	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=0;		// Window Handle
	ofn.hInstance=0;		// Application Instance
	ofn.lpstrFilter="OBJ Files (*.OBJ)\0*.OBJ\0";
	ofn.lpstrDefExt="OBJ";
	ofn.lpstrFile=filename;
	ofn.nMaxFile=1024;
	ofn.lpstrTitle="Export Selection (.OBJ)";
	ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;

	if(!GetSaveFileName(&ofn))
		return;

	if(!simplemesh.ExportAsOBJ(filename,true))
		AfxMessageBox("ExportSelection failed2");
}

void CPlugPanel::PickHighPolyName()
{
	assert(m_ip->GetSelNodeCount()!=0);		if(m_ip->GetSelNodeCount()==0)return;

	m_sHighPolySceneNames=GetNameFromMultiSelection();
	SetHighPolyDialogElement();		// update dialog element

	OutputDebugString("PickHighPolyName: ");
	OutputDebugString(m_sHighPolySceneNames.c_str());
	OutputDebugString("\n");
}


void CPlugPanel::PickCageName()
{
	assert(m_ip->GetSelNodeCount()!=0);		if(m_ip->GetSelNodeCount()==0)return;

	m_sCageSceneNames=GetNameFromMultiSelection();
	SetCageDialogElement();		// update dialog element

	OutputDebugString("PickCageName: ");
	OutputDebugString(m_sCageSceneNames.c_str());
	OutputDebugString("\n");
}


bool CPlugPanel::PickHighPolyMesh()
{
	CSimpleIndexedMesh mesh;

	if(!Convert(m_ip,mesh,m_sHighPolySceneNames,1,false,true))							// specify channel #1, but its not used
	{
		MessageBox(0,"PickHighPolyMesh failed","PolyBumpPlugin",MB_OK);
		m_sHighPolySceneNames="";

		SetHighPolyDialogElement();		// update dialog element
		return(false);
	}

	m_WorkerThreadData.PushHighPolyMesh(mesh);

	return(true);
}


bool CPlugPanel::PickCageMesh()
{
	if(!Convert(m_ip,m_WorkerThreadData.GetCageMesh(),m_sCageSceneNames,1,false,true))							// specify channel #1, but its not used
	{
		MessageBox(0,"PickCageMesh failed","PolyBumpPlugin",MB_OK);
		m_sCageSceneNames="";

		SetCageDialogElement();		// update dialog element
		return(false);
	}
	return(true);
}



void CPlugPanel::SetHighPolyDialogElement()
{
	if(strstr(m_sHighPolySceneNames.c_str(),"\n"))																// multiple high poly objects
		::SetDlgItemText(m_hPanel,IDC_HIGHPOLYNAME,"<multiple objects>");
	else																																					// single high poly object
		::SetDlgItemText(m_hPanel,IDC_HIGHPOLYNAME,m_sHighPolySceneNames.c_str());
}



void CPlugPanel::SetCageDialogElement()
{
	if(strstr(m_sCageSceneNames.c_str(),"\n"))																		// multiple high poly objects
		::SetDlgItemText(m_hPanel,IDC_CAGENAME,"<multiple objects>");
	else																																					// single high poly object
		::SetDlgItemText(m_hPanel,IDC_CAGENAME,m_sCageSceneNames.c_str());
}


void CPlugPanel::PickLowPolyName( const int iniChannelNo, const int iniMaterialId )
{
	assert(m_ip->GetSelNodeCount()==1);		if(m_ip->GetSelNodeCount()!=1)return;

	m_sLowPolySceneName = m_ip->GetSelNode(0)->GetName();
	m_iChannelNo = iniChannelNo;
	m_Properties.m_LowPolyMaterialName="";

	if(m_ip->GetSelNode(0)->GetMtl())
	{
		if(iniMaterialId!=-1)		// not all, but a specific material
		{
			Mtl *pSubMat = m_ip->GetSelNode(0)->GetMtl()->GetSubMtl(iniMaterialId);

			if(pSubMat)
			{
				MAXSTR &sMainMatName = pSubMat->GetName();

				m_Properties.m_LowPolyMaterialName=sMainMatName;
			}
			else
			{
				MessageBox(0,"Sub material requires name","Error",MB_OK);
				m_sLowPolySceneName="";
			}
		}
	}
	else 
	{
		MessageBox(0,"Object requires material assignment","Error",MB_OK);
		m_sLowPolySceneName="";
	}

	::SetDlgItemText(m_hPanel,IDC_LOWPOLYNAME,m_sLowPolySceneName.c_str());
}



bool CPlugPanel::PickLowPolyMesh()
{
	if(!Convert(m_ip,m_WorkerThreadData.GetLowMesh(),m_sLowPolySceneName,
		m_iChannelNo,																																// the chosen channel is used
		true,true))															
	{
		m_sLowPolySceneName="";
		HWND dlgitem=::GetDlgItem(m_hPanel,IDC_LOWPOLYNAME);
		SetWindowText(dlgitem,m_sLowPolySceneName.c_str());							// low poly name
		return(false);
	}

	return(true);
}



// calculates the normals for every vertex of a triangle
void GetVFLst( Mesh* dmesh, Tab<VNormal>* vnorms, Tab<Point3>* fnorms )	 
{
	DWORD nv=dmesh->getNumVerts();	
	DWORD nf=dmesh->getNumFaces();	
  
	(*fnorms).Resize(nf);
  (*fnorms).SetCount(nf);
  (*vnorms).Resize(nv);
  (*vnorms).SetCount(nv);
  
	Face *face = dmesh->faces;
  
	for (DWORD i=0; i<nv; i++) 
    (*vnorms)[i] = VNormal();

  for(DWORD i=0; i<(DWORD)dmesh->getNumFaces(); i++,face++) 		// Calculate the surface normal
  {
		Vec3 v[3];
	
		for(uint32 e=0;e<3;++e)
			v[e] = Vec3(dmesh->verts[face->v[e]].x,dmesh->verts[face->v[e]].y,dmesh->verts[face->v[e]].z);
		
		assert(i<(DWORD)dmesh->getNumFaces());

		Vec3 vNorm=(v[1]-v[0])^(v[2]-v[1]);

		(*fnorms)[i] = Point3(vNorm.x,vNorm.y,vNorm.z);
		(*fnorms)[i]=(*fnorms)[i].Normalize();
		
		assert(face->v[0]<nv);
		assert(face->v[1]<nv);
		assert(face->v[2]<nv);

		for(uint32 j=0; j<3; j++)
		{
			Vec3 vVecA=v[(j+2)%3] - v[j];
			Vec3 vVecB=v[(j+1)%3] - v[j];
			
			float angle = acos(vVecA.GetNormalizedSafe() * vVecB.GetNormalizedSafe());//Vec3::CalcAngleBetween( vVecA,vVecB );

			(*vnorms)[face->v[j]].AddNormal((*fnorms)[i]*angle,face->smGroup);
		}
  }

//	for(i=0; i<nv; i++) 
//		(*vnorms)[i].Normalize();
}


// 3dsmax doeas not have a transpose function
Matrix3 GetTranspose33( const Matrix3 &mMat )
{
	Matrix3 ret;

	for(int i=0;i<3;i++)
	{
		Point3 p=mMat.GetColumn3(i);
		ret.SetRow(i,p);
	}

	ret.SetRow(3,Point3(0,0,0));

	return ret;
}



// convert the 3DStudio Mesh to an internal, easy to use format
// \return build was successful
// /param inipSrc interface pointer to 3DStudio MAX (selection is the input object)
// /param outpDst output mesh, in our simple internal format
// /param iniTexChannel use this texture channel no
// /param iniMaterialId -1 means every MaterialID is allowed, 0.. otherwise
// /param inbNeedMapping true: show error if there is no mapping applied, false:otherwise
// /param inbNeedNormals true: recreate normals if they are not generated, false: normals doesn´t matter
bool Convert( Interface *inipSrc, CSimpleIndexedMesh &outpDst, std::string insName, int iniTexChannel, bool inbNeedMapping, bool inbNeedNormals )
{
//	assert(inipSrc->GetSelNodeCount()!=0);		if(inipSrc->GetSelNodeCount()==0)return(false);

	outpDst.FreeData();	

	int whole_numverts=0;
	int whole_numtverts=0;
	int whole_numfaces=0;
	int whole_nummatid=0;


	// 1st Pass (count needs)---------------------------------------------------------------------------------------------------------------------
	{
		char *sNameWalker=(char *)insName.c_str();

		for(;;)	// for all selected objects
		{
			std::string sObjName;

			// every '\n' within the name means a different selected object
			while(*sNameWalker)
			{
				if(*sNameWalker=='\n'){ sNameWalker++;break; }
				sObjName+=*sNameWalker++;
			}

			if(sObjName=="")
				break;	// no more selected objects

			OutputDebugString("Convert object :\"");
			OutputDebugString(sObjName.c_str());
			OutputDebugString("\"\n");

			INode *inode=inipSrc->GetINodeByName(sObjName.c_str());		if(!inode)return(false);														// no mesh was specified

			Object *obj = inode->EvalWorldState(0).obj;				// 0=time
			
			assert(obj);		if(!obj)return(false);

			if(obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
			{ 
				TriObject *tri = (TriObject *) obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));	assert(tri);			// 0=time
				// Note that the TriObject should only be deleted
				// if the pointer to it is not equal to the object
				// pointer that called ConvertToType()
				bool bShouldIDelete = (obj!=tri);

				Mesh& mesh=tri->GetMesh();

				whole_numverts+=mesh.getNumVerts();
				whole_numtverts+=mesh.getNumMapVerts(iniTexChannel);

				int nummaxfaces=mesh.getNumFaces();

				whole_numfaces+=nummaxfaces;

				{
					Face *face=mesh.faces;

					for(int i=0;i<nummaxfaces;i++,face++)
					{
						int shader_id=(DWORD)(face->getMatID());			// material id

						if(shader_id+1>whole_nummatid)
							whole_nummatid=shader_id+1;
					}
				}

				{
					char str[256];

					sprintf(str,"Triangles : %d\n",whole_numfaces);
					OutputDebugString(str);
				}

				if(bShouldIDelete)delete tri;
			}
			else return(false);		// cant convert to triangle object
		}
	}

	// allocate buffer -----------------------------------------------------------------------------------------------------------------------------

	assert(outpDst.m_CoorCount==0);
	assert(outpDst.m_NormCount==0);

	// allocate
	outpDst.m_pVerts=(CObjVert *)malloc(sizeof(CObjVert)*whole_numverts);
	if(whole_numtverts)outpDst.m_pCoors=(CObjCoor *)malloc(sizeof(CObjCoor)*whole_numtverts);
	if(inbNeedNormals)outpDst.m_pNorms=(CObjNorm *)malloc(sizeof(CObjNorm)*whole_numfaces*3);
	outpDst.m_pFaces=(CObjFace *)malloc(sizeof(CObjFace)*whole_numfaces);

	// materials
	if(whole_nummatid)outpDst.m_pMaterials=new CSimpleMaterial[whole_nummatid];
	outpDst.m_MaterialCount=whole_nummatid;


	// set
	outpDst.m_VertCount=whole_numverts;
	if(whole_numtverts)outpDst.m_CoorCount=whole_numtverts;
	if(inbNeedNormals)outpDst.m_NormCount=whole_numfaces*3;
	outpDst.m_FaceCount=whole_numfaces;

	CObjFace *FaceWritePtr=outpDst.m_pFaces;
	CObjVert *VertWritePrt=outpDst.m_pVerts;
	CObjNorm *NormWritePtr=outpDst.m_pNorms;
	CObjCoor *UVWritePtr=outpDst.m_pCoors;

	DWORD dwStartVertIndex=0;
	DWORD dwStartNormIndex=0;
	DWORD dwStartUVIndex=0;

	// 2nd Pass (fill buffers) ---------------------------------------------------------------------------------------------------------------------
	{
		char *sNameWalker=(char *)insName.c_str();

		for(;;)	// for all selected objects
		{
			CObjFace *ObjStartFacePtr=FaceWritePtr;
			std::string sObjName;

			// every '\n' within the name means a different selected object
			while(*sNameWalker)
			{
				if(*sNameWalker=='\n'){ sNameWalker++;break; }
				sObjName+=*sNameWalker++;
			}

			if(sObjName=="")
				break;	// no more selected objects

			INode *inode=inipSrc->GetINodeByName(sObjName.c_str());

			if(!inode)return(false);														// no mesh was specified

			Object *obj = inode->EvalWorldState(0).obj;				// 0=time

			if(obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
			{ 
				TriObject *tri = (TriObject *) obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));	assert(tri);		// 0=time
				// Note that the TriObject should only be deleted
				// if the pointer to it is not equal to the object
				// pointer that called ConvertToType()
				bool bShouldIDelete = (obj!=tri);

				Mesh& mesh=tri->GetMesh();

				bool mappedobject=mesh.mapFaces(iniTexChannel)!=0;			//(mesh.getTVertPtr(0)!=0);

				if(inbNeedMapping && !mappedobject)
				{
					AfxMessageBox("Object has no mapping coordinates");

					if(bShouldIDelete)delete tri;

					return(false);
				}

				bool normalsthere=(mesh.normalsBuilt!=0);

				// recreate normals if necessary
				if(inbNeedNormals)
				{ 
					mesh.buildRenderNormals();normalsthere=true;
				}

				Tab<VNormal> vnorms;
				Tab<Point3> fnorms;
				GetVFLst(&mesh,&vnorms,&fnorms);

				int nummaxfaces=mesh.getNumFaces();

				Matrix3 mTransformMatrix=inode->GetObjectTM(0);										// 0=time
				bool bTransformMatrixFlipped= mTransformMatrix.Parity()==TRUE;		// fix problems with mirrored objects (strange behaviour of max)

				// extract triangle (index for vertex and normal)
				{
					Face *face=mesh.faces;
					int n=0;

					for(int i=0;i<nummaxfaces;i++,face++)
					{
						FaceWritePtr->CObjFace::CObjFace();	// call constructor
		#ifdef USE_SMOOTH_TANGENTSPACE_BY_SMOOTHINGGROUPS
						FaceWritePtr->m_smoothinggroup=face->getSmGroup();			// smoothing group
		#endif

						FaceWritePtr->shader_id=(DWORD)(face->getMatID());			// material id

						Mtl *pMtl=inode->GetMtl();																								// object material

						if(pMtl)
						if(pMtl->ClassID() == Class_ID(MULTI_CLASS_ID, 0)) 
						{
							pMtl = pMtl->GetSubMtl(FaceWritePtr->shader_id % pMtl->NumSubMtls());		// material from the material id
						}

						CSimpleMaterial &refOutMat=outpDst.m_pMaterials[FaceWritePtr->shader_id];

						if(pMtl)
						if(refOutMat.m_iMaterialID==-1)
						{
							MAXSTR &sMainMatName = pMtl->GetName();

//							MessageBox(0,sMainMatName,"1",MB_OK);

							refOutMat.m_sMaterialName=sMainMatName;
							refOutMat.m_iMaterialID=FaceWritePtr->shader_id;
							
							Color cAmb=pMtl->GetAmbient();
							Color cDiff=pMtl->GetDiffuse();
							Color cSpec=pMtl->GetSpecular();

							refOutMat.m_AmbientColor[0]=cAmb.r;
							refOutMat.m_AmbientColor[1]=cAmb.g;
							refOutMat.m_AmbientColor[2]=cAmb.b;

							refOutMat.m_DiffuseColor[0]=cDiff.r;
							refOutMat.m_DiffuseColor[1]=cDiff.g;
							refOutMat.m_DiffuseColor[2]=cDiff.b;

							refOutMat.m_SpecularColor[0]=cSpec.r;
							refOutMat.m_SpecularColor[1]=cSpec.g;
							refOutMat.m_SpecularColor[2]=cSpec.b;

							refOutMat.m_fShininess=pMtl->GetShininess();
							refOutMat.m_fShiniStrength=pMtl->GetShinStr();

							refOutMat.m_fSelfIllum=pMtl->GetSelfIllum();
							refOutMat.m_fTransparency=pMtl->GetXParency();

							if(Texmap *tmap = pMtl->GetSubTexmap(ID_DI))
							if(tmap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) 
							{
								BitmapTex *bmt = (BitmapTex*) tmap;

								if(bmt)refOutMat.m_sDiffuseTexturePath=bmt->GetMapName();
							}

							if(Texmap *tmap = pMtl->GetSubTexmap(ID_BU))
							if(tmap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) 
							{
								BitmapTex *bmt = (BitmapTex*) tmap;

								if(bmt)refOutMat.m_sBumpTexturePath=bmt->GetMapName();
							}
						}

						if(!bTransformMatrixFlipped)
						{
							FaceWritePtr->v[0]=dwStartVertIndex+face->v[0];				// index to vertices
							FaceWritePtr->v[1]=dwStartVertIndex+face->v[1];
							FaceWritePtr->v[2]=dwStartVertIndex+face->v[2];
							FaceWritePtr->n[0]=dwStartNormIndex+n*3;							// index to normals
							FaceWritePtr->n[1]=dwStartNormIndex+n*3+1;
							FaceWritePtr->n[2]=dwStartNormIndex+n*3+2;
						}
						else
						{
							FaceWritePtr->v[0]=dwStartVertIndex+face->v[2];				// index to vertices
							FaceWritePtr->v[1]=dwStartVertIndex+face->v[1];
							FaceWritePtr->v[2]=dwStartVertIndex+face->v[0];
							FaceWritePtr->n[0]=dwStartNormIndex+n*3+2;						// index to normals
							FaceWritePtr->n[1]=dwStartNormIndex+n*3+1;
							FaceWritePtr->n[2]=dwStartNormIndex+n*3;
						}
						FaceWritePtr++;n++;
					}
				}

				// extract vertices (index for texture)
				if(mappedobject)
				{
					TVFace *tvface=mesh.mapFaces(iniTexChannel);
					Face *face=mesh.faces;

					if(tvface)
					if(face)
					{
						CObjFace *dst=ObjStartFacePtr;

						for(int i=0;i<nummaxfaces;i++,tvface++,face++)
						{
							MtlID MatId=face->getMatID();			// typedef unsigned short MtlID

							DWORD a=tvface->t[0]+dwStartUVIndex, b=tvface->t[1]+dwStartUVIndex, c=tvface->t[2]+dwStartUVIndex;

							if(!bTransformMatrixFlipped)
							{
								dst->t[0]=a;dst->t[1]=b;dst->t[2]=c;
							}
							else
							{
								dst->t[2]=a;dst->t[1]=b;dst->t[0]=c;
							}
							dst++;
						}
					}
				}


				// extract vertex position
				{
					int numverts=mesh.getNumVerts();

					for(int i=0;i<numverts;i++)
					{
						Point3 &xyz=mesh.getVert(i) * mTransformMatrix;

						VertWritePrt->x=xyz.x; VertWritePrt->y=xyz.y; VertWritePrt->z=xyz.z;
						VertWritePrt++;
						dwStartVertIndex++;
					}
				}

				// extract normals
				if(normalsthere)
				{
					Face *face=mesh.faces;

					Matrix3 mNormalTransformMatrix = Inverse(GetTranspose33(mTransformMatrix));
/*
					{
						char str[256];
						Point3 p=mTransformMatrix.GetRow(0);
						sprintf(str,"%.2f %.2f %.2f\n",		p.x,p.y,p.z);
						OutputDebugString(str);
						p=mTransformMatrix.GetRow(1);
						sprintf(str,"%.2f %.2f %.2f\n",		p.x,p.y,p.z);
						OutputDebugString(str);
						p=mTransformMatrix.GetRow(2);
						sprintf(str,"%.2f %.2f %.2f\n",		p.x,p.y,p.z);
						OutputDebugString(str);
						p=mTransformMatrix.GetRow(3);
						sprintf(str,"%.2f %.2f %.2f\n\n",	p.x,p.y,p.z);
						OutputDebugString(str);
					}
					
					{
						char str[256];
						Point3 p=mNormalTransformMatrix.GetRow(0);
						sprintf(str,"%.2f %.2f %.2f\n",  p.x,p.y,p.z);
						OutputDebugString(str);
						p=mNormalTransformMatrix.GetRow(1);
						sprintf(str,"%.2f %.2f %.2f\n",  p.x,p.y,p.z);
						OutputDebugString(str);
						p=mNormalTransformMatrix.GetRow(2);
						sprintf(str,"%.2f %.2f %.2f\n",p.x,p.y,p.z);
						OutputDebugString(str);
						p=mNormalTransformMatrix.GetRow(3);
						sprintf(str,"%.2f %.2f %.2f\n\n",p.x,p.y,p.z);
						OutputDebugString(str);
					}
*/
					for(int i=0;i<nummaxfaces;i++,face++)
					{
						Point3 n;

						if(face->smGroup==0)n=fnorms[i];
							else n=vnorms[face->v[0]].GetNormal(face->smGroup);

						// normals need to be transformed by the inverse
						n=n*mNormalTransformMatrix;

						n=n.Normalize();																						// because mat might scale the normals
						NormWritePtr->x=n.x;NormWritePtr->y=n.y;NormWritePtr->z=n.z;
						NormWritePtr++;

						if(face->smGroup==0)n=fnorms[i];
							else n=vnorms[face->v[1]].GetNormal(face->smGroup);
						n=n*mNormalTransformMatrix;
						n=n.Normalize();																						// because mat could scale the normals
						NormWritePtr->x=n.x;NormWritePtr->y=n.y;NormWritePtr->z=n.z;
						NormWritePtr++;

						if(face->smGroup==0)n=fnorms[i];
							else n=vnorms[face->v[2]].GetNormal(face->smGroup);
						n=n*mNormalTransformMatrix;
						n=n.Normalize();																						// because mat could scale the normals
						NormWritePtr->x=n.x;NormWritePtr->y=n.y;NormWritePtr->z=n.z;
						NormWritePtr++;

						dwStartNormIndex+=3;
					}
				}

				// extract texture data
				if(mappedobject)
				{
					UVVert *uv=mesh.mapVerts(iniTexChannel);

					int numtverts=mesh.getNumMapVerts(iniTexChannel);

					for(int i=0;i<numtverts;i++)
					{
						UVWritePtr->s=uv[i].x;
						UVWritePtr->t=uv[i].y;
						UVWritePtr++;
					}
					dwStartUVIndex+=numtverts;
				}

				if(bShouldIDelete)delete tri;
			}
		}
	}

	WeldSimilarNormals(outpDst);

	return true;
}

void CPlugPanel::OpenExistingSRF()
{
	OPENFILENAME ofn;

	memset(&ofn,0,sizeof(OPENFILENAME));
	char filename[1024]="";

	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=0;		// Window Handle
	ofn.hInstance=0;		// Application Instance
	ofn.lpstrFile=filename;
	ofn.nMaxFile=1024;
	ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;

	ofn.lpstrDefExt="srf";
	ofn.lpstrFilter="Crytek Surface Format Files (*.SRF)\0*.SRF\0";
	ofn.lpstrTitle="Open Crytek Surface Format (.SRF)";

	if(!GetOpenFileName(&ofn))
		return;			// user cancel

	CResourceCompilerHelper rc;

	// call rc.exe
	rc.CallResourceCompiler(filename);
}


void CPlugPanel::StartProcess()
{
	m_WorkerThreadData.m_Properties=m_Properties;

	if(!m_WorkerThreadData.StartComputationSetup(m_hPanel/*AfxGetMainWnd()->GetSafeHwnd())*/))
	{
		MessageBox(0,"Computation in progress","PolyBump",MB_OK);
		return;
	}

	{
		CResourceCompilerHelper rc;

		// test if rc.exe is there
		if(rc.CallResourceCompiler() != CResourceCompilerHelper::eRcCallResult_success)
			return;   // error was already shown

		if(!PickLowPolyMesh())
		{
			AfxMessageBox("Please specify the low detail mesh");
			return;
		}

		if(!PickHighPolyMesh())
		{
			AfxMessageBox("Please specify the high detail mesh");
			return;
		}

		PickCageMesh();


		m_WorkerThreadData.m_Properties.m_bSummariesString="";

		{	
			std::string sFilePathName = m_WorkerThreadData.m_Properties.m_szOutputFilename;

			{
				OPENFILENAME ofn;

				memset(&ofn,0,sizeof(OPENFILENAME));
				char filename[1024]="";

				strcpy(filename,sFilePathName.c_str());

				ofn.lStructSize=sizeof(OPENFILENAME);
				ofn.hwndOwner=0;		// Window Handle
				ofn.hInstance=0;		// Application Instance
				ofn.lpstrFile=filename;
				ofn.nMaxFile=1024;
				ofn.Flags=OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;	// |OFN_FILEMUSTEXIST;

				ofn.lpstrDefExt="srf";
				ofn.lpstrFilter="Crytek Surface Format Files (*.SRF)\0*.SRF\0";
				ofn.lpstrTitle="Save Output (.SRF)";

				if(!GetSaveFileName(&ofn))
					return;						// user cancel

				sFilePathName=filename;
			}

			m_WorkerThreadData.m_Properties.m_szOutputFilename = sFilePathName.c_str();
		}

		m_WorkerThreadData.StartComputation();
	}
}

// stop the worker thread
void CPlugPanel::EndProcess() 
{
	m_WorkerThreadData.StopThread();
}

void CPlugPanel::OnProcessFinished()
{
	assert(m_WorkerThreadData.GetSRFData());

		// output SRF file before expand border to get better compression
	CSRFData* pSRFData = m_WorkerThreadData.GetSRFData();
	if (!pSRFData)
	{
		MessageBox(0,"Missing SRF Data","Saving SRF",MB_OK|MB_ICONEXCLAMATION);
	}
	else if(!pSRFData->Save((m_WorkerThreadData.m_Properties.m_szOutputFilename).c_str()))
	{
		MessageBox(0,"file failed","Saving SRF",MB_OK|MB_ICONEXCLAMATION);
	}
	else
	{
		CResourceCompilerHelper rc;
		rc.CallResourceCompiler(m_WorkerThreadData.m_Properties.m_szOutputFilename.c_str());		// path needs to be absolute
	}

	m_WorkerThreadData.GetSRFData()->FreeData();
}
