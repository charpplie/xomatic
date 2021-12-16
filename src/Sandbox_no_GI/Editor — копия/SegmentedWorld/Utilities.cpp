#include "StdAfx.h"
#include "InternalCommon.h"



SW_NAMESPACE_BEGIN();
void SWLogV( bool bConsoleOnly,const char * pszLog, va_list args)
{

	char szLogBuf[MAX_PATH*2];
	strncpy(szLogBuf,"SW: ",4);
	char* pszPureString = szLogBuf + 4;

	char* pszEnd = pszPureString + vsprintf(pszPureString,pszLog,args);
	pszEnd[0] = '\n';
	pszEnd[1] = '\0';

	OutputDebugString(pszPureString);
	if (!bConsoleOnly)
	{
		CryLog(szLogBuf);
		//gEnv->pLog->LogToConsole(chLog);
	}
}

void SWLog( const char * pszLog, ...)
{
	va_list args;
	va_start(args,pszLog);
	SWLogV(false,pszLog,args);
	va_end(args);
}
void SWLogConsole( const char * pszLog, ...)
{
	va_list args;
	va_start(args,pszLog);
	SWLogV(true,pszLog,args);
	va_end(args);
}

bool SaveMemBlockToFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags/* = 0*/)
{
	if(!CFileUtil::CreatePath(pcFileName))
		return false;

	CFile file;
	if (!file.Open(pcFileName, CFile::modeCreate | CFile::modeWrite | nOpenFlags))
	{
		int error = GetLastError();
		return false;
	}
	CArchive ar(&file, CArchive::store);
	mem.Serialize(ar);
	return true;
}

bool LoadMemBlockFromFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags/* = 0*/)
{
	CFile file;
	if (!file.Open(pcFileName, CFile::modeRead | nOpenFlags))
	{
		int error = GetLastError();
		return false;
	}
	if ( file.GetLength() <= 0 ) // TODO: since original Sofia code have separate procedures for creating and saving, so it didn't skip those files which should not create in the first place. -RL 
		return false;

	CArchive ar(&file, CArchive::load);
	mem.Serialize(ar);
	return true;
}

bool SaveRawDataToFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags/* = 0*/)
{
	return SaveRawDataToFile(pcFileName, mem.GetBuffer(), mem.GetSize(), nOpenFlags);
}

// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.
bool SaveRawDataToFile( const char *pcFileName, void* pBuf, size_t cbSize, UINT nOpenFlags /*= 0*/ )
{
	CFile file;
	if (!CFileUtil::CreatePath(pcFileName) || !file.Open(pcFileName, CFile::modeCreate | CFile::modeWrite | nOpenFlags))
	{
		int error = GetLastError();
		return false;
	}

	file.Write(pBuf,cbSize);
	file.Close();
	return true;
}

bool LoadRawDataFromFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags/* = 0*/)
{
	CFile file;
	if (!file.Open(pcFileName, CFile::modeRead | nOpenFlags))
	{
		int error = GetLastError();
		return false;
	}
	if ( file.GetLength() <= 0 ) // TODO: since original Sofia code have separate procedures for creating and saving, so it didn't skip those files which should not create in the first place. -RL 
		return false;

	mem.Allocate( file.GetLength() );
	file.Read( mem.GetBuffer(),file.GetLength() );
	file.Close();

	return true;
}

SW_NAMESPACE_END();
