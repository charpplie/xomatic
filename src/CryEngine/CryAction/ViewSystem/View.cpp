/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 20:9:2004 : Created by Filippo De Luca

*************************************************************************/
#include "StdAfx.h"

#include <Cry_Camera.h>
#include "View.h"
#include "GameObjects/GameObject.h"

//FIXME:not very pretty
static ICVar *pCamShakeMult = 0;

//------------------------------------------------------------------------
CView::CView(ISystem *pSystem) :
	m_pSystem(pSystem),
	m_linkedTo(0)
{
	if (!pCamShakeMult)
		pCamShakeMult = gEnv->pConsole->GetCVar("c_shakeMult");
		
	m_SoundListenerID = gEnv->pSoundSystem->CreateListener();
}

//------------------------------------------------------------------------
CView::~CView()
{
	//m_shakes.clear();

	gEnv->pSoundSystem->RemoveListener(m_SoundListenerID);
}

//-----------------------------------------------------------------------
void CView::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CView::Update(float frameTime,bool isActive)
{
	//FIXME:some cameras may need to be updated always
	if (!isActive)
		return;

	CGameObject * pLinkedTo = GetLinkedGameObject();
	if (pLinkedTo && !pLinkedTo->CanUpdateView())
	  pLinkedTo = NULL;
	IEntity* pEntity = pLinkedTo ? 0 : GetLinkedEntity();

	if (pLinkedTo || pEntity)
	{
		m_viewParams.SaveLast();

		CCamera *pSysCam = &m_pSystem->GetViewCamera();

		//process screen shaking
		ProcessShaking(frameTime);
		
		//FIXME:to let the updateView implementation use the correct shakeVector
		m_viewParams.currentShakeShift = m_viewParams.rotation * m_viewParams.currentShakeShift;
		
    m_viewParams.frameTime=frameTime;
		//update view position/rotation
		if (pLinkedTo)
		{
			pLinkedTo->UpdateView(m_viewParams);
			if (!m_viewParams.position.IsValid())
			{
				m_viewParams.position = m_viewParams.GetPositionLast();
				CRY_ASSERT_MESSAGE(0, "Camera position is invalid, reverting to old position");
			}
			if (!m_viewParams.rotation.IsValid())
			{
				m_viewParams.rotation = m_viewParams.GetRotationLast();
				CRY_ASSERT_MESSAGE(0, "Camera rotation is invalid, reverting to old rotation");
			}
		}
		else
		{
			const Matrix34& mat = pEntity->GetWorldTM();
			m_viewParams.position = mat.GetTranslation();
			m_viewParams.rotation = Quat(mat);
		}

		m_viewParams.UpdateBlending(frameTime);

		if (pLinkedTo)
			pLinkedTo->PostUpdateView(m_viewParams);

		float fNearZ  = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetDefaultZNear();
		
		//see if the view have to use a custom near clipping plane
		float nearPlane = (m_viewParams.nearplane > 0.01f)?(m_viewParams.nearplane):(fNearZ/*pSysCam->GetNearPlane()*/);
		float farPlane = gEnv->p3DEngine->GetMaxViewDistance();
		float fov = m_viewParams.fov < 0.001 ? DEFAULT_FOV : m_viewParams.fov;
		
		float fPAR =  gEnv->pRenderer->GetPixelAspectRatio();

		m_camera.SetFrustum(pSysCam->GetViewSurfaceX(),pSysCam->GetViewSurfaceZ(),fov,nearPlane,farPlane, fPAR);

		//apply shake & set the view matrix
		m_viewParams.rotation *= m_viewParams.currentShakeQuat;
		m_viewParams.rotation.NormalizeSafe();
		m_viewParams.position += m_viewParams.currentShakeShift;

		Matrix34 viewMtx(m_viewParams.rotation);
		viewMtx.SetTranslation(m_viewParams.position);
		m_camera.SetMatrix(viewMtx);
	}
	else
	{
		m_linkedTo = 0;
		m_camera.SetPosition(Vec3(1,1,1));
	}
}


//------------------------------------------------------------------------
void CView::SetViewShake(Ang3 shakeAngle,Vec3 shakeShift,float duration,float frequency,float randomness,int shakeID, bool bFlipVec, bool bUpdateOnly, bool bGroundOnly)
{
	SShakeParams params;
	params.shakeAngle = shakeAngle;
	params.shakeShift = shakeShift;
	params.frequency = frequency;
	params.randomness = randomness;
	params.shakeID = shakeID;
	params.bFlipVec = bFlipVec;
	params.bUpdateOnly = bUpdateOnly;
	params.bGroundOnly = bGroundOnly;
	params.fadeInDuration = 0;					//
	params.fadeOutDuration = duration;  // originally it was faded out from start. that is why the values are set this way here, to preserve compatibility.
	params.sustainDuration = 0;		  		//
	
	SetViewShakeEx( params );
}


#define RANDOM() ((((float)cry_rand()/(float)RAND_MAX)*2.0f)-1.0f)

//------------------------------------------------------------------------
void CView::SetViewShakeEx( const SShakeParams& params )
{
	float shakeMult(pCamShakeMult->GetFVal());
	if (shakeMult<0.001f)
		return;

	int shakes(m_shakes.size());
	SShake *pSetShake(NULL);

	for (int i=0;i<shakes;++i)
	{
		SShake *pShake = &m_shakes[i];
		if (pShake->ID == params.shakeID)
		{
			pSetShake = pShake;
			break;
		}
	}

	if (!pSetShake)
	{
		m_shakes.push_back(SShake(params.shakeID));
		pSetShake = &m_shakes.back();
	}

	if (pSetShake)
	{
		// this can be set dynamically
		pSetShake->frequency = max(0.00001f, params.frequency);

		// the following are set on a 'new' shake as well
		if (params.bUpdateOnly == false)
		{
			pSetShake->amount = params.shakeAngle * shakeMult;
			pSetShake->amountVector = params.shakeShift * shakeMult;
			pSetShake->randomness = params.randomness;
			pSetShake->doFlip = params.bFlipVec;
			pSetShake->groundOnly = params.bGroundOnly;
			pSetShake->permanent = params.bPermanent;
			pSetShake->fadeInDuration = params.fadeInDuration;
			pSetShake->fadeOutDuration = params.fadeOutDuration;
			pSetShake->sustainDuration = params.sustainDuration;
			pSetShake->timeDone = 0;
			pSetShake->updating = true;
			pSetShake->interrupted = false;
		}
	}
}

void CView::ProcessShaking(float frameTime)
{
	m_viewParams.currentShakeQuat.SetIdentity();
	m_viewParams.currentShakeShift.zero();
	m_viewParams.shakingRatio = 0;
	m_viewParams.groundOnly = false;

	int shakes(m_shakes.size());
	for (int i=0;i<shakes;++i)
		ProcessShake(&m_shakes[i],frameTime);
}

void CView::ProcessShake(SShake *pShake,float frameTime)
{
	if (!pShake->updating)
		return;

	pShake->timeDone += frameTime;
	float endSustain = pShake->fadeInDuration + pShake->sustainDuration;
	float totalDuration = endSustain + pShake->fadeOutDuration;
	
	bool finalDamping = (!pShake->permanent && pShake->timeDone>totalDuration) || (pShake->interrupted && pShake->ratio<0.05f);
	
	if (finalDamping)
		ProcessShake_FinalDamping( pShake, frameTime );
	else
	{
		ProcessShake_CalcRatio( pShake, frameTime, endSustain );
		
		ProcessShake_DoShaking( pShake, frameTime );
		
		//for the global shaking ratio keep the biggest
		if (pShake->groundOnly)
			m_viewParams.groundOnly = true;
		m_viewParams.shakingRatio = max(m_viewParams.shakingRatio,pShake->ratio);
		m_viewParams.currentShakeQuat *= pShake->shakeQuat;
		m_viewParams.currentShakeShift += pShake->shakeVector;
	}
}

//...
void CView::ProcessShake_FinalDamping( SShake *pShake, float frameTime)
{
	pShake->shakeQuat = Quat::CreateSlerp(pShake->shakeQuat,IDENTITY,frameTime * 5.0f);
	m_viewParams.currentShakeQuat *= pShake->shakeQuat;

	pShake->shakeVector = Vec3::CreateLerp(pShake->shakeVector,ZERO,frameTime * 5.0f);
	m_viewParams.currentShakeShift += pShake->shakeVector;

	float svlen2(pShake->shakeVector.len2());
	bool quatIsIdentity(pShake->shakeQuat.IsEquivalent(IDENTITY,0.0001f));

	if (quatIsIdentity && svlen2<0.01f)
	{
		pShake->shakeQuat.SetIdentity();
		pShake->shakeVector.zero();

		pShake->ratio = 0.0f;
		pShake->nextShake = 0.0f;
		pShake->flip = false;

		pShake->updating = false;
	}
}


// "ratio" is the amplitude of the shaking
void CView::ProcessShake_CalcRatio( SShake *pShake, float frameTime, float endSustain )
{
	const float FADEOUT_TIME_WHEN_INTERRUPTED = 0.5f;
	
	if (pShake->interrupted)
		pShake->ratio = max(0.f, pShake->ratio - ( frameTime / FADEOUT_TIME_WHEN_INTERRUPTED ));  // fadeout after interrupted
	else
	if (pShake->timeDone>=endSustain && pShake->fadeOutDuration>0)  
	{
		float timeFading = pShake->timeDone - endSustain;
		pShake->ratio = clamp( 1.f - timeFading / pShake->fadeOutDuration, 0.f, 1.f ); // fadeOut
	}
	else
	if (pShake->timeDone>=pShake->fadeInDuration) 
	{
		pShake->ratio = 1.f; // sustain
	}
	else  
	{
		pShake->ratio = min( 1.f, pShake->timeDone / pShake->fadeInDuration ); // fadeIn
	}

	if (pShake->permanent && pShake->timeDone>=pShake->fadeInDuration && !pShake->interrupted)
	pShake->ratio = 1.f;  // permanent standing
}


void CView::ProcessShake_DoShaking( SShake *pShake, float frameTime)
{
	float t;
	if (pShake->nextShake <= 0.0f)
	{
		//angular
		pShake->goalShake.SetRotationXYZ(pShake->amount);
		if (pShake->flip)
			pShake->goalShake.Invert();

		//translational
		pShake->goalShakeVector = pShake->amountVector;
		if (pShake->flip)
			pShake->goalShakeVector = -pShake->goalShakeVector;

		if (pShake->doFlip)
			pShake->flip = !pShake->flip;

		//randomize it a little
		float randomAmt(pShake->randomness);
		float len(fabs(pShake->amount.x) + fabs(pShake->amount.y) + fabs(pShake->amount.z));
		len /= 3.0f;
		pShake->goalShake *= Quat::CreateRotationXYZ(Ang3(RANDOM()*len*randomAmt,RANDOM()*len*randomAmt,RANDOM()*len*randomAmt));

		//translational randomization
		len = fabs(pShake->amountVector.x) + fabs(pShake->amountVector.y) + fabs(pShake->amountVector.z);
		len /= 3.0f;
		pShake->goalShakeVector += Vec3(RANDOM()*len*randomAmt,RANDOM()*len*randomAmt,RANDOM()*len*randomAmt);

		//damp & bounce it in a non linear fashion
		t = 1.0f-(pShake->ratio*pShake->ratio);
		pShake->goalShake = Quat::CreateSlerp(pShake->goalShake,IDENTITY,t);
		pShake->goalShakeVector = Vec3::CreateLerp(pShake->goalShakeVector,ZERO,t);

		pShake->nextShake = pShake->frequency;
	}

	pShake->nextShake = max(0.0f,pShake->nextShake - frameTime);

	t = min(1.0f,frameTime * (1.0f/pShake->frequency));
	pShake->shakeQuat = Quat::CreateSlerp(pShake->shakeQuat,pShake->goalShake,t);
	pShake->shakeQuat.Normalize();
	pShake->shakeVector = Vec3::CreateLerp(pShake->shakeVector,pShake->goalShakeVector,t);
}


//------------------------------------------------------------------------
void CView::StopShake( int shakeID )
{
	uint32 num = m_shakes.size();
	for (uint32 i=0; i<num; ++i)
	{
		if (m_shakes[i].ID == shakeID && m_shakes[i].updating)
		{
			m_shakes[i].interrupted = true;
		}
	}
}


//------------------------------------------------------------------------
void CView::ResetShaking()
{
	// disable shakes
	std::vector<SShake>::iterator iter = m_shakes.begin();
	std::vector<SShake>::iterator iterEnd = m_shakes.end();
	while (iter != iterEnd)
	{
		SShake& shake = *iter;
		shake.updating = false;
		shake.timeDone = 0;
		++iter;
	}
}

//------------------------------------------------------------------------
void CView::LinkTo(IGameObject *follow)
{
	CRY_ASSERT(follow);
	m_linkedTo = follow->GetEntityId();
	m_viewParams.targetPos = follow->GetEntity()->GetWorldPos();
}

//------------------------------------------------------------------------
void CView::LinkTo(IEntity *follow)
{
	CRY_ASSERT(follow);
	m_linkedTo = follow->GetId();
	m_viewParams.targetPos = follow->GetWorldPos();
}

//------------------------------------------------------------------------
CGameObject * CView::GetLinkedGameObject()
{
	IEntity * pEntity = gEnv->pEntitySystem->GetEntity( m_linkedTo );

	if (!pEntity)
		return NULL;

	return (CGameObject*) pEntity->GetProxy( ENTITY_PROXY_USER );
}

//------------------------------------------------------------------------
IEntity * CView::GetLinkedEntity()
{
	IEntity * pEntity = gEnv->pEntitySystem->GetEntity( m_linkedTo );
	return pEntity;
}

void CView::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->Add(m_shakes);
}

void CView::Serialize(TSerialize ser)
{
	if (ser.IsReading())
	{
		ResetShaking();
	}
}

void CView::SetSoundListenerID( ListenerID nListenerID )
{
	// remove previous listener
	if (m_SoundListenerID != LISTENERID_INVALID)
		gEnv->pSoundSystem->RemoveListener(m_SoundListenerID);

	m_SoundListenerID = nListenerID;

}

#include UNIQUE_VIRTUAL_WRAPPER(IView)
