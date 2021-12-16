#include "stdafx.h"
#include <CryCharAnimationParams.h>
#include <IAnimAction.h>

#include "AARandom.h"
#include "AnimActionActor.h"

CRYREGISTER_CLASS(CAARandom)

CAARandom::CAARandom()
{
	m_pActor = NULL;
	m_pISkeletonAnim = NULL;
	m_pISkeletonPose = NULL;
}

CAARandom::~CAARandom()
{
}

//

void CAARandom::Init( IAnimActionActor* pActor )
{
	m_pActor = (CAnimActionActor *)pActor;
	m_pISkeletonAnim = m_pActor->GetCharacterInstance()->GetISkeletonAnim();
	m_pISkeletonPose = m_pActor->GetCharacterInstance()->GetISkeletonPose();
}

int CAARandom::GetRandomAnimationIndex() const
{
	f32 sumWeights = 0.0f;
	int32 num = m_pPrototype->GetAnimationCount();
	for (int index=0; index<num; ++index )
		sumWeights += m_pPrototype->GetAnimation(index).fParam[AAClass_CRandom::ECP_RandomWeight];

	float fRandom = Random(sumWeights);
	uint32 numChild = m_pPrototype->GetChildAnimations().size();
	for ( uint32 i=0; i<numChild; i++ )
	{
		fRandom -= m_pPrototype->GetAnimation(i).fParam[AAClass_CRandom::ECP_RandomWeight];
		if ( fRandom <= 0.0f )
			return i;
	}
	assert(0);
	return -1;
}

bool CAARandom::Update( const SPlaybackContext& context )
{
	SAnimActionStartParams par; par.m_nLayer=0;
	if (m_nCurrentAnimToken==0)
		Start(par);

	f32 fLookAt = m_pPrototype->GetAnimation(m_nCurrentAnimIndex).fParam[AAClass_CRandom::ECP_AllowLookAt];
	m_pISkeletonPose->SetLookIK( uint32(fLookAt), DEG2RAD(120), m_pActor->m_vCamPos );

	if (m_nNextAnimIndex<0)
		m_nNextAnimIndex = GetRandomAnimationIndex();
	assert(m_nCurrentAnimIndex>=0);

	f32 fTransitionTime = m_pPrototype->GetAnimation(m_nNextAnimIndex).fParam[AAClass_CRandom::ECP_TransitionTime];
	assert(fTransitionTime<60.0f); //transition times longer than 1 minute are useless
	assert(fTransitionTime>=0.0f); //negative transition times are useless

	bool bStartNext = false;
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

	if (bStartNext)
	{
		Start( context.params );
		return false;
	}

	return true;
}

bool CAARandom::Start( const SAnimActionStartParams &params )
{
	m_nCurrentAnimIndex = -1;
	bool bSuccess = StartNextAnimation( params );
	return bSuccess;
}

bool CAARandom::Stop( bool bAbort )
{
	return true;
}

bool CAARandom::StartNextAnimation( const SAnimActionStartParams &params )
{
	if (m_nNextAnimIndex>=0)
		m_nCurrentAnimIndex = m_nNextAnimIndex;
	m_nNextAnimIndex=-1;

	if (m_nCurrentAnimIndex<0)
		m_nCurrentAnimIndex=0;

	f32 fTransitionTime = m_pPrototype->GetAnimation(m_nCurrentAnimIndex).fParam[AAClass_CRandom::ECP_TransitionTime];
	assert(fTransitionTime<60.0f); //transition times longer than 1 minute are useless
	assert(fTransitionTime>=0.0f); //negative transition times are useless

	/*
	CryCharAnimationParams animParams;
	animParams.m_nFlags    |= CA_ALLOW_ANIM_RESTART | CA_REPEAT_LAST_KEY;
	animParams.m_fTransTime = fTransitionTime;
	animParams.m_nLayerID		= params.m_nLayer;
	animParams.m_nIndexAAC  = m_nCurrentAnimIndex;
	if (!((CAnimActionActor*)GetActor())->PlayAnimation( this,m_pPrototype,m_nCurrentAnimIndex,animParams ))
	{
		m_nCurrentAnimToken = 0;
		return false;
	}*/

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

void CAARandom::DebugDraw( SDebugDrawInfo &info )
{
	float prevx = info.x;

	ColorF color = ColorF(0.0f,0.0f,0.0f,1);
	int32 num=GetPrototype()->GetAnimationCount();
	for (int i=0; i<num; i++)
	{
		f32 fTransitionTime = m_pPrototype->GetAnimation(i).fParam[AAClass_CRandom::ECP_TransitionTime];
		f32 fLookAt         = m_pPrototype->GetAnimation(i).fParam[AAClass_CRandom::ECP_AllowLookAt];

		const char* strAnimName = GetPrototype()->GetAnimation(i).animation.c_str();
		f32 fTransWeight = m_pActor->GetTransWeightFromFIFO( strAnimName,i );
		f32 fPercent = m_pActor->GetAnimTimeFromFIFO( strAnimName,i);
		color.r=fTransWeight;
		DrawAnimActionDebugText( info.x + 30.0f,info.y,1.3f,color,"%s %s (%2.0f%%) ttime:%f lookat:%d",(m_nNextAnimIndex==i)?"+":" ",strAnimName,fPercent,fTransitionTime,uint32(fLookAt) );
		info.y += info.ystep;
	}
	info.x = prevx;
}
