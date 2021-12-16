////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   SaveReaderWriter_CryPak.cpp
//  Created:     15/02/2010 by Alex McCarthy.
//  Description: Implementation of the ISaveReader and ISaveWriter
//               interfaces using CryPak
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include <StdAfx.h>

#include "SaveReaderWriter_CryPak.h"

static const int INVALID_SEEK = -1;

int TranslateSeekMode(IPlatformOS::ISaveReader::ESeekMode mode)
{
	COMPILE_TIME_ASSERT(INVALID_SEEK != SEEK_SET);
	COMPILE_TIME_ASSERT(INVALID_SEEK != SEEK_CUR);
	COMPILE_TIME_ASSERT(INVALID_SEEK != SEEK_END);

	switch(mode)
	{
		case IPlatformOS::ISaveReader::ESM_BEGIN: return SEEK_SET;
		case IPlatformOS::ISaveReader::ESM_CURRENT: return SEEK_CUR;
		case IPlatformOS::ISaveReader::ESM_END: return SEEK_END;
		default:
			{
				CRY_ASSERT_TRACE(false, ("Unrecognized seek mode %i", static_cast<int>(mode)));
				return INVALID_SEEK;
			}
	}
}

////////////////////////////////////////////////////////////////////////////

CCryPakFile::CCryPakFile(const IPlatformOS::TFileName& fileName, const char* szMode)
: m_pFile(gEnv->pCryPak->FOpen(fileName.c_str(), szMode))
{
}

CCryPakFile::~CCryPakFile()
{
	CloseImpl();
}

IPlatformOS::EFileOperationCode CCryPakFile::CloseImpl()
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pCryPak);

	if (m_pFile && (gEnv->pCryPak->FClose(m_pFile) == 0))
	{
		return IPlatformOS::EFOC_SUCCESS;
	}
	return IPlatformOS::EFOC_FAILURE;
}

////////////////////////////////////////////////////////////////////////////

CSaveReader_CryPak::CSaveReader_CryPak(const IPlatformOS::TFileName& fileName)
: CCryPakFile(fileName, "rbx") // x=don't cache full file
{
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::Seek(long seek, ESeekMode mode)
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pCryPak);

	int translatedMode = TranslateSeekMode(mode);
	CRY_ASSERT(translatedMode != INVALID_SEEK);
	if (translatedMode == INVALID_SEEK)
	{
		return IPlatformOS::EFOC_FAILURE;
	}

	const bool bSuccess = (gEnv->pCryPak->FSeek(m_pFile, seek, translatedMode) == 0);
	return bSuccess ? IPlatformOS::EFOC_SUCCESS : IPlatformOS::EFOC_FAILURE;
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::GetFileCursor(long& fileCursor)
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pCryPak);

	fileCursor = gEnv->pCryPak->FTell(m_pFile);

	return IPlatformOS::EFOC_SUCCESS;
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::ReadBytes(void* data, size_t numBytes)
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pCryPak);

	size_t readBytes = gEnv->pCryPak->FReadRaw(data, 1, numBytes, m_pFile); // TODO: do we need to do an endian swap?
	
	return (numBytes == readBytes) ? IPlatformOS::EFOC_SUCCESS : IPlatformOS::EFOC_FAILURE;
}

IPlatformOS::EFileOperationCode CSaveReader_CryPak::GetNumBytes(size_t& numBytes)
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pCryPak);

	numBytes = gEnv->pCryPak->FGetSize(m_pFile);

	return IPlatformOS::EFOC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

CSaveWriter_CryPak::CSaveWriter_CryPak(const IPlatformOS::TFileName& fileName)
: CCryPakFile(fileName, "wb")
{
	m_bError = (m_pFile != NULL);
}

void CSaveWriter_CryPak::AppendBytes(const void* data, size_t length)
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pCryPak);

	if (m_pFile && length != 0)
	{
		if (gEnv->pCryPak->FWrite(data, 1, length, m_pFile) != length)
		{
			m_bError = true;
		}
	}
}

IPlatformOS::EFileOperationCode CSaveWriter_CryPak::Close()
{
	if (CloseImpl() == IPlatformOS::EFOC_FAILURE)
	{
		return IPlatformOS::EFOC_FAILURE;
	}
	return m_bError ? IPlatformOS::EFOC_FAILURE : IPlatformOS::EFOC_SUCCESS;
}
