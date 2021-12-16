////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   PreviewLMGView.h
//  Version:     v1.00
//  Created:     07/07/2009 by Pau Novau
//  Description: View used to hold preview viewports for the Lmg editor.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PreviewLMGView__h__
#define __PreviewLMGView__h__
#pragma once

#include "..\LMGEditor/DockingBaseView.h"

class CModelViewportLMG;
struct IModelViewportLMGListener;

class CPreviewLMGView
	: public CDockingBaseView
{
	DECLARE_DYNCREATE( CPreviewLMGView )

public:
	CPreviewLMGView();
	virtual ~CPreviewLMGView();

	void StopAnimation();
	void PlayAnimation( const CString& animationName );

	ICharacterInstance* LoadCharacter( const CString& characterFilename );

	void SetListener( IModelViewportLMGListener* pListener );

protected:

	virtual BOOL PreTranslateMessage( MSG* pMsg );

	virtual BOOL OnInitDialog();

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()

private:
	CModelViewportLMG* m_pViewport;
};


#endif