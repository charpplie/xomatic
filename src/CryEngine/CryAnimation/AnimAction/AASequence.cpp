#include "stdafx.h"
#include <CryCharAnimationParams.h>

#include "AASequence.h"
#include "AnimActionActor.h"

CRYREGISTER_CLASS(CAASequence)

CAASequence::CAASequence()
{
	m_pActor = NULL;
	m_pISkeletonAnim = NULL;
	m_pISkeletonPose = NULL;
}

CAASequence::~CAASequence()
{
}

//

void CAASequence::Init( IAnimActionActor* pActor )
{
	m_pActor = (CAnimActionActor *)pActor;
	m_pISkeletonAnim = m_pActor->GetCharacterInstance()->GetISkeletonAnim();
	m_pISkeletonPose = m_pActor->GetCharacterInstance()->GetISkeletonPose();
}

bool CAASequence::Update( const SPlaybackContext& context )
{
	bool bStartNext = true;
	//	SAnimActionStartParams par; par.m_nLayer=0;
	if (m_nCurrentAnimToken)
	{
		bStartNext = false;
		f32 fLookAt = m_pPrototype->GetAnimation(m_nCurrentAnimIndex).fParam[AAClass_CSequence::ECP_AllowLookAt];
		m_pISkeletonPose->SetLookIK( uint32(fLookAt), DEG2RAD(120), m_pActor->m_vCamPos );

		int32 nAnimationCount = m_pPrototype->GetAnimationCount();
		int32 nNextAnimIndex = m_nCurrentAnimIndex+1;
		if (nNextAnimIndex >= nAnimationCount)
			nNextAnimIndex=0; 

		assert(m_nCurrentAnimIndex>=0);
		f32 fTransitionTime = m_pPrototype->GetAnimation(nNextAnimIndex).fParam[AAClass_CSequence::ECP_TransitionTime];
		assert(fTransitionTime<60.0f); //transition times longer than 1 minute are useless
		assert(fTransitionTime>=0.0f); //negative transition times are useless

		CAnimation *pAnim = m_pISkeletonAnim->FindAnimInFIFO( m_nCurrentAnimToken,context.params.m_nLayer );
		if (pAnim)
		{
			float fRealAnimTime = pAnim->m_fAnimTime*pAnim->m_fCurrentDuration;
			f32 fStartTime = pAnim->m_fCurrentDuration-fTransitionTime;
			if (fStartTime<0.0f)
				fStartTime=0.0f;	
			if (fRealAnimTime>=fStartTime || pAnim->m_fAnimTime==1.0f) // start next animation before last one finishes
				bStartNext = true; // animation done. start transition to new one.
		}
		else
			bStartNext = true;  // no animation  in queue, so start one
	}


	if (bStartNext)
	{
		if (!StartNextAnimation( context.params ))
			return false;
	}

	return true;
}

bool CAASequence::Start( const SAnimActionStartParams &params )
{

	m_nCurrentAnimIndex = -1;

	bool bSuccess = StartNextAnimation( params );
	assert(bSuccess);

	return bSuccess;
};

bool CAASequence::Stop( bool bAbort )
{
	return true;
}

bool CAASequence::StartNextAnimation( const SAnimActionStartParams &params )
{
	m_nCurrentAnimIndex++;

	int32 nAnimationCount = m_pPrototype->GetAnimationCount();
	if (nAnimationCount == 0)
		return false;	//nor animations at all
	if (m_nCurrentAnimIndex >= nAnimationCount)
		m_nCurrentAnimIndex=0; //No more animations.
	
	f32 fTransitionTime = m_pPrototype->GetAnimation(m_nCurrentAnimIndex).fParam[AAClass_CSequence::ECP_TransitionTime];
	assert(fTransitionTime<60.0f); //transition times longer than 1 minute are useless
	assert(fTransitionTime>=0.0f); //negative transition times are useless

	CAnimActionActor* pAnimActionActor = (CAnimActionActor*)GetActor();
	int32 numAnimCount = m_pPrototype->GetAnimationCount();
	assert( m_nCurrentAnimIndex>=0 && m_nCurrentAnimIndex<numAnimCount );
	const SAnimActionAnimation &anim = m_pPrototype->GetAnimation(m_nCurrentAnimIndex);

	CryCharAnimationParams animParams;
	animParams.m_nFlags    |= CA_ALLOW_ANIM_RESTART | CA_REPEAT_LAST_KEY;
	animParams.m_fTransTime = fTransitionTime;
	animParams.m_nLayerID   = params.m_nLayer;
	animParams.m_nIndexAAC  = m_nCurrentAnimIndex;
	animParams.m_nUserToken = ++pAnimActionActor->m_nLastAnimationToken; // Animation can be found later by this token.
	bool status = m_pISkeletonAnim->StartAnimation( anim.animation.c_str(), animParams );
	if (status==0)
	{
		m_nCurrentAnimToken = 0;
		return false;
	}
	m_nCurrentAnimToken = animParams.m_nUserToken;
	return true;
}


//////////////////////////////////////////////////////////////////////////
void CAASequence::DebugDraw( SDebugDrawInfo &info )
{
	float prevx = info.x;

	ColorF color = ColorF(0.0f,0.0f,0.0f,1);
	int32 num=GetPrototype()->GetAnimationCount();
	for (int i=0; i<num; i++)
	{
		f32 fTransitionTime = m_pPrototype->GetAnimation(i).fParam[AAClass_CSequence::ECP_TransitionTime];
		f32 fLookAt         = m_pPrototype->GetAnimation(i).fParam[AAClass_CSequence::ECP_AllowLookAt];

		const char* strAnimName = GetPrototype()->GetAnimation(i).animation.c_str();
		f32 fTransWeight = m_pActor->GetTransWeightFromFIFO( strAnimName,i);
		f32 fPercent = m_pActor->GetAnimTimeFromFIFO( strAnimName,i);
		color.r=fTransWeight;
		DrawAnimActionDebugText( info.x + 30.0f,info.y,1.3f,color,"%s (%2.0f%%) ttime:%f lookat:%d",strAnimName,fPercent,fTransitionTime,uint32(fLookAt) );
		info.y += info.ystep;
	}
	info.x = prevx;
}
