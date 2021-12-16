////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   ModelViewportLMG.h
//  Version:     v1.00
//  Created:     07/07/2009 by Pau Novau
//  Description: Viewport control for LMG Editor.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ModelViewportLMG__h__
#define __ModelViewportLMG__h__
#pragma once

#include "CharacterEditor/ModelViewportCE.h"



struct IModelViewportLMGListener
{
	virtual void PreDrawCharacter( ICharacterInstance* pCharacter, f32 frameTime ) = 0;
};



class CModelViewportLMG
	: public CModelViewportCE
{
	DECLARE_DYNCREATE( CModelViewportLMG )

public:
	CModelViewportLMG();
	virtual ~CModelViewportLMG();

	void StopAnimation();
	void PlayAnimation( const CString& animationName );
	void SetDefaultPose();

	void SetListener( IModelViewportLMGListener* pListener );

protected:
	virtual bool UseAnimationDrivenMotion() const;
	virtual void DrawCharacter( ICharacterInstance* pInstance, const SRendParams& rp );

	f32 GetMinParameterValue( ISkeletonAnim* pSkeletonAnim, EMotionParamID motionParamId ) const;
	f32 GetMaxParameterValue( ISkeletonAnim* pSkeletonAnim, EMotionParamID motionParamId ) const;
	f32 GetCurrentParameterValue( ISkeletonAnim* pSkeletonAnim, EMotionParamID motionParamId ) const;

	DECLARE_MESSAGE_MAP()

private:
	IModelViewportLMGListener* m_pListener;
};

#endif