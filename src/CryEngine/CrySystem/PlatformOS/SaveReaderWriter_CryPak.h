////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   SaveReaderWriter_CryPak.h
//  Created:     15/02/2010 by Alex McCarthy.
//  Description: Implementation of the ISaveReader and ISaveWriter
//               interfaces using CryPak
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SAVE_READER_WRITER_CRYPAK_H__
#define __SAVE_READER_WRITER_CRYPAK_H__

#include <IPlatformOS.h>

class CCryPakFile
{
protected:
	CCryPakFile(const IPlatformOS::TFileName& fileName, const char* szMode);
	virtual ~CCryPakFile();

	IPlatformOS::EFileOperationCode CloseImpl();

	FILE* m_pFile;
};

class CSaveReader_CryPak : public IPlatformOS::ISaveReader, public CCryPakFile
{
public:
	CSaveReader_CryPak(const IPlatformOS::TFileName& fileName);

	// ISaveReader
	virtual IPlatformOS::EFileOperationCode Seek(long seek, ESeekMode mode);
	virtual IPlatformOS::EFileOperationCode GetFileCursor(long& fileCursor);
	virtual IPlatformOS::EFileOperationCode ReadBytes(void* data, size_t numBytes);
	virtual IPlatformOS::EFileOperationCode GetNumBytes(size_t& numBytes);
	virtual IPlatformOS::EFileOperationCode Close() { return CloseImpl(); }
	//~ISaveReader
};

class CSaveWriter_CryPak : public IPlatformOS::ISaveWriter, public CCryPakFile
{
public:
	CSaveWriter_CryPak(const IPlatformOS::TFileName& fileName);

	// ISaveWriter
	virtual void AppendBytes(const void* data, size_t length);
	virtual IPlatformOS::EFileOperationCode Close();
	//~ISaveWriter

private:
	bool m_bError;
};

#endif //__SAVE_READER_WRITER_CRYPAK_H__
