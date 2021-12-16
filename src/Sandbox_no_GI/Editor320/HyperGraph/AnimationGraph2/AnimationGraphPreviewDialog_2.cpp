////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   AnimationGraphPreviewDialog.cpp
//  Version:     v1.00
//  Created:     5/5/2006 by MichaelS.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnimationGraphPreviewDialog_2.h"
#include "CharacterEditor/ModelViewportCE.h"

#include <ICryAnimation.h>

IMPLEMENT_DYNAMIC(CAnimationGraphPreviewDialog2,CToolbarDialog)

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CAnimationGraphPreviewDialog2, CToolbarDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// CAnimationGraphPreviewDialog
//////////////////////////////////////////////////////////////////////////
CAnimationGraphPreviewDialog2::CAnimationGraphPreviewDialog2(const char* szCharacterName) : CToolbarDialog(IDD,NULL), m_sCharacterName(szCharacterName)
{
	m_pModelViewport = 0;
}

//////////////////////////////////////////////////////////////////////////
CAnimationGraphPreviewDialog2::~CAnimationGraphPreviewDialog2()
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimationGraphPreviewDialog2::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	CRect rcClient;
	GetClientRect(rcClient);
	if (!m_pModelViewport || !m_pModelViewport->m_hWnd)
		return;

	m_pModelViewport->MoveWindow( rcClient );

}

//////////////////////////////////////////////////////////////////////////
void CAnimationGraphPreviewDialog2::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}


//////////////////////////////////////////////////////////////////////////
BOOL CAnimationGraphPreviewDialog2::OnInitDialog()
{
	CRect rcClient;
	GetClientRect(rcClient);

	m_pModelViewport = new CModelViewportCE;
	m_pModelViewport->SetType( ET_ViewportModel );
	m_pModelViewport->SetParent(this);
	//m_pModelViewport->Create( &m_pane1,ET_ViewportModel,"Model Preview" );
	m_pModelViewport->MoveWindow(rcClient);
	m_pModelViewport->ShowWindow(SW_SHOW);

	if (!m_sCharacterName.IsEmpty())
	{
		m_pModelViewport->LoadObject(m_sCharacterName);
	}

	RecalcLayout();
	//MoveWindow(&rect);
	//ShowWindow(SW_SHOW);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CAnimationGraphPreviewDialog2::RedrawPreview()
{
	if (m_pModelViewport)
		m_pModelViewport->Update();
}

//////////////////////////////////////////////////////////////////////////
ICharacterInstance* CAnimationGraphPreviewDialog2::GetCharacter()
{
	ICharacterInstance* pInstance = 0;
	if (m_pModelViewport)
		pInstance = m_pModelViewport->GetCharacterBase();
	return pInstance;
}

//////////////////////////////////////////////////////////////////////////
void CAnimationGraphPreviewDialog2::SetCharacter(const char* szCharacterName)
{
	m_sCharacterName = szCharacterName;
	if ( m_pModelViewport )
		m_pModelViewport->LoadObject( m_sCharacterName );
}

const CString& CAnimationGraphPreviewDialog2::GetCharacterName()
{
	return m_sCharacterName;
}