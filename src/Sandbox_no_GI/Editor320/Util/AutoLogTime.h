#ifndef AutoLogTime_h__
#define AutoLogTime_h__

#pragma once

class CAutoLogTime
{
public:
	CAutoLogTime( const char *what );
	~CAutoLogTime();
private:
	const char *m_what;
	int m_t0,m_t1;
};

#endif // AutoLogTime_h__
