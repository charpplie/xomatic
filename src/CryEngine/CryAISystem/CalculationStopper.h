/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   Shape2.h
$Id$
Description: Temp file holding code extracted from CAISystem.h/cpp

-------------------------------------------------------------------------
History:

*********************************************************************/

#ifndef _CALCULATIONSHOPPER_H_
#define _CALCULATIONSHOPPER_H_



//#define CALIBRATE_STOPPER

/// Used to determine when a calculation should stop. Normally this would be after a certain time (dt). However, 
/// if m_useCounter has been set then the times are internally converted to calls to ShouldCalculationStop using
/// callRate, which should have been estimated previously
class CCalculationStopper
{
public:
	/// name is used during calibration. dt is the amount of time (seconds).
	/// callRate is for when we are running in "counter" mode - the time
	/// gets converted into a number of calls to ShouldCalculationStop
	CCalculationStopper(const char * name, float dt, float callRate);
	bool ShouldCalculationStop() const;
	float GetMilliSecondsRemaining() const;

	static bool m_useCounter;
private:
	CTimeValue m_endTime;
	mutable unsigned m_stopCounter; // if > 0 use this - stop when it's 0
	static bool m_neverStop; // for debugging
	float m_callRate;
#ifdef CALIBRATE_STOPPER
public:
	mutable unsigned m_calls;
	float m_dt;
	string m_name;
	/// the pair is calls and time (seconds)
	typedef std::map< string, std::pair<unsigned, float> > TMapCallRate;
	static TMapCallRate m_mapCallRate;
#endif
};




#endif