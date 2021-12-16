////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  Created:    27/5/2010 by Xiaomao Wu.
//  Compiler:   Visual Studio 2008 Professional
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ExportColladaDlg.h"

const string initPathText ("Please input or choose a path.");

CExportColladaDlg::CExportColladaDlg(const CString path, const CString fileName, CWnd* pParent)
: CDialog(CExportColladaDlg::IDD, pParent)
{
	bPathSet = false;
	m_Path = path;
	m_FileName = fileName;
}


void CExportColladaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EXPORT_PATH, m_ExportPath);
	DDX_Control(pDX, IDC_EXPORT_TYPE, m_ExportType);
}

BEGIN_MESSAGE_MAP(CExportColladaDlg, CDialog)
	ON_BN_CLICKED(IDC_CHOOSE_PATH, OnChooseExportPath)
	ON_CBN_SELENDOK(IDC_EXPORT_TYPE, OnExportTypeChange)
	ON_EN_UPDATE(IDC_EXPORT_PATH, OnExportPathChange)
	ON_EN_UPDATE(IDC_FPS, OnFPSChange)
END_MESSAGE_MAP()

void CExportColladaDlg::OnExportPathChange()
{
	m_ExportPath.GetWindowText(exportPath);
	if( string(exportPath.GetString()) != initPathText )
		bPathSet = true;
}

void CExportColladaDlg::OnExportTypeChange()
{
	exportType = m_ExportType.GetCurSel();
}

void CExportColladaDlg::OnFPSChange()
{
	fps = m_FPS.GetValue();

}

void CExportColladaDlg::OnChooseExportPath()
{
	char szFilters[] = "Collada Files (*.dae)|*.dae|";
	uint32 uSave = CFileUtil::SelectSaveFile( szFilters,"dae", m_Path, m_FileName );

	if(uSave)
		m_ExportPath.SetWindowText(m_FileName);
}

BOOL CExportColladaDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_FPS.Create(this, IDC_FPS);
	m_FPS.SetRange(0.0f, 120.0f);
	m_FPS.SetValue(30.0f);
	fps = 30.0f;

	m_ExportPath.SetWindowText(initPathText.c_str());
	m_ExportPath.GetWindowText(exportPath);

	m_ExportType.AddString("Global Data");
	m_ExportType.AddString("Local Data");
	m_ExportType.SetCurSel(0);

	exportType = EXPORT_GLOBAL_POSE;
	
	return TRUE;
}