//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimPreview.cpp
//  Implementation of the unit test to preview animations
//
//	History:
//	October 16, 2006: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "CharacterEditor\ModelViewportCE.h"
#include "CharacterEditor\CharPanel_Animation.h"

#include "ICryAnimation.h"
#include <I3DEngine.h>
#include <IPhysics.h>
#include <ITimer.h>
#include "IRenderAuxGeom.h"
#include "IMaterialEffects.h" // for footstep sound preview
#include "CharacterEditor/MotionAdaptorDlg.h"


#define NUM_FUTURE_POINTS (40)


extern uint32 g_ypos;


f32 g_TimeCount=0;

f32 g_AddRealTravelSpeed=0;
f32 g_AddRealTurnSpeed=0;

f32 g_RealTravelSpeedSec=0;
f32 g_RealTurnSpeedSec=0;


uint32 g_AnimEventCounter=0; 
AnimEventInstance g_LastAnimEvent; 
f32 g_fGroundHeight=0;

namespace
{
	void DrawArrow( IRenderer* pRenderer, const QuatT& location, f32 length, ColorB col );
	void DrawArrow( IRenderer* pRenderer, const QuatT& location, const Vec3& vTravelDir, f32 length, ColorB col );
}


//------------------------------------------------------------------------------
//---              animation previewer test-application                     ---
//------------------------------------------------------------------------------
void CModelViewportCE::AnimPreview_UnitTest( ICharacterInstance* pInstance,IAnimationSet* pIAnimationSet, const SRendParams &rRP )
{

	float color1[4] = {1,1,1,1};

	f32 FrameTime = GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();
	//m_AverageFrameTime = pInstance->GetAverageFrameTime(); 

	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	pAuxGeom->SetRenderFlags( renderFlags );

	GetISystem()->GetIAnimationSystem()->SetScalingLimits( Vec2(0.1f, 7.5f) );

	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
	ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();
	pISkeletonAnim->SetCharEditMode(1);

	IInput* pIInput = GetISystem()->GetIInput(); // Cache IInput pointer.
	pIInput->Update(true);

	uint32 nGroundAlign = 0;
	if (m_pCharPanel_Animation)
	{
		nGroundAlign = m_pCharPanel_Animation->GetGroundAlign();

		const char* RootName = pISkeletonPose->GetJointNameByID(0);
		//if (RootName[0]=='B' && RootName[1]=='i' && RootName[2]=='p')
		{
			uint32 AimIK	= m_pCharPanel_Animation->GetAimIK();
//			pISkeletonPose->SetAimIK(AimIK,m_vCamPos);
			if (IAnimationPoseBlenderAim* pPoseBlenderAim = pISkeletonPose->GetPoseBlenderAim())
			{
				pPoseBlenderAim->SetTarget(m_vCamPos);
				pPoseBlenderAim->SetArms(
					IAnimationPoseBlenderAim::eArm_Left |
					IAnimationPoseBlenderAim::eArm_Right);
				pPoseBlenderAim->SetBlendTime(-1.0f);
			}
			pISkeletonPose->SetPoseBlenderAimState(AimIK);
		}


/*
		static f32 cyaw=0;
		static f32 cpitch=0;
		cyaw+=m_relCameraRotZ;
		cpitch+=m_relCameraRotX;

		m_renderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"yew: %f  pitch: %f",cyaw,cpitch );
		g_ypos+=14;

		m_relCameraRotZ=0.0f;
		m_relCameraRotX=0.0f;
*/

		uint8 hasLookIK = pISkeletonPose->IsLookIKEnabled();
		if (hasLookIK != 0xca)
		{
			int32 LookIK				= m_pCharPanel_Animation->GetLookIK();
			pISkeletonPose->SetLookIK(LookIK,DEG2RAD(120),m_vCamPos);
		}
		else
		{
			int32 LookIK				= m_pCharPanel_Animation->GetLookIK();
			pISkeletonPose->SetLookIK(0xca,DEG2RAD(120),m_vCamPos);
		}


		//------------------------------------------------------
		//enable linear morph-target animation
		//------------------------------------------------------
		if (m_pCharacterAnim)
		{
			IMorphing* pIMorphing = m_pCharacterAnim->GetIMorphing();
			pIMorphing->SetLinearMorphSequence(-1);
			f32 lmslider	=	m_pCharPanel_Animation->GetLinearMorphSliderFlex(1.0f,m_AverageFrameTime);
			uint32 lms		=	m_pCharPanel_Animation->GetLinearMorphSequence();
			if (lms)
				pIMorphing->SetLinearMorphSequence(lmslider);
		}

		uint32 nLayer=m_pCharPanel_Animation->GetLayer();
		uint32 nDesiredLocomotionSpeed	=	m_pCharPanel_Animation->GetDesiredLocomotionSpeed();
		uint32 nDesiredTurnSpeed				=	m_pCharPanel_Animation->GetDesiredTurnSpeed();
		uint32 nLockMoveBody				    =	m_pCharPanel_Animation->GetLockMoveBody();

		/*
		if (nLockMoveBody)
		GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_EnableAssetTurning")->Set( 1 );
		else
		GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_EnableAssetTurning")->Set( 0 );
		*/
		f32 fBlendSpaceX=m_pCharPanel_Animation->GetBlendSpaceFlexX(1.0f,m_AverageFrameTime);
		f32 fBlendSpaceY=m_pCharPanel_Animation->GetBlendSpaceFlexY(1.0f,m_AverageFrameTime);
		f32 fBlendSpaceZ=m_pCharPanel_Animation->GetBlendSpaceFlexZ(1.0f,m_AverageFrameTime);

	//	f32 fManualAnimTime = m_pCharPanel_Animation->GetManualUpdateFlex(0.25f,m_AverageFrameTime);

		//strafe control
		if (mv_useKeyStrafe)
		{
			uint32 key_W=0,key_A=0,key_D=0,key_S=0;
			if (CheckVirtualKey(VK_NUMPAD8)) { key_S=1; }
			if (CheckVirtualKey(VK_NUMPAD2)) { key_W=1; }
			if (CheckVirtualKey(VK_NUMPAD4)) { key_A=1; }
			if (CheckVirtualKey(VK_NUMPAD6)) { key_D=1; }
			if (key_W) key_S=0;		//forward has priority
			if (key_A) key_D=0;		//left has priority

			Vec2 DesiredLocalMoveDirection	=	Vec2(0,0);
			if (key_W) DesiredLocalMoveDirection+=Vec2( 0,+1); //forward
			if (key_S) DesiredLocalMoveDirection+=Vec2( 0,-1); //back
			if (key_D) DesiredLocalMoveDirection+=Vec2( 1, 0);  //right
			if (key_A) DesiredLocalMoveDirection+=Vec2(-1, 0);  //left
			if ((key_W+key_S+key_D+key_A)==0)	DesiredLocalMoveDirection+=Vec2( 0,+1); //forward

			f32 length = DesiredLocalMoveDirection.GetLength();
			if (length>1.0f)
				DesiredLocalMoveDirection.Normalize();

			SmoothCD(m_vWorldDesiredMoveDirectionSmooth, m_vWorldDesiredMoveDirectionSmoothRate, FrameTime, DesiredLocalMoveDirection, 0.25f);


			//------------------------------------------------------------------------------
			// PMG strafing direction control

			static f32 strafAngleSmooth;
			static f32 strafeSmoothRate;
			f32 angleRad = ( m_vWorldDesiredMoveDirectionSmooth * Vec2(0, 1) ) / m_vWorldDesiredMoveDirectionSmooth.GetLength();
			f32 strafAngle = acos(angleRad);
			Vec3 crossZ = Vec3(.0f, 1.0f, .0f) ^ Vec3(m_vWorldDesiredMoveDirectionSmooth.x, m_vWorldDesiredMoveDirectionSmooth.y, .0f);
			f32 signAngle = crossZ.z >.0f? 1.0f: -1.0f;
			strafAngle *= signAngle;

			ISkeletonAnim* pISkeletonAnim =GetCharacterAnim()->GetISkeletonAnim();
			if(pISkeletonAnim)
			{
				f32 fDesiredSpeed					=	(fBlendSpaceX+1)*2.8f;  
				const f32 dirLen = m_vWorldDesiredMoveDirectionSmooth.GetLength();	

				pISkeletonAnim->SetControlParam( strafAngle, "TravelDir", 1);

				f32 finalSpeed = fDesiredSpeed * dirLen;
				if(finalSpeed < 0.2f)
					finalSpeed = .0f;
				pISkeletonAnim->SetControlParam( finalSpeed, "MoveSpeed", 0);	
			}
		}
		else
		{
			f32 key_lr=0;
			uint32 strg=0;
			if (CheckVirtualKey(VK_LCONTROL))  strg=1;
			if (CheckVirtualKey(VK_RCONTROL))  strg=1;
			if (strg==0)
			{
				if (CheckVirtualKey(VK_NUMPAD4)) { key_lr=+1.0f; }
				if (CheckVirtualKey(VK_NUMPAD6)) { key_lr=-1.0f; }
			}
			m_MoveDirRad += key_lr*FrameTime*1;
			m_vWorldDesiredMoveDirectionSmooth = Vec2(Matrix33::CreateRotationZ(m_MoveDirRad).GetColumn1());	
		}


		//------------------------------------------------------------------
		//parameterization for uphill/downhill
		//------------------------------------------------------------------

		g_fGroundHeight=0;
		f32 fGroundRadian=0;
		f32 fGroundRadianMoveDir=0;
		f32 fGroundDegreeMoveDir=0;

		if (nGroundAlign)
		{
			Lineseg ls_middle;
			ls_middle.start	=	Vec3(m_AnimatedCharacter.t.x,m_AnimatedCharacter.t.y,+9999.0f);
			ls_middle.end		=	Vec3(m_AnimatedCharacter.t.x,m_AnimatedCharacter.t.y,-9999.0f);

			Vec3 vGroundIntersection(0,0,-9999.0f);
			int8 ground = Intersect::Lineseg_OBB( ls_middle,m_GroundOBBPos,m_GroundOBB, vGroundIntersection );
			assert(ground);

			f32 fDesiredSpeed=(fBlendSpaceX+1)*5.0f;
			f32 t=clamp( (fDesiredSpeed-1)/7.0f,0.0f,1.0f);	t=t*t;
			f32 fScaleLimit = clamp(0.8f*(1.0f-t)+0.0f*t,0.2f,1.0f)-0.2f; 

			Vec3 vGroundNormal	=	m_GroundOBB.m33.GetColumn2()*m_AnimatedCharacter.q;
			fGroundRadian		= acos_tpl(vGroundNormal.z);
			g_fGroundHeight	= (vGroundIntersection.z-(fGroundRadian*0.15f));//*fScaleLimit;

			Vec3 gnormal	= Vec3(0,vGroundNormal.y,vGroundNormal.z);
			f32 cosine		=	Vec3(0,0,1)|gnormal;
			Vec3 sine			=	Vec3(0,0,1)%gnormal;
			fGroundRadianMoveDir = atan2( sgn(sine.x)*sine.GetLength(),cosine );
			fGroundDegreeMoveDir = RAD2DEG( fGroundRadianMoveDir );

			m_renderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"vGroundIntersection_z: %f", vGroundIntersection.z );
			//	m_renderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"GroundAngle: rad:%f  degree:%f  fGroundDegreeMoveDir:%f", fGroundAngle, RAD2DEG(fGroundAngle), fGroundDegreeMoveDir );
			g_ypos+=14;
		}


		//------------------------------------------------------------------

		uint32 BlendSpaceCode0=0;
		uint32 BlendSpaceCode1=0;

		uint32 numAnimsLayer = pISkeletonAnim->GetNumAnimsInFIFO(0);
		if (numAnimsLayer)
		{
			CAnimation& animation=pISkeletonAnim->GetAnimFromFIFO(0,0);
			//	uint32 ManualUpdateFlag = animation.m_AnimParams.m_nFlags&CA_MANUAL_UPDATE;
			//	if (ManualUpdateFlag)
			//		animation.m_fAnimTime=fManualAnimTime; //set animation time
			//	assert(animation.m_fAnimTime>=0.0f && animation.m_fAnimTime<=1.0f);

			int32 id0=animation.m_Parametric.m_nParametricID;
			if (id0>=0)
				BlendSpaceCode0 = pIAnimationSet->GetBlendSpaceCode(animation.m_Parametric.m_nParametricID);
		}


		{
			uint32 numAnimsLayer = pISkeletonAnim->GetNumAnimsInFIFO(nLayer);
			uint32 IsAdditive = 0;
			IAnimationSet* pAnimations = m_pCharacterAnim->GetIAnimationSet();
			if (pAnimations && numAnimsLayer)
			{
				CAnimation& animation=pISkeletonAnim->GetAnimFromFIFO(nLayer,0);
				if (animation.m_Parametric.m_nParametricID<0)
				{
					uint32 nAnimId = animation.m_Parametric.m_nAnimID[0];
					const char* name = pAnimations->GetNameByAnimID(nAnimId);
					if (name)
					{
						uint32 flags = pAnimations->GetAnimationFlags(nAnimId);
						IsAdditive=flags&CA_ASSET_ADDITIVE;
					}
				} 
			}
			m_pCharPanel_Animation->EnableAdditiveWeightWindow(numAnimsLayer!=0 && IsAdditive);

			m_pCharPanel_Animation->SmoothAdditiveWeights(m_AverageFrameTime);
			for (uint32 i=0; i<16; i++)
			{
				f32 fAddWeight=m_pCharPanel_Animation->GetAdditiveWeight(i);
				pISkeletonAnim->SetAdditiveWeight(i,fAddWeight);
			}
		}

		pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelSpeed, 0, 0) ;
		pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TurnSpeed, 0, 0) ;
		pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelSlope, 0, 0) ;
		pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_WeightShift, 0, 0) ;

		Vec2 vAvrgLocalMoveDirection;
		if (nLockMoveBody)
			vAvrgLocalMoveDirection=Vec2(m_vWorldDesiredMoveDirectionSmooth);
		else
			vAvrgLocalMoveDirection=Vec2(Vec3(m_vWorldDesiredMoveDirectionSmooth)*m_AnimatedCharacter.q);

		//--------------------------------------------------------------
		//----   calculate the natural moving speed                  ---
		//--------------------------------------------------------------
		f32 fDesiredSpeed					=	(fBlendSpaceX+1)*5.0f;  //thats the parameter we have to adjust
		f32 fDesiredTurnSpeed			= -fBlendSpaceY*gf_PI;
		f32 fTravelAngle					= -atan2f(vAvrgLocalMoveDirection.x,vAvrgLocalMoveDirection.y);
		f32 fNaturalDesiredSpeed	= fDesiredSpeed;

		if (mv_useNaturalSpeed)
		{
			//when a locomotion is slow (0.5-2.0f), then we can do this motion in all direction more or less at the same speed 
			f32 fMinTravelSpeed=1.0f;	
			f32 fMaxTravelSpeed=7.0f;	
			f32 fMinScaleSlow=0.8f;	
			f32 fMaxScaleFast=0.3f;	
			f32 t = sqr( clamp( (fDesiredSpeed-fMinTravelSpeed)/fMaxTravelSpeed,  0.0f,1.0f)  );
			f32 fScaleLimit = fMinScaleSlow*(1.0f-t)+fMaxScaleFast*t; //linar blend between min/max scale

			//adjust desired speed for turns
			f32 fSpeedScale=1.0f-fabsf(fDesiredTurnSpeed*0.40f)/gf_PI;
			fSpeedScale=clamp(fSpeedScale, fScaleLimit,1.0f);	

			//adjust desired speed when strafing and running backward
			f32 fStrafeSlowDown = (gf_PI-fabsf(fTravelAngle*0.60f))/gf_PI;
			fStrafeSlowDown=clamp(fStrafeSlowDown, fScaleLimit,1.0f);	

			//adjust desired speed when running uphill & downhill
			f32 fSlopeSlowDown = (gf_PI-fabsf(fGroundDegreeMoveDir/12.0f))/gf_PI;
			fSlopeSlowDown=clamp(fSlopeSlowDown, fScaleLimit,1.0f);	

			//BINGO! thats it
			fNaturalDesiredSpeed = fDesiredSpeed*min(fSpeedScale,min(fStrafeSlowDown,fSlopeSlowDown));
			m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"fScaleLimit: %f   fSlopeSlowDown: %f    fNaturalDesiredSpeed: %f",fScaleLimit,fSlopeSlowDown,fNaturalDesiredSpeed);
			g_ypos+=10;
		}



		//IMPORTANT: this function is overwriting the speed-parameter in SetIWeight() and changing the layerSpeed 
		if (nDesiredLocomotionSpeed)
			pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelSpeed,fNaturalDesiredSpeed,FrameTime);
		else
		{
			pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelSpeed, (1.0f+fBlendSpaceX)*0.5f, 1) ;
			pISkeletonAnim->SetLayerUpdateMultiplier(nLayer,1); //no scaling
		}

		pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_Scale, pInstance->GetUniformScale(), FrameTime);
		if (nDesiredTurnSpeed)
			pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TurnSpeed, -fBlendSpaceY*gf_PI, FrameTime) ;
		else
			pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TurnSpeed, (1.0f+fBlendSpaceY)*0.5f, 1) ;


		pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelSlope, (1.0f+fBlendSpaceZ)*0.5f, 1) ;
		if (nGroundAlign)
		{
			f32 fBlendVal = fGroundDegreeMoveDir/21.0f;
			if (fBlendVal<-1.0f) fBlendVal=-1.0f;
			if (fBlendVal>+1.0f) fBlendVal=+1.0f;
			pISkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TravelSlope, (1.0f+fBlendVal)*0.5f, 1) ;
		}

		//	f32 fTravelAngle = -atan2f(vAvrgLocalMoveDirection.x,vAvrgLocalMoveDirection.y);
		f32 fTravelScale = vAvrgLocalMoveDirection.GetLength();
		pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelAngle,fTravelAngle,FrameTime);
		//pISkeleton->SetDesiredMotionParam(eMotionParamID_TravelAngle,Vec2(vAvrgLocalMoveDirection.x,vAvrgLocalMoveDirection.y),FrameTime);
		pISkeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelDistScale, fTravelScale, FrameTime) ;




		//YELLOW arrow (desired direction)
		//		QuatT DesiredTravelArrowLocation=QuatT(m_AnimatedCharacter);

		uint32 numAnimsLayer0 = pISkeletonAnim->GetNumAnimsInFIFO(0);
		if (numAnimsLayer0==0)
		{
			QuatT DesiredTravelArrowLocation=QuatT(IDENTITY);
			f32 yaw = -atan2f(vAvrgLocalMoveDirection.x,vAvrgLocalMoveDirection.y);
			DesiredTravelArrowLocation.t += Vec3(0.0f,0.0f,0.3f);
			DesiredTravelArrowLocation.q *= Quat::CreateRotationZ(yaw);
			DrawArrow( m_renderer, DesiredTravelArrowLocation,vAvrgLocalMoveDirection.GetLength(),RGBA8(0xff,0xff,0xff,0x00) );
		}

		/*
		{
		static QuatT DesiredLocalLocation( Quat(IDENTITY), Vec3(0,1,0) );

		uint32 strg=0;
		if (CheckVirtualKey(VK_LCONTROL))  strg=1;
		if (CheckVirtualKey(VK_RCONTROL))  strg=1;
		if (strg)
		{
		if (CheckVirtualKey(VK_NUMPAD4)) { DesiredLocalLocation.t.x+=m_AverageFrameTime; }
		if (CheckVirtualKey(VK_NUMPAD6)) { DesiredLocalLocation.t.x-=m_AverageFrameTime; }

		if (CheckVirtualKey(VK_NUMPAD2)) { DesiredLocalLocation.t.y+=m_AverageFrameTime; }
		if (CheckVirtualKey(VK_NUMPAD8)) { DesiredLocalLocation.t.y-=m_AverageFrameTime; }
		}

		Vec3 line = DesiredLocalLocation.q.GetColumn2()*5;
		pAuxGeom->DrawLine(DesiredLocalLocation.t,RGBA8(0x00,0x00,0xff,0x00), DesiredLocalLocation.t+line,RGBA8(0x00,0x00,0xff,0x00) );
		pISkeleton->SetDesiredLocalLocation(DesiredLocalLocation, m_AverageFrameTime, 0);

		QuatT DesiredTravelArrowLocation	=	m_AnimatedCharacter;
		f32 yaw = -atan2f(DesiredLocalLocation.t.x,DesiredLocalLocation.t.y);
		DesiredTravelArrowLocation.t += Vec3(0.0f,0.0f,0.3f);
		DesiredTravelArrowLocation.q *= Quat::CreateRotationZ(yaw);
		DrawArrow(DesiredTravelArrowLocation,1.5f,RGBA8(0xff,0xff,0x00,0x00) );
		}
		*/

		//	m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false, "vAvrgLocalMoveDirection: %f %f",vAvrgLocalMoveDirection.x,vAvrgLocalMoveDirection.y); 
		//	g_ypos+=10;

		IAnimationSet* pIAnimationSet = GetCharacterBase()->GetIAnimationSet();
		if (pIAnimationSet == 0)
			return;

		uint32 numAnimsLayer2 = pISkeletonAnim->GetNumAnimsInFIFO(0);
		if (numAnimsLayer2)
		{

			static uint32 rcr_key_R=0;
			rcr_key_R<<=1;
			if ( CheckVirtualKey('R') )	rcr_key_R|=1;
			if ((rcr_key_R&3)==1) 
				pISkeletonPose->ApplyRecoilAnimation(0.20f,0.10f);

			const char* aname=0;
			CAnimation& animation=pISkeletonAnim->GetAnimFromFIFO(0,0);
			if (animation.m_Parametric.m_nParametricID>=0)
				aname = pIAnimationSet->GetNameByAnimID(animation.m_Parametric.m_nParametricID);			
			else
				aname = pIAnimationSet->GetNameByAnimID(animation.m_Parametric.m_nAnimID[0]); 

				if (mv_showMotionCaps)
				{
					int32 nAnimID = pIAnimationSet->GetAnimIDByName( aname );
					if (nAnimID >= 0)
					{
						uint32 flags = pIAnimationSet->GetAnimationFlags(nAnimID);
						if (flags & CA_ASSET_LMG)
						{
							LMGCapabilities caps = pIAnimationSet->GetLMGPropertiesByName( aname, vAvrgLocalMoveDirection, -fBlendSpaceY*gf_PI, fBlendSpaceZ );
							char name[5];	const char* bc = (const char*)(&caps.m_BlendType);
							name[0]=bc[0];	name[1]=bc[1];	name[2]=bc[2];	name[3]=bc[3]; name[4]=0;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_BlendType: %s",name);	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_bIsValid %d  caps.m_bIsLMG: %d",caps.m_bIsValid, caps.m_bIsLMG );	g_ypos+=16;

							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_bHasStrafing %d",caps.m_bHasStrafingAsset );	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_bHasTurning %d",caps.m_bHasTurningAsset );	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_bHasSlope %d",caps.m_bHasSlopeAsset );	g_ypos+=16;

							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_fSlowDuration: %f",caps.m_fSlowDuration );	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_fFastDuration: %f",caps.m_fFastDuration );	g_ypos+=16;

							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_fSlowTurnLeft: %f",caps.m_fSlowTurnLeft );	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_fSlowTurnRight: %f",caps.m_fSlowTurnRight );	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_fFastTurnLeft: %f",caps.m_fFastTurnLeft );	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_fFastTurnRight: %f",caps.m_fFastTurnRight );	g_ypos+=16;

							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_vMinVelocity: %f %f  Speed: %f   StepDistance: %f",caps.m_vMinVelocity.x,caps.m_vMinVelocity.y, caps.m_vMinVelocity.GetLength(), caps.m_vMinVelocity.GetLength()*caps.m_fSlowDuration );	g_ypos+=16;
							m_renderer->Draw2dLabel(12,g_ypos,1.8f,color1,false,"caps.m_vMaxVelocity: %f %f  Speed: %f   StepDistance: %f",caps.m_vMaxVelocity.x,caps.m_vMaxVelocity.y, caps.m_vMaxVelocity.GetLength(), caps.m_vMaxVelocity.GetLength()*caps.m_fFastDuration );	g_ypos+=16;
							g_ypos+=20;
						}
					}
				}

			if (mv_showStartLocation)
			{
				const QuatT& StartLocation = pIAnimationSet->GetAnimationStartLocation(aname);
				m_renderer->Draw2dLabel(12,g_ypos,1.6f,color1,false,"InvStartRot: %f (%f %f %f)",StartLocation.q.w,StartLocation.q.v.x,StartLocation.q.v.y,StartLocation.q.v.z );	g_ypos+=15;	
				m_renderer->Draw2dLabel(12,g_ypos,1.6f,color1,false,"InvStartPos: %f %f %f",StartLocation.t.x,StartLocation.t.y,StartLocation.t.z );	g_ypos+=15;	

				static Ang3 angle=Ang3(ZERO);
				angle.x += 0.1f;
				angle.y += 0.01f;
				angle.z += 0.001f;
				AABB sAABB = AABB(Vec3(-0.05f,-0.05f,-0.05f),Vec3(+0.05f,+0.05f,+0.05f));
				OBB obb =	OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle), sAABB );
				pAuxGeom->DrawOBB(obb,StartLocation.t,1,RGBA8(0xff,0xff,0xff,0xff),eBBD_Extremes_Color_Encoded);

				//draw start coordinates;
				Vec3 xaxis=StartLocation.q.GetColumn0()*0.5f;
				Vec3 yaxis=StartLocation.q.GetColumn1()*0.5f;
				Vec3 zaxis=StartLocation.q.GetColumn2()*0.5f;
				pAuxGeom->DrawLine( StartLocation.t+Vec3(0,0,0.02f),RGBA8(0xff,0x00,0x00,0x00), StartLocation.t+xaxis+Vec3(0,0,0.02f),RGBA8(0xff,0x00,0x00,0x00) );
				pAuxGeom->DrawLine( StartLocation.t+Vec3(0,0,0.02f),RGBA8(0x00,0xff,0x00,0x00), StartLocation.t+yaxis+Vec3(0,0,0.02f),RGBA8(0x00,0xff,0x00,0x00) );
				pAuxGeom->DrawLine( StartLocation.t+Vec3(0,0,0.02f),RGBA8(0x00,0x00,0xff,0x00), StartLocation.t+zaxis+Vec3(0,0,0.02f),RGBA8(0x00,0x00,0xff,0x00) );
			}
		}

	}

	//--------------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------
	uint32 useFootAnchoring=0;
	if (m_pCharPanel_Animation)
		useFootAnchoring = m_pCharPanel_Animation->GetFootAnchoring();


	pISkeletonAnim->SetCharEditMode(1);
	pISkeletonPose->SetForceSkeletonUpdate(1);
	pISkeletonPose->SetFootAnchoring( useFootAnchoring );

	int PreProcessCallback(ICharacterInstance* pInstance,void* pPlayer);
	pISkeletonAnim->SetPreProcessCallback(PreProcessCallback,this);

	int PostProcessCallback(ICharacterInstance* pInstance,void* pPlayer);
	if (!m_weaponIK)
		pISkeletonPose->SetPostProcessCallback(PostProcessCallback,this);

	int AnimEventCallback(ICharacterInstance* pInstance,void* pPlayer);
	pISkeletonAnim->SetEventCallback(AnimEventCallback,this);

	bool bTransRot2000	= UseAnimationDrivenMotion();
	pISkeletonAnim->SetAnimationDrivenMotion(bTransRot2000);

	bool bMirror	= (m_pCharPanel_Animation ? m_pCharPanel_Animation->GetMirrorAnimation() : false);
	pISkeletonAnim->SetMirrorAnimation(bMirror);


	uint32 nAimIKLayer	= (m_pCharPanel_Animation ? m_pCharPanel_Animation->GetAimIKLayerValue() : 16);
	pISkeletonPose->SetPoseBlenderAimLayer(nAimIKLayer);

	//--------------------------------------------------------------------------------------------------------------

	if(m_pMotionAdaptorDlg)	{
		for (uint32 i=0; i<16; i++)		{ // maximal layer = 16
			uint32 nAnimsInQueue=pISkeletonAnim->GetNumAnimsInFIFO(i);
			if (nAnimsInQueue){
				m_pMotionAdaptorDlg->m_dlgMain.ClearAnimation();
				break;
			}
		}
		uint32 numJoints = m_pMotionAdaptorDlg->m_dlgMain.GetViconJointNum();
		uint32 numFrames = 0;

		if(numJoints>0)
			numFrames = m_pMotionAdaptorDlg->m_dlgMain.GetViconFrameCount();
		if (numFrames)
		{
			m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"Motion Adaptor Updateloop:  numJoints: %d     numFrames: %d",numJoints, numFrames );	
			g_ypos+=10;
			m_pMotionAdaptorDlg->m_dlgMain.MotionPlayback(GetCharacterBase());
		}			
	}

	//--------------------------------------------------------------------------------------------------------------



	static f32 RadomRot=0.0f;
	//	RadomRot+=FrameTime*2.0f;
	m_PhysEntityLocation.q.SetRotationZ( RadomRot );
	m_PhysEntityLocation.t = Vec3(ZERO);

	m_AnimatedCharacter.t.x	=	0.0f;
	m_AnimatedCharacter.t.y	=	0.0f;
	if (nGroundAlign)
		m_AnimatedCharacter.t.z	=	g_fGroundHeight;

	ICompoundCharacter *pCharacter = GetCompoundCharacter();
	if( pCharacter && fabs(pInstance->GetUniformScale()-m_fUniformScaling)>0.0001f )
	{
		pCharacter->SetScale(m_fUniformScaling);
		Physicalize();
	}
	m_AnimatedCharacter.s=m_fUniformScaling;


	//	pInstance->SetWorldLocation(m_AnimatedCharacter);
	pInstance->SkeletonPreProcess( m_PhysEntityLocation,m_AnimatedCharacter, GetCamera(),0 );
	pInstance->FinishAnimationComputations();

	m_AnimatedCharacter.t.x	=	0.0f;
	m_AnimatedCharacter.t.y	=	0.0f;
	if (nGroundAlign)
		m_AnimatedCharacter.t.z	=	g_fGroundHeight;


	if (mv_printDebugText)
	{
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"m_AnimatedCharacter.t.z: %f",m_AnimatedCharacter.t.z );	
		g_ypos+=10;
	}

	m_PhysEntityLocation.t=Vec3(ZERO);


	//	pInstance->SetWorldLocation(m_AnimatedCharacter);

	if ( ! pISkeletonAnim->GetAnimationDrivenMotion() )
	{
		m_AnimatedCharacter.q.SetIdentity();
	}
	DrawLocator( QuatT( m_AnimatedCharacter ), pISkeletonAnim->GetCurrentVelocity() );

	pInstance->SkeletonPostProcess( m_PhysEntityLocation, m_AnimatedCharacter, 0, (m_PhysEntityLocation.t-m_absCameraPos).GetLength(), 0 ); 


	//int32 idx=pISkeleton->GetIDByName("weapon_bone");
	//QuatT WeaponBone = pISkeleton->GetAbsJointByID(idx);
	//m_renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"WeaponBone: %f %f %f", WeaponBone.t.x,WeaponBone.t.y,WeaponBone.t.z );	ypos+=10;


	m_absCurrentSpeed			=	pISkeletonAnim->GetCurrentVelocity().GetLength();
	m_absCurrentSlope			=	pISkeletonAnim->GetCurrentSlope();


	//YELLOW real travel direction
	Vec3 vCurrentLocalMoveDirection =	pISkeletonAnim->GetCurrentVelocity(); //.GetNormalizedSafe( Vec3(0,1,0) );;
	//QuatT offset = pISkeletonAnim->GetRelMovement().GetInverted();
	//QuatT CurrentTravelArrowLocation=QuatT(m_AnimatedCharacter)*offset;
	//f32 yaw = -atan2f(vCurrentLocalMoveDirection.x,vCurrentLocalMoveDirection.y);
	//CurrentTravelArrowLocation.t += Vec3(0.0f,0.0f,0.3f);
	//CurrentTravelArrowLocation.q *= Quat::CreateRotationZ(yaw);
	//DrawArrow(CurrentTravelArrowLocation,vCurrentLocalMoveDirection.GetLength(),RGBA8(0x7f,0x7f,0x00,0x00) );

	//GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_DrawLocator")->Set( mv_showLocator );

	Vec3 CurrentVelocity =  m_AnimatedCharacter.q * pISkeletonAnim->GetCurrentVelocity() ;
	//	Vec3 CurrentBodyDir =  m_AnimatedCharacter.q.GetColumn1() ;
	//	Vec3 absPelvisPos	=	 m_PhysEntityLocation.q * pISkeleton->GetAbsJointByID(1).t;
	//	pAuxGeom->DrawLine( absPelvisPos,RGBA8(0xff,0xff,0x00,0x00), absPelvisPos+CurrentVelocity*2,RGBA8(0x00,0x00,0x00,0x00) );
	//	pAuxGeom->DrawLine( absPelvisPos,RGBA8(0xff,0x00,0x00,0x00), absPelvisPos+CurrentBodyDir*2,RGBA8(0x00,0x00,0x00,0x00) );


	if (mv_printDebugText)
	{
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false, "CurrTravelDirection: %f %f",CurrentVelocity.x,CurrentVelocity.y); 
		g_ypos+=10;
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false, "vCurrentLocalMoveDirection: %f %f",vCurrentLocalMoveDirection.x,vCurrentLocalMoveDirection.y); 
		g_ypos+=10;
	}


	//draw the diagonal lines;
	//	pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3( 1, 1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );
	//	pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3(-1, 1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );
	//	pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3( 1,-1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );
	//	pAuxGeom->DrawLine( absRoot.t+Vec3(0,0,0.02f),RGBA8(0xff,0x7f,0x00,0x00), absRoot.t+Vec3(0,0,0.02f)+Vec3(-1,-1, 0)*90,RGBA8(0x00,0x00,0x00,0x00) );

	assert(rRP.pMatrix);
	assert(rRP.pPrevMatrix);

	const QuatT& qt = pISkeletonAnim->GetRelMovement();
	Vec3 MotionTranslation = -qt.t;
	f32 MotionRotation = Ang3::GetAnglesXYZ(qt.q).z;

	bool bVerticalMovement	= (m_pCharPanel_Animation ? m_pCharPanel_Animation->GetVerticalMovement() : false);
	if (bVerticalMovement==0)
		MotionTranslation.z=0;

	if (mv_showGrid)
		DrawGrid( m_AnimatedCharacter.q, MotionTranslation, Vec3(ZERO), m_GroundOBB.m33);
	if (mv_showBase)
		DrawCoordSystem( IDENTITY,10.0f); 		//DrawCoordSystem( QuatT(m_AnimatedCharacter) ,10.0f);


	m_EntityMat			=	Matrix34(m_PhysEntityLocation);
	SRendParams rp = rRP;
	rp.pMatrix			= &m_EntityMat;
	rp.pPrevMatrix	= &m_PrevEntityMat;
	rp.fDistance		= (m_PhysEntityLocation.t-m_Camera.GetPosition()).GetLength();
	AABB aabb = pInstance->GetAABB();	
	uint32 visible = GetCamera().IsAABBVisible_E( aabb );
	if (visible)
		pInstance->Render( rp, QuatTS(IDENTITY) );


	//-------------------------------------------------
	//---      draw path of the past
	//-------------------------------------------------
	Matrix33 m33=Matrix33(m_AnimatedCharacter.q);
	Matrix34 m34=Matrix34(m_AnimatedCharacter);


	uint32 numEntries=m_arrAnimatedCharacterPath.size();
	for (uint32 i=0; i<numEntries; i++)
		m_arrAnimatedCharacterPath[i] += m33*MotionTranslation;

	for (int32 i=(numEntries-2); i>-1; i--)
		m_arrAnimatedCharacterPath[i+1] = m_arrAnimatedCharacterPath[i];
	m_arrAnimatedCharacterPath[0] = Vec3(ZERO); 

	for (uint32 i=0; i<numEntries; i++)
	{
		AABB aabb;
		aabb.min=Vec3(-0.01f,-0.01f,-0.01f)+m_arrAnimatedCharacterPath[i]+m_AnimatedCharacter.t;
		aabb.max=Vec3(+0.01f,+0.01f,+0.01f)+m_arrAnimatedCharacterPath[i]+m_AnimatedCharacter.t;
		pAuxGeom->DrawAABB(aabb,1, RGBA8(0x00,0x00,0xff,0x00),eBBD_Extremes_Color_Encoded );
	}








	GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_DrawSkeleton")->Set( mv_showSkeleton );
	if(m_pCharPanel_Animation)
	{	
		int32 mt=m_pCharPanel_Animation->GetUseMorphTargets();
		GetIEditor()->GetSystem()->GetIConsole()->GetCVar("ca_UseMorph")->Set( mt );
	}

	//	f32 last_time; 
	//	const char* last_AnimPath; 
	//	const char* last_EventName; 
	//	const char* last_SoundName;

	if (mv_printDebugText)
	{
		g_ypos+=20;
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"AnimEventCounter: %d",g_AnimEventCounter);	
		g_ypos+=10;
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"time: %f     EventName:''%s''     Parameter:''%s''    PathName:''%s''",g_LastAnimEvent.m_time, g_LastAnimEvent.m_EventName,g_LastAnimEvent.m_CustomParameter, g_LastAnimEvent.m_AnimPathName);	
		g_ypos+=10;
	}


	g_TimeCount += FrameTime;

	g_AddRealTravelSpeed	+= MotionTranslation.GetLength(); 
	g_AddRealTurnSpeed		+= MotionRotation; 

	if (g_TimeCount>1.0f)
	{
		g_TimeCount			= 0;

		g_RealTravelSpeedSec	=	g_AddRealTravelSpeed;
		g_RealTurnSpeedSec		=	g_AddRealTurnSpeed;
		g_AddRealTravelSpeed	= 0; 
		g_AddRealTurnSpeed		= 0; 
	}

	if (mv_showMotionParam)
	{
		static float colorRed[4] = {1.0f, .0f, .0f, 1.0f};
		m_renderer->Draw2dLabel(1,1,1.5f,colorRed,false,"Real Travel Speed %f ",g_RealTravelSpeedSec);	
		m_renderer->Draw2dLabel(1,20,1.5f,colorRed,false,"Real Turn Speed %f ",g_RealTurnSpeedSec);	
	}


	//--------------------------------------------------------------
	//--------------------------------------------------------------
	//--------------------------------------------------------------

	if (m_pCharPanel_Animation)
	{
		ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
		uint32 nAnimLayer0 = pISkeletonAnim->GetNumAnimsInFIFO(0);
		uint32 nAnimLayer1 = pISkeletonAnim->GetNumAnimsInFIFO(1);
		uint32 nAnimLayer2 = pISkeletonAnim->GetNumAnimsInFIFO(2);
		uint32 status=0;
		status|=(nAnimLayer0 && nAnimLayer1);
		status|=(nAnimLayer0 && nAnimLayer2);

		if (nAnimLayer0+nAnimLayer1+nAnimLayer2)
		{
			uint32 IsLMG=0;
			uint32 BothSlotsUsed=0;
			if (nAnimLayer0)
			{
				CAnimation& animation=pISkeletonAnim->GetAnimFromFIFO(0,0);
				IsLMG |= (animation.m_Parametric.m_nParametricID > -1);
			}
			if (nAnimLayer1)
			{
				CAnimation& animation=pISkeletonAnim->GetAnimFromFIFO(1,0);
				IsLMG |= (animation.m_Parametric.m_nParametricID > -1);
			}
			if (nAnimLayer2)
			{
				CAnimation& animation=pISkeletonAnim->GetAnimFromFIFO(2,0);
				IsLMG |= (animation.m_Parametric.m_nParametricID > -1);
			}

			m_pCharPanel_Animation->EnableWindow_BlendSpaceSliderX(IsLMG);
			m_pCharPanel_Animation->EnableWindow_BlendSpaceSliderY(IsLMG);
			m_pCharPanel_Animation->EnableWindow_BlendSpaceSliderZ(IsLMG);
		}
		else
		{
			m_pCharPanel_Animation->EnableWindow_BlendSpaceSliderX(0);
			m_pCharPanel_Animation->EnableWindow_BlendSpaceSliderY(0);
			m_pCharPanel_Animation->EnableWindow_BlendSpaceSliderZ(0);
		}
	}

	//------------------------------------------------------------------------
	//---        Attach camera to socket                                   ---
	//------------------------------------------------------------------------
	if (mv_AttachCamera)
	{
		static Vec3 vCamPos				=	Vec3(0.0f,-4.3f, 3.0f);
		static Vec3 vCamPosSmooth	=	Vec3(0.0f,-4.3f, 3.0f);
		static Vec3 vCamPosRate		=	Vec3(0.0f,-4.3f, 3.0f);

		static Quat vCamRot       = Quat(IDENTITY);
		static Quat vCamRotSmooth = Quat(IDENTITY);
		static Quat vCamRotRate   = Quat(IDENTITY);

		IAttachmentManager* pIAttachmentManager = pInstance->GetIAttachmentManager();
		if (pIAttachmentManager)
		{
			IAttachment*  pIAttachment = pIAttachmentManager->GetInterfaceByName("#camera");
			if (pIAttachment)
			{
				QuatTS qt	=	pIAttachment->GetAttWorldAbsolute();
				qt =  m_AnimatedCharacter.GetInverted()*qt;

				f32 sign = sgnnz(vCamRotSmooth|qt.q);
				vCamRot	=	qt.q*sign;
				vCamPos	=	qt.t;

				SmoothCD(vCamRotSmooth, vCamRotRate, FrameTime, vCamRot, mv_CameraSmoothRot);	vCamRotSmooth.NormalizeSafe();
				SmoothCD(vCamPosSmooth, vCamPosRate, FrameTime, vCamPos, mv_CameraSmoothPos);
				SetViewTM( Matrix34(Matrix33(m_AnimatedCharacter.q*vCamRotSmooth),m_AnimatedCharacter*vCamPosSmooth) );
			}
		}
	}


	//------------------------------------------------------------------------
	//---  camera control
	//------------------------------------------------------------------------


	//--------------------------------------------------------------------------------------------------


	if (0)
	{
		static Vec3 vLookAt					=	Vec3(0.0f, 0.0f, 1.2f);
		static Vec3 vLookAtSmooth		=	Vec3(0.0f, 0.0f, 1.2f);
		static Vec3 vLookAtRate			=	Vec3(0.0f, 0.0f, 1.2f);

		static Vec3 vLookFrom				=	Vec3(0.0f,-4.3f, 3.0f);
		static Vec3 vLookFromSmooth	=	Vec3(0.0f,-4.3f, 3.0f);
		static Vec3 vLookFromRate		=	Vec3(0.0f,-4.3f, 3.0f);

		if ( CheckVirtualKey('7') ) 
		{
			vLookAt		=	Vec3(0,0,0.7f);
			vLookFrom	=	Vec3(-2.0f,4.0f,1.0f);
		}

		if ( CheckVirtualKey('8') ) 
		{
			vLookAt		=	Vec3(0,0,0.7f);
			vLookFrom	=	Vec3(-5.0f,0.0f,2.0f);
		}

		if ( CheckVirtualKey('9') ) 
		{
			vLookAt		=	Vec3(0,0,0.7f);
			vLookFrom	=	Vec3(-2.0f,-3.0f,3.5f);
		}

		SmoothCD(vLookAtSmooth,		vLookAtRate,		FrameTime, vLookAt, 0.15f);
		SmoothCD(vLookFromSmooth, vLookFromRate,	FrameTime, vLookFrom, 0.30f);
		m_absCameraPos=vLookFromSmooth;
		Matrix33 orientation	= Matrix33::CreateRotationVDir( (vLookAtSmooth-vLookFromSmooth).GetNormalized() );
		SetViewTM( Matrix34(orientation,vLookFromSmooth) );
	}

}








int AnimEventCallback(ICharacterInstance* pInstance,void* pPlayer)
{

	//process bones specific stuff (IK, torso rotation, etc)
	((CModelViewportCE*)pPlayer)->AnimEventProcessing(pInstance);
	return 1;
}


void CModelViewportCE::AnimEventProcessing(ICharacterInstance* pInstance)
{

	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
	ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();
	g_AnimEventCounter++;
	g_LastAnimEvent = pISkeletonAnim->GetLastAnimEvent();


	// If the event is a sound event, play the sound.
	uint32 s0 = stricmp(g_LastAnimEvent.m_EventName, "sound") == 0;
	uint32 s1 = stricmp(g_LastAnimEvent.m_EventName, "sound_tp") == 0;
	if (g_LastAnimEvent.m_EventName && (s0 || s1))
	{
		// TODO verify looping sounds not get "lost" and continue to play forever
		_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound( g_LastAnimEvent.m_CustomParameter, FLAG_SOUND_DEFAULT_3D|FLAG_SOUND_LOAD_SYNCHRONOUSLY );
		if (pSound != 0)
		{
			if (!(pSound->GetFlags() & FLAG_SOUND_EVENT))
				pSound->GetInterfaceDeprecated()->SetMinMaxDistance( 1, 50 );

			pSound->Play();
			m_SoundIDs.push_back(pSound->GetId());
		}
	}
	// If the event is an footstep event, play a generic footstep sound.
	else if (g_LastAnimEvent.m_EventName && stricmp(g_LastAnimEvent.m_EventName, "footstep") == 0)
	{

		// setup sound params
		SMFXRunTimeEffectParams params;
		params.angle = 0;
		params.soundSemantic = eSoundSemantic_Physics_Footstep;

		IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;
		TMFXEffectId effectId = InvalidEffectId;

		// note: for some reason material libraries are named "footsteps" when queried by name
		// and "footstep" when queried by ID
/*		if (g_LastAnimEvent.m_CustomParameter[0])
		{
			effectId = pMaterialEffects->GetEffectIdByName("footsteps", g_LastAnimEvent.m_CustomParameter);
		}
		else
		{
			effectId = pMaterialEffects->GetEffectIdByName("footsteps", "default");
		}*/

		if (effectId != 0)
			pMaterialEffects->ExecuteEffect(effectId, params);
		else
			gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR,VALIDATOR_WARNING,VALIDATOR_FLAG_SOUND,0,"Failed to find material for footstep sounds");

	}
	// If the event is an effect event, spawn the event.
	else if (g_LastAnimEvent.m_EventName && stricmp(g_LastAnimEvent.m_EventName, "effect") == 0)
	{
		if (GetCharacterBase())
		{
			ISkeletonAnim* pSkeletonAnim = GetCharacterBase()->GetISkeletonAnim();
			ISkeletonPose* pSkeletonPose = GetCharacterBase()->GetISkeletonPose();
			m_effectManager.SetSkeleton(pSkeletonAnim,pSkeletonPose);
			m_effectManager.SpawnEffect(g_LastAnimEvent.m_AnimID, g_LastAnimEvent.m_AnimPathName, g_LastAnimEvent.m_CustomParameter,
				g_LastAnimEvent.m_BonePathName, g_LastAnimEvent.m_vOffset, g_LastAnimEvent.m_vDir);
		}
	}



	//----------------------------------------------------------------------------------------

	IAttachmentManager* pIAttachmentManager = pInstance->GetIAttachmentManager();
	IAttachment* pSAtt1 = pIAttachmentManager->GetInterfaceByName("riflepos01");
	IAttachment* pSAtt2 = pIAttachmentManager->GetInterfaceByName("riflepos02");
	IAttachment* pWAtt = pIAttachmentManager->GetInterfaceByName("weapon");
	if (g_LastAnimEvent.m_EventName)
	{

		int32 nWeaponIdx	=	pISkeletonPose->GetJointIDByName("weapon_bone");
		int32 nSpine2Idx	=	pISkeletonPose->GetJointIDByName("Bip01 Spine2");
		if (nWeaponIdx<0) 
			return;
		if (nSpine2Idx<0) 
			return;

		QuatT absWeaponBone			= pISkeletonPose->GetAbsJointByID(nWeaponIdx);
		QuatT absSpine2					= pISkeletonPose->GetAbsJointByID(nSpine2Idx);
		QuatT defSpine2				  = pISkeletonPose->GetDefaultAbsJointByID(nSpine2Idx);

		//QuatT relative1 					= (absWeaponBone.GetInverted()*absSpine2)*defSpine2;
		QuatT AttachmentLocation		= defSpine2*(absSpine2.GetInverted()*absWeaponBone);

		if (stricmp(g_LastAnimEvent.m_EventName, "WeaponDrawRight") == 0)
		{
			pSAtt1->HideAttachment(1); //hide
			pSAtt2->HideAttachment(0); //show 
			pWAtt->HideAttachment(0);  //show
		}

		if (stricmp(g_LastAnimEvent.m_EventName, "WeaponDrawLeft") == 0)
		{
			pSAtt1->HideAttachment(0);
			pSAtt2->HideAttachment(1);
			pWAtt->HideAttachment(0);  //show
		}

	}

	uint32 i=0;
}







//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
int PreProcessCallback(ICharacterInstance* pInstance,void* pPlayer)
{
	//process bones specific stuff (IK, torso rotation, etc)
	((CModelViewportCE*)pPlayer)->PreProcessCallback(pInstance);
	return 1;
}
void CModelViewportCE::PreProcessCallback(ICharacterInstance* pInstance)
{
	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();

	// this is the kinematic update
	QuatT relmove = pISkeletonAnim->GetRelMovement();

	bool bVerticalMovement	= (m_pCharPanel_Animation ? m_pCharPanel_Animation->GetVerticalMovement() : false);
	if (bVerticalMovement==0)
		relmove.t.z=0;

	float color1[4] = {1,1,1,1};
	if (mv_printDebugText)
	{
		IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"Path Following Callback" );
		g_ypos+=10;
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"relmove: rot:(%f %f %f %f)  pos:(%f %f %f)",relmove.q.w, relmove.q.v.x,relmove.q.v.y,relmove.q.v.z,   relmove.t.x,relmove.t.y,relmove.t.z );
		g_ypos+=10;
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"m_AnimatedCharacter: rot:(%f %f %f %f)  pos:(%f %f %f)",m_AnimatedCharacter.q.w, m_AnimatedCharacter.q.v.x,m_AnimatedCharacter.q.v.y,m_AnimatedCharacter.q.v.z,   m_AnimatedCharacter.t.x,m_AnimatedCharacter.t.y,m_AnimatedCharacter.t.z );
		g_ypos+=10;

		pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
		if (m_AverageFrameTime > 0.0f)
		{
			Vec3 rel = (-m_AnimatedCharacter.q*(relmove.t*relmove.q))/m_AverageFrameTime;
			pAuxGeom->DrawLine(Vec3(0,0,0.1f),RGBA8(0x00,0x00,0x7f,0x00), Vec3(0,0,0.1f)+rel,RGBA8(0xff,0xff,0xff,0x00) );
		}

		static f32 kold=0.0f;	
		static f32 knew=0.0f;
		kold=knew;
		ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();
		knew = pISkeletonPose->GetAbsJointByID(0).t.GetLength();
		m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"root move: %f" , knew-kold );
		g_ypos+=10;
		if (m_AverageFrameTime > 0.0f)
		{
			Vec3 rel = Vec3(0,(knew-kold)/m_AverageFrameTime,0);
			pAuxGeom->DrawLine(Vec3(0,0,0.2f),RGBA8(0x00,0x00,0x7f,0x00), Vec3(0,0,0.2f)+rel,RGBA8(0xff,0x00,0x00,0x00) );
		}
	}

	m_AnimatedCharacter = m_AnimatedCharacter*relmove; 
	m_AnimatedCharacter.q.Normalize();


	return;
}







//---------------------------------------------------------------------
//---------------------------------------------------------------------
//---------------------------------------------------------------------
uint32 CModelViewportCE::UseHumanLimbIK(ICharacterInstance* pInstance, const char* strLimb)
{

	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
	ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();

	AABB aabb = pInstance->GetAABB();
	f32 fRadius=sqrt_tpl((aabb.max-aabb.min).GetLengthFast()*0.8f);	
	
	static Vec3 target=Vec3(ZERO);

	Matrix34 VMat = GetCamera().GetViewMatrix();
	Vec3 lr=Vec3(VMat.m00,VMat.m01,VMat.m02)*m_AverageFrameTime*fRadius;
	Vec3 fb=Vec3(VMat.m10,VMat.m11,VMat.m12)*m_AverageFrameTime*fRadius;
	Vec3 ud=Vec3(VMat.m20,VMat.m21,VMat.m22)*m_AverageFrameTime*fRadius;
	int32 STRG = CheckVirtualKey(VK_CONTROL);
	int32 np1 = CheckVirtualKey(VK_NUMPAD1);
	int32 np2 = CheckVirtualKey(VK_NUMPAD2);
	int32 np3 = CheckVirtualKey(VK_NUMPAD3);
	int32 np4 = CheckVirtualKey(VK_NUMPAD4);
	int32 np5 = CheckVirtualKey(VK_NUMPAD5);
	int32 np6 = CheckVirtualKey(VK_NUMPAD6);
	int32 np7 = CheckVirtualKey(VK_NUMPAD7);
	int32 np8 = CheckVirtualKey(VK_NUMPAD8);
	int32 np9 = CheckVirtualKey(VK_NUMPAD9);

	if(STRG&&np8) target+=ud;
	else if (STRG&&np2)	 target-=ud;
	if(!STRG&&np8) target+=fb;
	else if (!STRG&&np2) target-=fb;
	if(np6) target+=lr;
	else if (np4) target-=lr;

	static Ang3 angle=Ang3(ZERO);
	angle.x += 0.1f;
	angle.y += 0.01f;
	angle.z += 0.001f;
	AABB sAABB = AABB(Vec3(-0.03f,-0.03f,-0.03f)*fRadius,Vec3(+0.03f,+0.03f,+0.03f)*fRadius);
	OBB obb =	OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle), sAABB );
	pAuxGeom->DrawOBB(obb,target,1,RGBA8(0xff,0xff,0xff,0xff),eBBD_Extremes_Color_Encoded);

	pISkeletonPose->SetHumanLimbIK(target,strLimb);

	return 1;
}






//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void CModelViewportCE::DrawLocator( const QuatT& location, const Vec3& moveDirectionLocal )
{
	if ( ! mv_showLocator )
	{
		return;
	}

	bool animationSystemIsDrawingLocator = ( GetIEditor()->GetSystem()->GetIConsole()->GetCVar( "ca_DrawLocator" )->GetIVal() != 0 );
	if ( animationSystemIsDrawingLocator )
	{
		return;
	}
		
	QuatT viewingDirectionArrowLocation = location;
	viewingDirectionArrowLocation.t += Vec3( 0, 0, 0.01f );
	DrawArrow( m_renderer, viewingDirectionArrowLocation, 1.0f, RGBA8( 0x00, 0xff, 0x00, 0x00 ) );

	QuatT locatorArrowLocation = location;
	f32 moveDirLength = moveDirectionLocal.GetLength();
	f32 yaw = -atan2f( moveDirectionLocal.x, moveDirectionLocal.y );
	locatorArrowLocation.q *= Quat::CreateRotationZ( yaw );
	DrawArrow( m_renderer, locatorArrowLocation, moveDirectionLocal, moveDirLength, RGBA8( 0xff, 0xff, 0x00, 0x00 ) );
}



//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
namespace
{

	void DrawArrow( IRenderer* pRenderer, const QuatT& location, f32 length, ColorB col )
	{
		assert( pRenderer != NULL );
		IRenderAuxGeom* pAuxGeom = pRenderer->GetIRenderAuxGeom();
		SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
		pAuxGeom->SetRenderFlags( renderFlags );

		Vec3 absAxisY	=	location.q.GetColumn1();

		const f32 scale=1.0f;
		const f32 size=0.009f;
		AABB yaabb = AABB(Vec3(-size*scale, -0.0f*scale, -size*scale),Vec3(size*scale,	 length*scale, size*scale));

		OBB obb =	OBB::CreateOBBfromAABB( Matrix33(location.q), yaabb );
		pAuxGeom->DrawOBB(obb,location.t,1,col,eBBD_Extremes_Color_Encoded);
		pAuxGeom->DrawCone(location.t+absAxisY*length*scale,absAxisY,0.03f,0.15f,col);

	}


	void DrawArrow( IRenderer* pRenderer, const QuatT& location, const Vec3& vTravelDir, f32 length, ColorB col )
	{
		assert( pRenderer != NULL );
		SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
		IRenderAuxGeom* pAuxGeom = pRenderer->GetIRenderAuxGeom();
		pAuxGeom->SetRenderFlags( renderFlags );

		Vec3 absAxisY	=	location.q.GetColumn1();

		Vec3 vdir=vTravelDir.GetNormalized();
		Matrix33 m;
		m.m00=1;	m.m01=0;	m.m02=0;
		m.m10=0;	m.m11=0;	m.m12=-1;
		m.m20=0;	m.m21=1;	m.m22=0;
		f64 l = sqrt(vdir.x*vdir.x + vdir.y*vdir.y);
		if (l>0.0001)	
		{
			f32 rad			= f32( atan2(-vdir.z*(vdir.y/l),l) );
			m.SetRotationX(-rad);
		}
		//----------------------------------------------------------------------

		const f32 scale = 1.0f;
		const f32 size=0.009f;
		AABB yaabb = AABB(Vec3(-size*scale, -0.0f*scale, -size*scale),Vec3(size*scale,	 length*scale, size*scale));

		Matrix33 m2=Matrix33(location.q)*m;
		OBB obb =	OBB::CreateOBBfromAABB( Matrix33(location.q)*m, yaabb );
		pAuxGeom->DrawOBB(obb,location.t,1,col,eBBD_Extremes_Color_Encoded);

		if (l>0.0001)	
		{
			f64 xl=-vdir.x/l; f64 yl=vdir.y/l;
			m.m00=f32(yl);	m.m01=f32(vdir.x);		m.m02=f32(xl*vdir.z);
			m.m10=f32(xl);	m.m11=f32(vdir.y);		m.m12=f32(-vdir.z*yl);
			m.m20=0;				m.m21=f32(vdir.z);		m.m22=f32(l);
		}
		pAuxGeom->DrawCone(m2*(Vec3(0,1,0)*length*scale)+location.t, m2*Vec3(0,1,0),0.03f,0.15f,col);
	}
}
