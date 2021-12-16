#include "StdAfx.h"
#include "CalculationStopper.h"

bool CCalculationStopper::m_neverStop = false;
bool CCalculationStopper::m_useCounter;

#ifdef CALIBRATE_STOPPER
std::map< string, std::pair<unsigned, float> > CCalculationStopper::m_mapCallRate;
#endif

//===================================================================
// CCalculationStopper
//===================================================================
CCalculationStopper::CCalculationStopper(const char * name, float dt, float callRate) 
: m_stopCounter(0),  m_callRate(callRate)
{
	m_endTime = gEnv->pTimer->GetAsyncTime() + CTimeValue(dt);
	if (m_useCounter)
	{
		m_stopCounter = (unsigned)(dt * callRate);
		if (m_stopCounter < 1)
			m_stopCounter = 1;
	}
#ifdef CALIBRATE_STOPPER
	m_name = name;
	m_calls = 0;
	m_dt = dt;
#endif
}

bool CCalculationStopper::ShouldCalculationStop() const
{
	if (m_neverStop)
		return false;
	if (m_useCounter)
	{
		if (m_stopCounter > 0) --m_stopCounter;
		return m_stopCounter == 0;
	}
	else
	{
#ifdef CALIBRATE_STOPPER
		++m_calls;
		if (gEnv->pTimer->GetAsyncTime() > m_endTime)
		{
			std::pair<unsigned, float> &record = m_mapCallRate[m_name];
			record.first += m_calls;
			record.second += m_dt;
			return true;
		}
		else
		{
			return false;
		}
#else
		return gEnv->pTimer->GetAsyncTime() > m_endTime;
#endif
	}
}

float CCalculationStopper::GetMilliSecondsRemaining() const
{
	if (m_useCounter) return 1000 * m_stopCounter / m_callRate; else return (m_endTime - gEnv->pTimer->GetAsyncTime()).GetMilliSeconds();
}