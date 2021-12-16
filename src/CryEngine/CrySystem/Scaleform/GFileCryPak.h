//////////////////////////////////////////////////////////////////////////////
// A wrapper around Scaleform's file interface to delegate all I/O to ICryPak
//////////////////////////////////////////////////////////////////////////////

#ifndef _GFILE_CRYPAK_H_
#define _GFILE_CRYPAK_H_

#pragma once


#ifndef EXCLUDE_SCALEFORM_SDK


#pragma warning(push)
#pragma warning(disable : 6326) // Potential comparison of a constant with another constant
#pragma warning(disable : 6011) // Dereferencing NULL pointer
#include <GFile.h>
#pragma warning(pop)


struct ICryPak;


class GFileCryPak : public GFile
{
public:
	GFileCryPak(const char* pPath);
	virtual ~GFileCryPak();

	virtual const char* GetFilePath();
	virtual bool IsValid();
	virtual bool IsWritable();
	virtual SInt Tell();
	virtual SInt64 LTell();
	virtual SInt GetLength();
	virtual SInt64 LGetLength();
	virtual SInt GetErrorCode();
	virtual SInt Write(const UByte* pBuf, SInt numBytes);
	virtual SInt Read(UByte* pBuf, SInt numBytes);
	virtual SInt SkipBytes(SInt numBytes);
	virtual SInt BytesAvailable();
	virtual bool Flush();
	virtual SInt Seek(SInt offset, SInt origin = Seek_Set);
	virtual SInt64 LSeek(SInt64 offset, SInt origin = Seek_Set);
	virtual bool ChangeSize(SInt newSize);
	virtual SInt CopyFromStream(GFile *pStream, SInt byteSize);
	virtual bool Close();

private:
	bool IsValidInternal() const {return m_fileHandle != 0;}
	long TellInternal();
	size_t GetLengthInternal();
	long SeekInternal(long offset, int origin);

private:
	ICryPak* m_pPak;
	FILE* m_fileHandle;
	string m_relativeFilePath;
};


#endif // #ifndef EXCLUDE_SCALEFORM_SDK


#endif // #ifndef _GFILE_CRYPAK_H_