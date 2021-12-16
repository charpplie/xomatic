/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	Created by Sergiy Migdalskiy
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
#include "Controller.h"


// old motion format cry bone controller
class CControllerPQLog: public IController
{
public:

	CControllerPQLog(GlobalAnimationHeaderCAF * header);

	CControllerPQLog()
	{
		m_lastTime = -FLT_MAX;
		m_lastTimeLM = -FLT_MAX;

	}

	~CControllerPQLog();

	uint32 numKeys() const
	{
		return m_arrKeys.size();
	}

	uint32 GetID () const {return m_nControllerId;}

	QuatT GetValue3 (f32 fTime);


	Status4 GetOPS ( f32 realtime, Quat& quat, Vec3& pos, Diag33& scale);
	Status4 GetOP  ( f32 realtime, Quat& quat, Vec3& pos);
	uint32 GetO    ( f32 realtime, Quat& quat);
	uint32 GetP    ( f32 realtime, Vec3& pos);
	uint32 GetS    ( f32 realtime, Diag33& pos);

	QuatT GetValueByKey(uint32 key);


	int32 GetO_numKey()
	{
		uint32 numKey = numKeys();
		return numKey;
	}
	int32 GetP_numKey()
	{
		uint32 numKey = numKeys();
		return numKey;
	}


	CInfo GetControllerInfoRoot()
	{
		CInfo info;
		info.numKeys = m_arrKeys.size();
		info.quat	   = !exp(m_arrKeys[info.numKeys-1].vRotLog);
		info.pos		 = m_arrKeys[info.numKeys-1].vPos;
		info.etime	 = m_arrTimes[info.numKeys-1];
		return info;
	}








	// returns the start time
	f32 GetTimeStart ()
	{
		return f32(m_arrTimes[0]);
	}

	// returns the end time
	f32 GetTimeEnd()
	{
		assert (numKeys() > 0);
		return f32(m_arrTimes[numKeys()-1]);
	}


	size_t SizeOfThis ()const;


	CInfo GetControllerInfo() const
	{
		CInfo info;
		info.numKeys = m_arrKeys.size();
		info.quat	   = !exp(m_arrKeys[info.numKeys-1].vRotLog);
		info.pos		 = m_arrKeys[info.numKeys-1].vPos;
		info.stime		= m_arrTimes[0];
		info.etime		= m_arrTimes[info.numKeys-1];
		info.realkeys =	(info.etime-info.stime)/TICKS_PER_FRAME+1;
		return info;
	}
	/*	
	void GetControllerData( std::vector<PQLog>& arrKeys, std::vector<int>& arrTimes,uint32 &nControllerId ) const
	{
	arrKeys				=	m_arrKeys;
	arrTimes			=	m_arrTimes;
	nControllerId	=	m_nControllerId;
	}*/

	void SetControllerData( const std::vector<PQLog>& arrKeys, const std::vector<int>& arrTimes )
	{
		m_arrKeys	=arrKeys;
		m_arrTimes=arrTimes;
	}

//--------------------------------------------------------------------------------------------------

//protected:
	std::vector<PQLog> m_arrKeys;
	std::vector<int> m_arrTimes;

	float m_lastTime;
	QuatT m_lastValue;

	float m_lastTimeLM;
	QuatT m_lastValueLM;

	unsigned m_nControllerId;
};


TYPEDEF_AUTOPTR(CControllerPQLog);

#endif

