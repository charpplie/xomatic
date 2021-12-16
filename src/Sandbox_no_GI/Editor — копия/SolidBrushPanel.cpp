////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   SolidBrushPanel.h
//  Version:     v1.00
//  Created:     11/1/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: CSolidBrushPanel implementation file.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "SolidBrushPanel.h"

#include "Brush\SolidBrushObject.h"
#include ".\solidbrushpanel.h"

// CSolidBrushPanel dialog

IMPLEMENT_DYNAMIC(CSolidBrushPanel, CDialog)
CSolidBrushPanel::CSolidBrushPanel(CWnd* pParent /*=NULL*/)
	: CDialog(CSolidBrushPanel::IDD, pParent)
{
	Create( IDD,pParent );
}

CSolidBrushPanel::~CSolidBrushPanel()
{
}

void CSolidBrushPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control( pDX,IDC_GEOMETRY_EDIT,m_geomEdit );
	DDX_Control( pDX,IDC_SAVE_TO_CGF,m_btn[0] );
	DDX_Control( pDX,IDC_RESET_TRANSFORM,m_btn[2] );
	DDX_Control( pDX,IDC_SNAP_TO_GRID,m_btn[8] );
}


BEGIN_MESSAGE_MAP(CSolidBrushPanel, CDialog)
	ON_BN_CLICKED(IDC_SAVE_TO_CGF, OnSaveToCgf)
	ON_BN_CLICKED(IDC_RESET_TRANSFORM, OnBnClickedResetTransform)
	ON_BN_CLICKED(IDC_SNAP_TO_GRID, OnBnClickedSnapToGrid)
	ON_BN_CLICKED(IDC_UV_MAPPING, OnBnClickedUvMapping)
	ON_BN_CLICKED(IDC_CLIP_TOOL, OnBnClickedClipTool)
END_MESSAGE_MAP()


// CSolidBrushPanel message handlers
BOOL CSolidBrushPanel::OnInitDialog()
{
	BOOL res = __super::OnInitDialog();

	m_geomEdit.SetToolName( "EditTool.SubObjMode" );

	CSelectionGroup* pSel = GetIEditor()->GetSelection();
	if(pSel && pSel->GetCount()>1)
	{
		GetDlgItem(IDC_SAVE_TO_CGF)->EnableWindow(FALSE);
	}
	
	return res;
}

//////////////////////////////////////////////////////////////////////////
void CSolidBrushPanel::SetBrush( CSolidBrushObject *pObject )
{
	m_pObject = pObject;
}

//////////////////////////////////////////////////////////////////////////
void CSolidBrushPanel::OnSaveToCgf()
{
	if (!m_pObject)
		return;

	CString filename;	
	if (CFileUtil::SelectSaveFile( "CGF Files|*.cgf","*.cgf",Path::GetGameFolder()+CString("\\objects"),filename ))
	{
		m_pObject->SaveToCgf( filename );
	}
}

//////////////////////////////////////////////////////////////////////////
void CSolidBrushPanel::OnBnClickedResetTransform()
{
	GetIEditor()->ExecuteCommand( "Brush.ResetTransform" );
}
//////////////////////////////////////////////////////////////////////////
void CSolidBrushPanel::OnBnClickedSnapToGrid()
{
	GetIEditor()->ExecuteCommand( "Brush.SnapPointsToGrid" );
}

//////////////////////////////////////////////////////////////////////////
void CSolidBrushPanel::OnCancel()
{
	GetIEditor()->ClearSelection();
}
//////////////////////////////////////////////////////////////////////////
void CSolidBrushPanel::OnBnClickedUvMapping()
{
	GetIEditor()->SetEditTool("EditTool.TextureBrush",true);
}
//////////////////////////////////////////////////////////////////////////
void CSolidBrushPanel::OnBnClickedClipTool()
{
	GetIEditor()->SetEditTool("EditTool.ClipBrush",true);
}
//////////////////////////////////////////////////////////////////////////