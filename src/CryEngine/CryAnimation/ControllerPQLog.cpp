#include "stdafx.h"
#include "CharacterManager.h"
#include <SpuUtils.h>
#include <float.h>


CControllerPQLog::CControllerPQLog()//:	m_nControllerId(0)
{
	m_lastTime = -FLT_MAX;
	m_lastTimeLM = -FLT_MAX;
}

CControllerPQLog::~CControllerPQLog() { }






inline real DLength( Vec3 &v ) { 
	real dx=v.x*v.x;	
	real dy=v.y*v.y;	
	real dz=v.z*v.z;	
	return sqrt( dx + dy + dz );
}


// blends the pqFrom and pqTo with weights 1-fBlend and fBlend into pqResult
SPU_NO_INLINE void PQLog::blendPQ (const PQLog& pqFrom, const PQLog& pqTo, f32 fBlend)
{
	assert (fBlend >= 0 && fBlend <=1);
	vPos = pqFrom.vPos * (1-fBlend) + pqTo.vPos * fBlend;
	Quat qFrom = exp(pqFrom.vRotLog);
	Quat qTo   = exp(pqTo.vRotLog);
	qFrom = Quat::CreateNlerp(qFrom,qTo,fBlend);
	vRotLog = log(qFrom);
}




// adjusts the rotation of these PQs: if necessary, flips them or one of them (effectively NOT changing the whole rotation,but
// changing the rotation angle to Pi-X and flipping the rotation axis simultaneously)
// this is needed for blending between animations represented by quaternions rotated by ~PI in quaternion space
// (and thus ~2*PI in real space)
SPU_NO_INLINE void AdjustLogRotations (Vec3& vRotLog1, Vec3& vRotLog2)
{
	real dLen1 = DLength(vRotLog1);
	if (dLen1 > gf_PI/2)
	{
		vRotLog1 *= (f32)(1.0f - gf_PI/dLen1); 
		// now the length of the first vector is actually gf_PI - dLen1,
		// and it's closer to the origin than its complementary
		// but we won't need the dLen1 any more
	}

	real dLen2 = DLength(vRotLog2);
	// if the flipped 2nd rotation vector is closer to the first rotation vector,
	// then flip the second vector
	if ((vRotLog1|vRotLog2) < (dLen2 - gf_PI/2) * dLen2 && dLen2 > 0.000001f)
	{
		// flip the second rotation also
		vRotLog2 *= (f32)(1.0f - gf_PI / dLen2);
	}
}









//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

Status4 CControllerPQLog::GetOPS ( f32 ntime, Quat& quat, Vec3& pos, Diag33& scale)
{
	QuatT pq=	DecodeKey( ntime);
	quat	=	pq.q;
	pos		=	pq.t;
	return Status4(1,1,0,1); 
}

SPU_INDIRECT(CommandBufferExecute(MLL))
Status4 CControllerPQLog::GetOP( f32 ntime, Quat& quat, Vec3& pos)
{
	//scaling is NOT supported by physics
	QuatT pq=	DecodeKey( ntime);
	quat	=	pq.q;
	pos		=	pq.t;
	return Status4(1,1,0,1); 
}

SPU_INDIRECT(CommandBufferExecute(ML))
uint32 CControllerPQLog::GetO ( f32 ntime, Quat& quat)
{
	QuatT pq=	DecodeKey(ntime);
	quat=pq.q;
	return 1;
}
uint32 CControllerPQLog::GetP( f32 ntime, Vec3& pos)
{
	QuatT pq=	DecodeKey(ntime);
	pos=pq.t;
	return 1;
}
uint32 CControllerPQLog::GetS( f32 normalized_time, Diag33& pos)
{
	return 0;
}



//////////////////////////////////////////////////////////////////////////
// retrieves the position and orientation (in the logarithmic space,
// i.e. instead of quaternion, its logarithm is returned)
// may be optimal for motion interpolation
QuatT CControllerPQLog::DecodeKey ( f32 ntime)
{
#ifdef DEFINE_PROFILER_FUNCTION
	DEFINE_PROFILER_FUNCTION();
#endif
	f32 realtime = ntime;

	int nArrTimesSize = m_arrTimes.size();
	int nNumKeysPQ = numKeysPQ();
	
	// for SPUs use a stack array to speed up computation time
#if defined(__SPU__)
	int * __restrict stackArrTimes = SPU_LOCAL_PTR( (int*)alloca(nArrTimesSize*sizeof(int) ) );
	memcpy( stackArrTimes, &m_arrTimes[0], nArrTimesSize*sizeof(int) );
#endif

	// use local variables to spare checking this everytime(optimization for SPU simulation time)
	PQLog * __restrict arrKeys  = &m_arrKeys[0];
	int*		__restrict arrTimes = SPU_PTR_SELECT( &m_arrTimes[0], stackArrTimes );
	float & RESTRICT_REFERENCE rLastTime = m_lastTime;

	if (realtime == m_lastTime)
	{
		return m_lastValue;
	}
	rLastTime = realtime;

	PQLog pq;
	assert(numKeysPQ());

	uint32 numKey = numKeysPQ()-1;
	f32 keytime_start = (f32)arrTimes[0];
	f32 keytime_end = (f32)arrTimes[numKey];

	f32 test_end = keytime_end;
	if( realtime < keytime_start )
		test_end += realtime;

	if( realtime < keytime_start )
	{
		pq = arrKeys[0];
		m_lastValue = QuatT(!exp(pq.vRotLog),pq.vPos);
		return m_lastValue;
	}

	if( realtime >= keytime_end )
	{
		pq = arrKeys[numKey];
		m_lastValue = QuatT(!exp(pq.vRotLog),pq.vPos);
		return m_lastValue;
	}

	uint32 nK = nNumKeysPQ;
	assert(numKeysPQ()>1);	

	int nPos  = nNumKeysPQ>>1;
	int nStep = nNumKeysPQ>>2;

	// use binary search
	while(nStep)
	{
		if(realtime < arrTimes[nPos])
			nPos = nPos - nStep;
		else
			if(realtime > arrTimes[nPos])
				nPos = nPos + nStep;
			else 
				break;

		nStep = nStep>>1;
	}

	// fine-tuning needed since time is not linear
	while(realtime > arrTimes[nPos])
		nPos++;

	while(realtime < arrTimes[nPos-1])
		nPos--;

	assert(nPos > 0 && nPos < (int)nNumKeysPQ);  
	int32 k0=arrTimes[nPos];
	int32 k1=arrTimes[nPos-1];
//	assert(arrTimes[nPos] != arrTimes[nPos-1]);
	if (k0==k1)
		return QuatT(!exp(arrKeys[nPos].vRotLog),arrKeys[nPos].vPos);

	f32 t = (f32(realtime-arrTimes[nPos-1])) / (arrTimes[nPos] - arrTimes[nPos-1]);
	PQLog pKeys[2] = { arrKeys[nPos-1],arrKeys[nPos] };
	AdjustLogRotations (pKeys[0].vRotLog, pKeys[1].vRotLog);
	pq.blendPQ (pKeys[0],pKeys[1], t);

	m_lastValue = QuatT(!exp(pq.vRotLog),pq.vPos);
	return m_lastValue;
}




//////////////////////////////////////////////////////////////////////////
// retrieves the position and orientation (in the logarithmic space,
// i.e. instead of quaternion, its logarithm is returned)
// may be optimal for motion interpolation
QuatT CControllerPQLog::GetValueByKey(uint32 key)
{
	PQLog pq;
	uint32 numKey = numKeysPQ();
//	assert(numKey==9);
//	assert(numKey==0x11);
	assert(numKey);
	if( key >= numKey )
	{
		pq = m_arrKeys[numKey-1];
		return QuatT(!exp(pq.vRotLog),pq.vPos);
	}
	pq = m_arrKeys[key];
	return QuatT(!exp(pq.vRotLog),pq.vPos);
}




SPU_INDIRECT(CommandBufferExecute(M))
EControllerInfo CControllerPQLog::GetControllerType()
{
	return ePQLog;
}

