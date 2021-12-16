#include "StdAfx.h"


#ifndef EXCLUDE_SCALEFORM_SDK


#include "GFileCryPak.h"

#include <ISystem.h>
#include <ICryPak.h>


GFileCryPak::GFileCryPak(const char* pPath)
: m_pPak(0)
, m_fileHandle(0)
, m_relativeFilePath(pPath)
{
	m_pPak = gEnv->pCryPak;
	assert(m_pPak);
	m_fileHandle = m_pPak->FOpen(pPath, "rb");
}


GFileCryPak::~GFileCryPak()
{
	if (IsValidInternal())
		Close();
}


const char* GFileCryPak::GetFilePath()
{
	return m_relativeFilePath.c_str();
}


bool GFileCryPak::IsValid()
{
	return IsValidInternal();
}


bool GFileCryPak::IsWritable()
{
	// writable files are not supported!
	return false;
}


inline long GFileCryPak::TellInternal()
{
	if (IsValidInternal())
		return m_pPak->FTell(m_fileHandle);
	return 0;
}


SInt GFileCryPak::Tell()
{
	COMPILE_TIME_ASSERT(sizeof(SInt) >= sizeof(long));
	return TellInternal();
}


SInt64 GFileCryPak::LTell()
{
	COMPILE_TIME_ASSERT(sizeof(SInt64) >= sizeof(long));
	return TellInternal();
}


inline size_t GFileCryPak::GetLengthInternal()
{
	if (IsValidInternal())
		return m_pPak->FGetSize(m_fileHandle);
	return 0;
}


SInt GFileCryPak::GetLength()
{
	size_t len = GetLengthInternal();
	assert(len <= UInt(-1) >> 1);
	return (SInt) len;
}


SInt64 GFileCryPak::LGetLength()
{
	size_t len = GetLengthInternal();
#if defined(WIN64)
	assert(len <= ((UInt64)-1) >> 1);
#endif
	return (SInt64) len;
}


SInt GFileCryPak::GetErrorCode()
{
	if (IsValidInternal())
	{
		if (!m_pPak->FError(m_fileHandle))
			return 0;
		return GFileConstants::Error_IOError;
	}
	return GFileConstants::Error_FileNotFound;
}


SInt GFileCryPak::Write(const UByte* pBuf, SInt numBytes)
{
	// writable files are not supported!
	return -1;
}


SInt GFileCryPak::Read(UByte* pBuf, SInt numBytes)
{
	if (IsValidInternal())
		return m_pPak->FReadRaw(pBuf, 1, numBytes, m_fileHandle);
	return -1;
}


SInt GFileCryPak::SkipBytes(SInt numBytes)
{
	if (IsValidInternal())
	{
		long pos = m_pPak->FTell(m_fileHandle);
		if (pos == -1)
			return 0;

		long newPos = SeekInternal(numBytes, SEEK_CUR);
		if (newPos == -1)
			return -1;

		return newPos - pos;
	}
	return 0;
}


SInt GFileCryPak::BytesAvailable()
{
	if (IsValidInternal())
	{
		long pos = m_pPak->FTell(m_fileHandle);
		if (pos == -1)
			return 0;

		size_t endPos = m_pPak->FGetSize(m_fileHandle);
		assert(endPos <= ((unsigned long)-1) >> 1);

		return long(endPos) - pos;
	}
	return 0;
}


bool GFileCryPak::Flush()
{
	// writable files are not supported!
	return false;
}


inline long GFileCryPak::SeekInternal(long offset, int origin)
{
	if (IsValidInternal())
	{
		int newOrigin = 0;
		switch(origin)
		{
		case Seek_Set: newOrigin = SEEK_SET; break;
		case Seek_Cur: newOrigin = SEEK_CUR; break;
		case Seek_End: newOrigin = SEEK_END; break;
		}

		if (newOrigin == SEEK_SET)
		{
			long curPos = m_pPak->FTell(m_fileHandle);
			if (offset == curPos)
				return curPos;
		}

		if (!m_pPak->FSeek(m_fileHandle, offset, newOrigin))
			return m_pPak->FTell(m_fileHandle);
	}
	return -1;
}


SInt GFileCryPak::Seek(SInt offset, SInt origin)
{
	COMPILE_TIME_ASSERT(sizeof(SInt) >= sizeof(long));
	return SeekInternal(offset, origin);
}


SInt64 GFileCryPak::LSeek(SInt64 offset, SInt origin)
{
	assert(offset <= (long)(((unsigned long)-1) >> 1));

	COMPILE_TIME_ASSERT(sizeof(SInt) >= sizeof(long));
	return SeekInternal((long) offset, origin);
}


bool GFileCryPak::ChangeSize(SInt newSize)
{
	// not supported!
	return false;
}


SInt GFileCryPak::CopyFromStream(GFile* pStream, SInt byteSize)
{
	// not supported!
	return -1;
}


bool GFileCryPak::Close()
{
	if (IsValidInternal())
	{
		bool res = !m_pPak->FClose(m_fileHandle);
		m_fileHandle = 0;
		return res;
	}
	return false;
}


#endif // #ifndef EXCLUDE_SCALEFORM_SDK