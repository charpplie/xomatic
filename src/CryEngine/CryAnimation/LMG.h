///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   LMG.h
//  Version:     v1.00
//  Created:     07/03/2009 by Jaewon Jung
//  Description: Locomotion Group utility functions
// -------------------------------------------------------------------------
//
///////////////////////////////////////////////////////////////////////////

#ifndef _LMG_H_
#define _LMG_H_

#pragma once

#include <CryCharAnimationParams.h>
#include "AnimationManager.h"

struct ModelAnimationHeader;
class CAnimationSet;

namespace LMG
{	
	// A utility structure for time-warping
	struct TW
	{
		f32 m_fDuration;
		f32 m_fSpeed;
		f32 m_fDistance;
		TW()
		{
			m_fDuration=0;
			m_fSpeed=0;
			m_fDistance=0;
		}
	};

	// Parse an LMG XML file and fill the GAH accordingly.
	bool ParseXML(GlobalAnimationHeaderLMG& globalAnim );
	// Compute necessary data after loading LMG assets.
	void ExtractParameters(CAnimationSet* pAnimSet, GlobalAnimationHeaderLMG& globalAnimHeader);
	
	// Return a run-time LMG structure.
	SParametric BuildRuntimeStructCAF(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim, int nID);
	SParametric BuildRuntimeStructAIM(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim, int nID);
	SParametric BuildRuntimeStructLMG(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim, int nID);
	SParametric BuildRuntimeStructPMG(CAnimationSet* pAnimSet, const ModelAnimationHeader* pAnim, int nID);

	// Compute blending weights for LMG.
	void ComputeWeight(CAnimationSet* pAnimSet, SParametric& lmg);
	void CheckBlendWeights(CAnimationSet* pAnimationSet, SParametric& lmg);
	TW GetTimewarpedDuration( CAnimationSet* pAnimationSet, SParametric& lmg);
	
	// Return the capability info about an LMG.
	LMGCapabilities GetCapabilities(CAnimationSet* pAnimSet, const SParametric& lmg);

	bool IsStrafingLMGCode(uint32 blendCode);
	bool IsValidBlendCode(uint32 blendCode);

	// singleton class to to map an animcode to [0, .., n] (encoded as enums) to allow
	// refactoring the getLMGCapatibilietes and ComputeWeight functions into smaller parts
	// with a switch statement
	struct AnimCodeLookup
	{
		enum AnimCode {
			#define DRY(X) X,
			#include "LMG_Types.inc"
			LAST		// used to compute size of static array
			#undef DRY
		};
		
		struct AnimCodePair
		{
			uint32 		key;
			AnimCode 	code;

			bool operator<(const AnimCodePair &other ) const
			{
				return key < other.key;
			}
		};

		static AnimCode ComputeAnimCode( uint32 value );

#if defined(__SPU__)
		static void AnimCodeLookup::initSPULookup();
#endif

		AnimCodeLookup();
	private:
	};

	//////////////////////////////////////////////////////////////////////////
	// Utility structures for weight computation														//
	//////////////////////////////////////////////////////////////////////////

	struct StrafeWeights4
	{
		f32 f,l,b,r;
		ILINE StrafeWeights4(){};
		ILINE StrafeWeights4( f32 vf, f32 vl, f32 vb, f32 vr ) { f=vf; l=vl; b=vb; r=vr; };
		ILINE void operator () ( f32 vf, f32 vl, f32 vb, f32 vr ) { f=vf; r=vl; l=vb; b=vr; };
		ILINE StrafeWeights4 operator * (f32 k) const { return StrafeWeights4(f*k,l*k,b*k,r*k); }
		ILINE StrafeWeights4 operator + (StrafeWeights4& v1) const { return StrafeWeights4(f+v1.f,l+v1.l,b+v1.b,r+v1.r); }
		ILINE StrafeWeights4& operator /= (f32 k) {	k=1.0f/k; f*=k; l*=k; b*=k; r*=k; return *this;	}
		ILINE StrafeWeights4& operator += (StrafeWeights4& v1) { f+=v1.f; l+=v1.l; b+=v1.b; r+=v1.r; return *this; }
	};

	struct StrafeWeights6
	{
		f32 f,fl,l, b,br,r;
		ILINE StrafeWeights6(){};
		ILINE StrafeWeights6( f32 va, f32 vb, f32 vc, f32 vd, f32 ve, f32 vf ) { f=va; fl=vb; l=vc; b=vd; br=ve; r=vf; };
		ILINE void operator () ( f32 va, f32 vb, f32 vc, f32 vd, f32 ve, f32 vf ) { f=va; fl=vb; l=vc; b=vd; br=ve; r=vf; };
		ILINE StrafeWeights6 operator * (f32 k) const { return StrafeWeights6( f*k,fl*k,l*k,b*k,br*k,r*k ); }
		ILINE StrafeWeights6 operator + (StrafeWeights6& v1) const { return StrafeWeights6(f+v1.f, fl+v1.fl, l+v1.l, b+v1.b, br+v1.br, r+v1.r); }
		ILINE StrafeWeights6& operator /= (f32 k) {	k=1.0f/k; f*=k; fl*=k; l*=k; b*=k; br*=k; r*=k; return *this;	}
		ILINE StrafeWeights6& operator += (StrafeWeights6& v1) { f+=v1.f; fl+=v1.fl; l+=v1.l; b+=v1.b; br+=v1.br; r+=v1.r; return *this; }
	};

	struct StrafeWeights8
	{
		f32 f,fl,l,lb, b,br,r,rf;
		ILINE StrafeWeights8(){};
		ILINE StrafeWeights8( f32 v1,f32 v2,f32 v3,f32 v4,   f32 v5,f32 v6,f32 v7,f32 v8 ) { f=v1;fl=v2;l=v3;lb=v4;   b=v5;br=v6;r=v7;rf=v8; };
		ILINE void operator () ( f32 v1,f32 v2,f32 v3,f32 v4,   f32 v5,f32 v6,f32 v7,f32 v8  ) { f=v1;fl=v2;l=v3;lb=v4;  b=v5;br=v6;r=v7;rf=v8; };
		ILINE StrafeWeights8 operator * (f32 k) const { return StrafeWeights8( f*k,fl*k,l*k,lb*k, b*k,br*k,r*k,rf*k ); }
		ILINE StrafeWeights8 operator + (StrafeWeights8& v) const { return StrafeWeights8(f+v.f, fl+v.fl, l+v.l, lb+v.lb,   b+v.b,br+v.br,r+v.r,rf+v.rf); }
		ILINE StrafeWeights8& operator /= (f32 k) {	k=1.0f/k;   f*=k; fl*=k; l*=k; lb*=k;   b*=k; br*=k; r*=k; rf*=k; return *this;	}
		ILINE StrafeWeights8& operator += (StrafeWeights8& v) { f+=v.f; fl+=v.fl; l+=v.l; lb+=v.lb;   b+=v.b; br+=v.br; r+=v.r; rf+=v.rf; return *this; }
	};

	struct IdleStepWeights6
	{
		f32 fr,rr,br, fl,ll,bl;
		ILINE IdleStepWeights6(){};
		ILINE IdleStepWeights6( f32 vfr, f32 vrr, f32 vbr, f32 vfl, f32 vll, f32 vbl ) { fr=vfr; rr=vrr; br=vbr;   fl=vfl; ll=vll; bl=vbl; };
		ILINE void operator() ( f32 vfr, f32 vrr, f32 vbr, f32 vfl, f32 vll, f32 vbl ) { fr=vfr; rr=vrr; br=vbr;   fl=vfl; ll=vll; bl=vbl; };
		ILINE IdleStepWeights6& operator /= (f32 k) {	k=1.0f/k; fr*=k; rr*=k; br*=k;  fl*=k; ll*=k; bl*=k; return *this;	}
		ILINE IdleStepWeights6& operator += (IdleStepWeights6& v1) { fr+=v1.fr; rr+=v1.rr; br+=v1.br;   fl+=v1.fl; ll+=v1.ll; bl+=v1.bl; return *this; }
	};

	struct LocomotionWeights6
	{
		f32 fl,ff,fr,  sl,sf,sr;
		ILINE LocomotionWeights6(){};
		ILINE LocomotionWeights6( f32 a,f32 b,f32 c,    f32 d,f32 e,f32 f ) {	fl=a; ff=b; fr=c;	 sl=d; sf=e; sr=f; };
		ILINE void operator () (  f32 a,f32 b,f32 c,    f32 d,f32 e,f32 f ) {	fl=a; ff=b; fr=c;  sl=d; sf=e; sr=f; };
		ILINE LocomotionWeights6& operator * (f32 k) { fl*=k; ff*=k; fr*=k; 		sl*=k; sf*=k; sr*=k; return *this; }
		ILINE LocomotionWeights6& operator /= (f32 k) {	k=1.0f/k; fl*=k; ff*=k; fr*=k; 		sl*=k; sf*=k; sr*=k; return *this; }
		ILINE LocomotionWeights6& operator += (LocomotionWeights6& v1) { fl+=v1.fl;	ff+=v1.ff;	fr+=v1.fr;	   sl+=v1.sl;	sf+=v1.sf;	sr+=v1.sr; return *this; }
	};

	struct LocomotionWeights7
	{
		f32 fl,ff,fr,  mf,  sl,sf,sr;
		ILINE LocomotionWeights7(){};
		ILINE LocomotionWeights7( f32 a,f32 b,f32 c,   f32 d,    f32 e,f32 f,f32 g ) {	fl=a; ff=b; fr=c;	mf=d; sl=e; sf=f; sr=g; };
		ILINE void operator () (  f32 a,f32 b,f32 c,   f32 d,    f32 e,f32 f,f32 g ) {	fl=a; ff=b; fr=c; mf=d;	sl=e; sf=f; sr=g; };
		ILINE LocomotionWeights7& operator * (f32 k) {	fl*=k; ff*=k; fr*=k; 	mf*=k; 	sl*=k; sf*=k; sr*=k; return *this; }
		ILINE LocomotionWeights7& operator /= (f32 k) {	k=1.0f/k; fl*=k; ff*=k; fr*=k; 	mf*=k; 	sl*=k; sf*=k; sr*=k; return *this; }
		ILINE LocomotionWeights7& operator += (LocomotionWeights7& v1) { fl+=v1.fl;	ff+=v1.ff;	fr+=v1.fr;	mf+=v1.mf; sl+=v1.sl;	sf+=v1.sf;	sr+=v1.sr; 	return *this; }
	};
}

#endif	// #ifndef _LMG_H_
