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
#ifndef __aescryptography_h__
#define __aescryptography_h__

#pragma once

const int CryptBlockSize = 16;
const int KeyValueSize = 32;

enum ECryptDirection
{
	eCD_Encrypt,
	eCD_Decrypt
};

class CAesCryptography
{
public:
	CAesCryptography();
	CAesCryptography(const uint8* key, int keyLen);
	virtual ~CAesCryptography();

	///////////////////////////////////////////////////////
	// Description:
	//   Setup key value 
	// Arguments:
	//   key : unsigned char array
	//   keyLen : less than KeyValueSize
	///////////////////////////////////////////////////////
	void SetKeyValue(const uint8* key, int keyLen);

	bool EncryptFile(const char* filename);
	bool DecryptFile(const char* filename);

	///////////////////////////////////////////////////////////////////////////////////
	// Arguments:
	//   outputBuffer : must be CryptBlockSize bytes long than inputBufferSize.
	///////////////////////////////////////////////////////////////////////////////////
	bool EncryptBuffer(const uint8* inputBuffer, size_t inputBufferSize, uint8* outputBuffer);
	bool DecryptBuffer(const uint8* inputBuffer, size_t inputBufferSize, uint8* outputBuffer);

	bool DoSuccessTest(const char* filename);
	bool DoFailTest(const char* filename);

protected:
	bool CryptFileImpl(ECryptDirection dir, const char* filename);
	bool CryptBufferImpl( ECryptDirection dir, const uint8* inputBuffer, size_t inputBufferSize, uint8* outputBuffer );
	bool Compare(const char* f0, const char* f1);

private:
	uint8 m_keyValue[KeyValueSize];
};


///////////////////////////////////////////////////////
// Description:
//   Auto Decrypt -> Encrypt
//   kind of AutoLock
//////////////////////////////////////////////////////
class CAesDecryptGuard
{
public:
	CAesDecryptGuard(const uint8* key, int keyLen, const char* filename);
	virtual ~CAesDecryptGuard();

private:
	uint8*	m_key;
	char*		m_filename;
	int			m_keyLen;
};

#endif // __cryptfile_h__