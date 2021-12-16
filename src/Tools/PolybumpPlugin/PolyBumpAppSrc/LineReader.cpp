#include "StdAfx.h"
#include "linereader.h"
#include <fcntl.h>														// filesize
#include <io.h>																// filesize
#include <mmsystem.h>													// timeGetTime()

CLineReader::CLineReader( const char *szFileName )
{
	m_dwCursorInBuffer=0;
	m_Pos=0;
	m_Handle = fopen(szFileName,"rbS");

	m_dwStartTime = timeGetTime();

	if(m_Handle)
	{
//		m_Buffer.resize(4*1024*1024);		// 4MB
//		setvbuf(m_Handle,&m_Buffer[0],_IOFBF,m_Buffer.size());

		{
			int handle;

			handle=open(szFileName,O_RDONLY);

			if(handle!=-1)
				m_Size=filelength(handle);

			close(handle);
		}
	}
}

CLineReader::~CLineReader()
{
	if(m_Handle)
		fclose(m_Handle);


	uint32 dwTime = timeGetTime()-m_dwStartTime;

	if(dwTime)
	{
		char str[256];

		// e.g.
		// CLineReader stats: 259 sec, 366451 KB, 1.376892 MB/sec - fgetc()
		// CLineReader stats: 81 sec, 366451 KB, 4.394295 MB/sec
		// CLineReader stats: 114 sec, 366451 KB, 3.134828 MB/sec
		// CLineReader stats: 65 sec, 366451 KB, 5.464803 MB/sec - without normal gen - set to default
		// CLineReader stats: 71 sec, 366451 KB, 5.008294 MB/sec
		// CLineReader stats: 47 sec, 366451 KB, 7.558774 MB/sec - string->char[]
		// CLineReader stats: 45 sec, 366451 KB, 7.881395 MB/sec - completely removed string from inner loop
		// CLineReader stats: 45 sec, 366451 KB, 7.999790 MB/sec - improved strcmp() - not worth it
		// CLineReader stats: 45 sec, 366451 KB, 7.900360 MB/sec - 8MB blocks instead of 4MB - not worth it
		// CLineReader stats: 49 sec, 366451 KB, 7.362065 MB/sec
		// CLineReader stats: 22 sec, 366451 KB, 16.536325 MB/sec without Recreate normals 

		sprintf(str,"CLineReader stats: %d sec, %d KB, %f MB/sec\n",(dwTime+500)/1000,m_Size/1024,
			(double)m_Size/(1024.0*1024.0/1000.0)/dwTime);
	
		OutputDebugString(str);
	}
}

bool CLineReader::IsValid() const
{
	return m_Handle!=0;
}

void CLineReader::Restart()
{
	rewind(m_Handle);

	if(m_Pos<m_Buffer.size())
	{
		// keep first buffer
		m_Pos=0;m_dwCursorInBuffer=0;
		return;
	}

	m_Pos=0;m_dwCursorInBuffer=0;
	m_Buffer.resize(0);
}



uint32 CLineReader::ComputePercent()
{
	return (uint32)(m_Pos/(m_Size/100+1));
}


bool CLineReader::GetLine( unsigned char szLine[], const uint32 dwSize )
{
	uint32 dwCursor=0;

	for(;;)
	{
		if(m_dwCursorInBuffer==m_Buffer.size())
		{
			{
				char str[80];

				sprintf(str,"%d %%\n",ComputePercent());
				OutputDebugString(str);
			}

			if(m_Pos==m_Size)
			{
				m_Buffer.clear();m_Buffer.resize(0);		// free buffer
				break;
			}

			m_Buffer.resize(min(m_Size-m_Pos,size_t(8*1024*1024)));		// 4MB

			size_t CntRead = fread(&m_Buffer[0],m_Buffer.size(),1,m_Handle);

			assert(CntRead==1);

			m_dwCursorInBuffer=0;
		}

		unsigned char c=m_Buffer[m_dwCursorInBuffer++];

		++m_Pos;

		if(c==10 || c==13)
		{
			if(dwCursor!=0)
			{
				szLine[dwCursor]=0;
				return true;
			}
			else
				continue;
		}

		if(dwCursor<dwSize-1)
			szLine[dwCursor++]=c;
	}

	return false;
}

