////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   aescryptography.h
//  Version:     v1.00
//  Created:     06/08/2009 by Younggi Lim
//  Compilers:   Visual Studio.NET
//  Description: encrypt/decrypt file or buffer using rijndael algorithm
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AesCryptography.h"
#include "rijndael.h"

static uint8 KeyXorTable[] =
{
	0xF1,0x23,0x2F,0xF8,0xC2,0xDA,0x2B,0xFB,0x42,0xF8,0x98,0x9A,0xC2,0x32,0xDE,0x3D,
	0x31,0x2D,0x6F,0x08,0xC8,0x1A,0xCB,0xF0,0x92,0xF7,0x78,0x92,0xD2,0x37,0xDF,0xAD
};

static uint8 DataXorTable[] = 
{
	0xF0,0x20,0xAF,0xF7,0xE2,0x8A,0x9B,0xFB,0x4C,0xFC,0xF8,0xBA,0xC0,0x72,0xDF,0x2E
};

CAesCryptography::CAesCryptography()
{
	const int defaultKeyValueSize = 8;
	uint8 defaultKey[defaultKeyValueSize] = {0xFB,0x4C,0x78,0x92,0xD2,0xF8,0xC2,0xDA};
	SetKeyValue(defaultKey, defaultKeyValueSize);
}

CAesCryptography::CAesCryptography(const uint8* key, int keyLen)
{
	SetKeyValue(key, keyLen);
}

CAesCryptography::~CAesCryptography()
{
}

void CAesCryptography::SetKeyValue(const uint8* key, int keyLen)
{
	for(int i=0;i<KeyValueSize; ++i)
		m_keyValue[i] = key[i%keyLen]^KeyXorTable[i];
}

bool CAesCryptography::EncryptFile(const char* filename)
{
	return CryptFileImpl(eCD_Encrypt, filename);
}

bool CAesCryptography::DecryptFile(const char* filename)
{
	return CryptFileImpl(eCD_Decrypt, filename);
}

bool CAesCryptography::CryptFileImpl(ECryptDirection dir, const char* filename)
{
	CCryFile fileRwObj;
	if (false == fileRwObj.Open(filename, "rb"))
		return false;

	uint8* fileBuffer = new uint8[fileRwObj.GetLength()];
	size_t readBufferSize = fileRwObj.ReadRaw(fileBuffer, fileRwObj.GetLength());
	if (0 == readBufferSize)
		return false;
	uint8* outputBuffer = new uint8[readBufferSize+CryptBlockSize];

	fileRwObj.Close();

	if (false == CryptBufferImpl(dir, fileBuffer, readBufferSize, outputBuffer))
		return false;

	CString encryptFilename = Path::ReplaceExtension(filename,"cct");

	if (false == fileRwObj.Open(encryptFilename, "wb"))
		return false;

	int writeResult = fileRwObj.Write(outputBuffer, readBufferSize);
	if (0 == writeResult)
		return false;
	fileRwObj.Flush();
	fileRwObj.Close();

	SetFileAttributes(filename,FILE_ATTRIBUTE_NORMAL);
	DeleteFile(filename);
	if (false == MoveFileEx(encryptFilename, filename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
		return false;

	delete[] fileBuffer;
	delete[] outputBuffer;
	return true;
}

bool CAesCryptography::EncryptBuffer(const uint8* inputBuffer, size_t inputBufferSize, uint8* outputBuffer)
{
	return CryptBufferImpl(eCD_Encrypt, inputBuffer, inputBufferSize, outputBuffer);
}

bool CAesCryptography::DecryptBuffer(const uint8* inputBuffer, size_t inputBufferSize, uint8* outputBuffer)
{
	return CryptBufferImpl(eCD_Decrypt, inputBuffer, inputBufferSize, outputBuffer);
}

bool CAesCryptography::CryptBufferImpl( ECryptDirection dir, const uint8* inputBuffer, size_t inputBufferSize, uint8* outputBuffer ) 
{
	Rijndael::Direction cryptDirection = Rijndael::Encrypt;
	if (eCD_Decrypt == dir)
		cryptDirection = Rijndael::Decrypt;

	Rijndael encryptObj;
	if (RIJNDAEL_SUCCESS != encryptObj.init(Rijndael::CBC, cryptDirection, m_keyValue, Rijndael::Key32Bytes))
		return false;

	size_t encryptBufferSize = 0;
	if (eCD_Encrypt == dir)
		encryptBufferSize = encryptObj.blockEncrypt(inputBuffer, inputBufferSize*CHAR_BIT, outputBuffer);
	else
		encryptBufferSize = encryptObj.blockDecrypt(inputBuffer, inputBufferSize*CHAR_BIT, outputBuffer);
	encryptBufferSize /= CHAR_BIT;

	int addLen = inputBufferSize%CryptBlockSize;
	memcpy(outputBuffer+encryptBufferSize, inputBuffer+encryptBufferSize, addLen);
	for(int i=0; i<addLen; ++i)
		*(outputBuffer+encryptBufferSize+i) ^= DataXorTable[i];
	return true;
}

bool CAesCryptography::DoSuccessTest(const char* filename)
{
	CString backupFile = Path::ReplaceExtension(filename,"cbk");
	SetFileAttributes(backupFile,FILE_ATTRIBUTE_NORMAL);
	DeleteFile(backupFile);
	CopyFile(filename, backupFile, false);
	SetKeyValue((uint8*)"5dbc2", 5);
	if (false == EncryptFile(filename))
		return false;
	if (false == DecryptFile(filename))
		return false;

	bool result = Compare(filename, backupFile);
	DeleteFile(backupFile);
	if (false == result)
		return false;

	const int BufferMax = 1024*1024*32;
	FILE* r = fopen(filename, "rb");
	if (NULL == r)
		return false;

	uint8* fileBuffer = new uint8[BufferMax];
	memset(fileBuffer, 0, BufferMax);
	size_t readBufferSize = fread(fileBuffer, 1, BufferMax, r);
	uint8* encryptBuffer = new uint8[readBufferSize+CryptBlockSize];
	uint8* resultBuffer = new uint8[readBufferSize+CryptBlockSize];

	fclose(r);

	if (false == EncryptBuffer(fileBuffer, readBufferSize, encryptBuffer))
		return false;
	if (false == DecryptBuffer(encryptBuffer, readBufferSize, resultBuffer))
		return false;

	for(int i=0; i<readBufferSize; ++i)
	{
		if (fileBuffer[i] != resultBuffer[i])
			return false;
	}

	delete[] fileBuffer;
	delete[] encryptBuffer;
	delete[] resultBuffer;
	return true;
}

bool CAesCryptography::DoFailTest(const char* filename)
{
	CString backupFile = Path::ReplaceExtension(filename,"cbk");
	SetFileAttributes(backupFile,FILE_ATTRIBUTE_NORMAL);
	DeleteFile(backupFile);
	CopyFile(filename, backupFile, false);
	SetKeyValue((uint8*)"5DBC-7996", 9);
	if (false == EncryptFile(filename))
		return true;
	SetKeyValue((uint8*)"5DBC-5996", 9);
	if (false == DecryptFile(filename))
		return true;

	bool result = Compare(filename, backupFile);
	DeleteFile(backupFile);
	return result;
}

bool CAesCryptography::Compare(const char* f0, const char* f1)
{
	typedef unsigned char uint8;
	const int BufferMax = 1024*1024*32;
	FILE* r = fopen(f0, "rb");
	if (NULL == r)
		return false;

	uint8* fileBuffer0 = new uint8[BufferMax];
	size_t fileSize0 = fread(fileBuffer0, 1, BufferMax, r);
	fclose(r);

	r = fopen(f1, "rb");
	if (NULL == r)
		return false;

	uint8* fileBuffer1 = new uint8[BufferMax];
	size_t fileSize1 = fread(fileBuffer1, 1, BufferMax, r);
	fclose(r);

	if (fileSize0 != fileSize1)
		return false;

	for(size_t i=0; i<fileSize0; ++i)
	{
		if (fileBuffer0[i] != fileBuffer1[i])
			return false;
	}

	delete[] fileBuffer0;
	delete[] fileBuffer1;
	return true;
}

CAesDecryptGuard::CAesDecryptGuard(const uint8* key, int keyLen, const char* filename) :
		m_key(NULL), m_filename(NULL)
{
	m_key = new uint8[keyLen];
	memset(m_key, 0, keyLen);
	size_t filenameLen = strlen(filename)+1;
	m_filename = new char[filenameLen];
	memset(m_filename, 0, filenameLen);
	m_keyLen = keyLen;
	memcpy(m_key, key, keyLen);
	strcpy_s(m_filename, filenameLen, filename);

	CAesCryptography cryptObj(m_key, m_keyLen);
	cryptObj.DecryptFile(m_filename);
}

CAesDecryptGuard::~CAesDecryptGuard()
{
	if (NULL == m_key || NULL == m_filename)
		return;

	CAesCryptography cryptObj(m_key, m_keyLen);
	cryptObj.EncryptFile(m_filename);

	delete[] m_key;
	delete[] m_filename;
}