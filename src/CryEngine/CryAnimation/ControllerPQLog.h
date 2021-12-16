/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	Created by Ivo Herzeg
//	
//  Notes:
//    CControllerPQLog class declaration
//    CControllerPQLog is implementation of IController which is compatible with
//    the old (before 6/27/02) caf file format that contained only CryBoneKey keys.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CRYTEK_CONTROLLER_PQLOG_
#define _CRYTEK_CONTROLLER_PQLOG_

#include "CGFContent.h"




// old motion format cry bone controller
class CControllerPQLog: public IController
{
public:

	CControllerPQLog();
	~CControllerPQLog();

	uint32 numKeysPQ() const
	{
		return m_arrKeys.size();
	}
	
	virtual EControllerInfo GetControllerType(); 


	QuatT DecodeKey( f32 fTime);

	Status4 GetOPS ( f32 normalized_time, Quat& quat, Vec3& pos, Diag33& scale);
	Status4 GetOP ( f32 normalized_time, Quat& quat, Vec3& pos);
	uint32 GetO ( f32 normalized_time, Quat& quat);
	uint32 GetP ( f32 normalized_time, Vec3& pos);
	uint32 GetS ( f32 normalized_time, Diag33& pos);

	QuatT GetValueByKey(uint32 key);

	QuatT GetKey0()
	{
		uint32 numKey = numKeysPQ();
		assert(numKey);
		PQLog pq = m_arrKeys[0];
		return QuatT(!exp(pq.vRotLog),pq.vPos);
	}

	int32 GetO_numKey()
	{
		uint32 numKey = numKeysPQ();
		return numKey;
	}
	int32 GetP_numKey()
	{
		uint32 numKey = numKeysPQ();
		return numKey;
	}

	uint32 HasPositionChannel()
	{
		return 0;
	}










	// returns the start time
	f32 GetTimeStart ()
	{
		return f32(m_arrTimes[0]);
	}

	// returns the end time
	f32 GetTimeEnd()
	{
		assert (numKeysPQ() > 0);
		return f32(m_arrTimes[numKeysPQ()-1]);
	}


	size_t SizeOfController()const
	{
		size_t s0=sizeof(*this);
		size_t s1=m_arrKeys.get_alloc_size();
		size_t s2=m_arrTimes.get_alloc_size();
		return s0+s1+s2;
	}


	size_t ApproximateSizeOfThis() const { return SizeOfController(); }


	CInfo GetControllerInfo() const
	{
		CInfo info;
		info.m_numKeys		= m_arrKeys.size();
		info.m_nStartKey	= m_arrTimes[0];
		info.m_nEndKey			= m_arrTimes[info.m_numKeys-1];
		info.m_TotalKeys	=	(info.m_nEndKey-info.m_nStartKey)/TICKS_PER_FRAME+1;
		return info;
	}


	void SetControllerData( const DynArray<PQLog>& arrKeys, const DynArray<int>& arrTimes )
	{
		m_arrKeys	=arrKeys;
		m_arrTimes=arrTimes;
	}

	size_t GetRotationKeysNum() 
	{
		return 0;
	}

	size_t GetPositionKeysNum()
	{
		return 0;
	}

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_arrKeys);
		pSizer->AddObject(m_arrTimes);
	}
//--------------------------------------------------------------------------------------------------

//protected:
	DynArray<PQLog> m_arrKeys;
	DynArray<int> m_arrTimes;

	float m_lastTime;
	QuatT m_lastValue;

	float m_lastTimeLM;
	QuatT m_lastValueLM;

//	unsigned m_nControllerId;
};


TYPEDEF_AUTOPTR(CControllerPQLog);

#endif

