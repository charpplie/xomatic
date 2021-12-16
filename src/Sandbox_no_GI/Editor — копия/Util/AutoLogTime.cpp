#include <stdafx.h>
#include "AutoLogTime.h"

CAutoLogTime::CAutoLogTime( const char *what )
{
	m_what = what;
	CLogFile::FormatLine( "---- Start: %s",m_what );
	m_t0 = GetTickCount();
}

CAutoLogTime::~CAutoLogTime()
{
	m_t1 = GetTickCount();
	CLogFile::FormatLine( "---- End: %s (%d seconds)",m_what,(m_t1-m_t0)/1000 );
}
