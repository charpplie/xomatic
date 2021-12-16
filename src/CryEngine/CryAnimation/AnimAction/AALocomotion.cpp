#include "stdafx.h"
#include <CryCharAnimationParams.h>
#include <ICryAnimation.h>

#include "AnimActionActor.h"
#include "AALocomotion.h"

CRYREGISTER_CLASS(CAALocomotion)

extern f32 g_YLine;

CAALocomotion::CAALocomotion()
{
	m_pActor = NULL;
	m_pISkeletonAnim = NULL;
	m_pIAnimationSet = NULL;

	m_fNaturalSpeedSmooth=0.0f;
	m_fNaturalSpeedSmoothRate=0.0f;
	m_keyrcr_SPEED=0;

	m_fGamePadNoise=0.0f;
	m_fGamePadNoiseSmooth=0.0f;
	m_fGamePadNoiseSmoothRate=0.0f;
	m_fScaleNoise=0.0f;

	m_fDesiredTurnSpeedOld=0.0f;
	m_fDesiredTurnSpeedSmooth=0.0f;
	m_fDesiredTurnSpeedSmoothRate=0.0f;

	uint32 num = sizeof(m_arrDesiredTurnSpeed)/sizeof(f32);
	for (uint32 i=0; i<num; i++)
		m_arrDesiredTurnSpeed[i]=0.0f;

	m_Qparams.m_nAnimActionToken=0xDeadBeef;
	m_fDesiredSpeed	=	0;
	m_fGroundSlopeMoveDir = 0.0f;
	m_fGroundSlope	=	0.0f;
	m_vWorldDesiredBodyDirection = Vec2(0,1);
	m_vWorldDesiredMoveDirection = Vec2(0,1);
};

CAALocomotion::~CAALocomotion()
{
}

//

void CAALocomotion::Init( IAnimActionActor* pActor )
{
	m_pActor = (CAnimActionActor *)pActor;
	m_pISkeletonAnim = m_pActor->GetCharacterInstance()->GetISkeletonAnim();
	m_pIAnimationSet = m_pActor->GetCharacterInstance()->GetIAnimationSet();
}

bool CAALocomotion::Update( const SPlaybackContext& context )
{

//	f32 fC1[4] = {0,1,0,1};
//	gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC1, false,"m_fDesiredSpeed: %f", m_pActor->m_fDesiredSpeed );	
//	g_YLine+=12.0f;
	if (m_fDesiredSpeed<0)
		return 0;

	m_pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelSpeed, 0, 0) ;
	m_pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TurnSpeed, 0, 0) ;
	m_pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelAngle, 0, 0) ;
	m_pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelDistScale, 0, 0) ;
	m_pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelDist, 0, 0) ;
	m_pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelSlope, 0, 0) ;
	m_pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_WeightShift, 0, 0) ;

	f32 fLocomotionWeight = 0.0f;
	uint32 numAnimsInFIFO = m_pISkeletonAnim->GetNumAnimsInFIFO(0);
	for (uint32 i=0; i<numAnimsInFIFO; i++)
	{
		CAnimation& anim = m_pISkeletonAnim->GetAnimFromFIFO(0,i);
		if (anim.m_bActivated)
		{
			if (anim.m_AnimParams.m_nSubStateAAC==STATE_RUN)
				fLocomotionWeight += anim.m_fTransitionWeight;
		}
	}

	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	f32 AverageFrameTime = m_pActor->GetCharacterInstance()->GetAverageFrameTime();
	Vec2 vWorldCurrentBodyDirection = Vec2(m_pActor->GetAnimLocation().GetColumn1());
	f32 fDesiredTurnSpeed	= Ang3::CreateRadZ(vWorldCurrentBodyDirection,m_vWorldDesiredBodyDirection);
	SmoothCD(m_fDesiredTurnSpeedSmooth, m_fDesiredTurnSpeedSmoothRate, AverageFrameTime, fDesiredTurnSpeed, 0.11f);
//	m_fDesiredTurnSpeedSmooth = fDesiredTurnSpeed;


	//--------------------------------------------------------------------------------------
	//----           Framerate independent Game-Pad noise detection     --------------------
	//--------------------------------------------------------------------------------------
	uint32 num = sizeof(m_arrDesiredTurnSpeed)/sizeof(f32);
	for (int32 i=(num-2); i>-1; i--)
		m_arrDesiredTurnSpeed[i+1] = m_arrDesiredTurnSpeed[i];
	m_arrDesiredTurnSpeed[0] = m_fDesiredTurnSpeedOld;

	uint32 index=min(uint32(0.12f/AverageFrameTime),num-1);
	m_fGamePadNoise=clamp( fabsf(m_arrDesiredTurnSpeed[index]-fDesiredTurnSpeed),0.0f,20.0f );
	SmoothCD(m_fGamePadNoiseSmooth, m_fGamePadNoiseSmoothRate, AverageFrameTime, m_fGamePadNoise, 0.30f);
	f32 noise=max(m_fGamePadNoiseSmooth,0.0f);
	m_fScaleNoise=clamp(1.0f-(noise*noise),0.0f,1.0f);
	m_fDesiredTurnSpeedOld = fDesiredTurnSpeed;

	//--------------------------------------------------------------------------------------
	//--------               calculate natural speed                          --------------
	//--------------------------------------------------------------------------------------
	f32 fGroundSlope=m_fGroundSlope*0.45f;

	f32 fGroundSlopeMoveDir=m_fGroundSlopeMoveDir;
	f32 fUpHillslowdown=1.0f;
	if (fGroundSlopeMoveDir>0)
		fUpHillslowdown=1.40f;

	f32 fSlopeSlowDown = 1.0f-(fGroundSlope*fUpHillslowdown);
	if (fSlopeSlowDown<0.3f)
		fSlopeSlowDown=0.3f;


	/*
	static uint32 key_Q=0;
	static uint32 rcr_key_Q=0;
	rcr_key_Q<<=1;
	if ( CryGetAsyncKeyState('Q') )	rcr_key_Q |= 1;
	if ((rcr_key_Q&3)==1) 
		key_Q ^= 1;
	if (key_Q)
	{
		f32 fC1[4] = {0,1,0,1};
		gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC1, false,"Just Walking");	
		g_YLine+=12.0f;
		if (m_pActor->m_fDesiredSpeed>1.5f)
			m_pActor->m_fDesiredSpeed=1.5f;
	}
*/

	f32 fDesiredSpeed = m_fDesiredSpeed; 

	// TODO: Implement back cvar!
/*
	//This is just for debugging: we will use this to test transitions between sub-states of an AAC
	//The idea that the new sub-state always takes on the parameters of the old state that blends-out
	if (Console::GetInst().ca_DebugSubstateTransitions)
		fDesiredSpeed = Debug_Substate_Transitions( m_fDesiredSpeed );
*/
//	f32 fC1[4] = {0,1,0,1};
//	gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC1, false,"fDesiredSpeed:%f   fGroundSlope: %f   fGroundSlopeMoveDir: %f",fDesiredSpeed,fGroundSlope,fGroundSlopeMoveDir );	
//	g_YLine+=12.0f;

	f32 fScaleLimit=1.0f;
	if (fDesiredSpeed>0.001f)
		fScaleLimit = 0.8f/fDesiredSpeed;
	fScaleLimit=clamp(fScaleLimit,0.4f,1.0f); //never scale more then 0.4 down 


	f32 fTurnSlowDown=1.0f-fabsf(m_fDesiredTurnSpeedSmooth*0.60f)/gf_PI;
//	f32 fTurnSlowDown=1.0f-fabsf(m_fDesiredTurnSpeedSmooth*1.00f)/gf_PI;
	fTurnSlowDown = clamp(fTurnSlowDown,fScaleLimit,1.0f);	
	fTurnSlowDown = (1.0f-fLocomotionWeight) + fTurnSlowDown*fLocomotionWeight;


	//adjust DesiredSpeed when strafing and running backward
	f32 fRadBodySmooth = -atan2f(m_vWorldDesiredBodyDirection.x,m_vWorldDesiredBodyDirection.y);
	Vec2 vWorldDesiredMoveDirectionSmooth=Matrix33::CreateRotationZ(fRadBodySmooth)*Vec2(0.0f,1.0f);
	f32 fBodyMoveRadian = Ang3::CreateRadZ( m_vWorldDesiredBodyDirection, vWorldDesiredMoveDirectionSmooth );
	f32 fStrafeSlowDown = (gf_PI-fabsf(fBodyMoveRadian*0.60f))/gf_PI;
	fStrafeSlowDown=clamp(fStrafeSlowDown, fScaleLimit,1.0f);	


	//BINGO! thats it
	f32 fNaturalSpeed = fDesiredSpeed * min(fTurnSlowDown*fSlopeSlowDown,fStrafeSlowDown);
	SmoothCD(m_fNaturalSpeedSmooth, m_fNaturalSpeedSmoothRate, AverageFrameTime, fNaturalSpeed, 0.20f);


	

	//-------------------------------------------------------------------------------------
	f32 fTurnSpeedMultiplier = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_TurnSpeed].GetFloat();
	f32 fTurnSpeed = m_fDesiredTurnSpeedSmooth*(fabsf(fTurnSpeedMultiplier*m_fDesiredTurnSpeedSmooth)+1);


	const char* name = m_pPrototype->GetAAClassName();
	
	m_keyrcr_SPEED = (m_keyrcr_SPEED<<1)|(fDesiredSpeed>0.001f); 
	f32 fTurnAngle = -atan2f(0.0f,1.0f); // -atan2f(m_vLocalDesiredMoveDirectionSmooth.x,m_vLocalDesiredMoveDirectionSmooth.y);
	if ( (m_keyrcr_SPEED&3)==1 )
		fTurnAngle = Ang3::CreateRadZ(vWorldCurrentBodyDirection,GetWorldDesiredBodyDirection());

	f32 fWeightShift = Ang3::CreateRadZ(vWorldCurrentBodyDirection,GetWorldDesiredBodyDirection());// / (gf_PI*0.5f);
//	fWeightShift *= 1.0f;
	fWeightShift = -clamp(fWeightShift,-1.0f,1.0f); //never scale more then 0.4 down 
	int32 sign=sgn(fWeightShift);

	//f32 fC2[4] = {0,1,0,1};
	//gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 2.6f, fC2, false,"fWeightShift: %f",fWeightShift );	
	//g_YLine+=26.0f;

//	SnapTurn(fDesiredSpeed,fLocomotionWeight,m_pActor->m_vWorldDesiredBodyDirection,vWorldCurrentBodyDirection );


	CAnimation* pAnimation = m_pActor->FindInFIFOByAAToken( m_Qparams.m_nLayerID, m_Qparams.m_nAnimActionToken, 0 );

//	if ( pAnimation == NULL )
//		return false;
	StateMachine( pAnimation, fDesiredSpeed );


	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelSpeed,     m_fNaturalSpeedSmooth, AverageFrameTime);
	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TurnSpeed,       fTurnSpeed,		AverageFrameTime);
	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TurnAngle,       fTurnAngle,		AverageFrameTime);								//thats how we set the turn-direction of an Idle2Move
	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelAngle,     0,							AverageFrameTime);								//thats how we set the strafing direction
	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelSlope,     RAD2DEG(fGroundSlopeMoveDir),	AverageFrameTime);	//thats how we set the slope
	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_WeightShift,     sign*(fWeightShift*fWeightShift),	AverageFrameTime);								//thats how we set the shallow turn

	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelDistScale, 1.0f, AverageFrameTime);
	m_pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelDist,      -1,	 AverageFrameTime);


	//--------------------------------------------------------------------------------------------------------------
	//---  just debugging output         ---------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------------------------
	if (0)
	{

/*
		for ( f32 t=0.0f; t<1.0f; t+=(1.0f/100.0f) )
		{
			Vec3 mnew = Vec3::CreateSlerp(vWorldCurrentBodyDirection,m_pActor->m_vWorldDesiredBodyDirection,t);
			g_pAuxGeom->DrawLine(m_pActor->m_AnimLocation.t,RGBA8(0x00,0x00,0x00,0x00), m_pActor->m_AnimLocation.t+mnew,RGBA8(0x00,0xff,0x00,0x00) );
		}*/

		/*
		f32 wrad	= -atan2f(vWorldCurrentBodyDirection.x,vWorldCurrentBodyDirection.y);
		for ( f32 t=0.0f; t<1.0f; t+=(1.0f/100.0f) )
		{
		f32 new_wrad = (m_fDesiredTurnSpeedSmooth*t) + wrad;
		Vec2 mdir = Vec2(-cry_sinf(new_wrad),cry_cosf(new_wrad));
		g_pAuxGeom->DrawLine(m_pActor->m_AnimLocation.t,RGBA8(0x00,0x00,0x00,0x00), m_pActor->m_AnimLocation.t+mdir*1.2f,RGBA8(0x00,0xff,0xff,0x00) );
		}
		*/

		//just for debugging
		f32 fRadBody = -atan2f(m_vWorldDesiredBodyDirection.x,m_vWorldDesiredBodyDirection.y);
		Matrix33 MatBody33	=	Matrix33::CreateRotationZ(fRadBody); 
		const f32 size	= 0.0009f;
		const f32 length= 2.0f;
		AABB yaabb = AABB(Vec3(-size,0.0f,-size),Vec3(size,length,size));
		OBB obb =	OBB::CreateOBBfromAABB( MatBody33, yaabb );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawOBB(obb,m_pActor->m_AnimLocation.t,1,RGBA8(0x00,0x00,0xff,0x00),eBBD_Extremes_Color_Encoded);
//		gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(m_pActor->m_AnimLocation.t+MatBody33.GetColumn1()*length, MatBody33.GetColumn1(),0.03f,0.15f,  RGBA8(0x00,0x00,0xff,0x00));
		
		f32 fC2[4] = {0,1,1,1};
		gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC2, false,"fDesiredSpeed: %f   m_fDesiredTurnSpeedSmooth: %f",fDesiredSpeed, m_fDesiredTurnSpeedSmooth );	
		g_YLine+=16.0f;
		//m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"fScaleLimit: %f",fScaleLimit);
		//g_ypos+=10;
	//	gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC2, false,"m_fGamePadNoiseSmooth: %f g_ScaleNoise: %f  index: %x", m_fGamePadNoiseSmooth, m_fScaleNoise, index );	
	//	g_YLine+=16.0f;
		gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC2, false,"fSlopeSlowDown: %f   fTurnSlowDown: %f   fStrafeSlowDown: %f", fSlopeSlowDown,fTurnSlowDown,fStrafeSlowDown );	
		g_YLine+=16.0f;
		gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC2, false,"fNaturalSpeedSmooth: %f",m_fNaturalSpeedSmooth );	
		g_YLine+=16.0f;

	}

	return true;
}

bool CAALocomotion::StateMachine( CAnimation* pAnimation, f32 fDesiredSpeed )
{
	const char* strIdle      = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Idle].GetString().c_str();
	const char* strWalk      = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Walk].GetString().c_str();
	const char* strRun       = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Run].GetString().c_str();
	const char* strIdle2Walk = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Idle2Walk].GetString().c_str();
	const char* strIdle2Run  = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Idle2Run].GetString().c_str();
	const char* strRunStop   = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_RunStop].GetString().c_str();
	const char* strWalkStop  = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_WalkStop].GetString().c_str();

	if (fDesiredSpeed==0)
	{
		StandingAnims( pAnimation, fDesiredSpeed, strIdle,strRun, 0.0f ); //standing idle
	}
	else
		Locomotion( pAnimation, fDesiredSpeed, strIdle, strIdle2Run,strWalk,strRun, STATE_RUN, STATE_IDLE2RUN ); //running

	return true;
}

void CAALocomotion::StandingAnims( CAnimation* pAnimation, f32 fDesiredSpeed,  const char* strIdle, const char* strIdle2Move, f32 yaw_radiant)
{
	CryCharAnimationParams AParams = m_Qparams;

	int32 id=-7;
	uint32 state = -1;

	if (pAnimation==0)
	{
		uint32 i=0;
	}

	if (pAnimation)
	{
		if (pAnimation->m_Parametric.m_nParametricID>=0)
			id = pAnimation->m_Parametric.m_nParametricID;		//get id of current locomotion group
		else
			id = pAnimation->m_Parametric.m_nAnimID[0];  //get id of current animation

		if (pAnimation->m_bActivated==0)
		{
			int8 index;
			m_pActor->FindInFIFOByAAToken( m_Qparams.m_nLayerID, m_Qparams.m_nAnimActionToken, &index );
			m_pISkeletonAnim->RemoveAnimFromFIFO(0,index);
			pAnimation = m_pActor->FindInFIFOByAAToken( m_Qparams.m_nLayerID, m_Qparams.m_nAnimActionToken, 0 );
		}

		if (pAnimation)
			state = pAnimation->m_AnimParams.m_nSubStateAAC;
	}

	if ( pAnimation && state==0 )
	{
		int32 id0=m_pIAnimationSet->GetAnimIDByName(strIdle);
		if ( id==id0 )
		{
			//character is standing. As long as he has no idle steps he can do nothing
			f32 fTurnAngleLocked	=	0;
			f32 fTurnAngle				= yaw_radiant;			
			m_pISkeletonAnim->SetDesiredMotionParam( eMotionParamID_TurnAngle, fTurnAngleLocked, m_pActor->GetCharacterInstance()->GetAverageFrameTime() );
		}

		int32 id3=m_pIAnimationSet->GetAnimIDByName(strIdle2Move);
		if (id==id3)
		{
			//blend into idle immediately
			AParams.m_nFlags = CA_LOOP_ANIMATION;
			AParams.m_fTransTime	 = 0.25f;
			AParams.m_nSubStateAAC = STATE_STAND;
			m_pISkeletonAnim->StartAnimation( strIdle, AParams);
		}

	}
	else
	{
		//blend into idle after run-stop..
		AParams.m_nFlags = CA_LOOP_ANIMATION;
	//	if ( pAnimation == NULL ) //...or when the previous animation end
	//		AParams.m_nFlags |= CA_START_AFTER;
		AParams.m_fTransTime	 = 0.30f;
		AParams.m_nSubStateAAC = STATE_STAND;
		m_pISkeletonAnim->StartAnimation( strIdle, AParams);
	}
}


//uint32 Idle2Move=0;
void CAALocomotion::Locomotion( CAnimation* pAnimation,f32 fDesiredSpeed, const char* strIdle, const char* strIdle2Move, const char* strWalk,const char* strRun, uint32 STATE_MOVE, uint32 STATE_IDLE2MOVE )
{
	f32 fWalk2Run_Threshold  = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Walk2Run_Threshold].GetFloat();
	f32 fRun2Walk_Threshold  = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Run2Walk_Threshold].GetFloat();

	CryCharAnimationParams AParams = m_Qparams;

	int32 id=-1;
	if (pAnimation)
	{
		id = pAnimation->m_Parametric.m_nParametricID;  //get id of current LMG
		if (id<-1)
			id = pAnimation->m_Parametric.m_nAnimID[0];  //get id of current animation asset
	}

	//check if we are in what state we are 
	int32 id0 = m_pIAnimationSet->GetAnimIDByName(strIdle);
	if ( (id==id0 && pAnimation) || pAnimation == NULL)
	{
		if (pAnimation==0)
		{
			//only when the Idle2Move is over, we blend into the locomotion-lmg
			AParams.m_nFlags = CA_LOOP_ANIMATION|CA_TRANSITION_TIMEWARPING;
			AParams.m_fTransTime	=	1.30f;
			AParams.m_nSubStateAAC= STATE_MOVE;
			if (fDesiredSpeed>fWalk2Run_Threshold)
				m_pISkeletonAnim->StartAnimation( strRun, AParams);
			else
				m_pISkeletonAnim->StartAnimation( strWalk, AParams);
		}
		else
		{
			//We are in Idle-State. This means we can use one of our cool Idle2Moves
			AParams.m_nFlags = CA_REPEAT_LAST_KEY;
			AParams.m_fTransTime	=	0.00f;	//fast transition from idle to Idle2Move
			AParams.m_nSubStateAAC= STATE_IDLE2MOVE;
			m_pISkeletonAnim->StartAnimation( strIdle2Move, AParams);

			//only when the Idle2Move is over, we blend into the locomotion-lmg
			AParams.m_nFlags = CA_LOOP_ANIMATION|CA_IDLE2MOVE|CA_TRANSITION_TIMEWARPING;
			AParams.m_fTransTime	=	0.30f;
			AParams.m_nSubStateAAC= STATE_MOVE;
			if (fDesiredSpeed>fWalk2Run_Threshold)
				m_pISkeletonAnim->StartAnimation( strRun, AParams);
			else
				m_pISkeletonAnim->StartAnimation( strWalk, AParams);
		}

	}
	else
	{
		//We are in Idle2Move or in Locomotion-State.
		uint32 numAnimsInFIFO = m_pISkeletonAnim->GetNumAnimsInFIFO(0);
		if (numAnimsInFIFO)
		{
			CAnimation& anim = m_pISkeletonAnim->GetAnimFromFIFO(0,numAnimsInFIFO-1);
			if (anim.m_AnimParams.m_nSubStateAAC==STATE_RUN)
			{
				if (anim.m_bActivated==0)
				{
					//We are in Idle2Move state
					m_pISkeletonAnim->RemoveAnimFromFIFO(0,numAnimsInFIFO-1);
					AParams.m_nFlags				= CA_LOOP_ANIMATION|CA_IDLE2MOVE|CA_TRANSITION_TIMEWARPING;
					AParams.m_fTransTime		=	0.30f;
					AParams.m_nSubStateAAC	= STATE_MOVE;
					if (fDesiredSpeed>fWalk2Run_Threshold)
						m_pISkeletonAnim->StartAnimation( strRun,  AParams);
					else
						m_pISkeletonAnim->StartAnimation( strWalk, AParams);
				}
				else 
				{
					//We are in an activated Locomotion-State. Use smooth and long transitions between walks&runs 
					AParams.m_nFlags				= CA_TRANSITION_TIMEWARPING|CA_LOOP_ANIMATION;
					AParams.m_fTransTime		=	1.00f;
					AParams.m_nSubStateAAC	= STATE_MOVE;
					if (fDesiredSpeed>fWalk2Run_Threshold)
						m_pISkeletonAnim->StartAnimation( strRun,  AParams);
					else
						m_pISkeletonAnim->StartAnimation( strWalk, AParams);
				}
			}
		}
		else
		{
			AParams.m_nFlags = CA_TRANSITION_TIMEWARPING|CA_LOOP_ANIMATION;
			AParams.m_fTransTime	=	0.30f;
			AParams.m_nSubStateAAC= STATE_MOVE;
			if (fDesiredSpeed>fWalk2Run_Threshold)
				m_pISkeletonAnim->StartAnimation( strRun, AParams);
			else
				m_pISkeletonAnim->StartAnimation( strWalk, AParams);
		}
	}
}


//-------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------


void CAALocomotion::SnapTurn( f32 fDesiredSpeed, f32 fLocomotionWeight, const Vec2& vWorldDesiredBodyDirection, const Vec2& vWorldCurrentBodyDirection )
{
	const char* strSharpTransitionRight90		= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpTransitionRight90].GetString().c_str();
	const char* strSharpStartRight90				= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpStartRight90].GetString().c_str();
//	const char* strSharpTransitionRight180	= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpTransitionRight180].GetString().c_str();
//	const char* strSharpStartRight180				= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpStartRight180].GetString().c_str();

	const char* strSharpTransitionLeft90		= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpTransitionLeft90].GetString().c_str();
	const char* strSharpStartLeft90					= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpStartLeft90].GetString().c_str();
//	const char* strSharpTransitionLeft180		= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpTransitionLeft180].GetString().c_str();
//	const char* strSharpStartLeft180				= m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_SharpStartLeft180].GetString().c_str();

	static uint32 m_keyrcr_SNAP = 0;
	m_keyrcr_SNAP = (m_keyrcr_SNAP<<1);
	if (fLocomotionWeight>0.9f && fabsf(m_fDesiredTurnSpeedSmooth)<0.8f && fDesiredSpeed>0.5f)
	{
		f32 snap = Ang3::CreateRadZ(vWorldDesiredBodyDirection,vWorldCurrentBodyDirection);
		if ( fabs(snap) > 1.7f )
		{
			//f32 fC1[4] = {1,0,1,1};
			//gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 12.6f, fC1, false,"SnapTurn" );	
			//g_YLine+=16.0f;
			m_keyrcr_SNAP |= uint32(1); 
		}
	}

	static f32 last_rad=0;
	static f32 last_turnspeed=0;
	static f32 last_weight=0;
	//	if (m_pActor->m_nSnapTurn && fLocomotionWeight>0.5f && fDesiredSpeed>0.1f)
	//	if (m_pActor->m_nSnapTurn && fLocomotionWeight>0.5f)
	if (  (m_keyrcr_SNAP&3)==1  )
	{
		last_rad = Ang3::CreateRadZ(vWorldDesiredBodyDirection,vWorldCurrentBodyDirection);
		last_turnspeed = m_fDesiredTurnSpeedSmooth;
		f32 pi2 = (gf_PI*0.5f);
		if (last_rad > pi2 )
		{
			//	f32 fC1[4] = {0,1,1,1};
			//	gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 10.6f, fC1, false,"SnapTurn" );	
			//	g_YLine+=16.0f;

			last_weight = (last_rad - pi2) / pi2; 

			CryCharAnimationParams Params0 = m_Qparams;
			Params0.m_nAnimActionToken=1;
			Params0.m_fTransTime=0.20f;
			Params0.m_nFlags |= CA_REPEAT_LAST_KEY;
			Params0.m_nSubStateAAC	= STATE_SNAPTURN;
			m_pISkeletonAnim->StartAnimation( strSharpTransitionRight90, Params0);

			CryCharAnimationParams Params1 = m_Qparams;
			Params1.m_nAnimActionToken=1;
			Params1.m_fTransTime=0.00f;
			Params1.m_nFlags = CA_START_AFTER|CA_REPEAT_LAST_KEY;
			//	Params1.m_nSubStateAAC	= STATE_SNAPTURN;
			m_pISkeletonAnim->StartAnimation( strSharpStartRight90, Params1);

			CryCharAnimationParams Params2 = m_Qparams;
			const char* strRun       = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Run].GetString().c_str();
			Params2.m_fTransTime=0.40f;
			Params2.m_fKeyTime=0.30f;
			Params2.m_nFlags = CA_START_AT_KEYTIME|CA_TRANSITION_TIMEWARPING|CA_LOOP_ANIMATION;
			Params2.m_nSubStateAAC	= STATE_RUN;
			m_pISkeletonAnim->StartAnimation( strRun, Params2);
		}

		if (last_rad < -pi2 )
		{
			//f32 fC1[4] = {0,1,1,1};
			//gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 10.6f, fC1, false,"SnapTurn" );	
			//g_YLine+=16.0f;

			last_weight = (-pi2-last_rad ) / pi2; 

			CryCharAnimationParams Params0 = m_Qparams;
			Params0.m_nAnimActionToken=1;
			Params0.m_fTransTime=0.20f;
			Params0.m_nFlags |= CA_REPEAT_LAST_KEY;
			Params0.m_nSubStateAAC	= STATE_SNAPTURN;
			m_pISkeletonAnim->StartAnimation( strSharpTransitionLeft90, Params0);

			CryCharAnimationParams Params1 = m_Qparams;
			Params1.m_nAnimActionToken=1;
			Params1.m_fTransTime=0.00f;
			Params1.m_nFlags = CA_START_AFTER|CA_REPEAT_LAST_KEY;
			//	Params1.m_nSubStateAAC	= -1;
			m_pISkeletonAnim->StartAnimation( strSharpStartLeft90, Params1);

			CryCharAnimationParams Params2 = m_Qparams;
			const char* strRun       = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_Run].GetString().c_str();
			Params2.m_fTransTime=0.40f;
			Params2.m_fKeyTime=0.30f;
			Params2.m_nFlags = CA_START_AT_KEYTIME|CA_TRANSITION_TIMEWARPING|CA_LOOP_ANIMATION;
			Params2.m_nSubStateAAC	= STATE_RUN;
			m_pISkeletonAnim->StartAnimation( strRun, Params2);
		}
	}

	//	f32 fC2[4] = {0,1,1,1};
	//	gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fC2, false,"last_rad: %f   last_turnspeed: %f  last_weight: %f",last_rad, last_turnspeed,last_weight );	
	//	g_YLine+=16.0f;
}


//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------

f32 CAALocomotion::Debug_Substate_Transitions( f32 fDesiredSpeedSmooth )
{
	f32 AverageFrameTime = m_pActor->GetCharacterInstance()->GetAverageFrameTime();

	static f32 fBreathing = 1.0f;
	static f32 fBreathingSmooth = 1.0f;
	static f32 fBreathingRate = 1.0f;
	SmoothCD(fBreathingSmooth, fBreathingRate, AverageFrameTime, fBreathing, 0.30f);

	static f32 fLookAround = 1.0f;
	static f32 fLookAroundSmooth = 1.0f;
	static f32 fLookAroundRate = 1.0f;
	SmoothCD(fLookAroundSmooth, fLookAroundRate, AverageFrameTime, fLookAround, 0.30f);

	static uint32 ThreatLevel = -1;
	static uint32 keyrcr_1 = 0;
	static uint32 keyrcr_2 = 0;
	static uint32 keyrcr_3 = 0;
	keyrcr_1 = (keyrcr_1<<1)|uint32(CryGetAsyncKeyState('1')); 
	keyrcr_2 = (keyrcr_2<<1)|uint32(CryGetAsyncKeyState('2')); 
	keyrcr_3 = (keyrcr_3<<1)|uint32(CryGetAsyncKeyState('3')); 

	static uint32 keyrcr_0 = 0;
	static uint32 keyrcr_8 = 0;
	static uint32 keyrcr_9 = 0;
	keyrcr_8 = (keyrcr_8<<1)|uint32(CryGetAsyncKeyState('8')); 
	keyrcr_9 = (keyrcr_9<<1)|uint32(CryGetAsyncKeyState('9')); 
	keyrcr_0 = (keyrcr_0<<1)|uint32(CryGetAsyncKeyState('0')); 

	static uint32 keyrcr_Q = 0;
	static uint32 key_Q = 0;
	keyrcr_Q = (keyrcr_Q<<1)|uint32(CryGetAsyncKeyState('Q')); 

	if ((keyrcr_Q&3)==1) 
		key_Q ^= 1;

	CryCharAnimationParams AParams;
	if ( (keyrcr_1&3)==1 )
	{
		SAnimActionStartParams par;
		m_pActor->StartAnimAction( "_RELAXED_NW_01",par);  //walk or run
		ThreatLevel = 1;
		fBreathing  = 0.5f;
		fLookAround  = 1.00f;

		CryCharAnimationParams Params0(0);
		Params0.m_nLayerID=1;
		Params0.m_fTransTime=1.0f;
		Params0.m_fPlaybackSpeed=0.50f;
		Params0.m_nFlags = CA_LOOP_ANIMATION|CA_ALLOW_ANIM_RESTART;
		m_pISkeletonAnim->StartAnimation("Additive_Breathing_Slow_01", Params0);
	}
	if ( (keyrcr_2&3)==1 )
	{
		SAnimActionStartParams par;
		m_pActor->StartAnimAction( "_APPREHENSIVE_NW_01", par);  //walk or run
		ThreatLevel = 2;
		fBreathing  = 1.3f;

		CryCharAnimationParams Params0(0);
		Params0.m_nLayerID=1;
		Params0.m_fTransTime=1.0f;
		Params0.m_fPlaybackSpeed=1.5f;
		Params0.m_nFlags = CA_LOOP_ANIMATION|CA_ALLOW_ANIM_RESTART;
		m_pISkeletonAnim->StartAnimation("Additive_Breathing_Slow_01", Params0);

		m_pISkeletonAnim->StopAnimationInLayer(2,1.5f);
	}
	if ( (keyrcr_3&3)==1 )
	{
		SAnimActionStartParams par;
		m_pActor->StartAnimAction( "_THREAT_NW_01",par);  //walk or run
		ThreatLevel = 3;
		fBreathing  = 1.0f;

		CryCharAnimationParams Params0(0);
		Params0.m_nLayerID=1;
		Params0.m_fTransTime=1.0f;
		Params0.m_fPlaybackSpeed=2.0f;
		Params0.m_nFlags = CA_LOOP_ANIMATION|CA_ALLOW_ANIM_RESTART;
		m_pISkeletonAnim->StartAnimation("Additive_Breathing_Slow_01", Params0);

	//	Params0.m_nLayerID=2;
	//	Params0.m_fPlaybackSpeed=1.20f;
	//	m_pISkeletonAnim->StartAnimation("Additive_LookAround", Params0);
	//	m_pISkeletonAnim->StartAnimation("Additive_ThreadIdle2", Params0);
	}



	if (fDesiredSpeedSmooth<0.5)
	{
		if (ThreatLevel==1)
		{
			f32 fC1[4] = {0,1,0,1};
			gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Idle Relaxed");	
			g_YLine+=12.0f;
			fBreathing  = 0.50f;
			fLookAround = 1.00f;
			CryCharAnimationParams Params0(0);
			Params0.m_nLayerID=2;
			Params0.m_fTransTime=1.0f;
			Params0.m_fPlaybackSpeed=1.00f;
			Params0.m_nFlags = CA_LOOP_ANIMATION;
			m_pISkeletonAnim->StartAnimation("Additive_Idle_Relaxed_01", Params0);
		}
		if (ThreatLevel==2)
		{
			f32 fC1[4] = {0,1,0,1};
			gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Idle Apprehensive");	
			g_YLine+=12.0f;
			fBreathing  = 1.00f;
			fLookAround = 1.0f;
			CryCharAnimationParams Params0(0);
			Params0.m_nLayerID=2;
			Params0.m_fTransTime=1.0f;
			Params0.m_fPlaybackSpeed=1.00f;
			Params0.m_nFlags = CA_LOOP_ANIMATION;
			m_pISkeletonAnim->StartAnimation("Additive_Idle_Apprehensive_01", Params0);
		}
		if (ThreatLevel==3)
		{
			fBreathing  = 1.00f;
			fLookAround = 1.00f;
			f32 fC1[4] = {0,1,0,1};
			gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Idle Threat");	
			g_YLine+=12.0f;
			CryCharAnimationParams Params0(0);
			Params0.m_nLayerID=2;
			Params0.m_fTransTime=1.0f;
			Params0.m_fPlaybackSpeed=1.00f;
			Params0.m_nFlags = CA_LOOP_ANIMATION;
			m_pISkeletonAnim->StartAnimation("Additive_Idle_Threat_01", Params0);
		}
	}



	if (key_Q)
	{
		if (fDesiredSpeedSmooth>1.0f)
		{
			if (ThreatLevel==1)
			{
				fDesiredSpeedSmooth=1.7f;
				f32 fC1[4] = {0,1,0,1};
				gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Walking Relaxed");	
				g_YLine+=12.0f;

				fLookAround  = 0.50f;
				CryCharAnimationParams Params0(0);
				Params0.m_fTransTime=1.0f;
				Params0.m_nLayerID=2;
				Params0.m_fPlaybackSpeed=0.50f;
				Params0.m_nFlags = CA_LOOP_ANIMATION;
				m_pISkeletonAnim->StartAnimation("Additive_LookAround_01", Params0);
			}

			if (ThreatLevel==2)
			{
				fLookAround  = 0.00f;
				fDesiredSpeedSmooth=0.50f;
				f32 fC1[4] = {0,1,0,1};
				gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Walking Apprehensive");	
				g_YLine+=12.0f;

				CryCharAnimationParams Params0(0);
				Params0.m_fTransTime=1.0f;
				Params0.m_nLayerID=2;
				Params0.m_fPlaybackSpeed=1.0f;
				Params0.m_nFlags = CA_LOOP_ANIMATION;
				m_pISkeletonAnim->StartAnimation("Additive_LookAround_01", Params0);
			}

			if (ThreatLevel==3)
			{
				fLookAround  = 0.60f;
				fDesiredSpeedSmooth=1.3f;
				f32 fC1[4] = {0,1,0,1};
				gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Walking Threat");	
				g_YLine+=12.0f;

				CryCharAnimationParams Params0(0);
				Params0.m_fTransTime=1.0f;
				Params0.m_nLayerID=2;
				Params0.m_fPlaybackSpeed=1.0f;
				Params0.m_nFlags = CA_LOOP_ANIMATION;
				m_pISkeletonAnim->StartAnimation("Additive_LookAround_01", Params0);
			}
		}
	}
	else
	{
		if (fDesiredSpeedSmooth>1.0f)
		{
			if (ThreatLevel==1)
			{
				fDesiredSpeedSmooth=4.0f;
				f32 fC1[4] = {0,1,0,1};
				gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Running Relaxed");	
				g_YLine+=12.0f;

				fLookAround  = 0.30f;
				CryCharAnimationParams Params0(0);
				Params0.m_fTransTime=1.0f;
				Params0.m_nLayerID=2;
				Params0.m_fPlaybackSpeed=1.00f;
				Params0.m_nFlags = CA_LOOP_ANIMATION;
				m_pISkeletonAnim->StartAnimation("Additive_LookAround_01", Params0);
			}
			if (ThreatLevel==2)
			{
				fLookAround  = 0.50f;
				fDesiredSpeedSmooth=4.5f;
				f32 fC1[4] = {0,1,0,1};
				gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Running Apprehensive");	
				g_YLine+=12.0f;

				m_pISkeletonAnim->StopAnimationInLayer(1,1.5f);
				m_pISkeletonAnim->StopAnimationInLayer(2,1.5f);
			}
			if (ThreatLevel==3)
			{
				fLookAround  = 0.30f;
				fDesiredSpeedSmooth=6.0f;
				f32 fC1[4] = {0,1,0,1};
				gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 5.6f, fC1, false,"Running Threat");	
				g_YLine+=12.0f;

				m_pISkeletonAnim->StopAnimationInLayer(1,1.5f);

				CryCharAnimationParams Params2(0);
				Params2.m_fTransTime=1.0f;
				Params2.m_nLayerID=2;
				Params2.m_fPlaybackSpeed=1.0f;
				Params2.m_nFlags = CA_LOOP_ANIMATION;
				m_pISkeletonAnim->StartAnimation("Additive_leanforward_01", Params2);
			}
		}
	}

	m_pISkeletonAnim->SetAdditiveWeight(1,fBreathingSmooth);
	m_pISkeletonAnim->SetAdditiveWeight(2,fLookAroundSmooth);
	m_pISkeletonAnim->SetAdditiveWeight(3,1.0f);
	g_YLine+=12.0f;
	g_YLine+=12.0f;
	g_YLine+=12.0f;
	g_YLine+=12.0f;

	f32 fC1[4] = {0,1,0,1};
	gEnv->pRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC1, false,"fBreathingSmooth: %f  fLookAroundSmooth: %f",fBreathingSmooth,fLookAroundSmooth);	
	g_YLine+=12.0f;

	if ( (keyrcr_8&3)==1 )
	{
		CryCharAnimationParams Params0(0);
		Params0.m_nLayerID=3;
		Params0.m_fTransTime=0.0f;
		Params0.m_fPlaybackSpeed=1.00f;
		Params0.m_nFlags = CA_ALLOW_ANIM_RESTART;
		m_pISkeletonAnim->StartAnimation("Additive_flinch_left_01", Params0);
	}
	if ( (keyrcr_9&3)==1 )
	{
		CryCharAnimationParams Params0(0);
		Params0.m_nLayerID=3;
		Params0.m_fTransTime=0.0f;
		Params0.m_fPlaybackSpeed=1.00f;
		Params0.m_nFlags = CA_ALLOW_ANIM_RESTART;
		m_pISkeletonAnim->StartAnimation("Additive_flinch_back_01", Params0);
	}
	if ( (keyrcr_0&3)==1 )
	{
		CryCharAnimationParams Params0(0);
		Params0.m_nLayerID=3;
		Params0.m_fTransTime=0.0f;
		Params0.m_fPlaybackSpeed=1.00f;
		Params0.m_nFlags = CA_ALLOW_ANIM_RESTART;
		m_pISkeletonAnim->StartAnimation("Additive_flinch_right_01", Params0);
	}
	
	return fDesiredSpeedSmooth;
}



//////////////////////////////////////////////////////////////////////////
void CAALocomotion::DebugDraw( SDebugDrawInfo &info )
{
	f32 prevx = info.x;

	uint32 idxIdle =   AAClass_CLocomotion::EP_Idle;
	const char* strIdle      = m_pPrototype->GetProperties()[idxIdle].GetString().c_str();

	uint32 idxWalk =   AAClass_CLocomotion::EP_Walk;
	const char* strWalk      = m_pPrototype->GetProperties()[idxWalk].GetString().c_str();

	uint32 idxRun =   AAClass_CLocomotion::EP_Run;
	const char* strRun       = m_pPrototype->GetProperties()[idxRun].GetString().c_str();

	uint32 idxIdle2Walk =   AAClass_CLocomotion::EP_Idle2Walk;
	const char* strIdle2Walk = m_pPrototype->GetProperties()[idxIdle2Walk].GetString().c_str();

	uint32 idxIdle2Run =   AAClass_CLocomotion::EP_Idle2Run;
	const char* strIdle2Run  = m_pPrototype->GetProperties()[idxIdle2Run].GetString().c_str();

	//	const char* strRunStop   = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_RunStop].GetString().c_str();
	//	const char* strWalkStop  = m_pPrototype->GetProperties()[AAClass_CLocomotion::EP_WalkStop].GetString().c_str();

	int32 num=GetPrototype()->GetPropertyCount();
	const SAnimActionProperty& rAAProperty = GetPrototype()->GetProperty(0);

	f32 fTransWeight = 0.0f;
	ColorF color = ColorF(0.0f,0.0f,0.0f,1);


	fTransWeight=m_pActor->GetTransWeightFromFIFO( strIdle,-1 );
	color.r=fTransWeight;
	DrawAnimActionDebugText( info.x + 30.0f,info.y,1.3f,color,"%s (%2.0f%%)",strIdle,fTransWeight*100.0f );
	info.y += info.ystep;

	fTransWeight=m_pActor->GetTransWeightFromFIFO( strWalk,-1);
	color.r=fTransWeight;
	DrawAnimActionDebugText( info.x + 30.0f,info.y,1.3f,color,"%s (%2.0f%%)",strWalk,fTransWeight*100.0f );
	info.y += info.ystep;

	fTransWeight=m_pActor->GetTransWeightFromFIFO( strRun,-1 );
	color.r=fTransWeight;
	DrawAnimActionDebugText( info.x + 30.0f,info.y,1.3f,color,"%s (%2.0f%%)",strRun,fTransWeight*100.0f );
	info.y += info.ystep;

	fTransWeight=m_pActor->GetTransWeightFromFIFO( strIdle2Walk,-1 );
	color.r=fTransWeight;
	DrawAnimActionDebugText( info.x + 30.0f,info.y,1.3f,color,"%s (%2.0f%%)",strIdle2Walk,fTransWeight*100.0f );
	info.y += info.ystep;

	fTransWeight=m_pActor->GetTransWeightFromFIFO( strIdle2Run,-1 );
	color.r=fTransWeight;
	DrawAnimActionDebugText( info.x + 30.0f,info.y,1.3f,color,"%s (%2.0f%%)",strIdle2Run,fTransWeight*100.0f );
	info.y += info.ystep;

	info.x = prevx;
}

