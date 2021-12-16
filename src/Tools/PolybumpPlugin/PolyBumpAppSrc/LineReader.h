#pragma once

#include <string>			// STL string
#include <vector>			// STL vector<>
#include "../types.h"		// uint32

class CLineReader
{
public:
	CLineReader( const char *szFileName );
	virtual ~CLineReader();

	//! \param szLine[dwSize]
	//! \return false=file end outLine=""
	bool GetLine( unsigned char szLine[], const uint32 dwSize );

	bool IsValid() const;

	//! try to avoid this call
	void Restart();

	uint32 ComputePercent();

	// -------------------------------------------------------

	FILE *							m_Handle;						//!<
	std::vector<char>		m_Buffer;						//!<
	size_t							m_Pos;							//!<
	size_t							m_Size;							//!<
	uint32							m_dwStartTime;			//!< timeGetTime()
	uint32							m_dwCursorInBuffer;	//!<
};
