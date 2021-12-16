////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   DataProbe.h
//  Version:     v1.00
//  Created:     19/1/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __DataProbe_h__
#define __DataProbe_h__
#pragma once

#include <IDataProbe.h>
#include "RandGen.h"

enum EDataProbeCodeInfo
{
	DATAPROBE_CRC32 = 0,
	DATAPROBE_CRC64 = 1,
	DATAPROBE_ADLER32 = 2,
	DATAPROBE_PURE_CRC32 = 3,
	DATAPROBE_MD5 = 4,
};

//////////////////////////////////////////////////////////////////////////
// Timur.
// This is authentication functions, this code is not for public release!!
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CDataProbe implementation.
//////////////////////////////////////////////////////////////////////////
class CDataProbe : public IDataProbe
{
public:
	CDataProbe();
	virtual ~CDataProbe();

	VIRTUAL bool Dummy1( SDataProbeContext &ctx ) { return false; };
	VIRTUAL int Dummy2( void *pBuf,int aa,SDataProbeContext &ctx ) { return 2322; };
	VIRTUAL bool Dummy3( SDataProbeContext &ctx ) { return false; };

	VIRTUAL uint32 GetHash( const char *sString );
	// Hash of any buffer.
	VIRTUAL uint32 GetHash( const void *buffer,int len );

	VIRTUAL void RandomAlloc();

	VIRTUAL bool GetCode( SDataProbeContext &ctx );
	VIRTUAL bool GetRandomFileProbe( SDataProbeContext &ctx,bool bAtEnd );
	VIRTUAL bool GetRandomModuleProbe( SDataProbeContext &ctx );
	VIRTUAL bool GetModuleProbe( SDataProbeContext &ctx );

	VIRTUAL int GetLoadedModules( SModuleInfo **pModules );
	VIRTUAL void AddModule( SModuleInfo &moduleInfo );

	VIRTUAL	void RandSeed( uint32 seed );
	VIRTUAL	uint32 GetRand();
	VIRTUAL	float GetRand( float fMin,float fMax );

	//////////////////////////////////////////////////////////////////////////
	// Compress block of data with zlib.
	VIRTUAL int Compress( void *dest,unsigned int &destLen,const void *source, unsigned int sourceLen,int level=6 );
	// Uncompress block of data with zlib.
	VIRTUAL int Uncompress( void *dest,unsigned int &destLen,const void *source, unsigned int sourceLen );

	void GetMD5( const char *pSrcBuffer,int nSrcSize,char signatureMD5[16] );
	void AESDecryptBuffer( const char *pSrcBuffer,int nSrcSize,char *pDestBuffer,int &nDestSize,const char *sKey );

	// Local from system.
	bool CheckLoader( void *pFunc );
	bool CheckPaul();

private:
	bool GetDataCode( char* pBuffer,int nSize,SDataProbeContext &ctx );
	bool CheckLoaderFC();

	CSysPseudoRandGen m_rand;
	void *m_pRandAlloc;
	std::vector<SModuleInfo> m_loadedModules;
	string m_sBinPath;
};

#endif // __DataProbe_h__

