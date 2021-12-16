////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   ModelViewportLMG.cpp
//  Version:     v1.00
//  Created:     07/07/2009 by Pau Novau
//  Description: Viewport control for LMG Editor.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/ModelViewportLMG.h"


IMPLEMENT_DYNAMIC( CModelViewportLMG, CModelViewportCE )

BEGIN_MESSAGE_MAP( CModelViewportLMG, CModelViewportCE )
END_MESSAGE_MAP()


CModelViewportLMG::CModelViewportLMG()
: m_pListener( NULL )
{
	
}

CModelViewportLMG::~CModelViewportLMG()
{

}

void CModelViewportLMG::SetListener( IModelViewportLMGListener* pListener )
{
	m_pListener = pListener;
}

void CModelViewportLMG::PlayAnimation( const CString& animationName )
{
	if ( animationName.IsEmpty() )
	{
		return;
	}

	CryCharAnimationParams charAnimationParams;
	charAnimationParams.m_nLayerID = 0;
	charAnimationParams.m_nFlags |= CA_LOOP_ANIMATION;
	//charAnimationParams.m_nFlags |= CA_ALLOW_ANIM_RESTART;

	ISkeletonAnim* pSkeletonAnim = GetCharacterBase()->GetISkeletonAnim();
	if ( pSkeletonAnim == NULL )
	{
		return;
	}

	pSkeletonAnim->StartAnimation( animationName.GetString(), charAnimationParams );
}

void CModelViewportLMG::StopAnimation()
{
	ISkeletonAnim* pSkeletonAnim = GetCharacterBase()->GetISkeletonAnim();
	int animationsInLayer = pSkeletonAnim->GetNumAnimsInFIFO( 0 );
	if ( animationsInLayer == 0 )
	{
		return;
	}
	pSkeletonAnim->StopAnimationInLayer( 0, 0.5f );
}

void CModelViewportLMG::SetDefaultPose()
{
	ISkeletonPose* pSkeletonPose = GetCharacterBase()->GetISkeletonPose();
	if ( pSkeletonPose == NULL )
	{
		return;
	}

	pSkeletonPose->SetDefaultPose();
}

bool CModelViewportLMG::UseAnimationDrivenMotion() const
{
	return true;
}

void CModelViewportLMG::DrawCharacter( ICharacterInstance* pInstance, const SRendParams& rp )
{
	f32 frameTime = GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();

	if ( m_pListener != NULL )
	{
		m_pListener->PreDrawCharacter( pInstance, frameTime );
	}

	const f32 PARAMETER_INCREMENT_SCALE = 0.5f;
	static f32 parameterValue = 0;

	CModelViewport::DrawCharacter( pInstance, rp );

	IAnimationSet* pAnimationSet = pInstance->GetIAnimationSet();

	ISkeletonAnim* pSkeletonAnim = pInstance->GetISkeletonAnim();
	if ( pSkeletonAnim == NULL )
	{
		return;
	}

	pSkeletonAnim->SetCharEditMode( 1 );
	
	AnimPreview_UnitTest( pInstance, pAnimationSet, rp );
}
