#include "stdafx.h"
//#include "AnimationBase.h"

#include "AnimationCompiler.h"
#include "AnimationLoader.h"
#include "ControllerPQLog.h"
#include "AnimationManager.h"

#include <float.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CControllerPQLog::CControllerPQLog(GlobalAnimationHeaderCAF * header) : IController(header),  m_nControllerId(0)
{
	m_lastTime = -FLT_MAX;
	m_lastTimeLM = -FLT_MAX;
}

CControllerPQLog::~CControllerPQLog() { }






inline double DLength( Vec3 &v ) { 
	double dx=v.x*v.x;	
	double dy=v.y*v.y;	
	double dz=v.z*v.z;	
	return sqrt( dx + dy + dz );
}


// blends the pqFrom and pqTo with weights 1-fBlend and fBlend into pqResult
void PQLog::blendPQ (const PQLog& pqFrom, const PQLog& pqTo, f32 fBlend)
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
void AdjustLogRotations (Vec3& vRotLog1, Vec3& vRotLog2)
{
	double dLen1 = DLength(vRotLog1);
	if (dLen1 > gf_PI/2)
	{
		vRotLog1 *= (f32)(1.0f - gf_PI/dLen1); 
		// now the length of the first vector is actually gf_PI - dLen1,
		// and it's closer to the origin than its complementary
		// but we won't need the dLen1 any more
	}

	double dLen2 = DLength(vRotLog2);
	// if the flipped 2nd rotation vector is closer to the first rotation vector,
	// then flip the second vector
	if ((vRotLog1|vRotLog2) < (dLen2 - gf_PI/2) * dLen2)
	{
		// flip the second rotation also
		vRotLog2 *= (f32)(1.0f - gf_PI / dLen2);
	}
}







//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

Status4 CControllerPQLog::GetOPS ( f32 realtime, Quat& quat, Vec3& pos, Diag33& scale)
{
	QuatT pq=	GetValue3(realtime);
	quat	=	pq.q;
	pos		=	pq.t;
	return Status4(1,1,0,0); 
}

Status4 CControllerPQLog::GetOP ( f32 realtime, Quat& quat, Vec3& pos)
{
	QuatT pq=	GetValue3(realtime);
	quat	=	pq.q;
	pos		=	pq.t;
	return Status4(1,1,0,0); 
}

uint32 CControllerPQLog::GetO ( f32 realtime, Quat& quat)
{
	QuatT pq=	GetValue3(realtime);
	quat=pq.q;
	return 1;
}
uint32 CControllerPQLog::GetP ( f32 realtime, Vec3& pos)
{
	QuatT pq=	GetValue3(realtime);
	pos=pq.t;
	return 1;
}
uint32 CControllerPQLog::GetS ( f32 realtime, Diag33& pos)
{
	return 0;
}



//////////////////////////////////////////////////////////////////////////
// retrieves the position and orientation (in the logarithmic space,
// i.e. instead of quaternion, its logarithm is returned)
// may be optimal for motion interpolation
QuatT CControllerPQLog::GetValue3 (f32 fTime)
{
#ifdef DEFINE_PROFILER_FUNCTION
	DEFINE_PROFILER_FUNCTION();
#endif

	/*
	if (fTime == m_lastTime)
	{
		return m_lastValue;
	}*/
	m_lastTime = fTime;

	PQLog pq;
	assert(numKeys());

	uint32 numKey = numKeys()-1;
	f32 keytime_start = (f32)m_arrTimes[0];
	f32 keytime_end = (f32)m_arrTimes[numKey];

	f32 test_end = keytime_end;
	if( fTime < keytime_start )
		test_end += fTime;

	if( fTime < keytime_start )
	{
		pq = m_arrKeys[0];
		m_lastValue = QuatT(!exp(pq.vRotLog),pq.vPos);
		return m_lastValue;
	}

	if( fTime >= keytime_end )
	{
		pq = m_arrKeys[numKey];
		m_lastValue = QuatT(!exp(pq.vRotLog),pq.vPos);
		return m_lastValue;
	}

	uint32 nK = numKeys();
	assert(numKeys()>1);

	int nPos  = numKeys()>>1;
	int nStep = numKeys()>>2;

	// use binary search
	while(nStep)
	{
		if(fTime < m_arrTimes[nPos])
			nPos = nPos - nStep;
		else
			if(fTime > m_arrTimes[nPos])
				nPos = nPos + nStep;
			else 
				break;

		nStep = nStep>>1;
	}

	// fine-tuning needed since time is not linear
	while(fTime > m_arrTimes[nPos])
		nPos++;

	while(fTime < m_arrTimes[nPos-1])
		nPos--;

	assert(nPos > 0 && nPos < (int)numKeys());  
//	assert(m_arrTimes[nPos] != m_arrTimes[nPos-1]);

	f32 t;

	if (m_arrTimes[nPos] != m_arrTimes[nPos-1])
		t = (f32(fTime-m_arrTimes[nPos-1])) / (m_arrTimes[nPos] - m_arrTimes[nPos-1]);
	else
		t = 0;
	PQLog pKeys[2] = { m_arrKeys[nPos-1],m_arrKeys[nPos] };
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
	uint32 numKey = numKeys();
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



size_t CControllerPQLog::SizeOfThis ()const
{
	return sizeof(*this) + m_arrKeys.size() * (sizeof(m_arrKeys[0])+sizeof(m_arrTimes[0]));
}


