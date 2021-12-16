#include "stdafx.h"
#include "PolyBumpApplication.h"
#include "MainPanel.h"
#include "mainpanel.h"
#include "SRFData.h"										// CSRFData
#include "mainpanel.h"
#include "LowPolySettingsDialog.h"			// CLowPolySettingsDialog
#include "../SimpleIndexedMesh.h"				// CSimpleIndexedMesh
#include "OBJFileFormat.h"							// COBJFileFormat
#include "mainpanel.h"
#include "PreviewInput.h"								// CPreviewInput
#include "../PolyBump.h"								// to compute low poly radius

std::string ExtractName( std::string &rIn );				// PolybumpApplicationDoc.cpp


// CMainPanel dialog

IMPLEMENT_DYNAMIC(CMainPanel, CDialog)
CMainPanel::CMainPanel( CProperties &rProperties, CWnd* pParent /*=NULL*/)
	: m_rProperties(rProperties), CDialog(CMainPanel::IDD, pParent)
{
}

CMainPanel::~CMainPanel()
{
}

void CMainPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CAGEPOLYNAME, m_CageName);
	DDX_Control(pDX, IDC_PROJPOLYNAME, m_ProjName);
	DDX_Control(pDX, IDC_LOWPOLYNAME, m_LowPolyName);
	DDX_Control(pDX, IDC_HIGHPOLYNAME, m_HighPolyName);
	DDX_Control(pDX, IDC_HITMODE, m_cbHitMode);
	DDX_Control(pDX, IDC_HORIZONMODE, m_cbHorizon);
	DDX_Control(pDX, IDC_ANTIALIASING, m_cbAntialiasing);
	DDX_Control(pDX, IDC_WIDTH, m_cbWidth);
	DDX_Control(pDX, IDC_HEIGHT, m_cbHeight);
	DDX_Control(pDX, IDC_RAYLENGTH, m_RayLength);
	DDX_Control(pDX, IDC_HIGHPOLY_SMOOTH, m_HighPoly_Smooth);
	DDX_Control(pDX, IDC_LOWPOLYINFOTXT, m_LowPolyInfoText);
	DDX_Control(pDX, IDC_Debug, m_Debug);
	DDX_Control(pDX, IDC_THREADPRIO, m_cbThreadPrio);
}


BEGIN_MESSAGE_MAP(CMainPanel, CDialog)
	ON_BN_CLICKED(IDC_PICKHIGHPOLY, OnBnClickedPickhighpoly)
	ON_BN_CLICKED(IDC_PICKLOWPOLY, OnBnClickedPicklowpoly)
	ON_BN_CLICKED(IDC_PICKPROJPOLY, OnBnClickedPickprojpoly)
	ON_BN_CLICKED(IDC_PICKCAGEPOLY, OnBnClickedPickcagepoly)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_PREVIEWINPUT, OnBnClickedPreviewinput)
  ON_MESSAGE(DM_GETDEFID, OnGetDefID)
END_MESSAGE_MAP()

// to suppress Enter behavior to close the dialog
// (http://msdn.microsoft.com/msdnmag/issues/0700/c/)
LRESULT CMainPanel::OnGetDefID(WPARAM wp, LPARAM lp) 
{
	return MAKELONG(0,DC_HASDEFID); 
} 



BOOL CMainPanel::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cbWidth.AddString("64");
	m_cbWidth.AddString("128");
	m_cbWidth.AddString("256");
	m_cbWidth.AddString("512");
	m_cbWidth.AddString("1024");
	m_cbWidth.AddString("2048");
	m_cbWidth.AddString("4096");

	m_cbHeight.AddString("64");
	m_cbHeight.AddString("128");
	m_cbHeight.AddString("256");
	m_cbHeight.AddString("512");
	m_cbHeight.AddString("1024");
	m_cbHeight.AddString("2048");
	m_cbHeight.AddString("4096");

	m_cbAntialiasing.AddString("none");
	m_cbAntialiasing.AddString("sel 2x2");
	m_cbAntialiasing.AddString("sel 3x3");
	m_cbAntialiasing.AddString("full 2x2");
	m_cbAntialiasing.AddString("full 3x3");

	m_cbThreadPrio.AddString("-2 (minimum)");
	m_cbThreadPrio.AddString("-1");
	m_cbThreadPrio.AddString(" 0 (normal)");
	m_cbThreadPrio.AddString("+1");
	m_cbThreadPrio.AddString("+2 (maximum)");

	m_cbHorizon.AddString("none");
	m_cbHorizon.AddString("low");
	m_cbHorizon.AddString("med");
	m_cbHorizon.AddString("high");

	m_cbHitMode.AddString("nearest");
	m_cbHitMode.AddString("latest");

	// set UII data
	m_LowPolyName.SetWindowText(m_rProperties.m_LowPolyMeshFileName.c_str());
	m_HighPolyName.SetWindowText(m_rProperties.m_HighPolyMeshFileName.c_str());
	m_CageName.SetWindowText(m_rProperties.m_CageMeshFileName.c_str());

	m_cbHitMode.SetCurSel(m_rProperties.m_nHitMode);
	m_cbHorizon.SetCurSel(m_rProperties.m_iHorizonMode);
	m_cbAntialiasing.SetCurSel(m_rProperties.m_iAntiAliasingMode);
	m_cbThreadPrio.SetCurSel(m_rProperties.m_iThreadPrio+2);

	m_cbWidth.SetCurSel(m_rProperties.m_iTextureSizePX-3);			// "64"
	m_cbHeight.SetCurSel(m_rProperties.m_iTextureSizePY-3);			// "64"

	char str[256];

	sprintf(str,"%.3f",m_rProperties.m_fRayLength*100.0f);

	m_RayLength.SetWindowText(str);

	m_HighPoly_Smooth.SetCheck(m_rProperties.m_bLoadHighPolyWithSmoothing?BST_CHECKED:BST_UNCHECKED);
	m_Debug.SetCheck(m_rProperties.m_bOutputDebugInfo?BST_CHECKED:BST_UNCHECKED);

	m_LowPolyInfoText.SetWindowText(m_rProperties.m_LowPolyMaterialName.c_str());

	return TRUE;
}



bool CMainPanel::OpenOBJFile( const char *szTitle, CString &inoutFileName, const bool bMultipleFiles )
{
	char back[1024];

	if(GetCurrentDirectory(1024,back)==0)
		return false;

	char filestr[1024];		strcpy(filestr,inoutFileName.GetBuffer());

	OPENFILENAME ofn;

	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=GetSafeHwnd();
	ofn.hInstance=::AfxGetInstanceHandle();
	ofn.lpstrFilter="Alias Wavefront object (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
	ofn.lpstrCustomFilter = (LPTSTR)NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 1L;
	ofn.lpstrInitialDir=NULL;
	if(filestr[0]==0)ofn.lpstrInitialDir=back;
	ofn.lpstrTitle=szTitle;

	ofn.lpstrFileTitle=0;
	ofn.nMaxFileTitle=0;

	ofn.lpstrFile=filestr;
	ofn.nMaxFile=sizeof(filestr);

	ofn.lCustData=0;
	ofn.lpstrDefExt=".obj";
	ofn.Flags=OFN_LONGNAMES | OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if(bMultipleFiles)
		ofn.Flags |= OFN_ALLOWMULTISELECT;

	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;

	if(GetOpenFileName(&ofn))
	{
		inoutFileName="";

		if(bMultipleFiles)
		{
			char *p=filestr;

			CString path;

			while(*p!=0)
				path+=*p++;

			if(*p==0 && *(p+1)!=0)												// multiple file names
			{
				++p;

				while(*p!=0)
				{
					CString file;

					while(*p!=0)
						file+=*p++;

					if(*p==0 && *(p+1)!=0)
						++p;

					if(!inoutFileName.IsEmpty())
						inoutFileName+="\r\n";

					inoutFileName+=path+"\\"+file;
				}
			}
			else inoutFileName=path;					// single file name
		}
		else
		{
			inoutFileName=filestr;
		}

		OutputDebugString(inoutFileName.GetBuffer());
		OutputDebugString("\n");

		return true;
	}

	return false;
}


// CMainPanel message handlers

void CMainPanel::OnBnClickedPickhighpoly()
{
	CString str;

	// extract first filename
	{
		m_HighPolyName.GetWindowText(str);

		std::string multiplefilenames = str.GetBuffer();
		std::string filename = ExtractName(multiplefilenames);

		str = filename.c_str();
	}

	OpenOBJFile("Choose the high poly input file",str,true);

	m_HighPolyName.SetWindowText(str);
}



void CMainPanel::OnBnClickedPicklowpoly()
{
	CString str;

	m_LowPolyName.GetWindowText(str);

	if(OpenOBJFile("Choose the low poly input file",str,false))
	{
		COBJFileFormat obj(false);
		CSimpleIndexedMesh mesh;

		if(!obj.LoadOBJ(str,mesh))
		{
			MessageBox("Load LowPolyMesh failed","PolyBump",MB_OK);
			return;
		}

		if(!mesh.GetUVCount())		// needs UV assignment)
		{
			MessageBox("low poly object needs UV mapping","PolyBump",MB_OK);
			return;
		}

		CLowPolySettingsDialog lowpolysettings(mesh);

		if(!lowpolysettings.DoModal())
			return;

		m_LowPolyInfoText.SetWindowText(lowpolysettings.GetMaterial());
	}

	m_LowPolyName.SetWindowText(str);
}



void CMainPanel::OnBnClickedPickprojpoly()
{
	CString str;

	m_ProjName.GetWindowText(str);

	OpenOBJFile("Choose the projection mesh input file",str,false);

	m_ProjName.SetWindowText(str);
}



void CMainPanel::OnBnClickedPickcagepoly()
{
	CString str;

	m_CageName.GetWindowText(str);

	OpenOBJFile("Choose the cage mesh input file",str,false);

	m_CageName.SetWindowText(str);
}

void CMainPanel::OnBnClickedCancel()
{
	OnCancel();
}

float CMainPanel::GetRayLengthFromButton() const
{
	CString str;

	m_RayLength.GetWindowText(str);

	float fRet = (float)atof(str)*0.01f;

	if(fRet<0.0f)fRet=0.0f;
	if(fRet>100.0f)fRet=100.0f;

	return fRet;
}


void CMainPanel::OnBnClickedOk()
{
	UpdateData(false);	// get data form dialog

	CString str;

	m_LowPolyName.GetWindowText(str);				m_rProperties.m_LowPolyMeshFileName = str;
	m_HighPolyName.GetWindowText(str);			m_rProperties.m_HighPolyMeshFileName = str;
	m_CageName.GetWindowText(str);					m_rProperties.m_CageMeshFileName = str;

	m_rProperties.m_nHitMode = m_cbHitMode.GetCurSel();
	m_rProperties.m_iHorizonMode = m_cbHorizon.GetCurSel();
	m_rProperties.m_iAntiAliasingMode = m_cbAntialiasing.GetCurSel();
	m_rProperties.m_iThreadPrio = m_cbThreadPrio.GetCurSel()-2;

	m_rProperties.m_iTextureSizePX = m_cbWidth.GetCurSel()+3;
	m_rProperties.m_iTextureSizePY = m_cbHeight.GetCurSel()+3;

	m_rProperties.m_fRayLength = GetRayLengthFromButton();

	m_rProperties.m_bLoadHighPolyWithSmoothing = m_HighPoly_Smooth.GetCheck()==BST_CHECKED;
	m_rProperties.m_bOutputDebugInfo = m_Debug.GetCheck()==BST_CHECKED;

	m_LowPolyInfoText.GetWindowText(str);	// under construction
	m_rProperties.m_LowPolyMaterialName = str;

	OnOK();
}

void CMainPanel::OnBnClickedPreviewinput()
{
	CPreviewInput dialog;

	// load input -------------------------

	CString str;

	// high poly meshes
	{
		m_HighPolyName.GetWindowText(str);

		std::string fullname = str.GetBuffer();

		for(;;)
		{
			std::string filename = ExtractName(fullname);

			if(filename.empty())
				break;

			COBJFileFormat obj(false);

			obj.LoadOBJ(filename.c_str(),dialog.CreateMeshEntry(0x000000,"HighPoly",false));
		}
	}

	float fLowPolySize = 0.0f;

	// low poly
	{
		COBJFileFormat obj(false);

		m_LowPolyName.GetWindowText(str);

		CSimpleIndexedMesh &m1 = dialog.CreateMeshEntry(0xffffff,"LowPoly",true);

		obj.LoadOBJ(str,m1);

		// compute fLowPolySize
		{
			CPbLowMesh TempLowMesh;

			assert(m1.m_VertCount!=0);

			TempLowMesh.BuildNormalsAndBaseVectors(m1);

			Vec3 vMin, vMax;

			fLowPolySize = TempLowMesh.CalcBoundingVolume(vMin,vMax);
		}
	}

	// cage mesh
	{
		COBJFileFormat obj(false);

		m_CageName.GetWindowText(str);
		obj.LoadOBJ(str,dialog.CreateMeshEntry(0x0000ff,"CageMesh",true));
	}

	// ---------------------------------

	dialog.SetRayLengthInOS(GetRayLengthFromButton() * fLowPolySize);

	dialog.DoModal();
}
