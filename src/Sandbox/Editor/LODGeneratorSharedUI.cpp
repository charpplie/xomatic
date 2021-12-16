/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#include "StdAfx.h"
#include "LODGeneratorSharedUI.h"
#include "LODInterface.h"
#include "LODUtilities.h"

//////////////////////////////////////////////////////////////////////////
// File panel
//////////////////////////////////////////////////////////////////////////
const char * CLodGeneratorFilePanel::kPanelCaption = "Source File";
CLodGeneratorFilePanel::CLodGeneratorFilePanel(CWnd* pParent)
{	
	m_pToolTip = NULL;
	Create( IDD,pParent );
}

CLodGeneratorFilePanel::~CLodGeneratorFilePanel()
{
	m_pToolTip = NULL;
}

BEGIN_MESSAGE_MAP(CLodGeneratorFilePanel, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_MAT_LOD_OPEN, &CLodGeneratorFilePanel::OnBnClickedOpen)
	ON_BN_CLICKED(IDC_BUTTON_MAT_LOD_USE_SELECTED, &CLodGeneratorFilePanel::OnBnClickedSelected)
	ON_BN_CLICKED(IDC_BUTTON_MAT_LOD_MAT_EDITOR, &CLodGeneratorFilePanel::OnBnClickedMatEd)
END_MESSAGE_MAP()

BOOL CLodGeneratorFilePanel::OnInitDialog()
{
	BOOL ret = __super::OnInitDialog();

	m_pToolTip = new CToolTipCtrl();
	m_pToolTip->Create(this);

	CButton* pButton = (CButton*)GetDlgItem(IDC_BUTTON_MAT_LOD_MAT_EDITOR);
	if (pButton)
	{
		pButton->ModifyStyle(0,BS_ICON,0);
		CImage img;
		img.Load("Editor\\UI\\Icons\\Material_Editor_16x16.png");
		pButton->SetBitmap(img.Detach());
		m_pToolTip->AddTool(pButton,"Opens the material editor and selects the loaded material.");
	}

	pButton = (CButton*)GetDlgItem(IDC_BUTTON_MAT_LOD_USE_SELECTED);
	if (pButton)
	{
		m_pToolTip->AddTool(pButton,"Opens the selected object from the level into the lod tool.");
	}

	pButton = (CButton*)GetDlgItem(IDC_BUTTON_MAT_LOD_OPEN);
	if (pButton)
	{
		m_pToolTip->AddTool(pButton,"Opens the browse file dialog allowing you to select the cgf to lod.");
	}

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_FILE);
	if(pEdit)
	{
		m_pToolTip->AddTool(pEdit,"Input source file to generate lods for.");
	}

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_MATERIAL_PATH);
	if(pEdit)
	{
		m_pToolTip->AddTool(pEdit,"Input material file to generate material lods for.");
	}

	m_pToolTip->Activate(TRUE);

	return ret;
}

BOOL CLodGeneratorFilePanel::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

void CLodGeneratorFilePanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	CWnd *pControl=GetDlgItem(IDC_BUTTON_MAT_LOD_USE_SELECTED);
	if (pControl)
	{
		CRect rc;
		GetClientRect(rc);
		int right=rc.right;
		pControl->GetWindowRect(rc);
		ScreenToClient(rc);
		rc.left=right-rc.Width();
		rc.right=right;
		pControl->MoveWindow(rc, FALSE);

		pControl=GetDlgItem(IDC_BUTTON_MAT_LOD_OPEN);
		right = rc.left;
		rc.left = right-rc.Width();
		rc.right = right;
		pControl->MoveWindow(rc, FALSE);
		int mateditor = rc.right;

		int editRight=rc.left;
		pControl=GetDlgItem(IDC_EDIT_MAT_LOD_FILE);
		pControl->GetWindowRect(rc);
		ScreenToClient(rc);
		rc.right=editRight;
		pControl->MoveWindow(rc, FALSE);

		pControl=GetDlgItem(IDC_BUTTON_MAT_LOD_MAT_EDITOR);
		pControl->GetWindowRect(rc);
		ScreenToClient(rc);
		rc.left = mateditor - rc.Width();
		rc.right=mateditor;
		pControl->MoveWindow(rc, FALSE);

		editRight = rc.left;
		pControl=GetDlgItem(IDC_EDIT_MAT_LOD_MATERIAL_PATH);
		pControl->GetWindowRect(rc);
		ScreenToClient(rc);
		rc.right=editRight;
		pControl->MoveWindow(rc, FALSE);
	}
}

void CLodGeneratorFilePanel::OnBnClickedOpen()
{
	CString relFile;
	if (!CFileUtil::SelectSingleFile( EFILE_TYPE_GEOMETRY,relFile,"CGF Files (*.cgf)|*.cgf||"))
		return;

	CWaitCursor wait;
	CString material(CLodGeneratorInteractionManager::Instance()->GetDefaultBrushMaterial(relFile));
	SelectObject(relFile,material);
}

void CLodGeneratorFilePanel::OnBnClickedSelected()
{
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	CString filepath(pInstance->GetParameterFilePath());

	if( filepath.IsEmpty() )
		filepath = pInstance->GetSelectedBrushFilepath();
	else
	{
		OnOpenWithPathParameter(pInstance->GetParameterFilePath());
		return;
	}

	if ( filepath.IsEmpty() )
		return;

	CWaitCursor wait;
	CString material(pInstance->GetSelectedBrushMaterial());
	SelectObject(filepath,material);
}

void CLodGeneratorFilePanel::OnOpenWithPathParameter(const CString& objectPath )
{
	if(!objectPath.IsEmpty())
	{
		CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
		CString materialPath(pInstance->GetDefaultBrushMaterial(objectPath));
		SelectObject(objectPath,materialPath);
		pInstance->SetParameterFilePath("");
	}
}

void CLodGeneratorFilePanel::OnBnClickedMatEd()
{
	CWaitCursor wait;
	CLodGeneratorInteractionManager::Instance()->OpenMaterialEditor();
}

void CLodGeneratorFilePanel::SelectObject(const CString& objectPath, const CString& materialPath)
{
	CEdit *pControl=(CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_FILE);
	pControl->SetWindowTextA(objectPath);

	pControl=(CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_MATERIAL_PATH);
	pControl->SetWindowTextA(materialPath);

	GetParent()->GetParent()->PostMessage(WM_GEOM_LOD_FILE_OPENED);
}

const CString CLodGeneratorFilePanel::LoadedFile()
{
	CString filepath;
	CEdit *pControl=(CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_FILE);
	pControl->GetWindowTextA(filepath);
	return filepath;
}

const void CLodGeneratorFilePanel::RefreshMaterialFile()
{
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();

	CWaitCursor wait;
	CString material(CLodGeneratorInteractionManager::Instance()->GetDefaultBrushMaterial(LoadedFile()));
	CEdit *pControl=(CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_MATERIAL_PATH);
	pControl->SetWindowTextA(material);
}

const CString CLodGeneratorFilePanel::MaterialFile()
{
	CString filepath;
	CEdit *pControl=(CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_MATERIAL_PATH);
	pControl->GetWindowTextA(filepath);
	return filepath;
}