#include "stdafx.h"
#include "PoseModifierHelper.h"





namespace PoseModifierHelper 
{

	void IK_Solver2Bones( const Vec3r& goal, const IKLimbType& rIKLimbType,QuatT* const __restrict	pRelativePose,QuatT* const __restrict	pAbsolutePose)
	{
		int32 b0 = rIKLimbType.m_arrJointChain[0].m_idxJoint; assert(b0>0); //BaseRoot
		int32 b1 = rIKLimbType.m_arrJointChain[1].m_idxJoint; assert(b1>0); //Root of Chain
		int32 b2 = rIKLimbType.m_arrJointChain[2].m_idxJoint; assert(b2>0); //Middle Joint 
		int32 b3 = rIKLimbType.m_arrJointChain[3].m_idxJoint; assert(b3>0); //EndEffector

		uint32 numLinks = rIKLimbType.m_arrRootToEndEffector.size();
		pAbsolutePose[0]	= pRelativePose[0];
		for (uint32 i=1; i<numLinks; i++)
		{
			int32 p=rIKLimbType.m_arrRootToEndEffector[i-1];
			int32 c=rIKLimbType.m_arrRootToEndEffector[i];
			pAbsolutePose[c]	= pAbsolutePose[p] * pRelativePose[c];
		}

		Vec3r t0 = pAbsolutePose[b1].t;
		Vec3r t1 = pAbsolutePose[b2].t;
		Vec3r t2 = pAbsolutePose[b3].t;
		if ( ((goal-t2)|(goal-t2)) < real(0.00001) )
			return; //end-effector and new goal are very close

		Vec3r h,axis;
		Vec3r a = t1-t0;
		Vec3r b = t2-t1;
		Vec3r c = goal-t0;
		Vec3r cross=a%b;
		if ((cross|cross) < real(0.0000001) )
			return;

		cross = cross.GetNormalizedSafe(-pAbsolutePose[b2].q.GetColumn2());

		real alen = a.GetLength();
		real blen = b.GetLength();
		real clen = c.GetLength();
		if (alen*blen*clen < real(0.000001) )
			return;  //check for invalid configurations / broken bones

		real cos0	= clamp(	(alen*alen+clen*clen-blen*blen)/(real(2.0)*alen*clen) ,real(-1.0),real(+1.0) );
		real sin0	=	sqrt( max(real(1.0)-cos0*cos0,real(0.0)) );

		a/=alen;
		a = a.GetRotated( cross, cos0,sin0 );
		assert((fabs_tpl(1-(a|a))) < real(0.00001) );   //check if unit-vector

		c/=clen;
		h=(a+c).GetNormalized();
		axis=a%c;	axis/=max(axis.GetLength(),real(0.000001));
		pAbsolutePose[b1].q = Quat(f32(a|h), axis*(a%h).GetLength()) * pAbsolutePose[b1].q;
		pRelativePose[b1].q = !pAbsolutePose[b0].q * pAbsolutePose[b1].q;
		pRelativePose[b1].q.Normalize();
		pAbsolutePose[b2]  = pAbsolutePose[b1] * pRelativePose[b2];
		pAbsolutePose[b3]  = pAbsolutePose[b2] * pRelativePose[b3];
		//---------------------------------------------------
		c = (goal-pAbsolutePose[b2].t).GetNormalized(); 
		b = Vec3r(pAbsolutePose[b3].t-pAbsolutePose[b2].t)/blen;
		h = (b+c).GetNormalized();
		axis=b%c;	axis/=max(axis.GetLength(),real(0.000001));
		pAbsolutePose[b2].q = Quat(f32(b|h),axis*(b%h).GetLength())*pAbsolutePose[b2].q;
		pRelativePose[b2].q = !pAbsolutePose[b1].q * pAbsolutePose[b2].q;
		pRelativePose[b2].q.Normalize();
	}

	void IK_Solver3Bones( // from 0->3, Pelvis, Root, middle1, middle2, endeffector
		const Vec3r& goal, const IKLimbType& rIKLimbType,QuatT* const __restrict pRelativePose,QuatT* const __restrict pAbsolutePose)
	{
		if(Console::GetInst().ca_AlienLegIK == 2) //  Use Ivo's solution
		{
			IK_Solver3Bones_Ivo(goal, rIKLimbType, pRelativePose, pAbsolutePose);
			return;
		}

		//------------------------------------------------------------------------------
		// Get Bone ids
		int32 b[5];
		for(int32 i=0; i<5; ++i)
		{
			b[i] = rIKLimbType.m_arrJointChain[i].m_idxJoint;
			assert(b[i]>0);
		}

		// Calculate absolute poses
		uint32 numLinks = rIKLimbType.m_arrRootToEndEffector.size();
		pAbsolutePose[0]	= pRelativePose[0];
		for (uint32 i=1; i<numLinks; i++)
		{
			int32 p=rIKLimbType.m_arrRootToEndEffector[i-1];
			int32 c=rIKLimbType.m_arrRootToEndEffector[i];
			pAbsolutePose[c]	= pAbsolutePose[p] * pRelativePose[c];
		}

		// Get rotation axis for adjustment
		const Vec3 axisZ = pAbsolutePose[b[2]].q.GetColumn2();
		
		// Check is goal is close enough
		Vec3  Vc2 = goal - pAbsolutePose[b[1]].t;
		f32 Lc2 = Vc2.GetLength();
		if(Lc2 < 1e-3f) // Goal is close enough
			return;

		// Prepare vectors and get their lengthes
		Vec3  Va = pAbsolutePose[b[3]].t - pAbsolutePose[b[1]].t;
		Vec3  Vb = pAbsolutePose[b[4]].t - pAbsolutePose[b[3]].t;
		Vec3  Vc = pAbsolutePose[b[4]].t - pAbsolutePose[b[1]].t;
		f32 La = Va.GetLength();
		f32 Lb = Vb.GetLength();
		f32 Lc = Vc.GetLength();

		//------------------------------------------------------------------------------
		// Adjust joint 2 using bio-mechanical hints
		
		Vec3 V1 = pAbsolutePose[b[2]].t - pAbsolutePose[b[1]].t;
		Vec3 V2 = pAbsolutePose[b[3]].t - pAbsolutePose[b[2]].t;
		Vec3& V3 = Vb;
		f32 LV1 = V1.GetLength();
		f32 LV2 = V2.GetLength();
		f32 LV3 = Lb;

		f32 Lmax = LV1 + LV2 + LV3;
		f32 Lmin = fabs( min(LV1 - LV2 + LV3, LV1-LV2-LV3) ); 

		f32 r = Lc2 / Lc;
		f32 theta2 = acos( clamp( (V1|V2)/(LV1*LV2), -1.0f, 1.0f ) );
		f32 theta = gf_PI - theta2;

		if( (Lc2-Lmax) >= 1e-3f ) // Just stretch the leg
		{
			pRelativePose[b[2]].q = pRelativePose[b[3]].q = Quat::CreateIdentity();
			for(int32 i=2; i<=4; ++i)
				pAbsolutePose[b[i]] = pAbsolutePose[b[i-1]]*pRelativePose[b[i]];

			Vec3 v14 = pAbsolutePose[b[4]].t - pAbsolutePose[b[1]].t;
			f32 Lv14 = v14.GetLength();
			Vec3 axisH = v14 % Vc2;
			axisH.Normalize();
			f32 t = acos( clamp( (v14|Vc2) / (Lc2*Lv14), -1.0f, 1.0f) );
			pAbsolutePose[b[1]].q = Quat::CreateRotationAA(t, axisH)*pAbsolutePose[b[1]].q;
			pRelativePose[b[1]].q = !pAbsolutePose[b[0]].q * pAbsolutePose[b[1]].q;
			pRelativePose[b[1]].q.Normalize();

			for(int32 i=2; i<5; ++i)
				pAbsolutePose[b[i]] = pAbsolutePose[b[i-1]] * pRelativePose[b[i]];

			return;
		}
		else // Should adjust joint 2's rotation
		{
			// r ==1.0f is already considered before
			if(r < 1.0f)
				pAbsolutePose[b[2]].q = Quat::CreateRotationAA(-theta*(1.0f-r)*1.2f, axisZ) * pAbsolutePose[b[2]].q; //multiplier 1.2: make joint3 rotates more natural
			else if(r > 1.0f)
				pAbsolutePose[b[2]].q = Quat::CreateRotationAA(theta2*(Lc2-Lc)/(Lmax-Lc), axisZ) * pAbsolutePose[b[2]].q ;

			pRelativePose[b[2]].q = !pAbsolutePose[b[1]].q * pAbsolutePose[b[2]].q;
			pRelativePose[b[2]].q.Normalize();

			// Update the chain
			pAbsolutePose[b[3]] = pAbsolutePose[b[2]] * pRelativePose[b[3]];
			pAbsolutePose[b[4]] = pAbsolutePose[b[3]] * pRelativePose[b[4]];
		}

		//------------------------------------------------------------------------------
		// Adjust joint 3

		// Update va, vb, vc and lengths
		Va = pAbsolutePose[b[3]].t - pAbsolutePose[b[1]].t;
		Vb = pAbsolutePose[b[4]].t - pAbsolutePose[b[3]].t;
		Vc = pAbsolutePose[b[4]].t - pAbsolutePose[b[1]].t;
		La = Va.GetLength();
		Lb = Vb.GetLength();
		Lc = Vc.GetLength();

		// Pre-calculation
		f32 abSqrt = La*La + Lb*Lb;
		f32 ab2 = 2*La*Lb;

		if(ab2 < FLT_EPSILON)
			return;

		f32 alpha = acos( clamp( (abSqrt - Lc*Lc)/ab2, -1.0f, 1.0f ) );
		f32 alpha2 = acos( clamp( (abSqrt - Lc2*Lc2)/ab2, -1.0f, 1.0f ) );
		f32 dAlpha = alpha2 - alpha;

		V1 = pAbsolutePose[b[2]].t - pAbsolutePose[b[1]].t;
		V2 = pAbsolutePose[b[3]].t - pAbsolutePose[b[2]].t;
		V3 = pAbsolutePose[b[4]].t - pAbsolutePose[b[3]].t;

		Vec3 vK = Va%V3;
		vK.Normalize();
		f32 p = vK|axisZ;
		if( p < 0.0f)
			dAlpha = -(2*gf_PI - alpha - alpha2);

		pAbsolutePose[b[3]].q = Quat::CreateRotationAA(-dAlpha, axisZ) * pAbsolutePose[b[3]].q;
		pAbsolutePose[b[3]].q.Normalize();

		pRelativePose[b[3]].q = !pAbsolutePose[b[2]].q * pAbsolutePose[b[3]].q;
		pRelativePose[b[3]].q.Normalize();
		pAbsolutePose[b[4]] = pAbsolutePose[b[3]] * pRelativePose[b[4]];

		Vec3 V14 = pAbsolutePose[b[4]].t - pAbsolutePose[b[1]].t;
		f32 LV14 = V14.GetLength();

		f32 cosBeta = clamp((Vc2|V14)/(Lc2*LV14), -1.0f, 1.0f );
		f32 sinBetaHalf = sqrt((1-cosBeta)/2.0f);
		f32 cosBetaHalf = sqrt( max(1.0f-sinBetaHalf*sinBetaHalf,0.0f) );
	
		Vec3 axisS = V14 % Vc2;
		axisS.Normalize();

		pAbsolutePose[b[1]].q = Quat::CreateRotationAA(cosBetaHalf, sinBetaHalf, axisS) * pAbsolutePose[b[1]].q;
		pRelativePose[b[1]].q = !pAbsolutePose[b[0]].q * pAbsolutePose[b[1]].q;
		pRelativePose[b[1]].q.Normalize();

		// Update chain
		for(int32 i=2; i<=4; ++i)
			pAbsolutePose[b[i]] = pAbsolutePose[b[i-1]] * pRelativePose[b[i]];
	}

	void IK_Solver3Bones_Ivo( const Vec3r& goal, const IKLimbType& rIKLimbType,QuatT* const __restrict pRelativePose,QuatT* const __restrict pAbsolutePose)
	{
		int32 b0 = rIKLimbType.m_arrJointChain[0].m_idxJoint; assert(b0>0); //BaseRoot
		int32 b1 = rIKLimbType.m_arrJointChain[1].m_idxJoint; assert(b1>0); //Root of Chain
		int32 b2 = rIKLimbType.m_arrJointChain[2].m_idxJoint; assert(b2>0); //Middle Joint 
		int32 b3 = rIKLimbType.m_arrJointChain[3].m_idxJoint; assert(b3>0); //Middle Joint 
		int32 b4 = rIKLimbType.m_arrJointChain[4].m_idxJoint; assert(b4>0); //EndEffector

		uint32 numLinks = rIKLimbType.m_arrRootToEndEffector.size();
		pAbsolutePose[0]	= pRelativePose[0];
		for (uint32 i=1; i<numLinks; i++)
		{
			int32 p=rIKLimbType.m_arrRootToEndEffector[i-1];
			int32 c=rIKLimbType.m_arrRootToEndEffector[i];
			pAbsolutePose[c]	= pAbsolutePose[p] * pRelativePose[c];
		}

		Vec3r t1 = pAbsolutePose[b1].t;
		Vec3r t2 = pAbsolutePose[b2].t;
		Vec3r t4 = pAbsolutePose[b4].t;
		if ( ((goal-t4)|(goal-t4)) < real(0.00001) )
			return; //end-effector and new goal are very close

		Vec3r h,axis;
		Vec3r a=t2-t1;		real alen=a.GetLength();
		Vec3r b=t4-t2;		real blen=b.GetLength();
		Vec3r c=goal-t1;	real clen=c.GetLength();
		if (alen*blen*clen < real(0.000001) )
			return;  //check for invalid configurations / broken bones

		f64 ration = (t1-t4).GetLength()/clen-real(1.0);
		if (ration<0)	ration*=3.5f;  //open

		Vec3r zaxis = pRelativePose[b3].q.GetColumn2();
		Quat zrot; zrot.SetRotationAA(clamp(f32(ration),-1.1f,+0.4f),zaxis); 	
		pRelativePose[b3].q = pRelativePose[b3].q * zrot;
		pAbsolutePose[b3]  = pAbsolutePose[b2] * pRelativePose[b3];
		pAbsolutePose[b4]  = pAbsolutePose[b3] * pRelativePose[b4];

		t1 = pAbsolutePose[b1].t;
		t2 = pAbsolutePose[b2].t;
		t4 = pAbsolutePose[b4].t;
		a=t2-t1;		alen=a.GetLength();
		b=t4-t2;		blen=b.GetLength();
		c=goal-t1;	clen=c.GetLength();
		Vec3r cross=a%b;
		if ((cross|cross) < real(0.0000001) )
			return;
		cross = cross.GetNormalizedSafe(-pAbsolutePose[b2].q.GetColumn2());

		real cos0	= clamp(	(real)((alen*alen+clen*clen-blen*blen)/((2.0)*alen*clen)) ,real(-1.0),real(+1.0) );
		real sin0	=	sqrt( max(real(1.0)-cos0*cos0,real(0.0)) );

		a/=alen;
		a = a.GetRotated( cross, cos0,sin0 );
		assert((fabs_tpl(1-(a|a))) < real(0.00001) );   //check if unit-vector

		c/=clen;
		h=(a+c).GetNormalized();
		axis=a%c;	axis/=max(axis.GetLength(),real(0.000001));
		pAbsolutePose[b1].q = Quat(f32(a|h), axis*(a%h).GetLength()) * pAbsolutePose[b1].q;
		pRelativePose[b1].q = !pAbsolutePose[b0].q * pAbsolutePose[b1].q;
		pRelativePose[b1].q.Normalize();
		pAbsolutePose[b2]  = pAbsolutePose[b1] * pRelativePose[b2];
		pAbsolutePose[b3]  = pAbsolutePose[b2] * pRelativePose[b3];
		pAbsolutePose[b4]  = pAbsolutePose[b3] * pRelativePose[b4];
		//---------------------------------------------------
		c = (goal-pAbsolutePose[b2].t).GetNormalized(); 
		b = Vec3r(pAbsolutePose[b4].t-pAbsolutePose[b2].t)/blen;
		h = (b+c).GetNormalized();
		axis=b%c;	axis/=max(axis.GetLength(),real(0.000001));
		pAbsolutePose[b2].q = Quat(f32(b|h),axis*(b%h).GetLength())*pAbsolutePose[b2].q;
		pRelativePose[b2].q = !pAbsolutePose[b1].q * pAbsolutePose[b2].q;
		pRelativePose[b2].q.Normalize();
	}

	//----------------------------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------------
	void IK_SolverCCD(const Vec3& vTarget, const IKLimbType& rIKLimbType,QuatT* const __restrict pRelativePose,QuatT* const __restrict	pAbsolutePose)
	{

		uint32 numR2E = rIKLimbType.m_arrRootToEndEffector.size();
		pAbsolutePose[0]	= pRelativePose[0];
		for (uint32 i=1; i<numR2E; i++)
		{
			int32 p=rIKLimbType.m_arrRootToEndEffector[i-1];
			int32 c=rIKLimbType.m_arrRootToEndEffector[i];
			pAbsolutePose[c]	= pAbsolutePose[p] * pRelativePose[c];
		}

		f32 fTransCompensation=0;
		uint32 numLinks		= rIKLimbType.m_arrJointChain.size();
		for (uint32 i=2; i<numLinks; i++)
		{
			int32 p = rIKLimbType.m_arrJointChain[i-1].m_idxJoint;
			int32 c = rIKLimbType.m_arrJointChain[i].m_idxJoint;
			fTransCompensation += (pAbsolutePose[c].t-pAbsolutePose[p].t).GetLengthFast();
		}

		f32 inumLinks			= 1.0f/f32(numLinks);
		int32 nRootIdx		=	rIKLimbType.m_arrJointChain[1].m_idxJoint;  //Root
		int32 nEndEffIdx	=	rIKLimbType.m_arrJointChain[numLinks-1].m_idxJoint; //EndEffector
		assert(nRootIdx<nEndEffIdx);
		int32	iJointIterator	= 1;  //numLinks-2;

		// Cyclic Coordinate Descent
		for (uint32 i=0; i<rIKLimbType.m_nInterations; i++)
		{
			Vec3	vecEnd				=	pAbsolutePose[nEndEffIdx].t;			// Position of end effector
			int32 parentLinkIdx	= rIKLimbType.m_arrJointChain[iJointIterator-1].m_idxJoint; assert(parentLinkIdx>=0);
			int32 LinkIdx				= rIKLimbType.m_arrJointChain[iJointIterator].m_idxJoint;
			Vec3	vecLink				=	pAbsolutePose[LinkIdx].t;	// Position of current node
			Vec3	vecLinkTarget	= (vTarget-vecLink).GetNormalized();					// Vector current link -> target
			Vec3	vecLinkEnd		= (vecEnd-vecLink).GetNormalized();								// Vector current link -> current effector position

			Quat qrel; qrel.SetRotationV0V1(vecLinkEnd,vecLinkTarget); 
			assert((fabs_tpl(1-(qrel|qrel)))<0.001);   //check if unit-quaternion
			if (qrel.w<0) qrel.w = -qrel.w; 
			
			f32 ji = iJointIterator*inumLinks+0.3f;
			f32 t	=	min(rIKLimbType.m_fStepSize*ji,0.4f);
			qrel.w*=t+1.0f-t; qrel.v*=t;	qrel.Normalize();

			//calculate new relative IK-orientation 
			pRelativePose[LinkIdx].q = !pAbsolutePose[parentLinkIdx].q * qrel * pAbsolutePose[LinkIdx].q;
			pRelativePose[LinkIdx].q.Normalize();

			//calculate new absolute IK-orientation 
			for (uint32 j=iJointIterator; j<numLinks; j++)
			{
				int32 c = rIKLimbType.m_arrJointChain[j].m_idxJoint;
				int32 p = rIKLimbType.m_arrJointChain[j-1].m_idxJoint;	assert(p>=0);
				pAbsolutePose[c]= pAbsolutePose[p]*pRelativePose[c];
			}

			f32	fError	= (pAbsolutePose[nEndEffIdx].t-vTarget).GetLength();
			/*
			static Ang3 angle=Ang3(ZERO);
			angle.x += 0.1f;
			angle.y += 0.01f;
			angle.z += 0.001f;
			AABB sAABB = AABB(Vec3(-0.1f,-0.1f,-0.1f),Vec3(+0.1f,+0.1f,+0.1f));
			OBB obb =	OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle), sAABB );
			g_pAuxGeom->DrawOBB(obb,pAbsolutePose[nEndEffIdx].t,1,RGBA8(0xff,0x00,0x00,0xff),eBBD_Extremes_Color_Encoded);
			*/

			//	float fColor[4] = {0,1,0,1};
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"nUpdateSteps: %d   fError: %f  iJointIterator: %d  ji: %f  t: %f",i,fError,iJointIterator,ji,t );	
			//	g_YLine+=12.0f;

			if (fError < rIKLimbType.m_fThreshold) break; 

			iJointIterator++;	//Next link
			if( iJointIterator > (numLinks-3) )				
				iJointIterator = 1;
		}

		//	float fColor[4] = {0,1,0,1};
		//	f32	fError1	= (pAbsolutePose[nEndEffIdx].t-vTarget).GetLength();		
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"fError1: %f",fError1);	
		//	g_YLine+=12.0f;

		//-----------------------------------------------------------------------------
		//--do a cheap translation compensation zo fix the error
		//-----------------------------------------------------------------------------
		Vec3 absEndEffector =	pAbsolutePose[nEndEffIdx].t;			// Position of end effector
		Vec3 vDistance	= (vTarget-absEndEffector); //f32(numCCDJoints);
		f32 fDistance = vDistance.GetLengthFast();
		if (fDistance>fTransCompensation)
			vDistance*=fTransCompensation/fDistance;
		Vec3 bPartDistance	=	vDistance/f32(numLinks-0);
		Vec3 vAddDistance =	bPartDistance;

		for (int32 i=1; i<numLinks; i++)
		{
			int c = rIKLimbType.m_arrJointChain[i].m_idxJoint;
			int p = rIKLimbType.m_arrJointChain[i-1].m_idxJoint; assert(p>=0);
			pAbsolutePose[c].t += vAddDistance;
			vAddDistance += bPartDistance;

			pRelativePose[c] = pAbsolutePose[p].GetInverted() * pAbsolutePose[c];
		}

		//	f32	fError2	= (pAbsolutePose[nEndEffIdx].t-vTarget).GetLength();	
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"fError2: %f",fError2);	
		//	g_YLine+=12.0f;
	}




} // namespace PoseModifierHelper
