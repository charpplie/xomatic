#include "StdAfx.h"
#include <assert.h>												// assert()
#include "texturefallbacksystem.h"


// Calculate CRC64 of buffer.
static uint64 CRC_64( const char *buf,unsigned int len )
{
	const uint64 POLY64REV=0xd800000000000000ULL;
	static uint64 Table[256];
	uint64 code = 0;
	static int init = 0;

	if (!init) {
		int i;
		init = 1;
		for (i = 0; i <= 255; i++) {
			int j;
			uint64 part = i;
			for (j = 0; j < 8; j++) {
				if (part & 1)
					part = (part >> 1) ^ POLY64REV;
				else
					part >>= 1;
			}
			Table[i] = part;
		}
	}
	while (len--)
	{
		uint64 temp1 = code >> 8;
		uint64 temp2 = Table[(code ^ (uint64)*buf) & 0xff];
		code = temp1 ^ temp2;
		buf += 1;
	}
	return code;

}

template<class T>
bool writeT( FILE *file, const T &value )
{
	uint32 dwSize = sizeof(value);
	assert(dwSize<=8);

	unsigned char v[8];

	for(int dwI=0;dwI<dwSize;++dwI)
		v[dwSize-1-dwI] = (unsigned char)(value>>(dwI*8))&0xff;

	return fwrite(v,dwSize,1,file)==1;
}


template<class T>
bool readT( FILE *file, T &value )
{
	uint32 dwSize = sizeof(value);
	assert(dwSize<=8);

	unsigned char v[8];

	if(fread(v,dwSize,1,file)!=1)
		return false;

	value=0;

	for(uint32 dwI=0;dwI<dwSize;++dwI)
		value |= ((T)v[dwSize-1-dwI])<<(dwI*8);

	return true;
}


CTextureFallbackSystem::CTextureFallbackSystem()
{
	m_hMutex = CreateMutex(0, FALSE, 0);
}

uint64 CTextureFallbackSystem::ComputeCRC( const char *szFilename )
{
	// Synchronize this function.
	Lock lock(*this);

	uint32 dwNameLen = (uint32)strlen(szFilename);

	string sUnifiedPathName = szFilename;

	// unify path to get reliable CRC
	{
		string::iterator it, end=sUnifiedPathName.end();
		uint32 dwI=0, dwEnd=(uint32)sUnifiedPathName.size();

		for(it=sUnifiedPathName.begin();it!=end;++it,++dwI)
		{
			if(*it=='\\')*it='/';
				else
			if(*it=='.')dwEnd=dwI;
				else
					*it = (char)tolower((int)*it);
		}

		sUnifiedPathName.resize(dwEnd);

		/*
			OutputDebugString("CTextureFallbackSystem::AddEntry - '");
			OutputDebugString(sUnifiedPathName.c_str());
			OutputDebugString("'\n");
		*/
		/*
			// make sure all follow same path name convention
			for(uint32 dwI=0;dwI<dwNameLen;++dwI)
			{
				assert(szFilename[dwI]!='\\');
				assert(islower(szFilename[dwI]));
			}
		*/
	}
	
	return CRC_64(sUnifiedPathName.c_str(),dwNameLen);
}


bool CTextureFallbackSystem::AddEntry( const char *szFilename, unsigned char *pData, const uint32 dwSize )
{
	// Synchronize this function.
	Lock lock(*this);

	assert(szFilename);

	uint64 crc = ComputeCRC(szFilename);

	std::map<uint64,SMemoryBlock *>::const_iterator it = m_Entries.find(crc);

	if(it!=m_Entries.end())
	{
		if(it->second->m_bFresh)
		{
			// same CRC either means same name (input is wrong) or CRC collision (rename file)
			// we could also allow multiple files with same CRC but this would complicates code and bloat data
			// - as it is is almost never needed we should rename file
			assert(0);
			return false;
		}
		else
			delete it->second;
	}

	m_Entries[crc] = new SMemoryBlock(pData,dwSize);				// creates a copy of the input data

	return true;
}



bool CTextureFallbackSystem::IsActivated() const
{
	// Synchronize this function.
	Lock lock(*this);

	return !m_sFilename.empty();
}


bool CTextureFallbackSystem::Init( const char *szFilename )
{
	// Synchronize this function.
	Lock lock(*this);

	assert(szFilename);
	assert(m_Entries.empty());
	assert(m_sFilename.empty());

	m_sFilename=szFilename;		// mark as activated

	FILE *in = fopen(szFilename,"rb");
	
	if(!in)
		return true;			// just not file to read - still successful init

	// header
	char szFileID[4],szTestID[5];

	sprintf(szTestID,"tfs%c",m_iVersion);							// 4 byte CryTextureFallbackSystem 

	if(fread(szFileID,4,1,in)!=1)
		{ fclose(in); return false; }										// error

	if(szFileID[0]!=szTestID[0] 
	|| szFileID[1]!=szTestID[1] 
	|| szFileID[2]!=szTestID[2] 
	|| szFileID[3]!=szTestID[3])
		{ fclose(in);	return false; }										// not the right file type or version mismatch

	// index block
	uint32 dwEntriesCount = 0;

	if(!readT(in,dwEntriesCount))											// entries
	{ assert(0); fclose(in); return false;	}					// error

	// index block
	for(uint32 dwI=0;dwI<dwEntriesCount;++dwI)
	{
		uint32 dwSize;
		uint64 Key;

		if(!readT(in,Key))															// key
		{ assert(0); fclose(in); return false; }				// error

		if(!readT(in,dwSize))														// size
		{ assert(0); fclose(in); return false; }				// error

		SMemoryBlock *pBlock = new SMemoryBlock;

		pBlock->Alloc(dwSize);

		m_Entries[Key]=pBlock;
	}

	// data block
	std::map<uint64,SMemoryBlock *>::iterator it,end = m_Entries.end();

	for(it=m_Entries.begin();it!=end;++it)
	{
		SMemoryBlock &rBlock = *(it->second);

		if(fread(rBlock.m_pData,rBlock.m_dwSize,1,in)!=1)
			{ assert(0); fclose(in); return false; }			// error
	}

	fclose(in);

	// debugging test - only works if Entries contain DDSURFACEDESC2 header
//	DebugDump();

	return false;
}


bool CTextureFallbackSystem::SaveIfInitialized()
{
	// Synchronize this function.
	Lock lock(*this);

	if(!IsActivated())
		return true;

	FILE *out = fopen(m_sFilename.c_str(),"wb");

	if(!out)
		{ assert(0); return false; }										// error

	// header
	char szFileID[5];
	
	sprintf(szFileID,"tfs%c",m_iVersion);							// 4 byte CryTextureFallbackSystem 

	if(fwrite(szFileID,4,1,out)!=1)
		{ assert(0); fclose(out); return false; }				// error

	// index block
	std::map<uint64,SMemoryBlock *>::const_iterator it,end = m_Entries.end();

	uint32 dwSize = 0;

	uint32 dwEntriesCount = (uint32)m_Entries.size();

	if(!writeT(out,dwEntriesCount))										// entries
		{ assert(0); fclose(out); return false;	}				// error

	// index block
	for(it=m_Entries.begin();it!=end;++it)
	{
		uint64 Key = it->first;
		const SMemoryBlock &rBlock = *(it->second);

		if(!writeT(out,Key))														// key
		{ assert(0); fclose(out); return false; }				// error

		if(!writeT(out,rBlock.m_dwSize))								// size
		{ assert(0); fclose(out); return false; }				// error
	}

	// data block
	for(it=m_Entries.begin();it!=end;++it)
	{
		const SMemoryBlock &rBlock = *(it->second);

		if(fwrite(rBlock.m_pData,rBlock.m_dwSize,1,out)!=1)
			{ assert(0); fclose(out); return false; }			// error
	}

	fclose(out);
	return true;
}



CTextureFallbackSystem::~CTextureFallbackSystem()
{
	std::map<uint64,SMemoryBlock *>::iterator it,end = m_Entries.end();

	for(it=m_Entries.begin();it!=end;++it)
		delete it->second;
}

uint32 CTextureFallbackSystem::GetEntriesCount() const
{
	// Synchronize this function.
	Lock lock(*this);

	return (uint32)m_Entries.size();
}



void CTextureFallbackSystem::DebugDump()
{
	// Synchronize this function.
	Lock lock(*this);

	// only works if Entries contain DDSURFACEDESC2 header

	std::map<uint64,SMemoryBlock *>::const_iterator it,end = m_Entries.end();
	uint32 dwI=0;

	for(it=m_Entries.begin();it!=end;++it,++dwI)
	{
		SMemoryBlock *pBlock = it->second;

		char filename[256];

		sprintf_s(filename,sizeof(filename),"CTextureFallback_%d.dds",dwI);

		FILE *out=fopen(filename,"wb");

		uint32 dwMagic=' SDD';		// 'DDS '
		fwrite(&dwMagic,sizeof(uint32),1,out);

		fwrite(pBlock->m_pData,pBlock->m_dwSize,1,out);

		fclose(out);
	}
}
