///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   LMG_GetCapabilities.cpp
//  Version:     v1.00
//  Created:     07/06/2009 by Jaewon Jung
//  Description: Locomotion Group utility functions
// -------------------------------------------------------------------------
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMG.h"
#include "ModelAnimationSet.h"
#include <SpuUtils.h>

#if !defined(__SPU__)
typedef GlobalAnimationHeaderCAF&	GlobalAnimationHeaderType;
#endif

// global object to init AnimCodeLookup
LMG::AnimCodeLookup gAnimCodeLookup; // global object to force init
LMG::AnimCodeLookup::AnimCodePair	m_AnimCodes[LMG::AnimCodeLookup::LAST];

#if defined(__SPU__)
// spu local array to spare main memory lookups
SPU_LOCAL LMG::AnimCodeLookup::AnimCodePair spuAnimCodeLookup[LMG::AnimCodeLookup::LAST];
#endif

SPU_LOCAL const f32 proceduralTurnSpeedChangeRate = DEG2RAD(360.0f) * 4.0f;
SPU_LOCAL f32 curvingTurnSpeedChangeRate = proceduralTurnSpeedChangeRate;

namespace LMG
{
	AnimCodeLookup::AnimCodeLookup()
	{
		// utility structure to init lookuptable at runtime
		struct AnimCodeComputeHelper
		{
			char 		name[5];
			AnimCode 	code;
		};		

		 
		AnimCodeComputeHelper codes[] = {
			#define DRY(X) { #X, X },
			#include "LMG_Types.inc"
			#undef DRY
		};		

		const uint32 NUM_ANIM_CODES = sizeof(codes)/sizeof(codes[0]);	

		for( uint32 i = 0 ; i < NUM_ANIM_CODES ; ++i )
		{
			uint32 value = *(uint32*)codes[i].name;

			AnimCodePair pair = { value, codes[i].code };
			m_AnimCodes[i] = pair;
		}

		for(int i,j=1;j<NUM_ANIM_CODES;j++)
		{
			AnimCodePair key=m_AnimCodes[j];
			i=j-1;

			while( i >= 0 && m_AnimCodes[i].key > key.key)
			{
				m_AnimCodes[i+1]=m_AnimCodes[i];
				i--;
			}

			m_AnimCodes[i+1] = key;
		}
	}

	// lookup the enum for animcode in O(log n ) (binary search)
	AnimCodeLookup::AnimCode AnimCodeLookup::ComputeAnimCode( uint32 value )
	{
		uint32 low = 0;
		uint32 high = LAST;

		AnimCodePair *pair_array = SPU_PTR_SELECT( &m_AnimCodes[0], spuAnimCodeLookup);

		while (low <= high)
		{
			uint32 mid = (low + high) / 2;

			if ( pair_array[mid].key > value )
			{
				high = mid - 1;
			}
			else if ( pair_array[mid].key < value )
			{
				low = mid + 1;
			}
			else
			{
				return pair_array[mid].code; // found
			}
		}

		char errtx[5];	*(uint32*)errtx = value; 	errtx[4] = 0;
		CryFatalError("locomotion-group '%s' not identified. This a fatal-error.",errtx);
		assert(0);

		return LAST; // not found // just return the last value, the execution should never reach here
	}

#if defined(__SPU__)
	void AnimCodeLookup::initSPULookup()
	{
		memcpy(spuAnimCodeLookup, &m_AnimCodes[0],sizeof(m_AnimCodes) );
	}
#endif

	namespace _private
	{
		IdleStepWeights6 GetIdleStepWeights6(   Vec2 FR,Vec2 RR,Vec2 BR,   Vec2 FL,Vec2 LL,Vec2 BL,  Vec2 DesiredDirection  ) 
		{
			f32 fr=0,rr=0,br=0;
			f32 fl=0,ll=0,bl=0;
			f32 t	=	0.5f;
			f32 step=0.55f;

			f32 length_FR = FR.GetLength();
			f32 length_FL = FL.GetLength();
			Vec2 F = (FR+FL).GetNormalizedSafe();
			FR	=	F*length_FR;
			FL	=	F*length_FL;

			f32 length_BR = BR.GetLength();
			f32 length_BL = BL.GetLength();
			Vec2 B = (BR+BL).GetNormalizedSafe();
			BR	=	B*length_BR;
			BL	=	B*length_BL;

			uint32 limit=0x100;

			f32 dotFR = Ang3::CreateRadZ(FR,DesiredDirection);
			f32 dotRR = Ang3::CreateRadZ(RR,DesiredDirection);
			f32 dotBR = Ang3::CreateRadZ(BR,DesiredDirection);
			f32 dotFL = Ang3::CreateRadZ(FL,DesiredDirection);
			f32 dotLL = Ang3::CreateRadZ(LL,DesiredDirection);
			f32 dotBL = Ang3::CreateRadZ(BL,DesiredDirection);

			int32 QuadrantFRRR=0;
			if (dotFR<=0 && dotRR>=0)
				QuadrantFRRR=1;
			int32 QuadrantBRRR=0;
			if (dotBR>=0 && dotRR<=0)
				QuadrantBRRR=1;

			int32 QuadrantFLLL=0;
			if (dotFL>=0 && dotLL<=0)
				QuadrantFLLL=1;
			int32 QuadrantBLLL=0;
			if (dotBL<=0 && dotLL>=0)
				QuadrantBLLL=1;

			assert( QuadrantFRRR + QuadrantBRRR + QuadrantFLLL + QuadrantBLLL );

			f32 dot;

			if (QuadrantFRRR)
			{
				for (uint32 i=0; i<limit; i++)
				{
					fr=1.0f-t; 	rr=t;
					dot = Ang3::CreateRadZ(DesiredDirection,FR*fr+RR*rr);
					if ( fabs(dot)<0.00001f ) break;
					t+=step*sgn(dot);	step*=0.6f;
				}
				return IdleStepWeights6( f32(fr),f32(rr),f32(br), f32(fl),f32(ll),f32(bl) );
			}



			if (QuadrantFLLL)
			{
				for (uint32 i=0; i<limit; i++)
				{
					fl=1.0f-t;	ll=t;
					dot = Ang3::CreateRadZ(FL*fl+LL*ll,DesiredDirection);
					if ( fabs(dot)<0.00001f ) break;
					t+=step*sgn(dot); step*=0.6f;
				}
				return IdleStepWeights6( f32(fr),f32(rr),f32(br), f32(fl),f32(ll),f32(bl) );
			}


			if (QuadrantBRRR)
			{
				for (uint32 i=0; i<limit; i++)
				{
					br=1.0f-t;	rr=t;
					dot = Ang3::CreateRadZ(BR*br+RR*rr,DesiredDirection);
					if ( fabs(dot)<0.00001f )	break;
					t+=step*sgn(dot); step*=0.6f;
				}
				return IdleStepWeights6( f32(fr),f32(rr),f32(br), f32(fl),f32(ll),f32(bl) );
			}


			if (QuadrantBLLL)
			{
				for (uint32 i=0; i<limit; i++)
				{
					bl=1.0f-t;	ll=t;
					dot = Ang3::CreateRadZ(DesiredDirection,BL*bl+LL*ll);
					if ( fabs(dot)<0.00001f ) break;
					t+=step*sgn(dot); step*=0.6f;
				}
				return IdleStepWeights6( f32(fr),f32(rr),f32(br), f32(fl),f32(ll),f32(bl) );
			}

			return IdleStepWeights6(0,0,0,0,0,0);
		}

		IdleStepWeights6 GetIdle2MoveWeights6(  Vec2 DesiredDirection  ) 
		{
			IdleStepWeights6 I2M;
			I2M.fr=0.0f; I2M.rr=0.0f; I2M.br=0.0f;
			I2M.fl=0.0f; I2M.ll=0.0f; I2M.bl=0.0f;

			f32 rad = Ang3::CreateRadZ(Vec2(0,1),DesiredDirection);
			if (rad<0)
			{
				if (rad>=(-gf_PI*0.5f))
				{
					//forward-right quadrant
					f32 t = -(rad/(gf_PI*0.5f));
					I2M.fr=1-t;	I2M.rr=t;	return I2M;
				}
				else
				{
					//right-backward quadrant
					rad = rad+(gf_PI*0.5f);
					f32 t = -(rad/(gf_PI*0.5f));
					I2M.rr=1-t;	I2M.br=t;	return I2M;
				}
			}
			else
			{
				if (rad<=(gf_PI*0.5f))
				{
					//forward-left quadrant
					f32 t = rad/(gf_PI*0.5f);
					I2M.fl=1-t;	I2M.ll=t;	return I2M;
				}
				else
				{
					//left-backward quadrant
					rad = rad-(gf_PI*0.5f);
					f32 t = rad/(gf_PI*0.5f);
					I2M.ll=1-t;	I2M.bl=t;	return I2M;
				}
			}

			assert(0);
			return I2M;
		}

		void GetStrafeWeights(const Vec2& F, const Vec2& R, const Vec2& W, float& f, float& r)
		{
			float det = F.x * R.y - F.y * R.x;
			float det1 = W.x * R.y - W.y * R.x;
			const float invDet = 1.0f / det;
			float det2 = F.x * W.y - F.y * W.x;
			float fTemp = det1 * invDet;
			float rTemp = det2 * invDet;
			fTemp = (float)__fsel(-fTemp, 0.5f, fTemp); //f > 0.0f ? f : 0.5f;
			rTemp = (float)__fsel(-rTemp, 0.5f, rTemp);	//r > 0.0f ? r : 0.5f;

			float sum = 1.0f/(fTemp + rTemp);
			f = fTemp * sum;
			r = rTemp * sum;
		}

		StrafeWeights4 GetStrafingWeights4( Vec2 F, Vec2 L, Vec2 B, Vec2 R, Vec2 DesiredDirection ) 
		{

			f32 length=(DesiredDirection|DesiredDirection);
			if (length==0.0f)
				return StrafeWeights4(0.25f,0.25f,0.25f,0.25f);

			f32 f=0,r=0,l=0,b=0;
			f32 dotF	= Ang3::CreateRadZ(F,DesiredDirection);
			f32 dotL	= Ang3::CreateRadZ(L,DesiredDirection);

			if (dotF>=0 && dotL<=0)
			{
				if (dotF == 0.0f && dotL == 0.0f)
					return StrafeWeights4(0.25f,0.25f,0.25f,0.25f);
				GetStrafeWeights(F, L, DesiredDirection, f, l);
				return StrafeWeights4(f,l,b,r);
			}

			f32 dotR	= Ang3::CreateRadZ(R,DesiredDirection);

			if (dotF<=0 && dotR>=0)
			{
				if (dotF == 0.0f && dotR == 0.0f)
					return StrafeWeights4(0.25f,0.25f,0.25f,0.25f);

				GetStrafeWeights(F, R, DesiredDirection, f, r);
				return StrafeWeights4(f,l,b,r);
			}

			f32 dotB	= Ang3::CreateRadZ(B,DesiredDirection);

			if (dotB>=0 && dotR<=0)
			{
				if (dotB == 0.0f && dotR == 0.0f)
					return StrafeWeights4(0.25f,0.25f,0.25f,0.25f);

				GetStrafeWeights(B, R, DesiredDirection, b, r);
				return StrafeWeights4(f,l,b,r);
			}

			if (dotB<=0 && dotL>=0)
			{
				if (dotB == 0.0f && dotL == 0.0f)
					return StrafeWeights4(0.25f,0.25f,0.25f,0.25f);

				GetStrafeWeights(B, L, DesiredDirection, b, l);
				return StrafeWeights4(f,l,b,r);
			}

			return StrafeWeights4(0.25f,0.25f,0.25f,0.25f);
		}

		StrafeWeights6 GetStrafingWeights6( Vec2 F,Vec2 FL,Vec2 L,  Vec2 B,Vec2 BR,Vec2 R, const Vec2& DesiredDirection   ) 
		{

			f32 d6=1.0f/6.0f;
			f32 length=(DesiredDirection|DesiredDirection);
			if (length==0.0f)
				return StrafeWeights6(d6,d6,d6,d6,d6,d6);

			f32 f=0,fl=0,l=0, b=0,br=0,r=0;
			f32 dotF	= Ang3::CreateRadZ(F, DesiredDirection);
			f32 dotFL	= Ang3::CreateRadZ(FL,DesiredDirection);
			f32 dotL	= Ang3::CreateRadZ(L, DesiredDirection);

			if (dotF>=0 && dotFL<=0)
			{
				if (dotF != dotFL)
				{
					GetStrafeWeights(F, FL, DesiredDirection, f, fl);
					return StrafeWeights6(f,fl,l,b,br,r);
				}
			}
			if (dotFL>=0 && dotL<=0 )
			{
				GetStrafeWeights(FL, L, DesiredDirection, fl, l);
				return StrafeWeights6(f,fl,l,b,br,r);
			}
			
			f32 dotB	= Ang3::CreateRadZ(B, DesiredDirection);
			f32 dotBR	= Ang3::CreateRadZ(BR,DesiredDirection);
			f32 dotR	= Ang3::CreateRadZ(R, DesiredDirection);

			if (dotL>=0 && dotB<=0 )
			{ 
				GetStrafeWeights(L, B,  DesiredDirection, l,  b);
				return StrafeWeights6(f,fl,l,b,br,r);
			}

			if (dotB>=0  && dotBR<=0)
			{
				if (dotB != dotBR)
				{
					GetStrafeWeights(B, BR, DesiredDirection, b, br);
					return StrafeWeights6(f,fl,l,b,br,r);
				}
			}
			if (dotBR>=0 && dotR<=0)
			{
				GetStrafeWeights(BR, R, DesiredDirection, br, r);
				return StrafeWeights6(f,fl,l,b,br,r);
			}
			if (dotR>=0  && dotF<=0)
			{
				GetStrafeWeights(R, F,  DesiredDirection, r,  f);
				return StrafeWeights6(f,fl,l,b,br,r);
			}

			return StrafeWeights6(d6,d6,d6,d6,d6,d6);
		}


		StrafeWeights8 GetStrafingWeights8( Vec2 F,Vec2 FL,Vec2 L,Vec2 LB,  Vec2 B,Vec2 BR,Vec2 R,Vec2 RF, const Vec2& DesiredDirection   ) 
		{
			f32 d8=1.0f/8.0f;
			f32 length=(DesiredDirection|DesiredDirection);
			if (length==0.0f)
				return StrafeWeights8(d8,d8,d8,d8,d8,d8,d8,d8);

			f32 f=0,fl=0,l=0,lb=0,   b=0,br=0,r=0,rf=0;
			f32 dotF	= Ang3::CreateRadZ(F, DesiredDirection);
			f32 dotFL	= Ang3::CreateRadZ(FL,DesiredDirection);
			f32 dotL	= Ang3::CreateRadZ(L, DesiredDirection);
			f32 dotLB	= Ang3::CreateRadZ(LB,DesiredDirection);

			f32 dotB	= Ang3::CreateRadZ(B, DesiredDirection);
			f32 dotBR	= Ang3::CreateRadZ(BR,DesiredDirection);
			f32 dotR	= Ang3::CreateRadZ(R, DesiredDirection);
			f32 dotRF	= Ang3::CreateRadZ(RF,DesiredDirection);

			if (dotF>=0 && dotFL<=0)
			{
				if (dotF != dotFL)
				{
					GetStrafeWeights(F, FL, DesiredDirection, f, fl);
					return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
				}
			}
			if (dotFL>=0 && dotL<=0 )
			{
				GetStrafeWeights(FL, L, DesiredDirection, fl, l);
				return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
			}
			if (dotL>=0 && dotLB<=0 )
			{ 
				GetStrafeWeights(L, LB,  DesiredDirection, l,  lb);
				return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
			}
			if (dotLB>=0 && dotB<=0 )
			{ 
				GetStrafeWeights(LB, B,  DesiredDirection, lb,  b);
				return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
			}



			if (dotB>=0  && dotBR<=0)
			{
				if (dotB != dotBR)
				{
					GetStrafeWeights(B, BR, DesiredDirection, b, br);
					return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
				}
			}
			if (dotBR>=0 && dotR<=0)
			{
				GetStrafeWeights(BR, R, DesiredDirection, br, r);
				return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
			}
			if (dotR>=0  && dotRF<=0)
			{
				GetStrafeWeights(R, RF,  DesiredDirection, r,  rf);
				return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
			}
			if (dotRF>=0  && dotF<=0)
			{
				GetStrafeWeights(RF, F,  DesiredDirection, rf, f);
				return StrafeWeights8(f,fl,l,lb, b,br,r,rf);
			}


			return StrafeWeights8(d8,d8,d8,d8,d8,d8,d8,d8);
		}


		struct Property
		{
			static void GetProperties_STF1( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, LMGCapabilities& lmg_caps )
			{
				lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

				GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];
				GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ];
				GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ];
				GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ];
				GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ];

				Vec2 sF = Vec2(rsubGAH00.m_vVelocity);
				Vec2 sL = Vec2(rsubGAH01.m_vVelocity);
				Vec2 sB = Vec2(rsubGAH02.m_vVelocity);
				Vec2 sR = Vec2(rsubGAH03.m_vVelocity);
				StrafeWeights4 blend = GetStrafingWeights4(sF,sL,sB,sR, lmg.m_BlendSpace.m_strafe);
				Vec2 velocity = sF*blend.f + sL*blend.l + sB*blend.b + sR*blend.r;
				lmg_caps.m_bHasVelocity = 1;
				lmg_caps.m_vMinVelocity	= velocity;
				lmg_caps.m_vMaxVelocity	= velocity;

				f32 duration = rsubGAH00.m_fTotalDuration*blend.f + rsubGAH01.m_fTotalDuration*blend.l  + rsubGAH02.m_fTotalDuration*blend.b  + rsubGAH03.m_fTotalDuration*blend.r;
				lmg_caps.m_fSlowDuration	= duration;
				lmg_caps.m_fFastDuration	= duration;
			}

			static void GetProperties_STF2( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, LMGCapabilities& lmg_caps, GlobalAnimationHeaderLMG* pGHLMG )
			{
				lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

				LMGCapabilities lmg_fast;
				GetProperties_STF1(pAnimationSet, lmg,offset+0,lmg_fast);
				lmg_caps.m_fFastDuration=lmg_fast.m_fFastDuration;
				lmg_caps.m_vMaxVelocity=lmg_fast.m_vMaxVelocity;

				LMGCapabilities lmg_slow;
				GetProperties_STF1(pAnimationSet, lmg,offset+4,lmg_slow);
				lmg_caps.m_fSlowDuration=lmg_slow.m_fSlowDuration;
				lmg_caps.m_bHasVelocity = 1;
				lmg_caps.m_vMinVelocity=lmg_slow.m_vMinVelocity;
			}

			static void GetProperties_S1( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, LMGCapabilities& lmg_caps, GlobalAnimationHeaderLMG* pGHLMG  )
			{
				lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

				GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];
				GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ];
				GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ];
				GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ];
				GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ];
				GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x04]) ];
				GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x05]) ];

				Vec2 sF		= Vec2(rsubGAH00.m_vVelocity);
				Vec2 sFL	= Vec2(rsubGAH01.m_vVelocity);
				Vec2 sL		= Vec2(rsubGAH02.m_vVelocity);
				Vec2 sB		= Vec2(rsubGAH03.m_vVelocity);
				Vec2 sBR	= Vec2(rsubGAH04.m_vVelocity);
				Vec2 sR		= Vec2(rsubGAH05.m_vVelocity);
				assert( lmg.m_BlendSpace.m_strafe.IsValid() );

				f32 length = lmg.m_BlendSpace.m_strafe.GetLength();
				//	if (length==0)
				//		lmg.m_BlendSpace.m_strafe=Vec2(0,0);

				StrafeWeights6 blend = GetStrafingWeights6(sF,sFL,sL, sB,sBR,sR, lmg.m_BlendSpace.m_strafe );
				Vec2 velocity = (sF*blend.f)+(sFL*blend.fl)+(sL*blend.l)+(sB*blend.b)+(sBR*blend.br)+(sR*blend.r);

				f32 dF		= rsubGAH00.m_fTotalDuration;
				f32 dFL		= rsubGAH01.m_fTotalDuration;
				f32 dL		= rsubGAH02.m_fTotalDuration;
				f32 dB		= rsubGAH03.m_fTotalDuration;
				f32 dBR		= rsubGAH04.m_fTotalDuration;
				f32 dR		= rsubGAH05.m_fTotalDuration;
				f32 duration = (dF*blend.f)+(dFL*blend.fl)+(dL*blend.l)+(dB*blend.b)+(dBR*blend.br)+(dR*blend.r);

				lmg_caps.m_fSlowDuration	= duration;
				lmg_caps.m_fFastDuration	= duration;
				lmg_caps.m_bHasVelocity = 1;
				lmg_caps.m_vMinVelocity	= velocity;
				lmg_caps.m_vMaxVelocity	= velocity;
			}

			static void GetProperties_S2( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, LMGCapabilities& lmg_caps, GlobalAnimationHeaderLMG* pGHLMG  )
			{
				lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

				LMGCapabilities lmg_fast;
				GetProperties_S1(pAnimationSet,lmg,offset+0,lmg_fast, pGHLMG );
				lmg_caps.m_fFastDuration=lmg_fast.m_fFastDuration;
				lmg_caps.m_vMaxVelocity=lmg_fast.m_vMaxVelocity;

				LMGCapabilities lmg_slow;
				GetProperties_S1(pAnimationSet,lmg,offset+6,lmg_slow, pGHLMG );
				lmg_caps.m_fSlowDuration=lmg_slow.m_fSlowDuration;
				lmg_caps.m_vMinVelocity=lmg_slow.m_vMinVelocity;
			}

			static void GetProperties_ST2( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, LMGCapabilities& lmg_caps, GlobalAnimationHeaderLMG* pGHLMG  )
			{
				lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

				lmg_caps.m_bHasCurving = 1;
				lmg_caps.m_curvingChangeRate = 2.0f;
				lmg_caps.m_turnSpeedChangeRate = LERP(proceduralTurnSpeedChangeRate, curvingTurnSpeedChangeRate, 
					lmg.m_params[eMotionParamID_Curving].value);

				GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];

				GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ]; //sprint
				GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ];
				GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ];
				GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ];
				GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x04]) ];
				GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x05]) ];

				GlobalAnimationHeaderType rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x07]) ]; //slow
				GlobalAnimationHeaderType rsubGAH08 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x08]) ];
				GlobalAnimationHeaderType rsubGAH09 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x09]) ];
				GlobalAnimationHeaderType rsubGAH0a = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0a]) ];
				GlobalAnimationHeaderType rsubGAH0b = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0b]) ];
				GlobalAnimationHeaderType rsubGAH0c = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0c]) ];

				GlobalAnimationHeaderType rsubGAH0d = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0d]) ]; //fast left
				GlobalAnimationHeaderType rsubGAH0e = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0e]) ]; //fast right
				GlobalAnimationHeaderType rsubGAH0f = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0f]) ]; //slow left
				GlobalAnimationHeaderType rsubGAH10 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x10]) ]; //slow right


				Vec2 ssF	= Vec2(rsubGAH00.m_vVelocity);
				Vec2 ssFL	= Vec2(rsubGAH01.m_vVelocity);
				Vec2 ssL	= Vec2(rsubGAH02.m_vVelocity);
				Vec2 ssB	= Vec2(rsubGAH03.m_vVelocity);
				Vec2 ssBR	= Vec2(rsubGAH04.m_vVelocity);
				Vec2 ssR	= Vec2(rsubGAH05.m_vVelocity);
				StrafeWeights6 sblend = GetStrafingWeights6(ssF,ssFL,ssL, ssB,ssBR,ssR, lmg.m_BlendSpace.m_strafe );
				Vec3 strafe_velocity_fast = (ssF*sblend.f)+(ssFL*sblend.fl)+(ssL*sblend.l)+(ssB*sblend.b)+(ssBR*sblend.br)+(ssR*sblend.r);
				f32 dsF		= rsubGAH00.m_fTotalDuration;
				f32 dsFL	= rsubGAH01.m_fTotalDuration;
				f32 dsL		= rsubGAH02.m_fTotalDuration;
				f32 dsB		= rsubGAH03.m_fTotalDuration;
				f32 dsBR	= rsubGAH04.m_fTotalDuration;
				f32 dsR		= rsubGAH05.m_fTotalDuration;
				f32 strafe_duration_fast = (dsF*sblend.f)+(dsFL*sblend.fl)+(dsL*sblend.l)+(dsB*sblend.b)+(dsBR*sblend.br)+(dsR*sblend.r);

				Vec2 sfF	= Vec2(rsubGAH07.m_vVelocity);
				Vec2 sfFL	= Vec2(rsubGAH08.m_vVelocity);
				Vec2 sfL	= Vec2(rsubGAH09.m_vVelocity);
				Vec2 sfB	= Vec2(rsubGAH0a.m_vVelocity);
				Vec2 sfBR	= Vec2(rsubGAH0b.m_vVelocity);
				Vec2 sfR	= Vec2(rsubGAH0c.m_vVelocity);
				StrafeWeights6 fblend = GetStrafingWeights6(sfF,sfFL,sfL, sfB,sfBR,sfR, lmg.m_BlendSpace.m_strafe );
				Vec3 strafe_velocity_slow = (sfF*fblend.f)+(sfFL*fblend.fl)+(sfL*fblend.l)+(sfB*fblend.b)+(sfBR*fblend.br)+(sfR*fblend.r);
				f32 dfF		= rsubGAH07.m_fTotalDuration;
				f32 dfFL	= rsubGAH08.m_fTotalDuration;
				f32 dfL		= rsubGAH09.m_fTotalDuration;
				f32 dfB		= rsubGAH0a.m_fTotalDuration;
				f32 dfBR	= rsubGAH0b.m_fTotalDuration;
				f32 dfR		= rsubGAH0c.m_fTotalDuration;
				f32 strafe_duration_slow = (dfF*fblend.f)+(dfFL*fblend.fl)+(dfL*fblend.l)+(dfB*fblend.b)+(dfBR*fblend.br)+(dfR*fblend.r);

				f32 fast_FWL = rsubGAH0d.m_fSpeed;	//fast left
				f32 fast_FW  = rsubGAH00.m_fSpeed;	//fast
				f32 fast_FWR = rsubGAH0e.m_fSpeed;	//fast right
				f32 slow_FWL = rsubGAH0f.m_fSpeed;	//slow left
				f32 slow_FW	 = rsubGAH07.m_fSpeed;	//slow
				f32 slow_FWR = rsubGAH10.m_fSpeed;	//slow right

				f32 t=lmg.m_BlendSpace.m_turn;
				f32 L=0,F=0,R=0;
				if (t<0) { L=-t; F=1.0f+t; }	else { F=1.0f-t; R=t; }
				//	float fColor[4] = {1,1,0,1};
				//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"++++++++++++++++++++++++++         L %f   F %f    R %f",L,F,R); g_YLine+=0x10;

				f32 fw_speed_fast = fast_FWL*L + fast_FW*F + fast_FWR*R;
				f32 fw_speed_slow	= slow_FWL*L + slow_FW*F + slow_FWR*R;


				Vec3 turn_velocity_fast = Vec3(0,fw_speed_fast,0);
				Vec3 turn_velocity_slow	= Vec3(0,fw_speed_slow,0);



				f32 fTurnPriority=0;
				if (pAnimationSet->m_CharEditMode)
				{
					Vec2 strafe = lmg.m_BlendSpace.m_strafe.GetNormalizedSafe( Vec2(0,0) );
					f32 rad = atan2(strafe.x,strafe.y);
					fTurnPriority = fabsf( rad*2 );
					if (fTurnPriority>1.0f) fTurnPriority=1.0f;
					fTurnPriority=1.0f-fTurnPriority;
				}
				else
				{
					fTurnPriority = lmg.m_params[eMotionParamID_Curving].value;
				}

				f32 staf = 1.0f-fTurnPriority;
				f32 turn = fTurnPriority;
				lmg_caps.m_vMinVelocity = turn*turn_velocity_slow + staf*strafe_velocity_slow;
				lmg_caps.m_vMaxVelocity = turn*turn_velocity_fast + staf*strafe_velocity_fast;

				{
					lmg_caps.m_fSlowTurnLeft	= turn*rsubGAH0f.m_fTurnSpeed;
					lmg_caps.m_fSlowTurnRight	= turn*rsubGAH10.m_fTurnSpeed;

					lmg_caps.m_fFastTurnLeft	= turn*rsubGAH0d.m_fTurnSpeed;
					lmg_caps.m_fFastTurnRight	= turn*rsubGAH0e.m_fTurnSpeed;
				}


				//duration
				f32 fL	= rsubGAH0d.m_fTotalDuration;
				f32 fF	= rsubGAH00.m_fTotalDuration;
				f32 fR	= rsubGAH0e.m_fTotalDuration;
				f32 sL	= rsubGAH0f.m_fTotalDuration;
				f32 sF	= rsubGAH07.m_fTotalDuration;
				f32 sR	= rsubGAH10.m_fTotalDuration;
				f32 turn_duration_fast = fL*L + fF*F + fR*R;
				f32 turn_duration_slow = sL*L + sF*F + sR*R;
				lmg_caps.m_fSlowDuration = turn*turn_duration_slow + staf*strafe_duration_slow;
				lmg_caps.m_fFastDuration = turn*turn_duration_fast + staf*strafe_duration_fast;
			}

			static void GetProperties_ST2X( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, LMGCapabilities& lmg_caps, GlobalAnimationHeaderLMG* pGHLMG  )
			{
				lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

				GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];

				GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ];
				GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ];
				GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ];
				GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ];

				GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x05]) ];
				GlobalAnimationHeaderType rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x06]) ];
				GlobalAnimationHeaderType rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x07]) ];
				GlobalAnimationHeaderType rsubGAH08 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x08]) ];

				GlobalAnimationHeaderType rsubGAH09 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x09]) ];
				GlobalAnimationHeaderType rsubGAH0a = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0a]) ];
				GlobalAnimationHeaderType rsubGAH0b = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0b]) ];
				GlobalAnimationHeaderType rsubGAH0c = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0c]) ];


				Vec2 ssF	= Vec2(rsubGAH00.m_vVelocity);
				Vec2 ssL	= Vec2(rsubGAH01.m_vVelocity);
				Vec2 ssB	= Vec2(rsubGAH02.m_vVelocity);
				Vec2 ssR	= Vec2(rsubGAH03.m_vVelocity);
				StrafeWeights4 sblend = GetStrafingWeights4(ssF,ssL, ssB,ssR, lmg.m_BlendSpace.m_strafe );
				Vec3 strafe_velocity_fast = (ssF*sblend.f)+(ssL*sblend.l)+(ssB*sblend.b)+(ssR*sblend.r);
				f32 dsF		= rsubGAH00.m_fTotalDuration;
				f32 dsL		= rsubGAH01.m_fTotalDuration;
				f32 dsB		= rsubGAH02.m_fTotalDuration;
				f32 dsR		= rsubGAH03.m_fTotalDuration;
				f32 strafe_duration_fast = (dsF*sblend.f)+(dsL*sblend.l)+(dsB*sblend.b)+(dsR*sblend.r);

				Vec2 sfF	= Vec2(rsubGAH05.m_vVelocity);
				Vec2 sfL	= Vec2(rsubGAH06.m_vVelocity);
				Vec2 sfB	= Vec2(rsubGAH07.m_vVelocity);
				Vec2 sfR	= Vec2(rsubGAH08.m_vVelocity);
				StrafeWeights4 fblend = GetStrafingWeights4(sfF,sfL, sfB,sfR, lmg.m_BlendSpace.m_strafe );
				Vec3 strafe_velocity_slow = (sfF*fblend.f)+(sfL*fblend.l)+(sfB*fblend.b)+(sfR*fblend.r);
				f32 dfF		= rsubGAH05.m_fTotalDuration;
				f32 dfL		= rsubGAH06.m_fTotalDuration;
				f32 dfB		= rsubGAH07.m_fTotalDuration;
				f32 dfR		= rsubGAH08.m_fTotalDuration;
				f32 strafe_duration_slow = (dfF*fblend.f)+(dfL*fblend.l)+(dfB*fblend.b)+(dfR*fblend.r);



				f32 fast_FWL = rsubGAH09.m_fSpeed;	//fast left
				f32 fast_FW  = rsubGAH00.m_fSpeed;	//fast
				f32 fast_FWR = rsubGAH0a.m_fSpeed;	//fast right
				f32 slow_FWL = rsubGAH0b.m_fSpeed;	//slow left
				f32 slow_FW	 = rsubGAH05.m_fSpeed;	//slow
				f32 slow_FWR = rsubGAH0c.m_fSpeed;	//slow right
				f32 t=lmg.m_BlendSpace.m_turn;
				f32 L=0,F=0,R=0;
				if (t<0) { L=-t; F=1.0f+t; }	else { F=1.0f-t; R=t; }
				//float fColor[4] = {1,1,0,1};
				//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"++++++++++++++++++++++++++         L %f   F %f    R %f",L,F,R); g_YLine+=0x10;

				f32 fw_speed_fast = fast_FWL*L + fast_FW*F + fast_FWR*R;
				f32 fw_speed_slow	= slow_FWL*L + slow_FW*F + slow_FWR*R;
				Vec3 turn_velocity_fast = Vec3(0,fw_speed_fast,0);
				Vec3 turn_velocity_slow	= Vec3(0,fw_speed_slow,0);



				Vec2 strafe = lmg.m_BlendSpace.m_strafe.GetNormalizedSafe( Vec2(0,0) );
				f32 rad = atan2(strafe.x,strafe.y);
				f32 fTurnPriority = fabsf( rad*2 );
				if (fTurnPriority>1.0f) fTurnPriority=1.0f;
				fTurnPriority=1.0f-fTurnPriority;

				fTurnPriority = lmg.m_params[eMotionParamID_Curving].value;

				f32 staf = 1.0f-fTurnPriority;
				f32 turn = fTurnPriority;
				lmg_caps.m_bHasVelocity = 1;
				lmg_caps.m_vMinVelocity = turn*turn_velocity_slow + staf*strafe_velocity_slow;
				lmg_caps.m_vMaxVelocity = turn*turn_velocity_fast + staf*strafe_velocity_fast;


				//duration
				f32 fL	= rsubGAH09.m_fTotalDuration;
				f32 fF	= rsubGAH00.m_fTotalDuration;
				f32 fR	= rsubGAH0a.m_fTotalDuration;
				f32 sL	= rsubGAH0b.m_fTotalDuration;
				f32 sF	= rsubGAH05.m_fTotalDuration;
				f32 sR	= rsubGAH0c.m_fTotalDuration;
				f32 turn_duration_fast = fL*L + fF*F + fR*R;
				f32 turn_duration_slow = sL*L + sF*F + sR*R;
				lmg_caps.m_fSlowDuration = turn*turn_duration_slow + staf*strafe_duration_slow;
				lmg_caps.m_fFastDuration = turn*turn_duration_fast + staf*strafe_duration_fast;
			}

			static Vec2 GetLRTurn( GlobalAnimationHeaderLMG* rGlobalAnimHeader, CAnimationSet* pAnimationSet, const SParametric& lmg, GlobalAnimationHeaderLMG* pGHLMG  )
			{
				uint32 numAssets = rGlobalAnimHeader->m_arrBSAnimations.size();
				GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];
				f32 fTurnLeft	=-99999.0f;
				f32 fTurnRight=+99999.0f;
				for (uint32 i=0; i<numAssets; i++)
				{
					int32 gid = pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[i]);
					f32 turn = parrGlobalAnimations[ gid ].m_fTurnSpeed;
					if (fTurnLeft<turn)
						fTurnLeft=turn;
					if (fTurnRight>turn)
						fTurnRight=turn;
				}
				return Vec2(fTurnLeft,fTurnRight);
			}
		};

		SPU_NO_INLINE void GetCapabilities_IROT(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG )
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.0f;

			uint32 numAssets = pGHLMG->m_arrBSAnimations.size();
			f32 fMinDuration=+99999.0f;
			f32 fMaxDuration=-99999.0f;
			f32 fTurnLeft	=-99999.0f;
			f32 fTurnRight=+99999.0f;
			for (uint32 i=0; i<numAssets; i++)
			{
				int32 gid = pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[i]);

				f32 turn = parrGlobalAnimations[ gid ].m_fTurnSpeed;
				if (fTurnLeft<turn)
					fTurnLeft=turn;
				if (fTurnRight>turn)
					fTurnRight=turn;

				f32 duration = parrGlobalAnimations[ gid ].m_fTotalDuration;
				if (fMinDuration>duration)
					fMinDuration=duration;
				if (fMaxDuration<duration)
					fMaxDuration=duration;
			}

			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			//lmg_caps.m_bHasStrafingAsset=-1;
			//lmg_caps.m_bHasDistance=-1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=0;

			lmg_caps.m_fSlowDuration	=	fMinDuration;
			lmg_caps.m_fFastDuration	=	fMaxDuration;

			lmg_caps.m_vMinVelocity	= Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,0,0);

			lmg_caps.m_fFastTurnLeft		= fTurnLeft;
			lmg_caps.m_fFastTurnRight	= fTurnRight;

		}

		SPU_NO_INLINE void GetCapabilities_TSTP(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG )
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.0f;

			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=0;
			lmg_caps.m_bHasTurningDistAsset=1;
			lmg_caps.m_bHasVelocity = 0;
			lmg_caps.m_travelAngleChangeRate = 0.0f;
			lmg_caps.m_travelSpeedChangeRate = 0.0f;

			uint32 numAssets = pGHLMG->m_arrBSAnimations.size();

			f32 fMinDuration=+99999.0f;
			f32 fMaxDuration=-99999.0f;
			f32 fTurnLeft	=-99999.0f;
			f32 fTurnRight=+99999.0f;
			for (uint32 i=0; i<numAssets; i++)
			{
				int32 gid = pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[i]);

				f32 turn = parrGlobalAnimations[ gid ].m_fAssetTurn;
				if (fTurnLeft<turn) fTurnLeft=turn;
				if (fTurnRight>turn) fTurnRight=turn;

				f32 duration = parrGlobalAnimations[ gid ].m_fTotalDuration;
				if (fMinDuration>duration) fMinDuration=duration;
				if (fMaxDuration<duration) fMaxDuration=duration;
			}

			lmg_caps.m_fSlowDuration	=	fMinDuration;
			lmg_caps.m_fFastDuration	=	fMaxDuration;
			lmg_caps.m_vMinVelocity	= Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,0,0);
			lmg_caps.m_fFastTurnLeft		= fTurnLeft;
			lmg_caps.m_fFastTurnRight	= fTurnRight;
			lmg_caps.m_fAllowDesiredTurning=0;
		}

		void GetCapabilities_M2I1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 1.0f;
		}

		SPU_NO_INLINE void GetCapabilities_I2M1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 1.0f;

			Vec2 mdir = lmg.m_BlendSpace.m_strafe;
			mdir.NormalizeSafe();
			IdleStepWeights6 I2M = GetIdle2MoveWeights6(mdir);
			f32 speed = rsubGAH00.m_fSpeed*I2M.fr + rsubGAH01.m_fSpeed*I2M.rr + rsubGAH02.m_fSpeed*I2M.br   +   rsubGAH03.m_fSpeed*I2M.fl + rsubGAH04.m_fSpeed*I2M.ll + rsubGAH05.m_fSpeed*I2M.bl;
			f32 turn = rsubGAH00.m_fTurnSpeed*I2M.fr + rsubGAH01.m_fTurnSpeed*I2M.rr + rsubGAH02.m_fTurnSpeed*I2M.br   +   rsubGAH03.m_fTurnSpeed*I2M.fl + rsubGAH04.m_fTurnSpeed*I2M.ll + rsubGAH05.m_fTurnSpeed*I2M.bl;
			f32 duration = rsubGAH00.m_fTotalDuration*I2M.fr + rsubGAH01.m_fTotalDuration*I2M.rr + rsubGAH02.m_fTotalDuration*I2M.br   +   rsubGAH03.m_fTotalDuration*I2M.fl + rsubGAH04.m_fTotalDuration*I2M.ll + rsubGAH05.m_fTotalDuration*I2M.bl;

			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			//lmg_caps.m_bHasTurningAsset=-1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_travelAngleChangeRate = 0.0f;
			lmg_caps.m_travelSpeedChangeRate = 0.0f;
			lmg_caps.m_fAllowDesiredTurning=0;

			lmg_caps.m_fSlowDuration = duration;
			lmg_caps.m_fFastDuration = duration;

			lmg_caps.m_bHasVelocity = 0;
			lmg_caps.m_vMinVelocity	=	Vec3(0,speed,0);
			lmg_caps.m_vMaxVelocity	=	Vec3(0,speed,0);

			lmg_caps.m_fFastTurnLeft =turn;
			lmg_caps.m_fFastTurnRight=turn;
			lmg_caps.m_fAllowDesiredTurning=0;
		}

		SPU_NO_INLINE void GetCapabilities_I2M2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			GlobalAnimationHeaderType rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x06]) ];
			GlobalAnimationHeaderType rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x07]) ];
			GlobalAnimationHeaderType rsubGAH08 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x08]) ];
			GlobalAnimationHeaderType rsubGAH09 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x09]) ];
			GlobalAnimationHeaderType rsubGAH0a = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x0a]) ];
			GlobalAnimationHeaderType rsubGAH0b = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x0b]) ];

			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 1.0f;

			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			//lmg_caps.m_bHasTurningAsset=-1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_bHasVelocity = 0;
			lmg_caps.m_bHasDistance = 0;
			lmg_caps.m_travelAngleChangeRate = 0.0f;
			lmg_caps.m_travelSpeedChangeRate = 0.0f;
			lmg_caps.m_fAllowDesiredTurning=0;

			Vec2 mdir = lmg.m_BlendSpace.m_strafe; 
			mdir.NormalizeSafe();
			IdleStepWeights6 I2M = GetIdle2MoveWeights6(mdir); 

			f32 minspeed            = rsubGAH00.m_fSpeed*I2M.fr         + rsubGAH01.m_fSpeed*I2M.rr         + rsubGAH02.m_fSpeed*I2M.br           +   rsubGAH03.m_fSpeed*I2M.fl         + rsubGAH04.m_fSpeed*I2M.ll         + rsubGAH05.m_fSpeed*I2M.bl;
			lmg_caps.m_vMinVelocity	=	Vec3(0,minspeed-0.001f,0);
			//lmg_caps.m_fFastTurnLeft = rsubGAH00.m_fTurn*I2M.fr          + rsubGAH01.m_fTurn*I2M.rr          + rsubGAH02.m_fTurn*I2M.br            +   rsubGAH03.m_fTurn*I2M.fl          + rsubGAH04.m_fTurn*I2M.ll          + rsubGAH05.m_fTurn*I2M.bl;
			lmg_caps.m_fFastTurnLeft = 0;
			lmg_caps.m_fSlowDuration = rsubGAH00.m_fTotalDuration*I2M.fr + rsubGAH01.m_fTotalDuration*I2M.rr + rsubGAH02.m_fTotalDuration*I2M.br   +   rsubGAH03.m_fTotalDuration*I2M.fl + rsubGAH04.m_fTotalDuration*I2M.ll + rsubGAH05.m_fTotalDuration*I2M.bl;

			f32 maxspeed            = rsubGAH06.m_fSpeed*I2M.fr         + rsubGAH07.m_fSpeed*I2M.rr         + rsubGAH08.m_fSpeed*I2M.br           +   rsubGAH09.m_fSpeed*I2M.fl         + rsubGAH0a.m_fSpeed*I2M.ll         + rsubGAH0b.m_fSpeed*I2M.bl;
			lmg_caps.m_vMaxVelocity	=	Vec3(0,maxspeed-0.001f,0);
			//lmg_caps.m_fFastTurnRight = rsubGAH06.m_fTurn*I2M.fr          + rsubGAH07.m_fTurn*I2M.rr          + rsubGAH08.m_fTurn*I2M.br            +   rsubGAH09.m_fTurn*I2M.fl          + rsubGAH0a.m_fTurn*I2M.ll          + rsubGAH0b.m_fTurn*I2M.bl;
			lmg_caps.m_fFastTurnRight = 0;
			lmg_caps.m_fFastDuration = rsubGAH06.m_fTotalDuration*I2M.fr + rsubGAH07.m_fTotalDuration*I2M.rr + rsubGAH08.m_fTotalDuration*I2M.br   +   rsubGAH09.m_fTotalDuration*I2M.fl + rsubGAH0a.m_fTotalDuration*I2M.ll + rsubGAH0b.m_fTotalDuration*I2M.bl;

			lmg_caps.m_fAllowDesiredTurning=0.00f;
		}

		SPU_NO_INLINE void GetCapabilities_XIM2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			GlobalAnimationHeaderType rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x06]) ];
			GlobalAnimationHeaderType rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x07]) ];
			GlobalAnimationHeaderType rsubGAH08 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x08]) ];
			GlobalAnimationHeaderType rsubGAH09 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x09]) ];
			GlobalAnimationHeaderType rsubGAH0a = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x0a]) ];
			GlobalAnimationHeaderType rsubGAH0b = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x0b]) ];

			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 1.0f;
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=0;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_bHasTurningDistAsset=1;
			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_bHasDistance = 0;
			lmg_caps.m_travelAngleChangeRate = 0.0f;
			lmg_caps.m_travelSpeedChangeRate = -1;
			lmg_caps.m_fAllowDesiredTurning=0;
			lmg_caps.m_fFastTurnLeft	= gf_PI;
			lmg_caps.m_fFastTurnRight	= -gf_PI;

			Vec2 mdir = lmg.m_BlendSpace.m_strafe; 
			mdir.NormalizeSafe();
			IdleStepWeights6 I2M = GetIdle2MoveWeights6(mdir); 
			f32 minspeed            = rsubGAH00.m_fSpeed*I2M.fr         + rsubGAH01.m_fSpeed*I2M.rr         + rsubGAH02.m_fSpeed*I2M.br           +   rsubGAH03.m_fSpeed*I2M.fl         + rsubGAH04.m_fSpeed*I2M.ll         + rsubGAH05.m_fSpeed*I2M.bl;
			lmg_caps.m_vMinVelocity	=	Vec3(0,minspeed-0.001f,0);
			lmg_caps.m_fSlowDuration = rsubGAH00.m_fTotalDuration*I2M.fr + rsubGAH01.m_fTotalDuration*I2M.rr + rsubGAH02.m_fTotalDuration*I2M.br   +   rsubGAH03.m_fTotalDuration*I2M.fl + rsubGAH04.m_fTotalDuration*I2M.ll + rsubGAH05.m_fTotalDuration*I2M.bl;
			f32 maxspeed            = rsubGAH06.m_fSpeed*I2M.fr         + rsubGAH07.m_fSpeed*I2M.rr         + rsubGAH08.m_fSpeed*I2M.br           +   rsubGAH09.m_fSpeed*I2M.fl         + rsubGAH0a.m_fSpeed*I2M.ll         + rsubGAH0b.m_fSpeed*I2M.bl;
			lmg_caps.m_vMaxVelocity	=	Vec3(0,maxspeed-0.001f,0);
			lmg_caps.m_fFastDuration = rsubGAH06.m_fTotalDuration*I2M.fr + rsubGAH07.m_fTotalDuration*I2M.rr + rsubGAH08.m_fTotalDuration*I2M.br   +   rsubGAH09.m_fTotalDuration*I2M.fl + rsubGAH0a.m_fTotalDuration*I2M.ll + rsubGAH0b.m_fTotalDuration*I2M.bl;
		}

		SPU_NO_INLINE void GetCapabilities_ISTP(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			GlobalAnimationHeaderType rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x06]) ];
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.0f;

			Vec2 sFR = Vec2(0,rsubGAH00.m_vVelocity.y);
			Vec2 sRR = Vec2(rsubGAH01.m_vVelocity);
			Vec2 sBR = Vec2(0,rsubGAH02.m_vVelocity.y);

			Vec2 sFL = Vec2(0,rsubGAH03.m_vVelocity.y);
			Vec2 sRL = Vec2(rsubGAH04.m_vVelocity);
			Vec2 sBL = Vec2(0,rsubGAH05.m_vVelocity.y);
			IdleStepWeights6 blend = GetIdleStepWeights6(sFR,sRR,sBR, sFL,sRL,sBL, lmg.m_BlendSpace.m_strafe );
			//	f32 turn = rsubGAH00.m_fTurnSpeed*blend.fr + rsubGAH01.m_fTurnSpeed*blend.rr + rsubGAH02.m_fTurnSpeed*blend.br   +   rsubGAH03.m_fTurnSpeed*blend.fl + rsubGAH04.m_fTurnSpeed*blend.ll + rsubGAH05.m_fTurnSpeed*blend.bl;
			Vec2 velocity = sFR*blend.fr + sRR*blend.rr + sBR*blend.br + sFL*blend.fl + sRL*blend.ll + sBL*blend.bl;
			f32 duration = rsubGAH00.m_fTotalDuration*blend.fr + rsubGAH01.m_fTotalDuration*blend.rr + rsubGAH02.m_fTotalDuration*blend.br + rsubGAH03.m_fTotalDuration*blend.fl + rsubGAH04.m_fTotalDuration*blend.ll + rsubGAH05.m_fTotalDuration*blend.bl;

			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_bHasDistance = 1;
			lmg_caps.m_bHasVelocity = 0;
			lmg_caps.m_travelAngleChangeRate = 0.0f;
			lmg_caps.m_travelSpeedChangeRate = 0.0f;

			lmg_caps.m_fAllowDesiredTurning=0;
			//lmg_caps.m_fFastTurnLeft		= DEG2RAD(-0);
			//lmg_caps.m_fFastTurnRight	= DEG2RAD(+0);

			lmg_caps.m_travelAngleChangeRate = 0.0f;
			lmg_caps.m_travelSpeedChangeRate = 0.0f;

			lmg_caps.m_fSlowDuration	=	rsubGAH06.m_fTotalDuration;
			lmg_caps.m_fFastDuration	=	duration;

			//	lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity	= Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity	= velocity;
		}

		void GetCapabilities_T_IA(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
		}

		void GetCapabilities_TRN1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.0f;

			lmg_caps.m_fAllowDesiredTurning=0;
			lmg_caps.m_vMinVelocity	=	Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity	=	Vec3(0,0.001f,0);
		}

		SPU_NO_INLINE void GetCapabilities_STF1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;
			Property::GetProperties_STF1( pAnimationSet,lmg,0,lmg_caps );
		}

		SPU_NO_INLINE void GetCapabilities_STF2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;
			Property::GetProperties_STF2( pAnimationSet,lmg,0,lmg_caps, pGHLMG );
		}

		SPU_NO_INLINE void GetCapabilities_SUD2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_bHasSlopeAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;

			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0) { uh=t;	fw=1-t;	}
			else	{	fw=1+t;	dh=1-(t+1);	}
			LMGCapabilities ulmg;		Property::GetProperties_STF2( pAnimationSet,lmg, 0,ulmg, pGHLMG );
			LMGCapabilities plmg;		Property::GetProperties_STF2( pAnimationSet,lmg, 8,plmg, pGHLMG  );
			LMGCapabilities dlmg;		Property::GetProperties_STF2( pAnimationSet,lmg,16,dlmg, pGHLMG  );
			lmg_caps.m_fSlowDuration	= ulmg.m_fSlowDuration*uh+plmg.m_fSlowDuration*fw+dlmg.m_fSlowDuration*dh;
			lmg_caps.m_fFastDuration	= ulmg.m_fFastDuration*uh+plmg.m_fFastDuration*fw+dlmg.m_fFastDuration*dh;
			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity	= ulmg.m_vMinVelocity*uh+plmg.m_vMinVelocity*fw+dlmg.m_vMinVelocity*dh;
			lmg_caps.m_vMaxVelocity	= ulmg.m_vMaxVelocity*uh+plmg.m_vMaxVelocity*fw+dlmg.m_vMaxVelocity*dh;
		}

		SPU_NO_INLINE void GetCapabilities_S__1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_fAllowDesiredTurning=1;
			Property::GetProperties_S1( pAnimationSet,lmg,0,lmg_caps,pGHLMG  );
		}

		SPU_NO_INLINE void GetCapabilities_M__1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;
			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_fAllowDesiredTurning=1;
			Property::GetProperties_S1( pAnimationSet,lmg,0,lmg_caps, pGHLMG );
		}

		SPU_NO_INLINE void GetCapabilities_S__2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_fAllowDesiredTurning=1;
			Property::GetProperties_S2( pAnimationSet,lmg,0,lmg_caps, pGHLMG );
		}

		SPU_NO_INLINE void GetCapabilities_ST_2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;
			lmg_caps.m_bHasVelocity=1;
			Property::GetProperties_ST2( pAnimationSet,lmg, 0, lmg_caps,pGHLMG );
		}

		SPU_NO_INLINE void GetCapabilities_S_H2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF* parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_bHasSlopeAsset=0;
			lmg_caps.m_fAllowDesiredTurning=1;

			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0) { uh=t;	fw=1-t;	}
			else	{	fw=1+t;	dh=1-(t+1);	}
			LMGCapabilities ulmg;		Property::GetProperties_STF2( pAnimationSet,lmg, 0,ulmg, pGHLMG );
			LMGCapabilities plmg;		Property::GetProperties_S2( pAnimationSet,lmg, 8,plmg, pGHLMG  );
			LMGCapabilities dlmg;		Property::GetProperties_STF2( pAnimationSet,lmg,20,dlmg, pGHLMG  );
			lmg_caps.m_fSlowDuration	= ulmg.m_fSlowDuration*uh+plmg.m_fSlowDuration*fw+dlmg.m_fSlowDuration*dh;
			lmg_caps.m_fFastDuration	= ulmg.m_fFastDuration*uh+plmg.m_fFastDuration*fw+dlmg.m_fFastDuration*dh;
			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity	= ulmg.m_vMinVelocity*uh+plmg.m_vMinVelocity*fw+dlmg.m_vMinVelocity*dh;
			lmg_caps.m_vMaxVelocity	= ulmg.m_vMaxVelocity*uh+plmg.m_vMaxVelocity*fw+dlmg.m_vMaxVelocity*dh;
		}

		SPU_NO_INLINE void GetCapabilities_STH2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_bHasSlopeAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1.0f;

			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0) { uh=t;	fw=1-t;	}
			else	{	fw=1+t;	dh=1-(t+1);	}
			LMGCapabilities ulmg;		Property::GetProperties_STF2( pAnimationSet,lmg, 0,ulmg, pGHLMG );
			LMGCapabilities plmg;		Property::GetProperties_ST2(  pAnimationSet,lmg, 8,plmg, pGHLMG  );
			LMGCapabilities dlmg;		Property::GetProperties_STF2( pAnimationSet,lmg,25,dlmg, pGHLMG  );


			f32 dot=lmg.m_BlendSpace.m_strafe|Vec2(0,1);
			//float fColor[4] = {1,1,0,1};
			//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dot: %f  strafe: %f %f ",dot,lmg.m_BlendSpace.m_strafe.x,lmg.m_BlendSpace.m_strafe.y); g_YLine+=0x10;
			bool useCurveTurning = (dot>0.9999f); // somehow depend on body and travel directions, true only when they are identical.
			if (useCurveTurning)
			{
				lmg_caps.m_fSlowTurnLeft		= ulmg.m_fSlowTurnLeft*uh +plmg.m_fSlowTurnLeft*fw +dlmg.m_fSlowTurnLeft*dh;
				lmg_caps.m_fSlowTurnRight		= ulmg.m_fSlowTurnRight*uh+plmg.m_fSlowTurnRight*fw+dlmg.m_fSlowTurnRight*dh;
				lmg_caps.m_fFastTurnLeft		= ulmg.m_fFastTurnLeft*uh+plmg.m_fFastTurnLeft*fw+dlmg.m_fFastTurnLeft*dh;
				lmg_caps.m_fFastTurnRight		= ulmg.m_fFastTurnRight*uh+plmg.m_fFastTurnRight*fw+dlmg.m_fFastTurnRight*dh;
			}
			else
			{
				lmg_caps.m_fSlowTurnLeft		= 0.0f;
				lmg_caps.m_fSlowTurnRight		= 0.0f;
				lmg_caps.m_fFastTurnLeft		= 0.0f;
				lmg_caps.m_fFastTurnRight		= 0.0f;
			}

			lmg_caps.m_fSlowDuration		= ulmg.m_fSlowDuration*uh+plmg.m_fSlowDuration*fw+dlmg.m_fSlowDuration*dh;
			lmg_caps.m_fFastDuration		= ulmg.m_fFastDuration*uh+plmg.m_fFastDuration*fw+dlmg.m_fFastDuration*dh;
			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity		= ulmg.m_vMinVelocity*uh+plmg.m_vMinVelocity*fw+dlmg.m_vMinVelocity*dh;
			lmg_caps.m_vMaxVelocity		= ulmg.m_vMaxVelocity*uh+plmg.m_vMaxVelocity*fw+dlmg.m_vMaxVelocity*dh;

			lmg_caps.m_bHasCurving = plmg.m_bHasCurving;
			lmg_caps.m_curvingChangeRate = plmg.m_curvingChangeRate;
			lmg_caps.m_turnSpeedChangeRate = plmg.m_turnSpeedChangeRate;

		}

		SPU_NO_INLINE void GetCapabilities_STHX(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* pGHLMG)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=1;
			lmg_caps.m_bHasTurningAsset=0;
			lmg_caps.m_bHasSlopeAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;

			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0) { uh=t;	fw=1-t;	}
			else	{	fw=1+t;	dh=1-(t+1);	}

			LMGCapabilities ulmg;		Property::GetProperties_STF2( pAnimationSet,lmg, 0,ulmg,pGHLMG );
			LMGCapabilities plmg;		Property::GetProperties_ST2X( pAnimationSet,lmg, 8,plmg,pGHLMG );
			LMGCapabilities dlmg;		Property::GetProperties_STF2( pAnimationSet,lmg,21,dlmg,pGHLMG );
			lmg_caps.m_fSlowDuration	= ulmg.m_fSlowDuration*uh+plmg.m_fSlowDuration*fw+dlmg.m_fSlowDuration*dh;
			lmg_caps.m_fFastDuration	= ulmg.m_fFastDuration*uh+plmg.m_fFastDuration*fw+dlmg.m_fFastDuration*dh;
			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity	= ulmg.m_vMinVelocity*uh+plmg.m_vMinVelocity*fw+dlmg.m_vMinVelocity*dh;
			lmg_caps.m_vMaxVelocity	= ulmg.m_vMaxVelocity*uh+plmg.m_vMaxVelocity*fw+dlmg.m_vMaxVelocity*dh;
		}

		SPU_NO_INLINE void GetCapabilities_FLR1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];

			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid			=	1;
			lmg_caps.m_bIsLMG				=	1;
			lmg_caps.m_bHasStrafingAsset	=	0;
			lmg_caps.m_bHasTurningAsset	=	1;
			lmg_caps.m_fAllowDesiredTurning=1;

			f32 fast_FWL = rsubGAH00.m_fSpeed; //sprint left
			f32 fast_FW  = rsubGAH01.m_fSpeed; //sprint
			f32 fast_FWR = rsubGAH02.m_fSpeed; //sprint right

			f32 t=lmg.m_BlendSpace.m_turn;
			f32 L=0,F=0,R=0;
			if (t<0) { L=-t; F=1.0f+t; }	else { F=1.0f-t; R=t; }

			f32 fw_speed  = fast_FWL*L + fast_FW*F + fast_FWR*R;

			//	float fColor[4] = {1,1,0,1};
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"L %f   F %f    R %f",L,F,R); g_YLine+=0x10;

			//turning
			f32 fast_TL = rsubGAH00.m_fTurnSpeed; //sprint left
			f32 fast_TR = rsubGAH02.m_fTurnSpeed; //sprint right

			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity	= Vec3(0,fw_speed,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,fw_speed,0);

			lmg_caps.m_fFastTurnLeft		=	fast_TL;
			lmg_caps.m_fFastTurnRight	=	fast_TR;
			lmg_caps.m_fAllowDesiredTurning=1;


			f32 dfL	= rsubGAH00.m_fTotalDuration;
			f32 dfF	= rsubGAH01.m_fTotalDuration;
			f32 dfR	= rsubGAH02.m_fTotalDuration;
			lmg_caps.m_fFastDuration = dfL*L + dfF*F + dfR*R;
			lmg_caps.m_fSlowDuration = dfL*L + dfF*F + dfR*R;

		}

		SPU_NO_INLINE void GetCapabilities_FLR3(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			//	GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03] ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];
			GlobalAnimationHeaderType rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x06]) ];

			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=0;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;

			f32 fast_FWL = rsubGAH00.m_fSpeed;	//fast left
			f32 fast_FW  = rsubGAH01.m_fSpeed;	//fast
			f32 fast_FWR = rsubGAH02.m_fSpeed;	//fast right
			f32 slow_FWL = rsubGAH04.m_fSpeed;	//slow left
			f32 slow_FW	 = rsubGAH05.m_fSpeed;	//slow
			f32 slow_FWR = rsubGAH06.m_fSpeed;	//slow right

			f32 t=lmg.m_BlendSpace.m_turn;
			f32 L=0,F=0,R=0;
			if (t<0) { L=-t; F=1.0f+t; }	else { F=1.0f-t; R=t; }
			//	float fColor[4] = {1,1,0,1};
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"++++++++++++++++++++++++++         L %f   F %f    R %f",L,F,R); g_YLine+=0x10;

			f32 fw_speed_fast = fast_FWL*L + fast_FW*F + fast_FWR*R;
			f32 fw_speed_slow	= slow_FWL*L + slow_FW*F + slow_FWR*R;

			lmg_caps.m_fFastTurnLeft		= rsubGAH00.m_fTurnSpeed; //sprint left;
			lmg_caps.m_fFastTurnRight	= rsubGAH02.m_fTurnSpeed; //sprint right;
			lmg_caps.m_fAllowDesiredTurning=1;
			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity=Vec3(0,fw_speed_slow,0);
			lmg_caps.m_vMaxVelocity=Vec3(0,fw_speed_fast,0);



			f32 dfL	= rsubGAH00.m_fTotalDuration;
			f32 dfF	= rsubGAH01.m_fTotalDuration;
			f32 dfR	= rsubGAH02.m_fTotalDuration;
			f32 dsL	= rsubGAH04.m_fTotalDuration;
			f32 dsF	= rsubGAH05.m_fTotalDuration;
			f32 dsR	= rsubGAH06.m_fTotalDuration;
			lmg_caps.m_fFastDuration = dfL*L + dfF*F + dfR*R;
			lmg_caps.m_fSlowDuration = dsL*L + dsF*F + dsR*R;

			lmg_caps.m_fFastTurnLeft		=	rsubGAH00.m_fTurnSpeed;
			lmg_caps.m_fFastTurnRight	=	rsubGAH02.m_fTurnSpeed;

		}

		SPU_NO_INLINE void GetCapabilities_FLR2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=0;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;


			f32 fast_FWL = rsubGAH00.m_fSpeed; //fast left
			f32 fast_FW  = rsubGAH01.m_fSpeed; //fast
			f32 fast_FWR = rsubGAH02.m_fSpeed; //fast right

			f32 slow_FWL = rsubGAH03.m_fSpeed; //slow left
			f32 slow_FW  = rsubGAH04.m_fSpeed; //slow
			f32 slow_FWR = rsubGAH05.m_fSpeed; //slow right

			f32 t=lmg.m_BlendSpace.m_turn;
			f32 L=0,F=0,R=0;
			if (t<0) { L=-t; F=1.0f+t; }	else { F=1.0f-t; R=t; }
			//f32 white[] = {1.0f, 1.0f, 1.0f, 1.0f};
			//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, white, false,"--------------------------------------L: %f   F: %f   R: %f   turn: %f",L,F,R,t); g_YLine+=0x10;

			f32 fw_speed_fast = fast_FWL*L + fast_FW*F + fast_FWR*R;
			f32 fw_speed_slow = slow_FWL*L + slow_FW*F + slow_FWR*R;


			f32 dfL	= rsubGAH00.m_fTotalDuration;
			f32 dfF	= rsubGAH01.m_fTotalDuration;
			f32 dfR	= rsubGAH02.m_fTotalDuration;
			f32 dsL	= rsubGAH03.m_fTotalDuration;
			f32 dsF	= rsubGAH04.m_fTotalDuration;
			f32 dsR	= rsubGAH05.m_fTotalDuration;
			lmg_caps.m_fFastDuration = dfL*L + dfF*F + dfR*R;
			lmg_caps.m_fSlowDuration = dsL*L + dsF*F + dsR*R;

			lmg_caps.m_fFastTurnLeft		=	rsubGAH00.m_fTurnSpeed;
			lmg_caps.m_fFastTurnRight	=	rsubGAH02.m_fTurnSpeed;
			lmg_caps.m_fAllowDesiredTurning=1;

			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity= Vec3(0,fw_speed_slow,0);
			lmg_caps.m_vMaxVelocity= Vec3(0,fw_speed_fast,0);

		}

		void GetCapabilities_UDH1(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_vMinVelocity=Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity=Vec3(0,0.001f,0);
		}

		void GetCapabilities_UDH2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_vMinVelocity=Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity=Vec3(0,0.001f,0);
		}

		SPU_NO_INLINE void GetCapabilities_UDH3(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=0;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_bHasSlopeAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;


			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0)
			{
				uh=t;	fw=(1-t);
			}
			else
			{
				fw=t+1;	dh=1-(t+1);
			}

			GlobalAnimationHeaderCAF& rsubGAH0_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0+0]) ];
			GlobalAnimationHeaderCAF& rsubGAH1_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[1+0]) ];
			GlobalAnimationHeaderCAF& rsubGAH2_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[2+0]) ];
			GlobalAnimationHeaderCAF& rsubGAH6_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[6+0]) ];
			GlobalAnimationHeaderCAF& rsubGAH7_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[7+0]) ];
			GlobalAnimationHeaderCAF& rsubGAH8_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[8+0]) ];
			f32 fast_UHL = rsubGAH0_0.m_fSpeed; //sprint left
			f32 fast_UH  = rsubGAH1_0.m_fSpeed; //sprint
			f32 fast_UHR = rsubGAH2_0.m_fSpeed; //sprint right
			f32 slow_UHL = rsubGAH6_0.m_fSpeed; //slow jog left
			f32 slow_UH  = rsubGAH7_0.m_fSpeed; //slow jog
			f32 slow_UHR = rsubGAH8_0.m_fSpeed; //slow jog right
			f32 dfast_UHL = rsubGAH0_0.m_fTotalDuration; //sprint left
			f32 dfast_UH  = rsubGAH1_0.m_fTotalDuration; //sprint
			f32 dfast_UHR = rsubGAH2_0.m_fTotalDuration; //sprint right
			f32 dslow_UHL = rsubGAH6_0.m_fTotalDuration; //slow jog left
			f32 dslow_UH  = rsubGAH7_0.m_fTotalDuration; //slow jog
			f32 dslow_UHR = rsubGAH8_0.m_fTotalDuration; //slow jog right

			GlobalAnimationHeaderCAF& rsubGAH0_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0+9]) ];
			GlobalAnimationHeaderCAF& rsubGAH1_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[1+9]) ];
			GlobalAnimationHeaderCAF& rsubGAH2_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[2+9]) ];
			GlobalAnimationHeaderCAF& rsubGAH6_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[6+9]) ];
			GlobalAnimationHeaderCAF& rsubGAH7_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[7+9]) ];
			GlobalAnimationHeaderCAF& rsubGAH8_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[8+9]) ];
			f32 fast_FWL = rsubGAH0_9.m_fSpeed; //sprint left
			f32 fast_FW  = rsubGAH1_9.m_fSpeed; //sprint
			f32 fast_FWR = rsubGAH2_9.m_fSpeed; //sprint right
			f32 slow_FWL = rsubGAH6_9.m_fSpeed; //slow jog left
			f32 slow_FW  = rsubGAH7_9.m_fSpeed; //slow jog
			f32 slow_FWR = rsubGAH8_9.m_fSpeed; //slow jog right

			f32 dfast_FWL = rsubGAH0_9.m_fTotalDuration; //sprint left
			f32 dfast_FW  = rsubGAH1_9.m_fTotalDuration; //sprint
			f32 dfast_FWR = rsubGAH2_9.m_fTotalDuration; //sprint right
			f32 dslow_FWL = rsubGAH6_9.m_fTotalDuration; //slow jog left
			f32 dslow_FW  = rsubGAH7_9.m_fTotalDuration; //slow jog
			f32 dslow_FWR = rsubGAH8_9.m_fTotalDuration; //slow jog right

			GlobalAnimationHeaderCAF& rsubGAH0_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0+18]) ];
			GlobalAnimationHeaderCAF& rsubGAH1_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[1+18]) ];
			GlobalAnimationHeaderCAF& rsubGAH2_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[2+18]) ];
			GlobalAnimationHeaderCAF& rsubGAH6_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[6+18]) ];
			GlobalAnimationHeaderCAF& rsubGAH7_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[7+18]) ];
			GlobalAnimationHeaderCAF& rsubGAH8_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[8+18]) ];
			f32 fast_DHL = rsubGAH0_18.m_fSpeed; //sprint left
			f32 fast_DH  = rsubGAH1_18.m_fSpeed; //sprint
			f32 fast_DHR = rsubGAH2_18.m_fSpeed; //sprint right
			f32 slow_DHL = rsubGAH6_18.m_fSpeed; //slow jog left
			f32 slow_DH  = rsubGAH7_18.m_fSpeed; //slow jog
			f32 slow_DHR = rsubGAH8_18.m_fSpeed; //slow jog right

			f32 dfast_DHL = rsubGAH0_18.m_fTotalDuration; //sprint left
			f32 dfast_DH  = rsubGAH1_18.m_fTotalDuration; //sprint
			f32 dfast_DHR = rsubGAH2_18.m_fTotalDuration; //sprint right
			f32 dslow_DHL = rsubGAH6_18.m_fTotalDuration; //slow jog left
			f32 dslow_DH  = rsubGAH7_18.m_fTotalDuration; //slow jog
			f32 dslow_DHR = rsubGAH8_18.m_fTotalDuration; //slow jog right

			f32 L=0,F=0,R=0;
			if (lmg.m_BlendSpace.m_turn<0)
			{
				//left side
				L=-lmg.m_BlendSpace.m_turn;
				F=1.0f+lmg.m_BlendSpace.m_turn;
			}
			else
			{
				//right side
				F=1.0f-lmg.m_BlendSpace.m_turn;
				R=lmg.m_BlendSpace.m_turn;
			}

			f32 uh_maxspeed= fast_UHL*L + fast_UH*F + fast_UHR*R;
			f32 uh_minspeed= slow_UHL*L + slow_UH*F + slow_UHR*R;
			f32 fw_maxspeed= fast_FWL*L + fast_FW*F + fast_FWR*R;
			f32 fw_minspeed= slow_FWL*L + slow_FW*F + slow_FWR*R;
			f32 dh_maxspeed= fast_DHL*L + fast_DH*F + fast_DHR*R;
			f32 dh_minspeed= slow_DHL*L + slow_DH*F + slow_DHR*R;
			f32 maxspeed	= uh_maxspeed*uh + fw_maxspeed*fw + dh_maxspeed*dh;
			f32 minspeed	= uh_minspeed*uh + fw_minspeed*fw + dh_minspeed*dh;

			f32 duh_maxspeed= dfast_UHL*L + dfast_UH*F + dfast_UHR*R;
			f32 duh_minspeed= dslow_UHL*L + dslow_UH*F + dslow_UHR*R;
			f32 dfw_maxspeed= dfast_FWL*L + dfast_FW*F + dfast_FWR*R;
			f32 dfw_minspeed= dslow_FWL*L + dslow_FW*F + dslow_FWR*R;
			f32 ddh_maxspeed= dfast_DHL*L + dfast_DH*F + dfast_DHR*R;
			f32 ddh_minspeed= dslow_DHL*L + dslow_DH*F + dslow_DHR*R;
			f32 maxduration	= duh_maxspeed*uh + dfw_maxspeed*fw + ddh_maxspeed*dh;
			f32 minduration	= duh_minspeed*uh + dfw_minspeed*fw + ddh_minspeed*dh;

			/*
			float fColor[4] = {1,1,0,1};
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"left side fast: L %f  F %f  R %f ",L,F,R); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"uh_minspeed: %f",uh_minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"uh_maxspeed: %f",uh_maxspeed); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"fw_minspeed: %f",fw_minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"fw_maxspeed: %f",fw_maxspeed); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dh_minspeed: %f",dh_minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dh_maxspeed: %f",dh_maxspeed); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"minspeed: %f",minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"maxspeed: %f",maxspeed); g_YLine+=0x10;
			*/

			lmg_caps.m_fSlowDuration	= minduration;
			lmg_caps.m_fFastDuration	= maxduration;

			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity	= Vec3(0,minspeed,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,maxspeed,0);
		}

		SPU_NO_INLINE void GetCapabilities_PROT(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* rGlobalAnimHeader)
		{
			uint32 numAssets = rGlobalAnimHeader->m_arrBSAnimations.size();
			f32 fMinDuration=+99999.0f;
			f32 fMaxDuration=-99999.0f;
			f32 fTurnLeft	=-99999.0f;
			f32 fTurnRight=+99999.0f;
			for (uint32 i=0; i<numAssets; i++)
			{
				int32 gid = pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[i]);

				f32 turn = parrGlobalAnimations[ gid ].m_fTurnSpeed;
				if (fTurnLeft<turn)
					fTurnLeft=turn;
				if (fTurnRight>turn)
					fTurnRight=turn;

				f32 duration = parrGlobalAnimations[ gid ].m_fTotalDuration;
				if (fMinDuration>duration)
					fMinDuration=duration;
				if (fMaxDuration<duration)
					fMaxDuration=duration;
			}

			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			//lmg_caps.m_bHasStrafingAsset=-1;
			//lmg_caps.m_bHasDistance=-1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=0;

			//lmg_caps.m_fSlowDuration	=	fMinDuration;
			//lmg_caps.m_fFastDuration	=	fMaxDuration;

			lmg_caps.m_vMinVelocity	= Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,0,0);

			//lmg_caps.m_fFastTurnLeft		= fTurnLeft;
			//lmg_caps.m_fFastTurnRight	= fTurnRight;	
		}

		void GetCapabilities_PUSH(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=0;
		}

		SPU_NO_INLINE void GetCapabilities_CLMB(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* rGlobalAnimHeader)
		{
			uint32 numAssets = rGlobalAnimHeader->m_arrBSAnimations.size();
			f32 fMinDuration=+99999.0f;
			f32 fMaxDuration=-99999.0f;
			f32 fTurnLeft	=-99999.0f;
			f32 fTurnRight=+99999.0f;
			for (uint32 i=0; i<numAssets; i++)
			{
				int32 gid = pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[i]);

				f32 turn = parrGlobalAnimations[ gid ].m_fTurnSpeed;
				if (fTurnLeft<turn)
					fTurnLeft=turn;
				if (fTurnRight>turn)
					fTurnRight=turn;

				f32 duration = parrGlobalAnimations[ gid ].m_fTotalDuration;
				if (fMinDuration>duration)
					fMinDuration=duration;
				if (fMaxDuration<duration)
					fMaxDuration=duration;
			}

			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			//lmg_caps.m_bHasStrafingAsset=-1;
			//lmg_caps.m_bHasDistance=-1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=0;

			//lmg_caps.m_fSlowDuration	=	fMinDuration;
			//lmg_caps.m_fFastDuration	=	fMaxDuration;

			lmg_caps.m_vMinVelocity	= Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,0,0);

			//lmg_caps.m_fFastTurnLeft		= fTurnLeft;
			//lmg_caps.m_fFastTurnRight	= fTurnRight;

		}

		void GetCapabilities_COOP(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
		}

		SPU_NO_INLINE void GetCapabilities_SCAL(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG* rGlobalAnimHeader)
		{
			uint32 numAssets = rGlobalAnimHeader->m_arrBSAnimations.size();
			f32 fMinDuration=+99999.0f;
			f32 fMaxDuration=-99999.0f;
			f32 fTurnLeft	=-99999.0f;
			f32 fTurnRight=+99999.0f;
			for (uint32 i=0; i<numAssets; i++)
			{
				int32 gid = pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[i]);

				f32 turn = parrGlobalAnimations[ gid ].m_fTurnSpeed;
				if (fTurnLeft<turn)
					fTurnLeft=turn;
				if (fTurnRight>turn)
					fTurnRight=turn;

				f32 duration = parrGlobalAnimations[ gid ].m_fTotalDuration;
				if (fMinDuration>duration)
					fMinDuration=duration;
				if (fMaxDuration<duration)
					fMaxDuration=duration;
			}
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_fAllowDesiredTurning=0;
			lmg_caps.m_vMinVelocity	= Vec3(0,0,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,0,0);

		}


		SPU_NO_INLINE void GetCapabilities__TH2(CAnimationSet* pAnimationSet, const SParametric& lmg, LMGCapabilities& lmg_caps, GlobalAnimationHeaderCAF* parrGlobalAnimations, GlobalAnimationHeaderLMG* rGlobalAnimHeader )
		{
			lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.2f;

			//set LMG caps
			lmg_caps.m_bIsValid=1;
			lmg_caps.m_bIsLMG=1;
			lmg_caps.m_bHasStrafingAsset=0;
			lmg_caps.m_bHasTurningAsset=1;
			lmg_caps.m_bHasSlopeAsset=1;
			lmg_caps.m_fAllowDesiredTurning=1;


			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0)
			{
				uh=t;	fw=(1-t);
			}
			else
			{
				fw=t+1;	dh=1-(t+1);
			}

			GlobalAnimationHeaderCAF& rsubGAH0_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0+0]) ];
			GlobalAnimationHeaderCAF& rsubGAH1_0 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[1+0]) ];
			f32 fast_UH		= rsubGAH0_0.m_fSpeed; //sprint
			f32 slow_UH		= rsubGAH1_0.m_fSpeed; //slow jog
			f32 dfast_UH  = rsubGAH0_0.m_fTotalDuration; //sprint
			f32 dslow_UH  = rsubGAH1_0.m_fTotalDuration; //slow jog

			GlobalAnimationHeaderCAF& rsubGAH0_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0+2]) ];
			GlobalAnimationHeaderCAF& rsubGAH1_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[1+2]) ];
			GlobalAnimationHeaderCAF& rsubGAH2_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[2+2]) ];
			GlobalAnimationHeaderCAF& rsubGAH6_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[3+2]) ];
			GlobalAnimationHeaderCAF& rsubGAH7_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[4+2]) ];
			GlobalAnimationHeaderCAF& rsubGAH8_9 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[5+2]) ];
			f32 fast_FWL	= rsubGAH0_9.m_fSpeed; //sprint left
			f32 fast_FW		= rsubGAH1_9.m_fSpeed; //sprint
			f32 fast_FWR	= rsubGAH2_9.m_fSpeed; //sprint right
			f32 slow_FWL	= rsubGAH6_9.m_fSpeed; //slow jog left
			f32 slow_FW		= rsubGAH7_9.m_fSpeed; //slow jog
			f32 slow_FWR	= rsubGAH8_9.m_fSpeed; //slow jog right
			f32 dfast_FWL = rsubGAH0_9.m_fTotalDuration; //sprint left
			f32 dfast_FW  = rsubGAH1_9.m_fTotalDuration; //sprint
			f32 dfast_FWR = rsubGAH2_9.m_fTotalDuration; //sprint right
			f32 dslow_FWL = rsubGAH6_9.m_fTotalDuration; //slow jog left
			f32 dslow_FW  = rsubGAH7_9.m_fTotalDuration; //slow jog
			f32 dslow_FWR = rsubGAH8_9.m_fTotalDuration; //slow jog right

			GlobalAnimationHeaderCAF& rsubGAH0_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0+8]) ];
			GlobalAnimationHeaderCAF& rsubGAH1_18 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[1+8]) ];
			f32 fast_DH		= rsubGAH0_18.m_fSpeed; //sprint
			f32 slow_DH		= rsubGAH1_18.m_fSpeed; //slow jog
			f32 dfast_DH  = rsubGAH0_18.m_fTotalDuration; //sprint
			f32 dslow_DH  = rsubGAH1_18.m_fTotalDuration; //slow jog

			f32 fTurn = lmg.m_BlendSpace.m_turn*(1.0f-fabsf(lmg.m_BlendSpace.m_slope));
			f32 L=0,F=0,R=0;
			if (lmg.m_BlendSpace.m_turn<0)
			{
				//left side
				L=-fTurn;
				F=1.0f+fTurn;
			}
			else
			{
				//right side
				F=1.0f-fTurn;
				R=fTurn;
			}

			f32 uh_maxspeed= fast_UH*F;
			f32 uh_minspeed= slow_UH*F;
			f32 fw_maxspeed= fast_FWL*L + fast_FW*F + fast_FWR*R;
			f32 fw_minspeed= slow_FWL*L + slow_FW*F + slow_FWR*R;
			f32 dh_maxspeed= fast_DH*F;
			f32 dh_minspeed= slow_DH*F;
			f32 maxspeed	= uh_maxspeed*uh + fw_maxspeed*fw + dh_maxspeed*dh;
			f32 minspeed	= uh_minspeed*uh + fw_minspeed*fw + dh_minspeed*dh;

			f32 duh_maxspeed= dfast_UH*F;
			f32 duh_minspeed= dslow_UH*F;
			f32 dfw_maxspeed= dfast_FWL*L + dfast_FW*F + dfast_FWR*R;
			f32 dfw_minspeed= dslow_FWL*L + dslow_FW*F + dslow_FWR*R;
			f32 ddh_maxspeed= dfast_DH*F;
			f32 ddh_minspeed= dslow_DH*F;
			f32 maxduration	= duh_maxspeed*uh + dfw_maxspeed*fw + ddh_maxspeed*dh;
			f32 minduration	= duh_minspeed*uh + dfw_minspeed*fw + ddh_minspeed*dh;

			/*
			float fColor[4] = {1,1,0,1};
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"left side fast: L %f  F %f  R %f ",L,F,R); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"uh_minspeed: %f",uh_minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"uh_maxspeed: %f",uh_maxspeed); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"fw_minspeed: %f",fw_minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"fw_maxspeed: %f",fw_maxspeed); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dh_minspeed: %f",dh_minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dh_maxspeed: %f",dh_maxspeed); g_YLine+=0x10;

			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"minspeed: %f",minspeed); g_YLine+=0x10;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"maxspeed: %f",maxspeed); g_YLine+=0x10;
			*/

			lmg_caps.m_fSlowDuration	= minduration;
			lmg_caps.m_fFastDuration	= maxduration;

			lmg_caps.m_bHasVelocity = 1;
			lmg_caps.m_vMinVelocity	= Vec3(0,minspeed,0);
			lmg_caps.m_vMaxVelocity	= Vec3(0,maxspeed,0);

		}

		void GetCapabilities_T_IS(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			GetCapabilities_ISTP(pAnimationSet, lmg, lmg_caps, parrGlobalAnimations, rGlobalAnimHeader);
		}

		void GetCapabilities_T_WS(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
			
			GetCapabilities_S__1(pAnimationSet, lmg, lmg_caps, parrGlobalAnimations, rGlobalAnimHeader);
		}

		void GetCapabilities_WFW2(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
		}

		void GetCapabilities_RFW3(CAnimationSet* pAnimationSet, const SParametric & lmg, LMGCapabilities & lmg_caps, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG * rGlobalAnimHeader)
		{
		}
	} // namespace _private
} // namespace LMG

namespace LMG
{
	LMGCapabilities GetCapabilities(CAnimationSet* pAnimSet, const SParametric& lmg)
	{
		curvingTurnSpeedChangeRate = proceduralTurnSpeedChangeRate;

		if (Console::GetInst().ca_GameControlledStrafing == 0)
			curvingTurnSpeedChangeRate *= 0.25f;

		//	float fColor[4] = {1,1,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"GetLMGCapabilities"); g_YLine+=0x10;

		LMGCapabilities lmg_caps;
		lmg_caps.m_travelAngleChangeRate = -1;
		lmg_caps.m_travelSpeedChangeRate = 8.0f;
		lmg_caps.m_turnSpeedChangeRate = proceduralTurnSpeedChangeRate;
		lmg_caps.m_fDesiredLocalLocationLookaheadTime = 0.0f;

		int32 animID=lmg.m_nParametricID;
		if (animID<0)
			animID=lmg.m_nAnimID[0]; //its no LMG, maybe it is a simple asset
		if (animID<0)
			return lmg_caps; //its not even a simple asset! quit function

		GlobalAnimationHeaderCAF* rGlobalAnimHeaderCAF = pAnimSet->GetGAH_CAF(animID);
		if (rGlobalAnimHeaderCAF) if (rGlobalAnimHeaderCAF->IsAssetLMG() == 0)
		{
			lmg_caps.m_bIsValid=1;

			lmg_caps.m_fSlowDuration		=	rGlobalAnimHeaderCAF->m_fTotalDuration;
			lmg_caps.m_fFastDuration		=	rGlobalAnimHeaderCAF->m_fTotalDuration;

			lmg_caps.m_fFastTurnLeft		=	rGlobalAnimHeaderCAF->m_fTurnSpeed;
			lmg_caps.m_fFastTurnRight	=	rGlobalAnimHeaderCAF->m_fTurnSpeed;
			lmg_caps.m_fAllowDesiredTurning	=	0;

			// Don't set the speed here, since we don't have a flag for if we allow parameterized speed or not.
			// If we need the speed to be set here we also need to add a m_bHasVelocity flag to the caps.
			// NOTE: m_bHasVelocity has been added and is 0 by default.
			lmg_caps.m_vMinVelocity	=	rGlobalAnimHeaderCAF->m_vVelocity;
			lmg_caps.m_vMaxVelocity	=	rGlobalAnimHeaderCAF->m_vVelocity;
			return lmg_caps;
		}


		//-------------------------------------------------------------------------------------------
		//----------                evaluate Locomotion Group                             -----------
		//-------------------------------------------------------------------------------------------

		GlobalAnimationHeaderLMG* rGlobalAnimHeaderLMG = pAnimSet->GetGAH_LMG(animID);
		if (rGlobalAnimHeaderLMG) if (rGlobalAnimHeaderLMG->IsAssetLMG())
		{
			GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];
			lmg_caps.m_BlendType=rGlobalAnimHeaderLMG->m_nBlendCodeLMG;

			uint32 nBlendCode = rGlobalAnimHeaderLMG->m_nBlendCodeLMG;

			switch( AnimCodeLookup::ComputeAnimCode(nBlendCode) )
			{
			#define DRY(X) case AnimCodeLookup::X: \
											_private::GetCapabilities_##X( pAnimSet, lmg, lmg_caps, parrGlobalAnimations, rGlobalAnimHeaderLMG ); break;
			#include "LMG_Types.inc"
			#undef DRY
			}
			return lmg_caps;
		}

		if (rGlobalAnimHeaderCAF && rGlobalAnimHeaderLMG)
		if (rGlobalAnimHeaderCAF->IsAssetCreated() == 0 && rGlobalAnimHeaderLMG->IsAssetCreated() == 0)
		{
#if !defined(__SPU__)
			AnimFileWarning(pAnimSet->m_pModel->GetModelFilePath(),	"animation-asset '%s' is not loaded in function GetLMGCapabilities()",	rGlobalAnimHeaderLMG->GetFilePath());
#endif
			return lmg_caps;
		}

		return lmg_caps; 
	}
} // namespace LMG