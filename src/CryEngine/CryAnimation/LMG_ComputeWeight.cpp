///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   LMG_ComputeWeight.cpp
//  Version:     v1.00
//  Created:     07/06/2009 by Jaewon Jung
//  Description: Locomotion Group utility functions
// -------------------------------------------------------------------------
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <float.h>
#include "LMG.h"
#include "ModelAnimationSet.h"
#include <IRenderAuxGeom.h>
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include "CharacterManager.h"

// on spu a structure defined in forwardkinematicsspu.cpp is used to hold the needed data
#if !defined(__SPU__)
typedef GlobalAnimationHeaderCAF&	GlobalAnimationHeaderType;
#endif

namespace LMG
{
	namespace _private
	{
		
		void ComputeWeight4( Vec4 &Weight4, const Vec2& P, const Vec2 &p0, const Vec2 &p1, const Vec2 &p2, const Vec2 &p3 ) 
		{
			Vec4 w;	
			struct TW3
			{
				static ILINE void Weight3( const Vec2& p, const Vec2 &v0, const Vec2 &v1, const Vec2 &v2, f32&w0,f32&w1,f32&w2,f32&w3 )
				{
					w0=0;w1=0;w2=0;w3=0;
					Plane plane = Plane::CreatePlane( v0,v1,Vec3(v0.x,v0.y,1) ); 
					if ((plane|p)<=0)
					{
						Vec2 e0 = v0-v2;
						Vec2 e1 = v1-v2;
						Vec2 p2	=	p-v2;
						w0 =	p2.x*e1.y	- e1.x*p2.y;
						w1 =	e0.x*p2.y	-	p2.x*e0.y;
						w2 =	e0.x*e1.y -	e1.x*e0.y - w0-w1;
					}
				}
			};

			TW3::Weight3( P, p1,p3,p0,  w.y,w.w,w.x, w.z );
			Weight4=w;
			TW3::Weight3( P, p3,p1,p2,  w.w,w.y,w.z, w.x );
			Weight4+=w;
			TW3::Weight3( P, p2,p0,p1,  w.z,w.x,w.y, w.w );
			Weight4+=w;
			TW3::Weight3( P, p0,p2,p3,  w.x,w.z,w.w, w.y );
			Weight4+=w;

			Weight4 /= (Weight4.x+Weight4.y+Weight4.z+Weight4.w);

			if (Weight4.x<0)	Weight4.x=0;
			if (Weight4.y<0)	Weight4.y=0;
			if (Weight4.z<0)	Weight4.z=0;
			if (Weight4.w<0)	Weight4.w=0;
			if (Weight4.x>1)	Weight4.x=1;
			if (Weight4.y>1)	Weight4.y=1;
			if (Weight4.z>1)	Weight4.z=1;
			if (Weight4.w>1)	Weight4.w=1;
		}

		LocomotionWeights6 ComputeWeightLoco6( Vec2& P,Vec2 &FL,Vec2 &FF,Vec2 &FR,  Vec2 &SL,Vec2 &SF,Vec2 &SR) 
		{
			LocomotionWeights6 w,Weight6(0,0,0, 0,0,0);

			if (P.x<0)
			{
				Vec4 w4;
				ComputeWeight4( w4, P, FL,SL,SF,FF ); 
				Weight6.fl+=w4.x;
				Weight6.sl+=w4.y;
				Weight6.sf+=w4.z;
				Weight6.ff+=w4.w;
			}
			else
			{
				Vec4 w4;
				ComputeWeight4( w4, P, FF,SF,SR,FR ); 
				Weight6.ff+=w4.x;
				Weight6.sf+=w4.y;
				Weight6.sr+=w4.z;
				Weight6.fr+=w4.w;
			}


			f32 sum = (Weight6.fl+Weight6.ff+Weight6.fr + Weight6.sl+Weight6.sf+Weight6.sr );
			if (sum)
			{
				Weight6 /= sum;
			}
			else
			{
				Weight6.fl=-1; Weight6.ff=-1; Weight6.fr=-1;
				Weight6.sl=-1; Weight6.sf=-1; Weight6.sr=-1;
			}

			return Weight6;
		}

		LocomotionWeights7 ComputeWeightLoco7( Vec2& P,Vec2 &FL,Vec2 &FF,Vec2 &FR,Vec2 &MF,Vec2 &SL,Vec2 &SF,Vec2 &SR) 
		{

			LocomotionWeights7 w,Weight7(0,0,0, 0, 0,0,0);
			struct TW3
			{
				static ILINE void Weight3a( Vec2 p,  Vec2 v0,Vec2 v1,Vec2 v2,   f32&w0,f32&w1,f32&w2,  f32&w3, f32&w4,f32&w5,f32&w6 )
				{
					w0=0; w1=0; w2=0; w3=0; w4=0; w5=0; w6=0;
					Plane plane = Plane::CreatePlane( v0,v1,Vec3(v0.x,v0.y,1) ); 
					if ((plane|p)<=0)
					{
						Vec2 e0 = v0-v2;
						Vec2 e1 = v1-v2;
						Vec2 p2	=	p-v2;
						w0 =	p2.x*e1.y	- e1.x*p2.y;
						w1 =	e0.x*p2.y	-	p2.x*e0.y;
						w2 =	e0.x*e1.y -	e1.x*e0.y - w0-w1;
					}
				}
				static ILINE void Weight3b( Vec2 p,  Vec2 v0,Vec2 v1,Vec2 v2,   f32&w0,f32&w1,f32&w2,  f32&w3, f32&w4,f32&w5,f32&w6 )
				{
					w0=0; w1=0; w2=0; w3=0; w4=0; w5=0; w6=0;
					Plane plane1 = Plane::CreatePlane( v0,v1,Vec3(v0.x,v0.y,1) ); 
					Plane plane2 = Plane::CreatePlane( v2,v0,Vec3(v2.x,v2.y,1) ); 
					if ((plane1|p)<=0 && (plane2|p)<=0)
					{
						Vec2 e0 = v0-v2;
						Vec2 e1 = v1-v2;
						Vec2 p2	=	p-v2;
						w0 =	p2.x*e1.y	- e1.x*p2.y;
						w1 =	e0.x*p2.y	-	p2.x*e0.y;
						w2 =	e0.x*e1.y -	e1.x*e0.y - w0-w1;
					}
				}
			};

			if (P.x<0)
			{
				TW3::Weight3a( P, MF,SL,SF,  w.mf,w.sl,w.sf, w.fl,w.ff,w.fr,w.sr );
				Weight7+=w;
				TW3::Weight3b( P, MF,FL,SL,  w.mf,w.fl,w.sl, w.ff,w.sf,w.fr,w.sr );
				Weight7+=w;
				TW3::Weight3a( P, FL,MF,FF,  w.fl,w.mf,w.ff, w.sl,w.sf,w.fr,w.sr );
				Weight7+=w;
			}
			else
			{
				TW3::Weight3a( P, SR,MF,SF,  w.sr,w.mf,w.sf, w.fr,w.ff,w.fl,w.sl );
				Weight7+=w;
				TW3::Weight3b( P, MF,SR,FR,  w.mf,w.sr,w.fr, w.ff,w.sf,w.fl,w.sl );
				Weight7+=w;
				TW3::Weight3a( P, MF,FR,FF,  w.mf,w.fr,w.ff, w.sr,w.sf,w.fl,w.sl );
				Weight7+=w;
			}

			f32 sum = (Weight7.fl+Weight7.ff+Weight7.fr + Weight7.mf + Weight7.sl+Weight7.sf+Weight7.sr );
			if (sum)
			{
				Weight7 /= sum;
			}
			else
			{
				assert(0);
				Weight7.fl=-1; Weight7.ff=-1; Weight7.fr=-1;
				Weight7.mf=-1;
				Weight7.sl=-1; Weight7.sf=-1; Weight7.sr=-1;
			}

			return Weight7;
		}

		struct BlendWeights_STF2
		{
			f32    ff; 
			f32 fl,   fr; 
			f32	   fb;

			f32    sf; 
			f32 sl,   sr; 
			f32	   sb;
		};

		StrafeWeights4 GetStrafingWeights4( Vec2 F, Vec2 L, Vec2 B, Vec2 R, Vec2 DesiredDirection );

		void ComputeWeight_STF2( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, BlendWeights_STF2& stf2, GlobalAnimationHeaderLMG& rGHLMG )
		{
			GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];

		//	GlobalAnimationHeaderCAF& rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ]; 
			Vec3 vVelocity0=rGHLMG.m_arrBSAnimations[offset+0x00].m_qVelocity;
			Vec3 vVelocity1=rGHLMG.m_arrBSAnimations[offset+0x01].m_qVelocity;
			Vec3 vVelocity2=rGHLMG.m_arrBSAnimations[offset+0x02].m_qVelocity;
			Vec3 vVelocity3=rGHLMG.m_arrBSAnimations[offset+0x03].m_qVelocity;

		//	GlobalAnimationHeaderCAF& rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x04]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x05]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x06]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x07]) ]; 
			Vec3 vVelocity4=rGHLMG.m_arrBSAnimations[offset+0x04].m_qVelocity;
			Vec3 vVelocity5=rGHLMG.m_arrBSAnimations[offset+0x05].m_qVelocity;
			Vec3 vVelocity6=rGHLMG.m_arrBSAnimations[offset+0x06].m_qVelocity;
			Vec3 vVelocity7=rGHLMG.m_arrBSAnimations[offset+0x07].m_qVelocity;

			f32 x0	=	(lmg.m_BlendSpace.m_speed+1)*0.5f;
			f32 x1	=	1.0f-(lmg.m_BlendSpace.m_speed+1)*0.5f;
			Vec2 bF = Vec2(vVelocity0*x0 + vVelocity4*x1);
			Vec2 bL = Vec2(vVelocity1*x0 + vVelocity5*x1);
			Vec2 bB = Vec2(vVelocity2*x0 + vVelocity6*x1);
			Vec2 bR = Vec2(vVelocity3*x0 + vVelocity7*x1);
			StrafeWeights4 blend = GetStrafingWeights4(bF, bL, bB, bR, lmg.m_BlendSpace.m_strafe);

			f32 t = lmg.m_BlendSpace.m_strafe.GetLength();
			blend.f	=	blend.f*t + 0.25f*(1.0f-t);
			blend.l	=	blend.l*t + 0.25f*(1.0f-t);	
			blend.b	=	blend.b*t + 0.25f*(1.0f-t);
			blend.r	=	blend.r*t + 0.25f*(1.0f-t);	

			stf2.ff=blend.f*x0;	
			stf2.fl=blend.l*x0;	
			stf2.fb=blend.b*x0;
			stf2.fr=blend.r*x0;	

			stf2.sf=blend.f*x1;	
			stf2.sl=blend.l*x1;	
			stf2.sb=blend.b*x1;
			stf2.sr=blend.r*x1;	

			return;
		}

		struct BlendWeights_S2
		{
			f32      ff; 
			f32   ffl; 
			f32 fl,       fr; 
			f32	      fbr;
			f32	     fb;

			f32      sf; 
			f32   sfl; 
			f32 sl,       sr; 
			f32	      sbr;
			f32	     sb;
		};

		StrafeWeights6 GetStrafingWeights6( Vec2 F,Vec2 FL,Vec2 L,  Vec2 B,Vec2 BR,Vec2 R, const Vec2& DesiredDirection   );
		StrafeWeights8 GetStrafingWeights8( Vec2 F,Vec2 FL,Vec2 L,Vec2 LB,   Vec2 B,Vec2 BR,Vec2 R,Vec2 RF,   const Vec2& DesiredDirection );

		SPU_NO_INLINE void ComputeWeight_S2( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, BlendWeights_S2& s2, GlobalAnimationHeaderLMG& rGHLMG )
		{
			GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];

		//	GlobalAnimationHeaderCAF& rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x04]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x05]) ]; 
			Vec3 vVelocity00=rGHLMG.m_arrBSAnimations[offset+0x00].m_qVelocity;
			Vec3 vVelocity01=rGHLMG.m_arrBSAnimations[offset+0x01].m_qVelocity;
			Vec3 vVelocity02=rGHLMG.m_arrBSAnimations[offset+0x02].m_qVelocity;
			Vec3 vVelocity03=rGHLMG.m_arrBSAnimations[offset+0x03].m_qVelocity;
			Vec3 vVelocity04=rGHLMG.m_arrBSAnimations[offset+0x04].m_qVelocity;
			Vec3 vVelocity05=rGHLMG.m_arrBSAnimations[offset+0x05].m_qVelocity;

		//	GlobalAnimationHeaderCAF& rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x06]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x07]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH08 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x08]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH09 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x09]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH0a = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0a]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH0b = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0b]) ]; 
			Vec3 vVelocity06=rGHLMG.m_arrBSAnimations[offset+0x06].m_qVelocity;
			Vec3 vVelocity07=rGHLMG.m_arrBSAnimations[offset+0x07].m_qVelocity;
			Vec3 vVelocity08=rGHLMG.m_arrBSAnimations[offset+0x08].m_qVelocity;
			Vec3 vVelocity09=rGHLMG.m_arrBSAnimations[offset+0x09].m_qVelocity;
			Vec3 vVelocity0a=rGHLMG.m_arrBSAnimations[offset+0x0a].m_qVelocity;
			Vec3 vVelocity0b=rGHLMG.m_arrBSAnimations[offset+0x0b].m_qVelocity;

			f32 x0=(lmg.m_BlendSpace.m_speed+1)*0.5f;
			f32 x1=1.0f-(lmg.m_BlendSpace.m_speed+1)*0.5f;

			Vec2 sF		= Vec2(vVelocity00*x0 + vVelocity06*x1);
			Vec2 sFL	= Vec2(vVelocity01*x0 + vVelocity07*x1);
			Vec2 sL		= Vec2(vVelocity02*x0 + vVelocity08*x1);
			Vec2 sB		= Vec2(vVelocity03*x0 + vVelocity09*x1);
			Vec2 sBR	= Vec2(vVelocity04*x0 + vVelocity0a*x1);
			Vec2 sR		= Vec2(vVelocity05*x0 + vVelocity0b*x1);
			StrafeWeights6 blend = GetStrafingWeights6(sF,sFL,sL, sB,sBR,sR, lmg.m_BlendSpace.m_strafe );

			f32 t = lmg.m_BlendSpace.m_strafe.GetLength();
			f32 p =	(1.0f/6.0f);
			blend.f		=	blend.f*t		+ p*(1.0f-t);
			blend.fl	=	blend.fl*t	+ p*(1.0f-t);
			blend.l		=	blend.l*t		+ p*(1.0f-t);	
			blend.b		=	blend.b*t		+ p*(1.0f-t);
			blend.br	=	blend.br*t	+ p*(1.0f-t);
			blend.r		=	blend.r*t		+ p*(1.0f-t);	

			s2.ff		=x0*blend.f;	
			s2.ffl	=x0*blend.fl;	
			s2.fl		=x0*blend.l;	
			s2.fb		=x0*blend.b;	
			s2.fbr	=x0*blend.br;	
			s2.fr		=x0*blend.r;	

			s2.sf		=x1*blend.f;	
			s2.sfl	=x1*blend.fl;	
			s2.sl		=x1*blend.l;	
			s2.sb		=x1*blend.b;	
			s2.sbr	=x1*blend.br;	
			s2.sr		=x1*blend.r;	

			return;
		}

		struct BlendWeights_ST2
		{
			f32      ff; 
			f32   ffl; 
			f32 fl,       fr; 
			f32	      fbr;
			f32	     fb;

			f32      mf; 

			f32      sf; 
			f32   sfl; 
			f32 sl,       sr; 
			f32	      sbr;
			f32	     sb;

			f32	fftl,fftr; 
			f32	sftl,sftr; 
		};

		SPU_NO_INLINE void ComputeWeight_ST2( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, BlendWeights_ST2& st2, GlobalAnimationHeaderLMG& rGHLMG )
		{
			GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];

			GlobalAnimationHeaderCAF& rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x04]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x05]) ]; 
			Vec3 vVelocity00=rGHLMG.m_arrBSAnimations[offset+0x00].m_qVelocity;
			Vec3 vVelocity01=rGHLMG.m_arrBSAnimations[offset+0x01].m_qVelocity;
			Vec3 vVelocity02=rGHLMG.m_arrBSAnimations[offset+0x02].m_qVelocity;
			Vec3 vVelocity03=rGHLMG.m_arrBSAnimations[offset+0x03].m_qVelocity;
			Vec3 vVelocity04=rGHLMG.m_arrBSAnimations[offset+0x04].m_qVelocity;
			Vec3 vVelocity05=rGHLMG.m_arrBSAnimations[offset+0x05].m_qVelocity;

			GlobalAnimationHeaderCAF& rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x06]) ]; 
			Vec3 vVelocity06=rGHLMG.m_arrBSAnimations[offset+0x06].m_qVelocity;

			GlobalAnimationHeaderCAF& rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x07]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH08 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x08]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH09 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x09]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH0a = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0a]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH0b = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0b]) ]; 
	//		GlobalAnimationHeaderCAF& rsubGAH0c = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x0c]) ]; 
			Vec3 vVelocity07=rGHLMG.m_arrBSAnimations[offset+0x07].m_qVelocity;
			Vec3 vVelocity08=rGHLMG.m_arrBSAnimations[offset+0x08].m_qVelocity;
			Vec3 vVelocity09=rGHLMG.m_arrBSAnimations[offset+0x09].m_qVelocity;
			Vec3 vVelocity0a=rGHLMG.m_arrBSAnimations[offset+0x0a].m_qVelocity;
			Vec3 vVelocity0b=rGHLMG.m_arrBSAnimations[offset+0x0b].m_qVelocity;
			Vec3 vVelocity0c=rGHLMG.m_arrBSAnimations[offset+0x0c].m_qVelocity;

			//calculate the strafe-weights
			f32	speed = lmg.m_BlendSpace.m_speed;
			f32 fmid=1-fabsf(speed);
			//	f32 ffast=0;
			//	if (speed>0) ffast=speed; else fslow=1-(speed+1);
			f32 x0	=	(lmg.m_BlendSpace.m_speed+1)*0.5f; // fast
			f32 x1	= 1.0f-(lmg.m_BlendSpace.m_speed+1)*0.5f;	//slow

			Vec2 sF		= Vec2(vVelocity00*x0 + vVelocity07*x1);
			Vec2 sFL	= Vec2(vVelocity01*x0 + vVelocity08*x1);
			Vec2 sL		= Vec2(vVelocity02*x0 + vVelocity09*x1);
			Vec2 sB		= Vec2(vVelocity03*x0 + vVelocity0a*x1);
			Vec2 sBR	= Vec2(vVelocity04*x0 + vVelocity0b*x1);
			Vec2 sR		= Vec2(vVelocity05*x0 + vVelocity0c*x1);
			StrafeWeights6 blend = GetStrafingWeights6(sF,sFL,sL, sB,sBR,sR, lmg.m_BlendSpace.m_strafe );
			f32 ts		= lmg.m_BlendSpace.m_strafe.GetLength();
			f32 ps		=	(1.0f/6.0f);
			blend.f		=	blend.f*ts		+ ps*(1.0f-ts);
			blend.fl	=	blend.fl*ts	  + ps*(1.0f-ts);
			blend.l		=	blend.l*ts		+ ps*(1.0f-ts);	
			blend.b		=	blend.b*ts		+ ps*(1.0f-ts);
			blend.br	=	blend.br*ts	  + ps*(1.0f-ts);
			blend.r		=	blend.r*ts		+ ps*(1.0f-ts);	
			StrafeWeights6 StrafeWeights=blend;

			//	f32 L_fast=rsubGAH0d.m_fSpeed;	f32 R_fast=rsubGAH0e.m_fSpeed;
			f32 F_fast=rsubGAH00.m_fSpeed;	
			f32 F_mid	=rsubGAH06.m_fSpeed;
			//	f32 L_slow=rsubGAH0f.m_fSpeed;	f32 R_slow=rsubGAH10.m_fSpeed;
			f32 F_slow=rsubGAH07.m_fSpeed;	

			//	f32 TL_fast=rsubGAH0d.m_fTurnSpeed;		f32 TF_fast=rsubGAH00.m_fTurnSpeed;	f32 TR_fast=rsubGAH0e.m_fTurnSpeed;
			//	f32 TF_mid =rsubGAH06.m_fTurnSpeed;
			//	f32 TL_slow=rsubGAH0f.m_fTurnSpeed;	f32 TF_slow=rsubGAH07.m_fTurnSpeed;	f32 TR_slow=rsubGAH10.m_fTurnSpeed;


			f32 f0 = F_mid	-F_slow;
			f32 f1 = F_fast -F_slow;
			f32 mid=f0;
			if (f1)
			  mid = (f0/f1)*2-1;

			//calculate the turn-weights
			Vec2 FL=Vec2(-1, 1);	//fast left	
			Vec2 FF=Vec2( 0, 1);	//fast speed forward	
			Vec2 FR=Vec2( 1, 1);	//fast right	

			Vec2 MF=Vec2( 0, mid);	//middle speed forward	

			Vec2 SL=Vec2(-1,-1);	//slow left
			Vec2 SF=Vec2( 0,-1);	//slow speed forward	
			Vec2 SR=Vec2( 1,-1);	//slow right
			Vec2 p = Vec2(lmg.m_BlendSpace.m_turn,lmg.m_BlendSpace.m_speed);
			LocomotionWeights7 TurnWeights = ComputeWeightLoco7( p, FL,FF,FR, MF, SL,SF,SR );

			Vec2 strafe = lmg.m_BlendSpace.m_strafe.GetNormalizedSafe( Vec2(0,0) );
			f32 turnweight = fabsf(lmg.m_BlendSpace.m_turn*2);
			if (turnweight>1.0f)
				turnweight=1.0f;

			f32 fTurnPriority=0;

			if (pAnimationSet->m_CharEditMode)
			{
				f32 rad = atan2f(strafe.x,strafe.y);
				fTurnPriority = fabsf( rad*2 ); //*turnweight;
				if (fTurnPriority>1.0f) fTurnPriority=1.0f;
				fTurnPriority=1.0f-fTurnPriority;
			}
			else
			{
				fTurnPriority = lmg.m_params[eMotionParamID_Curving].value;
			}

			fTurnPriority*=ts;
			//	fTurnPriority*=0.5f;
			fTurnPriority*=clamp( (lmg.m_BlendSpace.m_fAllowLeaningSmooth*1.4f)-0.2f, 0.0f,1.0f);

			f32 staf = 1.0f-fTurnPriority;
			f32 turn = fTurnPriority;

			st2.ff	=	staf*x0*StrafeWeights.f+turn*TurnWeights.ff;	//fast forward speed
			st2.ffl	=	staf*x0*StrafeWeights.fl;	
			st2.fl	=	staf*x0*StrafeWeights.l;	
			st2.fb	=	staf*x0*StrafeWeights.b;	
			st2.fbr	=	staf*x0*StrafeWeights.br;	
			st2.fr	=	staf*x0*StrafeWeights.r;	

			st2.mf	=	turn*TurnWeights.mf;	//middle forward speed	

			st2.sf	=	staf*x1*StrafeWeights.f+turn*TurnWeights.sf;	//slow forward speed
			st2.sfl	=	staf*x1*StrafeWeights.fl;	
			st2.sl	=	staf*x1*StrafeWeights.l;	
			st2.sb	=	staf*x1*StrafeWeights.b;	
			st2.sbr	=	staf*x1*StrafeWeights.br;	
			st2.sr	=	staf*x1*StrafeWeights.r;	

			st2.fftl=turn*TurnWeights.fl;	//left
			st2.fftr=turn*TurnWeights.fr;	//right
			st2.sftl=turn*TurnWeights.sl;	//left
			st2.sftr=turn*TurnWeights.sr;	//right

			assert( fabsf((st2.ff+st2.ffl+st2.fl+st2.fb+st2.fbr+st2.fr + st2.mf + st2.sf+st2.sfl+st2.sl+st2.sb+st2.sbr+st2.sr +(st2.fftl+st2.fftr+st2.sftl+st2.sftr))-1.0f)<0.03f );

			st2.fftl=turn*TurnWeights.fl;	//left
			st2.fftr=turn*TurnWeights.fr;	//right
			st2.sftl=turn*TurnWeights.sl;	//left
			st2.sftr=turn*TurnWeights.sr;	//right
		}

		struct BlendWeights_STX
		{
			f32      ff; 
			f32 fl,       fr; 
			f32	     fb;

			f32      mf; 

			f32      sf; 
			f32 sl,       sr; 
			f32	     sb;

			f32	fftl,fftr; //9-a
			f32	sftl,sftr; //b-c
		};

		void ComputeWeight_STX( CAnimationSet* pAnimationSet, const SParametric& lmg, uint32 offset, BlendWeights_STX& stx, GlobalAnimationHeaderLMG& rGHLMG )
		{
			GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];

		//	GlobalAnimationHeaderCAF& rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x00]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x01]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x02]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x03]) ]; 
			Vec3 vVelocity00=rGHLMG.m_arrBSAnimations[offset+0x00].m_qVelocity;
			Vec3 vVelocity01=rGHLMG.m_arrBSAnimations[offset+0x01].m_qVelocity;
			Vec3 vVelocity02=rGHLMG.m_arrBSAnimations[offset+0x02].m_qVelocity;
			Vec3 vVelocity03=rGHLMG.m_arrBSAnimations[offset+0x03].m_qVelocity;

		//	GlobalAnimationHeaderCAF& rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x05]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x06]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x07]) ]; 
		//	GlobalAnimationHeaderCAF& rsubGAH08 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[offset+0x08]) ]; 
			Vec3 vVelocity05=rGHLMG.m_arrBSAnimations[offset+0x05].m_qVelocity;
			Vec3 vVelocity06=rGHLMG.m_arrBSAnimations[offset+0x06].m_qVelocity;
			Vec3 vVelocity07=rGHLMG.m_arrBSAnimations[offset+0x07].m_qVelocity;
			Vec3 vVelocity08=rGHLMG.m_arrBSAnimations[offset+0x08].m_qVelocity;

			//calculate the strafe-weights
			f32	t = lmg.m_BlendSpace.m_speed;
			f32 ffast=0,fmid=1-fabsf(t),fslow=0;
			if (t>0) ffast=t; else fslow=1-(t+1);
			f32 x0	=	(lmg.m_BlendSpace.m_speed+1)*0.5f; // fast
			f32 x1 = 1.0f-(lmg.m_BlendSpace.m_speed+1)*0.5f;	//slow

			Vec2 sF		= Vec2(vVelocity00*x0 + vVelocity00*x1);
			Vec2 sL		= Vec2(vVelocity01*x0 + vVelocity06*x1);
			Vec2 sB		= Vec2(vVelocity02*x0 + vVelocity07*x1);
			Vec2 sR		= Vec2(vVelocity03*x0 + vVelocity08*x1);
			StrafeWeights4 blend = GetStrafingWeights4(sF,sL,sB,sR, lmg.m_BlendSpace.m_strafe );

			Vec2 dirF	=	Vec2( 0,+1); //0
			Vec2 dirL	=	Vec2(-1, 0); //2
			Vec2 dirB	=	Vec2( 0,-1); //3
			Vec2 dirR	=	Vec2(+1, 0); //5
			Vec2 movedir= dirF*blend.f +  dirL*blend.l + dirB*blend.b + dirR*blend.r;
			f32 length = lmg.m_BlendSpace.m_strafe.GetLength();
			f32 lmd = movedir.GetLength();
			if (lmd>length)	{	movedir.NormalizeSafe(Vec2(0,1)); movedir*=length;	}
			Vec4 StrafeWeights;
			ComputeWeight4(StrafeWeights,movedir, dirF,dirL,dirB,dirR);

			//calculate the turn-weights
			Vec2 FL=Vec2(-1, 1);	//fast left	
			Vec2 FF=Vec2( 0, 1);	//fast speed forward	
			Vec2 FR=Vec2( 1, 1);	//fast right	

			Vec2 MF=Vec2( 0, 0);	//middle speed forward	

			Vec2 SL=Vec2(-1,-1);	//slow left
			Vec2 SF=Vec2( 0,-1);	//slow speed forward	
			Vec2 SR=Vec2( 1,-1);	//slow right
			Vec2 p = Vec2(lmg.m_BlendSpace.m_turn,lmg.m_BlendSpace.m_speed);
			LocomotionWeights7 TurnWeights = ComputeWeightLoco7( p, FL,FF,FR, MF, SL,SF,SR );

			Vec2 strafe = lmg.m_BlendSpace.m_strafe.GetNormalizedSafe( Vec2(0,0) );

			f32 turnweight = fabsf(lmg.m_BlendSpace.m_turn);

			f32 rad = atan2f(strafe.x,strafe.y);
			f32 fTurnPriority = fabsf( rad*2 ); //*turnweight;
			if (fTurnPriority>1.0f) fTurnPriority=1.0f;

			fTurnPriority = lmg.m_params[eMotionParamID_Curving].value;

			f32 staf = fTurnPriority;
			f32 turn = 1.0f-fTurnPriority;

			f32 tw = turnweight*4;
			if (tw>1.0f)
				tw=1.0f;

			//turn*=tw; 
			staf=1.0f-turn; 

			stx.ff	=	staf*ffast*StrafeWeights.x+turn*TurnWeights.ff;	//fast forward speed
			stx.fl	=	staf*x0*   StrafeWeights.y;	
			stx.fb	=	staf*x0*   StrafeWeights.z;	
			stx.fr	=	staf*x0*   StrafeWeights.w;	

			stx.mf	=	staf*fmid* StrafeWeights.x+turn*TurnWeights.mf;		//middle forward speed	

			stx.sf	=	staf*fslow*StrafeWeights.x+turn*TurnWeights.sf;	//slow forward speed
			stx.sl	=	staf*x1*   StrafeWeights.y;	
			stx.sb	=	staf*x1*   StrafeWeights.z;	
			stx.sr	=	staf*x1*   StrafeWeights.w;	

			stx.fftl=turn*TurnWeights.fl;	//left
			stx.fftr=turn*TurnWeights.fr;	//right
			stx.sftl=turn*TurnWeights.sl;	//left
			stx.sftr=turn*TurnWeights.sr;	//right
		}

		// [artem]: refactor this !!!
		float GetParamScale(const SParametric & lmg)
		{
			const MotionParam & param = lmg.m_params[eMotionParamID_Scale];
			return param.desc.m_fMin + param.blendspace.m_fAssetBlend * (param.desc.m_fMax - param.desc.m_fMin);
		}

		void ComputeWeight_IROT(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32	t = lmg.m_BlendSpace.m_turn;
			if (t<0)
			{
				lmg.m_fBlendWeight[1]=1+t;	//middle
				lmg.m_fBlendWeight[2]=-t;		//left
			}
			else
			{
				lmg.m_fBlendWeight[0]=t;		//right
				lmg.m_fBlendWeight[1]=1-t;	//middle
			}			}

		void ComputeWeight_TSTP(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32	t = lmg.m_BlendSpace.m_turn;
			t = CLAMP(t, -1.0f, 1.0f);
			if (t<0)
			{
				lmg.m_fBlendWeight[1]=1+t;	//middle
				lmg.m_fBlendWeight[2]=-t;		//left
			}
			else
			{
				lmg.m_fBlendWeight[0]=t;    //right
				lmg.m_fBlendWeight[1]=1-t;  //middle
			}			

#ifdef _DEBUG
			f32 fTotal=0.0f;
			for (uint32 i=0; i<3; i++)
				fTotal+=lmg.m_fBlendWeight[i];
			assert( fabsf(fTotal-1.0f)<0.005f );

			uint32 nExtrapolation1=0;
			for (uint32 i=0; i<3; i++)
				nExtrapolation1 |= uint32(lmg.m_fBlendWeight[i]<-0.2f);
			assert(nExtrapolation1==0);

			uint32 nExtrapolation2=0;
			for (uint32 i=0; i<3; i++)
				nExtrapolation2 |= uint32(lmg.m_fBlendWeight[i]>1.2f);
			assert(nExtrapolation2==0);
#endif

		}

		void ComputeWeight_M2I1(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			lmg.m_fBlendWeight[0]=1.0f;
		}

		IdleStepWeights6 GetIdle2MoveWeights6(  Vec2 DesiredDirection  );
		void ComputeWeight_I2M1(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32 length = lmg.m_BlendSpace.m_strafe.GetLength();
			Vec2 mdir = lmg.m_BlendSpace.m_strafe.GetNormalizedSafe(); //forward-right	
			IdleStepWeights6 I2M = GetIdle2MoveWeights6(mdir); 	
			lmg.m_fBlendWeight[0]=I2M.fr;
			lmg.m_fBlendWeight[1]=I2M.rr;
			lmg.m_fBlendWeight[2]=I2M.br;
			lmg.m_fBlendWeight[3]=I2M.fl;
			lmg.m_fBlendWeight[4]=I2M.ll;
			lmg.m_fBlendWeight[5]=I2M.bl;	
		}

		void ComputeWeight_I2M2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			Vec2 mdir = lmg.m_BlendSpace.m_strafe.GetNormalizedSafe(Vec2(0,1)); //forward-right
			IdleStepWeights6 I2M = GetIdle2MoveWeights6(mdir); 

			f32 t0=1.0f-(lmg.m_BlendSpace.m_speed+1)*0.5f;
			f32 t1=(lmg.m_BlendSpace.m_speed+1)*0.5f;

			// Force fastest speed
			t1=1; t0=0;

			lmg.m_fBlendWeight[ 0]=I2M.fr*t0;
			lmg.m_fBlendWeight[ 1]=I2M.rr*t0;
			lmg.m_fBlendWeight[ 2]=I2M.br*t0;
			lmg.m_fBlendWeight[ 3]=I2M.fl*t0;
			lmg.m_fBlendWeight[ 4]=I2M.ll*t0;
			lmg.m_fBlendWeight[ 5]=I2M.bl*t0;

			lmg.m_fBlendWeight[ 6]=I2M.fr*t1;
			lmg.m_fBlendWeight[ 7]=I2M.rr*t1;
			lmg.m_fBlendWeight[ 8]=I2M.br*t1;
			lmg.m_fBlendWeight[ 9]=I2M.fl*t1;
			lmg.m_fBlendWeight[10]=I2M.ll*t1;
			lmg.m_fBlendWeight[11]=I2M.bl*t1;
		}

		void ComputeWeight_XIM2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32 rad = lmg.m_BlendSpace.m_turn*gf_PI;
			Vec2 mdir = Vec2(cry_sinf(rad),cry_cosf(rad));
			IdleStepWeights6 I2M = GetIdle2MoveWeights6(mdir); 

			f32 t0=1.0f-(lmg.m_BlendSpace.m_speed+1)*0.5f;
			f32 t1=(lmg.m_BlendSpace.m_speed+1)*0.5f;

			lmg.m_fBlendWeight[ 0]=I2M.fr*t0;
			lmg.m_fBlendWeight[ 1]=I2M.rr*t0;
			lmg.m_fBlendWeight[ 2]=I2M.br*t0;
			lmg.m_fBlendWeight[ 3]=I2M.fl*t0;
			lmg.m_fBlendWeight[ 4]=I2M.ll*t0;
			lmg.m_fBlendWeight[ 5]=I2M.bl*t0;

			lmg.m_fBlendWeight[ 6]=I2M.fr*t1;
			lmg.m_fBlendWeight[ 7]=I2M.rr*t1;
			lmg.m_fBlendWeight[ 8]=I2M.br*t1;
			lmg.m_fBlendWeight[ 9]=I2M.fl*t1;
			lmg.m_fBlendWeight[10]=I2M.ll*t1;
			lmg.m_fBlendWeight[11]=I2M.bl*t1;
		}

		IdleStepWeights6 GetIdleStepWeights6(   Vec2 FR,Vec2 RR,Vec2 BR,   Vec2 FL,Vec2 LL,Vec2 BL,  Vec2 DesiredDirection  );
		void ComputeWeight_ISTP(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];
			Vec3 vVelocity00=rGHLMG.m_arrBSAnimations[0x00].m_qVelocity;
			Vec3 vVelocity01=rGHLMG.m_arrBSAnimations[0x01].m_qVelocity;
			Vec3 vVelocity02=rGHLMG.m_arrBSAnimations[0x02].m_qVelocity;
			Vec3 vVelocity03=rGHLMG.m_arrBSAnimations[0x03].m_qVelocity;
			Vec3 vVelocity04=rGHLMG.m_arrBSAnimations[0x04].m_qVelocity;
			Vec3 vVelocity05=rGHLMG.m_arrBSAnimations[0x05].m_qVelocity;

			f32 length = lmg.m_BlendSpace.m_strafe.GetLength();
			if (length>1.0f) length=1.0f;

			Vec2 sFR = Vec2(vVelocity00);
			Vec2 sRR = Vec2(vVelocity01);
			Vec2 sBR = Vec2(vVelocity02);
			Vec2 sFL = Vec2(vVelocity03);
			Vec2 sRL = Vec2(vVelocity04);
			Vec2 sBL = Vec2(vVelocity05);
			IdleStepWeights6 blend = GetIdleStepWeights6(sFR,sRR,sBR, sFL,sRL,sBL, lmg.m_BlendSpace.m_strafe.GetNormalizedSafe() );

			lmg.m_fBlendWeight[0]=blend.fr*length;	
			lmg.m_fBlendWeight[1]=blend.rr*length;	
			lmg.m_fBlendWeight[2]=blend.br*length;	
			lmg.m_fBlendWeight[3]=blend.fl*length;	
			lmg.m_fBlendWeight[4]=blend.ll*length;	
			lmg.m_fBlendWeight[5]=blend.bl*length;	
			lmg.m_fBlendWeight[6]=1.0f-length;

#ifdef _DEBUG
			f32 fTotal=0.0f;
			for (uint32 i=0; i<7; i++)
				fTotal+=lmg.m_fBlendWeight[i];
			assert( fabsf(fTotal-1.0f)<0.005f );

			uint32 nExtrapolation1=0;
			for (uint32 i=0; i<7; i++)
				nExtrapolation1 |= uint32(lmg.m_fBlendWeight[i]<-0.2f);
			assert(nExtrapolation1==0);

			uint32 nExtrapolation2=0;
			for (uint32 i=0; i<7; i++)
				nExtrapolation2 |= uint32(lmg.m_fBlendWeight[i]>1.2f);
			assert(nExtrapolation2==0);
#endif
		}

		// [artemk] ISTP for tactical move
		void ComputeWeight_T_IS(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			f32 length = lmg.m_BlendSpace.m_strafe.GetLength();
			if (length>1.0f) length=1.0f;

			Vec2 sFR = Vec2(rsubGAH00.m_vVelocity);
			Vec2 sRR = Vec2(rsubGAH01.m_vVelocity);
			Vec2 sBR = Vec2(rsubGAH02.m_vVelocity);
			Vec2 sFL = Vec2(rsubGAH03.m_vVelocity);
			Vec2 sRL = Vec2(rsubGAH04.m_vVelocity);
			Vec2 sBL = Vec2(rsubGAH05.m_vVelocity);
			IdleStepWeights6 blend = GetIdleStepWeights6(sFR,sRR,sBR, sFL,sRL,sBL, lmg.m_BlendSpace.m_strafe.GetNormalizedSafe() );

			lmg.m_fBlendWeight[0]=blend.fr*length;	
			lmg.m_fBlendWeight[1]=blend.rr*length;	
			lmg.m_fBlendWeight[2]=blend.br*length;	
			lmg.m_fBlendWeight[3]=blend.fl*length;	
			lmg.m_fBlendWeight[4]=blend.ll*length;	
			lmg.m_fBlendWeight[5]=blend.bl*length;	
			lmg.m_fBlendWeight[6]=1.0f-length;

			float scale;
			if (!pAnimationSet->m_CharEditMode)
			{
				scale = GetParamScale(lmg);
			}
			else
			{
				scale = 0.9f + (lmg.m_BlendSpace.m_speed * 0.5f + 0.5f) * 0.2f;
				scale = clamp(scale, 0.9f, 1.1f);
			}

			if (scale > 1.0f /*test stuff*/ /*&& gEnv->pConsole->GetCVar("ca_testInt")->GetIVal()*/)
			{
				float t = (scale - 1.0f) / (1.1f - 1.0f);
				t = clamp(t, 0.0f, 1.0f);

				for (int i = 0; i < 7; i++)
					lmg.m_fBlendWeight[i+7] = lmg.m_fBlendWeight[i] * t;

				for (int i = 0; i < 7; i++)
					lmg.m_fBlendWeight[i] *= (1.0f-t);
			}
		}

		// [artemk] Tactical idle aiming lmg
		void ComputeWeight_T_IA(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			float scale;
			if (!pAnimationSet->m_CharEditMode)
			{
				scale = GetParamScale(lmg);
			}
			else
			{
				scale = 0.9f + (lmg.m_BlendSpace.m_speed * 0.5f + 0.5f) * 0.2f;
				scale = clamp(scale, 0.9f, 1.1f);
			}

			if (scale <= 1.0f)
			{
				lmg.m_fBlendWeight[0] = 1.0f;
				lmg.m_fBlendWeight[1] = 0.0f;
			}
			else
			{
				float t = (scale - 1.0f) / (1.1f - 1.0f);
				t = clamp(t, 0.0f, 1.0f);
				lmg.m_fBlendWeight[0] = 1.0f - t;
				lmg.m_fBlendWeight[1] = t;
			}
		}

		void ComputeWeight_TRN1(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32	t = lmg.m_BlendSpace.m_turn*1;
			if (t<0)
			{
				if (t<-0.5f)
				{
					t=t+0.5f;
					lmg.m_fBlendWeight[4]=-t*2;		//left 90
					lmg.m_fBlendWeight[3]=1.0f-lmg.m_fBlendWeight[4];	//middle
				}
				else
				{
					lmg.m_fBlendWeight[3]=-t*2;		//left 90
					lmg.m_fBlendWeight[2]=1.0f-lmg.m_fBlendWeight[3];	//middle
				}
			}
			else
			{
				if (t<0.5f)
				{
					lmg.m_fBlendWeight[1]=t*2;    //right 90
					lmg.m_fBlendWeight[2]=1-t*2;  //middle
				}
				else
				{
					t=t-0.5f;
					lmg.m_fBlendWeight[0]=t*2;    //right 180
					lmg.m_fBlendWeight[1]=1-t*2;  //left 90
				}
			}			

#if !defined(__SPU__)
			f32 fColor[4] = {1,1,0,1};
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"r180: %f", lmg.m_fBlendWeight[0] ); g_YLine+=0x0d;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"r090: %f", lmg.m_fBlendWeight[1] ); g_YLine+=0x0d;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"f00:  %f", lmg.m_fBlendWeight[2] ); g_YLine+=0x0d;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"l090: %f", lmg.m_fBlendWeight[3] ); g_YLine+=0x0d;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"l180: %f", lmg.m_fBlendWeight[4] ); g_YLine+=0x0d;
#endif
		}

		void ComputeWeight_STF1(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			//GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			//GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			//GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			//GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			Vec3 vVelocity00=rGHLMG.m_arrBSAnimations[0x00].m_qVelocity;
			Vec3 vVelocity01=rGHLMG.m_arrBSAnimations[0x01].m_qVelocity;
			Vec3 vVelocity02=rGHLMG.m_arrBSAnimations[0x02].m_qVelocity;
			Vec3 vVelocity03=rGHLMG.m_arrBSAnimations[0x03].m_qVelocity;

			Vec2 sF = Vec2(vVelocity00);
			Vec2 sL = Vec2(vVelocity01);
			Vec2 sB = Vec2(vVelocity02);
			Vec2 sR = Vec2(vVelocity03);
			StrafeWeights4 blend = GetStrafingWeights4(sF,sL,sB,sR, lmg.m_BlendSpace.m_strafe);

			f32 t = lmg.m_BlendSpace.m_strafe.GetLength();

			lmg.m_fBlendWeight[0]=blend.f*t + 0.25f*(1.0f-t);
			lmg.m_fBlendWeight[1]=blend.l*t + 0.25f*(1.0f-t);	
			lmg.m_fBlendWeight[2]=blend.b*t + 0.25f*(1.0f-t);
			lmg.m_fBlendWeight[3]=blend.r*t + 0.25f*(1.0f-t);	
		}

		void ComputeWeight_STF2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			uint32 poffset=0;
			BlendWeights_STF2 stf2;	ComputeWeight_STF2( pAnimationSet, lmg,poffset, stf2, rGHLMG );

			lmg.m_fBlendWeight[0+0]=stf2.ff;	
			lmg.m_fBlendWeight[1+0]=stf2.fl;	
			lmg.m_fBlendWeight[2+0]=stf2.fb;
			lmg.m_fBlendWeight[3+0]=stf2.fr;	

			lmg.m_fBlendWeight[0+4]=stf2.sf;	
			lmg.m_fBlendWeight[1+4]=stf2.sl;	
			lmg.m_fBlendWeight[2+4]=stf2.sb;
			lmg.m_fBlendWeight[3+4]=stf2.sr;	
		}

		void ComputeWeight_SUD2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			uint32 uoffset=0;
			BlendWeights_STF2 ustf2;	ComputeWeight_STF2( pAnimationSet, lmg,uoffset, ustf2, rGHLMG );
			uint32 poffset=8;
			BlendWeights_STF2 pstf2;	ComputeWeight_STF2( pAnimationSet, lmg,poffset, pstf2, rGHLMG );
			uint32 doffset=16;
			BlendWeights_STF2 dstf2;	ComputeWeight_STF2( pAnimationSet, lmg,doffset, dstf2, rGHLMG );

			f32 uh=0,fw=0,dh=0;
			f32	q = lmg.m_BlendSpace.m_slope;
			if (q>0) { uh=q;	fw=1-q;	}
			else { fw=1+q;	dh=1-(q+1);	}

			lmg.m_fBlendWeight[0+0x00]=ustf2.ff*uh;	
			lmg.m_fBlendWeight[1+0x00]=ustf2.fl*uh;	
			lmg.m_fBlendWeight[2+0x00]=ustf2.fb*uh;
			lmg.m_fBlendWeight[3+0x00]=ustf2.fr*uh;	
			lmg.m_fBlendWeight[4+0x00]=ustf2.sf*uh;	
			lmg.m_fBlendWeight[5+0x00]=ustf2.sl*uh;	
			lmg.m_fBlendWeight[6+0x00]=ustf2.sb*uh;
			lmg.m_fBlendWeight[7+0x00]=ustf2.sr*uh;	

			lmg.m_fBlendWeight[0+0x08]=pstf2.ff*fw;	
			lmg.m_fBlendWeight[1+0x08]=pstf2.fl*fw;	
			lmg.m_fBlendWeight[2+0x08]=pstf2.fb*fw;
			lmg.m_fBlendWeight[3+0x08]=pstf2.fr*fw;	
			lmg.m_fBlendWeight[4+0x08]=pstf2.sf*fw;	
			lmg.m_fBlendWeight[5+0x08]=pstf2.sl*fw;	
			lmg.m_fBlendWeight[6+0x08]=pstf2.sb*fw;
			lmg.m_fBlendWeight[7+0x08]=pstf2.sr*fw;	

			lmg.m_fBlendWeight[0+0x10]=dstf2.ff*dh;	
			lmg.m_fBlendWeight[1+0x10]=dstf2.fl*dh;	
			lmg.m_fBlendWeight[2+0x10]=dstf2.fb*dh;
			lmg.m_fBlendWeight[3+0x10]=dstf2.fr*dh;	
			lmg.m_fBlendWeight[4+0x10]=dstf2.sf*dh;	
			lmg.m_fBlendWeight[5+0x10]=dstf2.sl*dh;	
			lmg.m_fBlendWeight[6+0x10]=dstf2.sb*dh;
			lmg.m_fBlendWeight[7+0x10]=dstf2.sr*dh;	
		}

		void ComputeWeight_S__1(SParametric& lmg, CAnimationSet* pAnimationSet, GlobalAnimationHeaderCAF* parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			Vec2 sF		= Vec2(rsubGAH00.m_vVelocity);
			Vec2 sFL	= Vec2(rsubGAH01.m_vVelocity);
			Vec2 sL		= Vec2(rsubGAH02.m_vVelocity);
			Vec2 sB		= Vec2(rsubGAH03.m_vVelocity);
			Vec2 sBR	= Vec2(rsubGAH04.m_vVelocity);
			Vec2 sR		= Vec2(rsubGAH05.m_vVelocity);
			StrafeWeights6 blend = GetStrafingWeights6(sF,sFL,sL, sB,sBR,sR, lmg.m_BlendSpace.m_strafe );

			f32 t = lmg.m_BlendSpace.m_strafe.GetLength();
			f32 p =	(1.0f/6.0f);
			blend.f		=	blend.f*t		+ p*(1.0f-t);
			blend.fl	=	blend.fl*t	+ p*(1.0f-t);
			blend.l		=	blend.l*t		+ p*(1.0f-t);	
			blend.b		=	blend.b*t		+ p*(1.0f-t);
			blend.br	=	blend.br*t	+ p*(1.0f-t);
			blend.r		=	blend.r*t		+ p*(1.0f-t);	

			lmg.m_fBlendWeight[0] = blend.f;	
			lmg.m_fBlendWeight[1] = blend.fl;	
			lmg.m_fBlendWeight[2] = blend.l;	
			lmg.m_fBlendWeight[3] = blend.b;	
			lmg.m_fBlendWeight[4] = blend.br;	
			lmg.m_fBlendWeight[5] = blend.r;	
		}


		void ComputeWeight_M__1(SParametric& lmg, CAnimationSet* pAnimationSet, GlobalAnimationHeaderCAF* parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];

			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];
			GlobalAnimationHeaderType rsubGAH06 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x06]) ];
			GlobalAnimationHeaderType rsubGAH07 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x07]) ];

			Vec2 sF		= Vec2(rsubGAH00.m_vVelocity);
			Vec2 sFL	= Vec2(rsubGAH01.m_vVelocity);
			Vec2 sL		= Vec2(rsubGAH02.m_vVelocity);
			Vec2 sLB	= Vec2(rsubGAH03.m_vVelocity);
			Vec2 sB		= Vec2(rsubGAH04.m_vVelocity);
			Vec2 sBR	= Vec2(rsubGAH05.m_vVelocity);
			Vec2 sR		= Vec2(rsubGAH06.m_vVelocity);
			Vec2 sRF	= Vec2(rsubGAH07.m_vVelocity);

			StrafeWeights8 blend = GetStrafingWeights8(sF,sFL,sL,sLB, sB,sBR,sR,sRF, lmg.m_BlendSpace.m_strafe );

			f32 t = lmg.m_BlendSpace.m_strafe.GetLength();
			f32 p =	(1.0f/8.0f);
			blend.f		=	blend.f*t		+ p*(1.0f-t);
			blend.fl	=	blend.fl*t	+ p*(1.0f-t);
			blend.l		=	blend.l*t		+ p*(1.0f-t);	
			blend.lb	=	blend.lb*t	+ p*(1.0f-t);	
			blend.b		=	blend.b*t		+ p*(1.0f-t);
			blend.br	=	blend.br*t	+ p*(1.0f-t);
			blend.r		=	blend.r*t		+ p*(1.0f-t);	
			blend.rf	=	blend.rf*t	+ p*(1.0f-t);	

			lmg.m_fBlendWeight[0] = blend.f;	
			lmg.m_fBlendWeight[1] = blend.fl;	
			lmg.m_fBlendWeight[2] = blend.l;	
			lmg.m_fBlendWeight[3] = blend.lb;	
			lmg.m_fBlendWeight[4] = blend.b;	
			lmg.m_fBlendWeight[5] = blend.br;	
			lmg.m_fBlendWeight[6] = blend.r;	
			lmg.m_fBlendWeight[7] = blend.rf;	
		}



		void ComputeWeight_T_WS(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			GlobalAnimationHeaderType rsubGAH00 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x00]) ];
			GlobalAnimationHeaderType rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x01]) ];
			GlobalAnimationHeaderType rsubGAH02 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x02]) ];
			GlobalAnimationHeaderType rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x03]) ];
			GlobalAnimationHeaderType rsubGAH04 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x04]) ];
			GlobalAnimationHeaderType rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[0x05]) ];

			Vec2 sF		= Vec2(rsubGAH00.m_vVelocity);
			Vec2 sFL	= Vec2(rsubGAH01.m_vVelocity);
			Vec2 sL		= Vec2(rsubGAH02.m_vVelocity);
			Vec2 sB		= Vec2(rsubGAH03.m_vVelocity);
			Vec2 sBR	= Vec2(rsubGAH04.m_vVelocity);
			Vec2 sR		= Vec2(rsubGAH05.m_vVelocity);
			StrafeWeights6 blend = GetStrafingWeights6(sF,sFL,sL, sB,sBR,sR, lmg.m_BlendSpace.m_strafe );

			f32 l = lmg.m_BlendSpace.m_strafe.GetLength();
			f32 p =	(1.0f/6.0f);
			blend.f		=	blend.f*l		+ p*(1.0f-l);
			blend.fl	=	blend.fl*l	+ p*(1.0f-l);
			blend.l		=	blend.l*l		+ p*(1.0f-l);	
			blend.b		=	blend.b*l		+ p*(1.0f-l);
			blend.br	=	blend.br*l	+ p*(1.0f-l);
			blend.r		=	blend.r*l		+ p*(1.0f-l);	

			lmg.m_fBlendWeight[0] = blend.f;	
			lmg.m_fBlendWeight[1] = blend.fl;	
			lmg.m_fBlendWeight[2] = blend.l;	
			lmg.m_fBlendWeight[3] = blend.b;	
			lmg.m_fBlendWeight[4] = blend.br;
			lmg.m_fBlendWeight[5] = blend.r;

			float scale;
			if (!pAnimationSet->m_CharEditMode)
			{
				scale = GetParamScale(lmg);
			}
			else
			{
				scale = 0.9f + (lmg.m_BlendSpace.m_speed * 0.5f + 0.5f) * 0.2f;
				scale = clamp(scale, 0.9f, 1.1f);
			}

			if (scale > 1.0f /*test stuff*/ /*&& gEnv->pConsole->GetCVar("ca_testInt")->GetIVal()*/)
			{
				float t = (scale - 1.0f) / (1.1f - 1.0f);
				t = clamp(t, 0.0f, 1.0f);

				for (int i = 0; i < 6; i++)
					lmg.m_fBlendWeight[i+6] = lmg.m_fBlendWeight[i] * t;

				for (int i = 0; i < 6; i++)
					lmg.m_fBlendWeight[i] *= (1.0f-t);
			}
		}

		void ComputeWeight_S__2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			uint32 poffset=0;
			BlendWeights_S2 s2;	ComputeWeight_S2( pAnimationSet, lmg,poffset, s2, rGHLMG  );

			lmg.m_fBlendWeight[0+0]=s2.ff;	
			lmg.m_fBlendWeight[1+0]=s2.ffl;	
			lmg.m_fBlendWeight[2+0]=s2.fl;	
			lmg.m_fBlendWeight[3+0]=s2.fb;
			lmg.m_fBlendWeight[4+0]=s2.fbr;
			lmg.m_fBlendWeight[5+0]=s2.fr;	

			lmg.m_fBlendWeight[0+6]=s2.sf;	
			lmg.m_fBlendWeight[1+6]=s2.sfl;	
			lmg.m_fBlendWeight[2+6]=s2.sl;	
			lmg.m_fBlendWeight[3+6]=s2.sb;
			lmg.m_fBlendWeight[4+6]=s2.sbr;
			lmg.m_fBlendWeight[5+6]=s2.sr;	
		}

		void ComputeWeight_ST_2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{	
			uint32 poffset=0;
			BlendWeights_ST2 st2;	ComputeWeight_ST2( pAnimationSet, lmg,poffset, st2, rGHLMG );

			lmg.m_fBlendWeight[poffset+0x00]=st2.ff;	//fast forward speed
			lmg.m_fBlendWeight[poffset+0x01]=st2.ffl;	
			lmg.m_fBlendWeight[poffset+0x02]=st2.fl;	
			lmg.m_fBlendWeight[poffset+0x03]=st2.fb;	
			lmg.m_fBlendWeight[poffset+0x04]=st2.fbr;	
			lmg.m_fBlendWeight[poffset+0x05]=st2.fr;	

			lmg.m_fBlendWeight[poffset+0x06]=st2.mf;		//middle forward speed	

			lmg.m_fBlendWeight[poffset+0x07]=st2.sf;	//slow forward speed
			lmg.m_fBlendWeight[poffset+0x08]=st2.sfl;	
			lmg.m_fBlendWeight[poffset+0x09]=st2.sl;	
			lmg.m_fBlendWeight[poffset+0x0a]=st2.sb;	
			lmg.m_fBlendWeight[poffset+0x0b]=st2.sbr;	
			lmg.m_fBlendWeight[poffset+0x0c]=st2.sr;	

			lmg.m_fBlendWeight[poffset+0x0d]=st2.fftl;	//left
			lmg.m_fBlendWeight[poffset+0x0e]=st2.fftr;	//right
			lmg.m_fBlendWeight[poffset+0x0f]=st2.sftl;	//left
			lmg.m_fBlendWeight[poffset+0x10]=st2.sftr;	//right
		}

		void ComputeWeight_S_H2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{

			f32 uh=0,fw=0,dh=0;
			f32	q = lmg.m_BlendSpace.m_slope;
			if (q>0) { uh=q;	fw=1-q;	}
			else { fw=1+q;	dh=1-(q+1);	}

			uint32 uoffset=0;
			BlendWeights_STF2 stu2;	ComputeWeight_STF2( pAnimationSet, lmg,uoffset, stu2, rGHLMG );
			lmg.m_fBlendWeight[uoffset+0x00]=stu2.ff*uh;	
			lmg.m_fBlendWeight[uoffset+0x01]=stu2.fl*uh;	
			lmg.m_fBlendWeight[uoffset+0x02]=stu2.fb*uh;	
			lmg.m_fBlendWeight[uoffset+0x03]=stu2.fr*uh;	
			lmg.m_fBlendWeight[uoffset+0x04]=stu2.sf*uh;	
			lmg.m_fBlendWeight[uoffset+0x05]=stu2.sl*uh;	
			lmg.m_fBlendWeight[uoffset+0x06]=stu2.sb*uh;	
			lmg.m_fBlendWeight[uoffset+0x07]=stu2.sr*uh;	

			uint32 poffset=8;
			BlendWeights_S2 s2;	ComputeWeight_S2( pAnimationSet, lmg,poffset, s2, rGHLMG );
			lmg.m_fBlendWeight[poffset+0x00]=s2.ff*fw;	//fast forward speed
			lmg.m_fBlendWeight[poffset+0x01]=s2.ffl*fw;	
			lmg.m_fBlendWeight[poffset+0x02]=s2.fl*fw;	
			lmg.m_fBlendWeight[poffset+0x03]=s2.fb*fw;	
			lmg.m_fBlendWeight[poffset+0x04]=s2.fbr*fw;	
			lmg.m_fBlendWeight[poffset+0x05]=s2.fr*fw;	

			lmg.m_fBlendWeight[poffset+0x06]=s2.sf*fw;	//slow forward speed
			lmg.m_fBlendWeight[poffset+0x07]=s2.sfl*fw;	
			lmg.m_fBlendWeight[poffset+0x08]=s2.sl*fw;	
			lmg.m_fBlendWeight[poffset+0x09]=s2.sb*fw;	
			lmg.m_fBlendWeight[poffset+0x0a]=s2.sbr*fw;	
			lmg.m_fBlendWeight[poffset+0x0b]=s2.sr*fw;	

			uint32 doffset=0x08+0x0c;
			BlendWeights_STF2 std2;	ComputeWeight_STF2( pAnimationSet, lmg,doffset, std2,rGHLMG);
			lmg.m_fBlendWeight[doffset+0x00]=std2.ff*dh;	
			lmg.m_fBlendWeight[doffset+0x01]=std2.fl*dh;	
			lmg.m_fBlendWeight[doffset+0x02]=std2.fb*dh;	
			lmg.m_fBlendWeight[doffset+0x03]=std2.fr*dh;	
			lmg.m_fBlendWeight[doffset+0x04]=std2.sf*dh;	
			lmg.m_fBlendWeight[doffset+0x05]=std2.sl*dh;	
			lmg.m_fBlendWeight[doffset+0x06]=std2.sb*dh;	
			lmg.m_fBlendWeight[doffset+0x07]=std2.sr*dh;	
		}

		void ComputeWeight_STH2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{

			f32 uh=0,fw=0,dh=0;
			f32	q = lmg.m_BlendSpace.m_slope;
			if (q>0) { uh=q;	fw=1-q;	}
			else { fw=1+q;	dh=1-(q+1);	}

			uint32 uoffset=0;
			BlendWeights_STF2 stu2;	ComputeWeight_STF2( pAnimationSet, lmg,uoffset, stu2, rGHLMG );
			lmg.m_fBlendWeight[uoffset+0x00]=stu2.ff*uh;	
			lmg.m_fBlendWeight[uoffset+0x01]=stu2.fl*uh;	
			lmg.m_fBlendWeight[uoffset+0x02]=stu2.fb*uh;	
			lmg.m_fBlendWeight[uoffset+0x03]=stu2.fr*uh;	

			lmg.m_fBlendWeight[uoffset+0x04]=stu2.sf*uh;	
			lmg.m_fBlendWeight[uoffset+0x05]=stu2.sl*uh;	
			lmg.m_fBlendWeight[uoffset+0x06]=stu2.sb*uh;	
			lmg.m_fBlendWeight[uoffset+0x07]=stu2.sr*uh;	

			uint32 poffset=8;
			BlendWeights_ST2 st2;	ComputeWeight_ST2( pAnimationSet, lmg,poffset, st2, rGHLMG  );

			lmg.m_fBlendWeight[poffset+0x00]=st2.ff*fw;	//fast forward speed
			lmg.m_fBlendWeight[poffset+0x01]=st2.ffl*fw;	
			lmg.m_fBlendWeight[poffset+0x02]=st2.fl*fw;	
			lmg.m_fBlendWeight[poffset+0x03]=st2.fb*fw;	
			lmg.m_fBlendWeight[poffset+0x04]=st2.fbr*fw;	
			lmg.m_fBlendWeight[poffset+0x05]=st2.fr*fw;	

			lmg.m_fBlendWeight[poffset+0x06]=st2.mf*fw;		//middle forward speed	

			lmg.m_fBlendWeight[poffset+0x07]=st2.sf*fw;	//slow forward speed
			lmg.m_fBlendWeight[poffset+0x08]=st2.sfl*fw;	
			lmg.m_fBlendWeight[poffset+0x09]=st2.sl*fw;	
			lmg.m_fBlendWeight[poffset+0x0a]=st2.sb*fw;	
			lmg.m_fBlendWeight[poffset+0x0b]=st2.sbr*fw;	
			lmg.m_fBlendWeight[poffset+0x0c]=st2.sr*fw;	

			lmg.m_fBlendWeight[poffset+0x0d]=st2.fftl*fw;	//left
			lmg.m_fBlendWeight[poffset+0x0e]=st2.fftr*fw;	//right
			lmg.m_fBlendWeight[poffset+0x0f]=st2.sftl*fw;	//left
			lmg.m_fBlendWeight[poffset+0x10]=st2.sftr*fw;	//right

			uint32 doffset=0x08+0x11;
			BlendWeights_STF2 std2;	ComputeWeight_STF2( pAnimationSet, lmg,doffset, std2, rGHLMG  );
			lmg.m_fBlendWeight[doffset+0x00]=std2.ff*dh;	
			lmg.m_fBlendWeight[doffset+0x01]=std2.fl*dh;	
			lmg.m_fBlendWeight[doffset+0x02]=std2.fb*dh;	
			lmg.m_fBlendWeight[doffset+0x03]=std2.fr*dh;	

			lmg.m_fBlendWeight[doffset+0x04]=std2.sf*dh;	
			lmg.m_fBlendWeight[doffset+0x05]=std2.sl*dh;	
			lmg.m_fBlendWeight[doffset+0x06]=std2.sb*dh;	
			lmg.m_fBlendWeight[doffset+0x07]=std2.sr*dh;	
		}

		SPU_NO_INLINE void ComputeWeight_STHX(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF* parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{

			f32 uh=0,fw=0,dh=0;
			f32	q = lmg.m_BlendSpace.m_slope;
			if (q>0) { uh=q;	fw=1-q;	}
			else { fw=1+q;	dh=1-(q+1);	}

			uint32 uoffset=0;
			BlendWeights_STF2 stu2;	ComputeWeight_STF2( pAnimationSet, lmg,uoffset, stu2, rGHLMG );
			lmg.m_fBlendWeight[uoffset+0x00]=stu2.ff*uh;	
			lmg.m_fBlendWeight[uoffset+0x01]=stu2.fl*uh;	
			lmg.m_fBlendWeight[uoffset+0x02]=stu2.fb*uh;	
			lmg.m_fBlendWeight[uoffset+0x03]=stu2.fr*uh;	

			lmg.m_fBlendWeight[uoffset+0x04]=stu2.sf*uh;	
			lmg.m_fBlendWeight[uoffset+0x05]=stu2.sl*uh;	
			lmg.m_fBlendWeight[uoffset+0x06]=stu2.sb*uh;	
			lmg.m_fBlendWeight[uoffset+0x07]=stu2.sr*uh;	

			uint32 poffset=8;
			BlendWeights_STX stx;	ComputeWeight_STX( pAnimationSet, lmg,poffset, stx, rGHLMG );

			lmg.m_fBlendWeight[poffset+0x00]=stx.ff*fw;	//fast forward speed
			lmg.m_fBlendWeight[poffset+0x01]=stx.fl*fw;	
			lmg.m_fBlendWeight[poffset+0x02]=stx.fb*fw;	
			lmg.m_fBlendWeight[poffset+0x03]=stx.fr*fw;	

			lmg.m_fBlendWeight[poffset+0x04]=stx.mf*fw;		//middle forward speed	

			lmg.m_fBlendWeight[poffset+0x05]=stx.sf*fw;	//slow forward speed
			lmg.m_fBlendWeight[poffset+0x06]=stx.sl*fw;	
			lmg.m_fBlendWeight[poffset+0x07]=stx.sb*fw;	
			lmg.m_fBlendWeight[poffset+0x08]=stx.sr*fw;	

			lmg.m_fBlendWeight[poffset+0x09]=stx.fftl*fw;	//left
			lmg.m_fBlendWeight[poffset+0x0a]=stx.fftr*fw;	//right
			lmg.m_fBlendWeight[poffset+0x0b]=stx.sftl*fw;	//left
			lmg.m_fBlendWeight[poffset+0x0c]=stx.sftr*fw;	//right

			uint32 doffset=0x08+0x0d;
			BlendWeights_STF2 std2;	ComputeWeight_STF2( pAnimationSet, lmg,doffset, std2, rGHLMG   );
			lmg.m_fBlendWeight[doffset+0x00]=std2.ff*dh;	
			lmg.m_fBlendWeight[doffset+0x01]=std2.fl*dh;	
			lmg.m_fBlendWeight[doffset+0x02]=std2.fb*dh;	
			lmg.m_fBlendWeight[doffset+0x03]=std2.fr*dh;	

			lmg.m_fBlendWeight[doffset+0x04]=std2.sf*dh;	
			lmg.m_fBlendWeight[doffset+0x05]=std2.sl*dh;	
			lmg.m_fBlendWeight[doffset+0x06]=std2.sb*dh;	
			lmg.m_fBlendWeight[doffset+0x07]=std2.sr*dh;	
		}

		void ComputeWeight_WFW2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32	SpeedWeight = (lmg.m_BlendSpace.m_speed+1)*0.5f;
			lmg.m_fBlendWeight[0]=1-SpeedWeight;
			lmg.m_fBlendWeight[1]=SpeedWeight;
		}

		void ComputeWeight_RFW3(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32	t = lmg.m_BlendSpace.m_speed;
			if (t<0)
			{
				lmg.m_fBlendWeight[0]=1-(t+1);
				lmg.m_fBlendWeight[1]=t+1;
			}
			else
			{
				lmg.m_fBlendWeight[0]=0;
				lmg.m_fBlendWeight[1]=(1-t);
			}
		}

		void ComputeWeight_FLR1(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			f32	t = lmg.m_BlendSpace.m_turn;
			if (t<0)
			{
				lmg.m_fBlendWeight[0]=-t;   //left
				lmg.m_fBlendWeight[1]=1+t;  //middle
			}
			else
			{
				lmg.m_fBlendWeight[1]=1-t;	//middle
				lmg.m_fBlendWeight[2]=t;		//right
			}			
		}

		void ComputeWeight_FLR2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF* parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];

			Vec2 FL=Vec2(rGlobalAnimHeader.m_arrBSAnimations[0].m_Position);	//fast	
			Vec2 FF=Vec2(rGlobalAnimHeader.m_arrBSAnimations[1].m_Position);	//fast	
			Vec2 FR=Vec2(rGlobalAnimHeader.m_arrBSAnimations[2].m_Position);	//fast	
			Vec2 SL=Vec2(rGlobalAnimHeader.m_arrBSAnimations[3].m_Position);	//slow	
			Vec2 SF=Vec2(rGlobalAnimHeader.m_arrBSAnimations[4].m_Position);	//slow	
			Vec2 SR=Vec2(rGlobalAnimHeader.m_arrBSAnimations[5].m_Position);	//slow	

			Vec2 p = Vec2(lmg.m_BlendSpace.m_turn,lmg.m_BlendSpace.m_speed);
			LocomotionWeights6 w = ComputeWeightLoco6( p, FL,FF,FR,  SL,SF,SR );
			lmg.m_fBlendWeight[0]=w.fl;	lmg.m_fBlendWeight[1]=w.ff;	lmg.m_fBlendWeight[2]=w.fr;	
			lmg.m_fBlendWeight[3]=w.sl;	lmg.m_fBlendWeight[4]=w.sf;	lmg.m_fBlendWeight[5]=w.sr;	
		}

		SPU_NO_INLINE void ComputeWeight_FLR3(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];

			Vec2 FL=Vec2(rGlobalAnimHeader.m_arrBSAnimations[ 0].m_Position);	//sprint	
			Vec2 FF=Vec2(rGlobalAnimHeader.m_arrBSAnimations[ 1].m_Position);	//sprint	
			Vec2 FR=Vec2(rGlobalAnimHeader.m_arrBSAnimations[ 2].m_Position);	//sprint	
			Vec2 MF=Vec2(rGlobalAnimHeader.m_arrBSAnimations[ 3].m_Position);	//fast forward	
			Vec2 SL=Vec2(rGlobalAnimHeader.m_arrBSAnimations[ 4].m_Position);	//slow	
			Vec2 SF=Vec2(rGlobalAnimHeader.m_arrBSAnimations[ 5].m_Position);	//slow	
			Vec2 SR=Vec2(rGlobalAnimHeader.m_arrBSAnimations[ 6].m_Position);	//slow	

			GlobalAnimationHeaderCAF& rsubGAH01 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[1]) ]; 
			GlobalAnimationHeaderCAF& rsubGAH03 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[3]) ]; 
			GlobalAnimationHeaderCAF& rsubGAH05 = parrGlobalAnimations[ pAnimationSet->GetGlobalIDByAnimID_Fast(lmg.m_nAnimID[5]) ]; 

			//	f32 L_fast=rsubGAH00.m_fSpeed;	f32 R_fast=rsubGAH02.m_fSpeed;
			f32 F_fast=rsubGAH01.m_fSpeed;	
			f32 F_mid	=rsubGAH03.m_fSpeed;
			//	f32 L_slow=rsubGAH04.m_fSpeed;	f32 R_slow=rsubGAH06.m_fSpeed;
			f32 F_slow=rsubGAH05.m_fSpeed;	

			//	f32 TL_fast=rsubGAH0d.m_fTurnSpeed;	f32 TF_fast=rsubGAH00.m_fTurnSpeed;	f32 TR_fast=rsubGAH0e.m_fTurnSpeed;
			//	f32 TF_mid =rsubGAH06.m_fTurnSpeed;
			//	f32 TL_slow=rsubGAH0f.m_fTurnSpeed;	f32 TF_slow=rsubGAH07.m_fTurnSpeed;	f32 TR_slow=rsubGAH10.m_fTurnSpeed;

			f32 f0 = F_mid	-F_slow;
			f32 f1 = F_fast -F_slow;
			MF.y = (f0/f1)*2-1;

			Vec2 p = Vec2(lmg.m_BlendSpace.m_turn,lmg.m_BlendSpace.m_speed);
			LocomotionWeights7 w = ComputeWeightLoco7( p, FL,FF,FR, MF, SL,SF,SR );

			lmg.m_fBlendWeight[0]=w.fl;	lmg.m_fBlendWeight[1]=w.ff;	lmg.m_fBlendWeight[2]=w.fr;	
			lmg.m_fBlendWeight[3]=w.mf;	
			lmg.m_fBlendWeight[4]=w.sl;	lmg.m_fBlendWeight[5]=w.sf;	lmg.m_fBlendWeight[6]=w.sr;	
		}


		void ComputeWeight_UDH1(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{

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

			f32 R=0,F=0,L=0;	
			t = lmg.m_BlendSpace.m_turn;
			if (t<0)
			{
				R=1-(t+1); F=t+1;
			}
			else
			{
				F=1-t; L=t;
			}			

			lmg.m_fBlendWeight[0]=R*uh;
			lmg.m_fBlendWeight[1]=F*uh;
			lmg.m_fBlendWeight[2]=L*uh;

			lmg.m_fBlendWeight[3]=R*fw;
			lmg.m_fBlendWeight[4]=F*fw;
			lmg.m_fBlendWeight[5]=L*fw;

			lmg.m_fBlendWeight[6]=R*dh;
			lmg.m_fBlendWeight[7]=F*dh;
			lmg.m_fBlendWeight[8]=L*dh;
		}

		SPU_NO_INLINE void ComputeWeight_UDH2(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];

			f32 uh=0;
			f32 fw=0;
			f32 dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0)
			{
				uh=t;
				fw=(1-t);
			}
			else
			{
				fw=t+1;
				dh=1-(t+1);
			}

			Vec2 p = Vec2(lmg.m_BlendSpace.m_turn,lmg.m_BlendSpace.m_speed);

			Vec3 bs06=rGlobalAnimHeader.m_arrBSAnimations[0x06].m_Position;		
			Vec3 bs07=rGlobalAnimHeader.m_arrBSAnimations[0x07].m_Position;		
			Vec3 bs08=rGlobalAnimHeader.m_arrBSAnimations[0x08].m_Position;		

			Vec3 bs09=rGlobalAnimHeader.m_arrBSAnimations[0x09].m_Position;		
			Vec3 bs0a=rGlobalAnimHeader.m_arrBSAnimations[0x0a].m_Position;		
			Vec3 bs0b=rGlobalAnimHeader.m_arrBSAnimations[0x0b].m_Position;		

			Vec2 p06=Vec2(bs06.x,bs06.y);		Vec2 p07=Vec2(bs07.x,bs07.y);		Vec2 p08=Vec2(bs08.x,bs08.y);
			Vec2 p09=Vec2(bs09.x,bs09.y);		Vec2 p0a=Vec2(bs0a.x,bs0a.y);		Vec2 p0b=Vec2(bs0b.x,bs0b.y);

			if (lmg.m_BlendSpace.m_turn<0)
			{
				//left side
				Vec4 w;
				ComputeWeight4(w, p, p09,p0a,p07,p06);
				lmg.m_fBlendWeight[0+ 0]=w.w*uh;	lmg.m_fBlendWeight[1+ 0]=w.z*uh;	
				lmg.m_fBlendWeight[3+ 0]=w.x*uh;	lmg.m_fBlendWeight[4+ 0]=w.y*uh;

				lmg.m_fBlendWeight[0+ 6]=w.w*fw;	lmg.m_fBlendWeight[1+ 6]=w.z*fw;	
				lmg.m_fBlendWeight[3+ 6]=w.x*fw;	lmg.m_fBlendWeight[4+ 6]=w.y*fw;

				lmg.m_fBlendWeight[0+12]=w.w*dh;	lmg.m_fBlendWeight[1+12]=w.z*dh;	
				lmg.m_fBlendWeight[3+12]=w.x*dh;	lmg.m_fBlendWeight[4+12]=w.y*dh;
			}
			else
			{
				//right side
				Vec4 w;
				ComputeWeight4(w, p, p0a,p0b,p08,p07);
				lmg.m_fBlendWeight[1+ 0]=w.w*uh;	lmg.m_fBlendWeight[2+ 0]=w.z*uh;	
				lmg.m_fBlendWeight[4+ 0]=w.x*uh;	lmg.m_fBlendWeight[5+ 0]=w.y*uh;

				lmg.m_fBlendWeight[1+ 6]=w.w*fw;	lmg.m_fBlendWeight[2+ 6]=w.z*fw;	
				lmg.m_fBlendWeight[4+ 6]=w.x*fw;	lmg.m_fBlendWeight[5+ 6]=w.y*fw;

				lmg.m_fBlendWeight[1+12]=w.w*dh;	lmg.m_fBlendWeight[2+12]=w.z*dh;	
				lmg.m_fBlendWeight[4+12]=w.x*dh;	lmg.m_fBlendWeight[5+12]=w.y*dh;
			}
		}

		SPU_NO_INLINE void ComputeWeight_UDH3(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];

			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0) { uh=t;	fw=1-t;	}
			else {	fw=1+t;	dh=1-(t+1);	}

			Vec2 p = Vec2(lmg.m_BlendSpace.m_turn,lmg.m_BlendSpace.m_speed);

			Vec3 bs09=rGlobalAnimHeader.m_arrBSAnimations[0x09].m_Position;		
			Vec3 bs0a=rGlobalAnimHeader.m_arrBSAnimations[0x0a].m_Position;		
			Vec3 bs0b=rGlobalAnimHeader.m_arrBSAnimations[0x0b].m_Position;		

			Vec3 bs0c=rGlobalAnimHeader.m_arrBSAnimations[0x0c].m_Position;		
			Vec3 bs0d=rGlobalAnimHeader.m_arrBSAnimations[0x0d].m_Position;		
			Vec3 bs0e=rGlobalAnimHeader.m_arrBSAnimations[0x0e].m_Position;		

			Vec3 bs0f=rGlobalAnimHeader.m_arrBSAnimations[0x0f].m_Position;		
			Vec3 bs10=rGlobalAnimHeader.m_arrBSAnimations[0x10].m_Position;		
			Vec3 bs11=rGlobalAnimHeader.m_arrBSAnimations[0x11].m_Position;		

			Vec2 p09=Vec2(bs09.x,bs09.y);		Vec2 p0a=Vec2(bs0a.x,bs0a.y);		Vec2 p0b=Vec2(bs0b.x,bs0b.y);
			Vec2 p0c=Vec2(bs0c.x,bs0c.y);		Vec2 p0d=Vec2(bs0d.x,bs0d.y);		Vec2 p0e=Vec2(bs0e.x,bs0e.y);
			Vec2 p0f=Vec2(bs0f.x,bs0f.y);		Vec2 p10=Vec2(bs10.x,bs10.y);		Vec2 p11=Vec2(bs11.x,bs11.y);

			if (lmg.m_BlendSpace.m_speed > 0)
			{
				//fast version
				if (lmg.m_BlendSpace.m_turn<0)
				{
					//left side
					Vec4 w;
					ComputeWeight4(w,p, p0c,p0d,p0a,p09);
					lmg.m_fBlendWeight[0+ 0]=w.w*uh;	lmg.m_fBlendWeight[1+ 0]=w.z*uh;	
					lmg.m_fBlendWeight[3+ 0]=w.x*uh;	lmg.m_fBlendWeight[4+ 0]=w.y*uh;

					lmg.m_fBlendWeight[0+ 9]=w.w*fw;	lmg.m_fBlendWeight[1+ 9]=w.z*fw;	
					lmg.m_fBlendWeight[3+ 9]=w.x*fw;	lmg.m_fBlendWeight[4+ 9]=w.y*fw;

					lmg.m_fBlendWeight[0+18]=w.w*dh;	lmg.m_fBlendWeight[1+18]=w.z*dh;	
					lmg.m_fBlendWeight[3+18]=w.x*dh;	lmg.m_fBlendWeight[4+18]=w.y*dh;
				}
				else
				{
					//right side
					Vec4 w;
					ComputeWeight4(w,p, p0d,p0e,p0b,p0a);
					lmg.m_fBlendWeight[1+ 0]=w.w*uh;	lmg.m_fBlendWeight[2+ 0]=w.z*uh;	
					lmg.m_fBlendWeight[4+ 0]=w.x*uh;	lmg.m_fBlendWeight[5+ 0]=w.y*uh;

					lmg.m_fBlendWeight[1+ 9]=w.w*fw;	lmg.m_fBlendWeight[2+ 9]=w.z*fw;	
					lmg.m_fBlendWeight[4+ 9]=w.x*fw;	lmg.m_fBlendWeight[5+ 9]=w.y*fw;

					lmg.m_fBlendWeight[1+18]=w.w*dh;	lmg.m_fBlendWeight[2+18]=w.z*dh;	
					lmg.m_fBlendWeight[4+18]=w.x*dh;	lmg.m_fBlendWeight[5+18]=w.y*dh;
				}
			}
			else
			{
				//slow version
				if (lmg.m_BlendSpace.m_turn<0)
				{
					//left side
					Vec4 w;
					ComputeWeight4(w,p, p0f,p10,p0d,p0c);
					lmg.m_fBlendWeight[3+ 0]=w.w*uh;	lmg.m_fBlendWeight[4+ 0]=w.z*uh;	
					lmg.m_fBlendWeight[6+ 0]=w.x*uh;	lmg.m_fBlendWeight[7+ 0]=w.y*uh;

					lmg.m_fBlendWeight[3+ 9]=w.w*fw;	lmg.m_fBlendWeight[4+ 9]=w.z*fw;	
					lmg.m_fBlendWeight[6+ 9]=w.x*fw;	lmg.m_fBlendWeight[7+ 9]=w.y*fw;

					lmg.m_fBlendWeight[3+18]=w.w*dh;	lmg.m_fBlendWeight[4+18]=w.z*dh;	
					lmg.m_fBlendWeight[6+18]=w.x*dh;	lmg.m_fBlendWeight[7+18]=w.y*dh;
				}
				else
				{
					//right side
					Vec4 w;
					ComputeWeight4(w, p, p10,p11,p0e,p0d);
					lmg.m_fBlendWeight[4+ 0]=w.w*uh;	lmg.m_fBlendWeight[5+ 0]=w.z*uh;	
					lmg.m_fBlendWeight[7+ 0]=w.x*uh;	lmg.m_fBlendWeight[8+ 0]=w.y*uh;

					lmg.m_fBlendWeight[4+ 9]=w.w*fw;	lmg.m_fBlendWeight[5+ 9]=w.z*fw;	
					lmg.m_fBlendWeight[7+ 9]=w.x*fw;	lmg.m_fBlendWeight[8+ 9]=w.y*fw;

					lmg.m_fBlendWeight[4+18]=w.w*dh;	lmg.m_fBlendWeight[5+18]=w.z*dh;	
					lmg.m_fBlendWeight[7+18]=w.x*dh;	lmg.m_fBlendWeight[8+18]=w.y*dh;
				}
			}
		}

		void ComputeWeight_PUSH(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			float s;
			float t;
			if (!pAnimationSet->m_CharEditMode)
			{
				s = GetParamScale(lmg);
				t = lmg.m_params[eMotionParamID_TurnAngle].blendspace.m_fAssetBlend ;
			}
			else
			{
				s = 0.9f + (lmg.m_BlendSpace.m_speed * 0.5f + 0.5f) * 0.2f;
				s = clamp(s, 0.9f, 1.1f);
				t = lmg.m_BlendSpace.m_turn;
			}

			for (int i = 0; i < 9; i++)
				lmg.m_fBlendWeight[i] = 0.0f;

			int idx0, idx1, idx2, idx3;
			float tl, tr, sb, st;

			if (t < 0)
			{
				tl = -1.0f; tr = 0.0f;

				if (s >= 1.0f)
				{
					sb = 1.0f; st = 1.1f;
					idx0 = 1; idx1 = 4; idx2 = 5; idx3 = 2;
				}
				else
				{
					sb = 0.9f; st = 1.0f;
					idx0 = 0; idx1 = 3; idx2 = 4; idx3 = 1;
				}
			}
			else
			{
				tl = 0.0f; tr = 1.0f;

				if (s >= 1.0f)
				{
					sb = 1.0f; st = 1.1f;
					idx0 = 4; idx1 = 7; idx2 = 8; idx3 = 5;
				}
				else
				{
					sb = 0.9f; st = 1.0f;
					idx0 = 3; idx1 = 6; idx2 = 7; idx3 = 4;
				}
			}

			const float areaInv = 1.0f / ((tr - tl) * (st - sb));
			const float t1 = t - tl;
			const float t2 = tr - t;
			const float s1 = s - sb;
			const float s2 = st - s;
			lmg.m_fBlendWeight[idx0] = t2 * s2 * areaInv;
			lmg.m_fBlendWeight[idx1] = t1 * s2 * areaInv;
			lmg.m_fBlendWeight[idx2] = t1 * s1 * areaInv;
			lmg.m_fBlendWeight[idx3] = t2 * s1 * areaInv;
		}

		void ComputeWeight_PROT(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
// Temporarily disabled for refactoring
//			m_pSkeletonPose->SetFootAnchoring(0); // Tell motion playback to calculate foot anchoring adjustments.
			//
			f32 t = 0;
			if (!pAnimationSet->m_CharEditMode)
			{
				t = lmg.m_params[eMotionParamID_TurnAngle].blendspace.m_fAssetBlend;
				//t = this->GetDesiredMotionParam(eMotionParamID_TurnAngle);
			}
			else
			{
				t = lmg.m_BlendSpace.m_turn;
			}

			//
			// calc
			//
			if (t<0)
			{
				lmg.m_fBlendWeight[1]=1+t;	//middle
				lmg.m_fBlendWeight[2]=-t;	//left
			}
			else
			{
				lmg.m_fBlendWeight[0]=t;	//right
				lmg.m_fBlendWeight[1]=1-t;	//middle
			}
		}

		void ComputeWeight_SCAL(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];

// Temporarily disabled for refactoring
//			m_pSkeletonPose->SetFootAnchoring(0); // Tell motion playback to not calculate foot anchoring adjustments.
			f32 scale;
			if (!pAnimationSet->m_CharEditMode)
			{
				scale = GetParamScale(lmg);
			}
			else
			{
				scale  = lmg.m_params[eMotionParamID_Scale].blendspace.m_fAssetBlend;
			}

			uint32 numAssets = rGlobalAnimHeader.m_arrBSAnimations.size();
			int idx = -1;
			f32 minDelta = FLT_MAX;
			for (uint32 i = 0; i < numAssets; i++)
			{
				float curScale = rGlobalAnimHeader.m_arrBSAnimations[i].m_Position.y;
				float curDelta = fabs(curScale - scale);
				if (curDelta < minDelta)
				{
					minDelta = curDelta;
					idx = i;
				}
			}
			assert(idx >= 0);

			f32 frontLerp = -1.0f;
			f32 scale0 = rGlobalAnimHeader.m_arrBSAnimations[idx].m_Position.y;
			f32 scale1;

			if (scale < scale0)
			{
				if (idx > 0)
				{
					--idx;
					scale1 = scale0;
					scale0 = rGlobalAnimHeader.m_arrBSAnimations[idx].m_Position.y;
				}
				else
				{
					frontLerp = 1.0f;
				}
			}
			else
			{
				if (idx < (int)numAssets - 1)
				{
					scale1 = rGlobalAnimHeader.m_arrBSAnimations[idx + 1].m_Position.y;
				}
				else
				{
					--idx;
					frontLerp = 0.0f;
				}
			}

			if (frontLerp == -1.0f)
			{
				frontLerp = (scale1 - scale) / (scale1 - scale0);
				frontLerp = clamp(frontLerp, 0.0f, 1.0f);
			}

			lmg.m_fBlendWeight[idx+0] = frontLerp;
			lmg.m_fBlendWeight[idx+1] = 1.0f - frontLerp;
		}

		void ComputeWeight_CLMB(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			const float minH = 1.0f;
			const float midH = 1.3f;
			const float maxH = 2.2f;
			const float minS = 0.833f;
			const float midS = 1.0f;
			const float maxS = 1.25f;

			float s = GetParamScale(lmg);
			float h = lmg.m_params[eMotionParamID_Height].blendspace.m_fAssetBlend;
			s = CLAMP(s, minS, maxS);
			h = CLAMP(h, minH, maxH);

			int idx0, idx1, idx2, idx3;
			float hl, hr, sb, st;

			if (h >= midH)
			{
				hl = midH; hr = maxH;
				if (s >= midS)
				{
					idx0 = 7; idx1 = 10; idx2 = 11; idx3 = 8;
					sb = midS; st = maxS;
				}
				else
				{
					idx0 = 6; idx1 = 9; idx2 = 10; idx3 = 7;
					sb = minS; st = midS;
				}
			}
			else
			{
				hl = minH; hr = midH;
				if (s >= midS)
				{
					idx0 = 1; idx1 = 4; idx2 = 5; idx3 = 2;
					sb = midS; st = maxS;
				}
				else
				{
					idx0 = 0; idx1 = 3; idx2 = 4; idx3 = 1;
					sb = minS; st = midS;
				}
			}

			for (int i = 0; i < 12; i++)
				lmg.m_fBlendWeight[i] = 0.0f;

			const float areaInv = 1.0f / ((hr - hl) * (st - sb));
			const float h1 = h - hl;
			const float h2 = hr - h;
			const float s1 = s - sb;
			const float s2 = st - s;
			lmg.m_fBlendWeight[idx0] = h2 * s2 * areaInv;
			lmg.m_fBlendWeight[idx1] = h1 * s2 * areaInv;
			lmg.m_fBlendWeight[idx2] = h1 * s1 * areaInv;
			lmg.m_fBlendWeight[idx3] = h2 * s1 * areaInv;
		}

		void ComputeWeight_COOP(SParametric & lmg, CAnimationSet * pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{
			const float minH = 2.2f;
			const float midH = 3.0f;
			const float maxH = 3.8f;
			const float minS = 0.9f;
			const float midS = 1.0f;
			const float maxS = 1.1f;

			float s = GetParamScale(lmg);
			float h = lmg.m_params[eMotionParamID_Height].blendspace.m_fAssetBlend;
			s = CLAMP(s, minS, maxS);
			h = CLAMP(h, minH, maxH);

			int idx0, idx1, idx2, idx3;
			float hl, hr, sb, st;

			if (h >= midH)
			{
				hl = midH; hr = maxH;
				if (s >= midS)
				{
					idx0 = 4; idx1 = 7; idx2 = 8; idx3 = 5;
					sb = midS; st = maxS;
				}
				else
				{
					idx0 = 3; idx1 = 6; idx2 = 7; idx3 = 4;
					sb = minS; st = midS;
				}
			}
			else
			{
				hl = minH; hr = midH;
				if (s >= midS)
				{
					idx0 = 1; idx1 = 4; idx2 = 5; idx3 = 2;
					sb = midS; st = maxS;
				}
				else
				{
					idx0 = 0; idx1 = 3; idx2 = 4; idx3 = 1;
					sb = minS; st = midS;
				}
			}

			for (int i = 0; i < 9; i++)
				lmg.m_fBlendWeight[i] = 0.0f;

			const float areaInv = 1.0f / ((hr - hl) * (st - sb));
			const float h1 = h - hl;
			const float h2 = hr - h;
			const float s1 = s - sb;
			const float s2 = st - s;
			lmg.m_fBlendWeight[idx0] = h2 * s2 * areaInv;
			lmg.m_fBlendWeight[idx1] = h1 * s2 * areaInv;
			lmg.m_fBlendWeight[idx2] = h1 * s1 * areaInv;
			lmg.m_fBlendWeight[idx3] = h2 * s1 * areaInv;
		}

		SPU_NO_INLINE void ComputeWeight__TH2(SParametric & lmg, CAnimationSet* pAnimationSet, GlobalAnimationHeaderCAF * parrGlobalAnimations, GlobalAnimationHeaderLMG& rGHLMG)
		{	
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];

			f32 uh=0,fw=0,dh=0;
			f32	t = lmg.m_BlendSpace.m_slope;
			if (t>0) { uh=t;	fw=1-t;	}
			else {	fw=1+t;	dh=1-(t+1);	}

			Vec2 p = Vec2(lmg.m_BlendSpace.m_turn,lmg.m_BlendSpace.m_speed);

			Vec3 bs09=rGlobalAnimHeader.m_arrBSAnimations[0x02].m_Position;		
			Vec3 bs0a=rGlobalAnimHeader.m_arrBSAnimations[0x03].m_Position;		
			Vec3 bs0b=rGlobalAnimHeader.m_arrBSAnimations[0x04].m_Position;		
			Vec3 bs0f=rGlobalAnimHeader.m_arrBSAnimations[0x05].m_Position;		
			Vec3 bs10=rGlobalAnimHeader.m_arrBSAnimations[0x06].m_Position;		
			Vec3 bs11=rGlobalAnimHeader.m_arrBSAnimations[0x07].m_Position;		

			Vec2 p09=Vec2(bs09.x,bs09.y);		Vec2 p0a=Vec2(bs0a.x,bs0a.y);		Vec2 p0b=Vec2(bs0b.x,bs0b.y);
			Vec2 p0f=Vec2(bs0f.x,bs0f.y);		Vec2 p10=Vec2(bs10.x,bs10.y);		Vec2 p11=Vec2(bs11.x,bs11.y);

			//fast version
			if (lmg.m_BlendSpace.m_turn<0)
			{
				//left side
				Vec4 w;
				ComputeWeight4(w, p, p0f,p10,p0a,p09);
				lmg.m_fBlendWeight[0]=(w.w+w.z)*uh;	
				lmg.m_fBlendWeight[1]=(w.x+w.y)*uh;	

				lmg.m_fBlendWeight[2]=w.w*fw;	lmg.m_fBlendWeight[3]=w.z*fw;	
				lmg.m_fBlendWeight[5]=w.x*fw;	lmg.m_fBlendWeight[6]=w.y*fw;

				lmg.m_fBlendWeight[8]=(w.w+w.z)*dh;	
				lmg.m_fBlendWeight[9]=(w.x+w.y)*dh;	
			}
			else
			{
				//right side
				Vec4 w;
				ComputeWeight4(w,p, p10,p11,p0b,p0a);
				lmg.m_fBlendWeight[0]=(w.w+w.z)*uh;	
				lmg.m_fBlendWeight[1]=(w.x+w.y)*uh;	

				lmg.m_fBlendWeight[3]=w.w*fw;	lmg.m_fBlendWeight[4]=w.z*fw;	
				lmg.m_fBlendWeight[6]=w.x*fw;	lmg.m_fBlendWeight[7]=w.y*fw;

				lmg.m_fBlendWeight[8]=(w.w+w.z)*dh;	
				lmg.m_fBlendWeight[9]=(w.x+w.y)*dh;	
			}

			f32 fw_fast = lmg.m_fBlendWeight[3];
			f32 fw_slow = lmg.m_fBlendWeight[6];
			if (lmg.m_BlendSpace.m_weightshift>0)
			{
				//weight shift to the right
				lmg.m_fBlendWeight[ 3] = fw_fast*(1.0f-lmg.m_BlendSpace.m_weightshift); //fast
				lmg.m_fBlendWeight[11] = fw_fast*(lmg.m_BlendSpace.m_weightshift);      //fast

				lmg.m_fBlendWeight[ 6] = fw_slow*(1.0f-lmg.m_BlendSpace.m_weightshift); //slow
				lmg.m_fBlendWeight[13] = fw_slow*(lmg.m_BlendSpace.m_weightshift);      //slow
			}
			else
			{
				//weight shift to the left
				lmg.m_fBlendWeight[ 3] = fw_fast*(1.0f-(-lmg.m_BlendSpace.m_weightshift));	//fast
				lmg.m_fBlendWeight[10] = fw_fast*(-lmg.m_BlendSpace.m_weightshift);					//fast

				lmg.m_fBlendWeight[ 6] = fw_slow*(1.0f-(-lmg.m_BlendSpace.m_weightshift));	//slow
				lmg.m_fBlendWeight[12] = fw_slow*(-lmg.m_BlendSpace.m_weightshift);					//slow
			}

#ifdef _DEBUG
			f32 fTotal=0.0f;
			for (uint32 i=0; i<14; i++)
				fTotal+=lmg.m_fBlendWeight[i];
			fTotal = fabsf(fTotal-1.0f);
			assert(fTotal<0.05f);
#endif
		}
	} // namespace _private
} // namespace LMG

namespace LMG
{
	void CheckBlendWeights(CAnimationSet* pAnimationSet, SParametric& lmg)
	{
		if (lmg.m_nParametricID>=0)
		{
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];


			f32 fTotal=0.0f;
			uint32 numAssets = rGlobalAnimHeader.m_arrBSAnimations.size();
			for (uint32 i=0; i<numAssets; i++)
				fTotal += lmg.m_fBlendWeight[i];
			assert( fabsf(fTotal-1.0f)<0.005f );

			uint32 nExtrapolation1=0;
			for (uint32 i=0; i<numAssets; i++)
				nExtrapolation1 |= uint32(lmg.m_fBlendWeight[i]<-0.2f);

			uint32 nExtrapolation2=0;
			for (uint32 i=0; i<numAssets; i++)
				nExtrapolation2 |= uint32(lmg.m_fBlendWeight[i]>1.2f);

			if( fabsf(fTotal-1.0f)>0.005f || nExtrapolation1 || nExtrapolation2 )
			{
				assert(!"Sum of weights is wrong");
				char blendCode[5];
				blendCode[0] = (rGlobalAnimHeader.m_nBlendCodeLMG >>  0) & 0xff;
				blendCode[1] = (rGlobalAnimHeader.m_nBlendCodeLMG >>  8) & 0xff;
				blendCode[2] = (rGlobalAnimHeader.m_nBlendCodeLMG >> 16) & 0xff;
				blendCode[3] = (rGlobalAnimHeader.m_nBlendCodeLMG >> 24) & 0xff;
				blendCode[4] = 0;
#if !defined(__SPU__)
				CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "Sum of weights is wrong. LMG blend code is \'%s\'", blendCode);
#endif
			}
		}
	}

	void ComputeWeight(CAnimationSet* pAnimationSet, SParametric& lmg)
	{
		for (uint32 i=0; i<MAX_LMG_ANIMS; i++)
			lmg.m_fBlendWeight[i]=0;

		if (lmg.m_nParametricID>=0)
		{
			const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID);
			assert(pAnim->m_nAssetType==LMG_File);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[pAnim->m_nGlobalAnimId];

			assert(rGlobalAnimHeader.m_nBlendCodeLMG);

			GlobalAnimationHeaderCAF* parrGlobalAnimations = &g_AnimationManager.m_arrGlobalCAF[0];		

			uint32 nBlendCode = rGlobalAnimHeader.m_nBlendCodeLMG;

			switch( AnimCodeLookup::ComputeAnimCode(nBlendCode) )
			{
			#define DRY(X) case AnimCodeLookup::X: \
				_private::ComputeWeight_##X( lmg, pAnimationSet, parrGlobalAnimations, rGlobalAnimHeader ); break;
			#include "LMG_Types.inc"
			#undef DRY
			}
		}
		else
		{
			lmg.m_fBlendWeight[0]=1;
		}
	}

	TW GetTimewarpedDuration(CAnimationSet* pAnimationSet, SParametric& lmg)
	{
		TW tw;
		
		PrefetchLine(lmg.m_fBlendWeight, 0);

		if (lmg.m_nAnimID[0]<0)
			return tw;
		
		PrefetchLine(lmg.m_fBlendWeight, 128);

		const int32 kNumAnims = lmg.m_numAnims;

		for (uint32 i=0; i<MAX_LMG_ANIMS; i++)
		{
			float newBlendWeight = (float)__fsel(lmg.m_fBlendWeight[i] - 0.001f, lmg.m_fBlendWeight[i], 0.0f);
			lmg.m_fBlendWeight[i] = (float)__fsel( -(lmg.m_fBlendWeight[i] - 0.999f), newBlendWeight, 1.0f);
		}
		
		for (int32 i=0; i<kNumAnims; i++)
		{
			int32 nAnimID = lmg.m_nAnimID[i];
			const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(nAnimID) );

			if (pAnim->m_nAssetType==CAF_File)
			{
				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[pAnim->m_nGlobalAnimId];
				int32 segcount = lmg.m_nSegmentCounter[i];
				f32 fSegTime = rGAH.GetSegmentDuration(segcount);
				f32 fTotTime = rGAH.m_fTotalDuration;
				
				if (fSegTime != fTotTime)
				{
					f32 d0 = rGAH.GetSegmentDuration(segcount);
					tw.m_fDuration	+= d0*lmg.m_fBlendWeight[i];
					//	tw.m_fDistance	= 0;
					//	tw.m_fSpeed			= 0;
					//TODO: temp solution because we want only the duration
					tw.m_fDistance  += lmg.m_fBlendWeight[i]*tw.m_fDuration;
					tw.m_fSpeed		  += lmg.m_fBlendWeight[i];
				}
				else
				{
					tw.m_fDuration	+= lmg.m_fBlendWeight[i]*rGAH.m_fTotalDuration;
					tw.m_fDistance	+= lmg.m_fBlendWeight[i]*rGAH.m_fDistance;
					tw.m_fSpeed			+= lmg.m_fBlendWeight[i]*rGAH.m_fSpeed;
				}
			}

			if (pAnim->m_nAssetType==AIM_File)
			{
				GlobalAnimationHeaderAIM& rGAH = g_AnimationManager.m_arrGlobalAIM[pAnim->m_nGlobalAnimId];
				tw.m_fDuration  += lmg.m_fBlendWeight[i]*rGAH.m_fTotalDuration;
				tw.m_fDistance	=0;
				tw.m_fSpeed			=0;
			}


		}
		// 	assert(fDuration);
		return tw;
	}
} // namespace LMG