////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   PreviewLMGView.cpp
//  Version:     v1.00
//  Created:     07/07/2009 by Pau Novau
//  Description: View used to hold preview viewports for the Lmg editor.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/PreviewLMGView.h"

#include "LMGEditor/ModelViewportLMG.h"

IMPLEMENT_DYNAMIC( CPreviewLMGView, CDockingBaseView )

BEGIN_MESSAGE_MAP( CPreviewLMGView, CDockingBaseView )
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CPreviewLMGView::CPreviewLMGView()
: m_pViewport( NULL )
{

}

CPreviewLMGView::~CPreviewLMGView()
{
	OnDestroy();
}

void CPreviewLMGView::OnDestroy()
{
	if ( m_pViewport != NULL )
	{
		// Memory is deallocated in CViewport::PostNcDestroy()
		m_pViewport->DestroyWindow();
		m_pViewport = NULL;
	}
}

BOOL CPreviewLMGView::OnInitDialog()
{
	__super::OnInitDialog();

	// Remove the annoying flickering when resizing the viewport:
	ModifyStyle( 0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS );

	CRect rc;
	GetClientRect( rc );

	m_pViewport = new CModelViewportLMG();
	m_pViewport->SetType( ET_ViewportModel );
	m_pViewport->SetParent( this );
	m_pViewport->SetOwner( this );
	m_pViewport->MoveWindow( rc );
	m_pViewport->ShowWindow( SW_SHOW );
	
	return TRUE;
}

BOOL CPreviewLMGView::PreTranslateMessage( MSG* pMsg )
{
	if ( GetOwner() != NULL )
	{
		if ( GetOwner()->PreTranslateMessage( pMsg ) )
		{
			return TRUE;
		}
	}

	if ( pMsg->message == WM_KEYDOWN )
	{
		return TRUE;
	}

	return __super::PreTranslateMessage( pMsg );
}

ICharacterInstance* CPreviewLMGView::LoadCharacter( const CString& characterFilename )
{
	if ( m_pViewport == NULL )
	{
		return NULL;
	}

	m_pViewport->LoadObject( characterFilename );

	return m_pViewport->GetCharacterBase();
}


void CPreviewLMGView::PlayAnimation( const CString& animationName )
{
	if ( m_pViewport == NULL )
	{
		return;
	}

	m_pViewport->PlayAnimation( animationName );
}

void CPreviewLMGView::StopAnimation()
{
	if ( m_pViewport == NULL )
	{
		return;
	}

	m_pViewport->StopAnimation();
	m_pViewport->SetDefaultPose();
}


void CPreviewLMGView::OnSize( UINT nType, int cx, int cy )
{
	__super::OnSize( nType, cx, cy );

	if ( m_pViewport == NULL )
	{
		return;
	}

	if ( m_pViewport->GetSafeHwnd() == NULL )
	{
		return;
	}

	CRect rc;
	GetClientRect( rc );
	
	m_pViewport->MoveWindow( rc );
}

void CPreviewLMGView::SetListener( IModelViewportLMGListener* pListener )
{
	if ( m_pViewport == NULL )
	{
		return;
	}

	m_pViewport->SetListener( pListener );
}