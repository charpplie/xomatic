#include "StdAfx.h"
#include "MTSafeAllocator.h"
#include <smartptr.h>
#include "ZipFileFormat.h"
#include "ZipDirStructures.h"
#include "ZipDirTree.h"
#include "ZipDirCache.h"
#include "ZipDirFind.h"
#include "ZipDirCacheFactory.h"
#include "zlib/zlib.h"
#include <IDiskProfiler.h>

using namespace ZipFile;

// initializes the instance structure
void ZipDir::Cache::Construct(FileExt& fNew, CMTSafeHeap* pHeap, size_t nDataSizeIn, unsigned int nFactoryFlags)
{
	m_nRefCount = 0;
	m_pFile = fNew;
	m_nDataSize = nDataSizeIn;
	m_nZipPathOffset = nDataSizeIn;
	m_pCacheData = new CacheData;
	m_pCacheData->m_pHeap = pHeap;
	m_nCacheFactoryFlags = nFactoryFlags;
	m_pRootData = (DirHeader*)GetDataPointer();
	if (m_nCacheFactoryFlags & CacheFactory::FLAGS_FILENAMES_AS_CRC32)
	{
		m_pRootData = NULL;
	}

	fNew.m_file = NULL; // we don't own the file anymore - it's in possession of the cache instance
	fNew.m_pData = NULL;
	fNew.m_nCursor = 0;
	fNew.m_nSize = 0;

}

// self-destruct when ref count drops to 0
void ZipDir::Cache::Delete()
{
	m_pFile.Close();

	CMTSafeHeap* pHeap = m_pCacheData->m_pHeap;
	SAFE_DELETE(m_pCacheData);
	pHeap->Free(this);
}


// initializes this object from the given Zip file: caches the central directory
// returns 0 if successfully parsed, error code if an error has occured
ZipDir::CachePtr ZipDir::NewCache (const char* szFileName, CMTSafeHeap* pHeap, InitMethodEnum nInitMethod)
{
	// try .. catch(Error)
	CacheFactory factory(pHeap, nInitMethod);
	return factory.New(szFileName, nInitMethod == ZD_INIT_VALIDATE_IN_MEMORY);
}

//////////////////////////////////////////////////////////////////////////
// nNameOffset of the FileEntry is used as a CRC32 of the filename
//////////////////////////////////////////////////////////////////////////
struct FileEntryNameAsCrc32Predicate
{
	bool operator()( const ZipDir::FileEntry &f1,unsigned int nCrc32 ) const
	{
		return f1.nNameOffset < nCrc32;
	}
	bool operator()( unsigned int nCrc32,const ZipDir::FileEntry &f1 ) const
	{
		return nCrc32 < f1.nNameOffset;
	}
};

// looks for the given file record in the Central Directory. If there's none, returns NULL.
// if there is some, returns the pointer to it.
// the Path must be the relative path to the file inside the Zip
// if the file handle is passed, it will be used to find the file data offset, if one hasn't been initialized yet
ZipDir::FileEntry* ZipDir::Cache::FindFile (const char* szPath, bool bRefresh)
{
	if (m_nCacheFactoryFlags & CacheFactory::FLAGS_FILENAMES_AS_CRC32)
	{
		void *pDataStart = (this + 1);
		FileEntry *pFirstEntry = reinterpret_cast<FileEntry*>(pDataStart);
		FileEntry *pLastEntry = pFirstEntry + (m_nDataSize/sizeof(FileEntry));

		uint32 uCRC32 = crc32(0L, Z_NULL, 0);
		uCRC32 = crc32(uCRC32, (Bytef*)szPath, strlen(szPath));
		uint32 nNameAsCrc32 = uCRC32;

		FileEntry *pEntry = std::lower_bound(pFirstEntry,pLastEntry,nNameAsCrc32,FileEntryNameAsCrc32Predicate() );
		if (pEntry != pLastEntry && pEntry->nNameOffset == nNameAsCrc32)
		{
			return pEntry;
		}
		return NULL;
	}

	ZipDir::FindFile fd (this);
	if (!fd.FindExact(szPath))
	{
		assert (!fd.GetFileEntry());
		return NULL;
	}
	assert (fd.GetFileEntry());
	return fd.GetFileEntry();

}




// loads the given file into the pCompressed buffer (the actual compressed data)
// if the pUncompressed buffer is supplied, uncompresses the data there
// buffers must have enough memory allocated, according to the info in the FileEntry
// NOTE: there's no need to decompress if the method is 0 (store)
// returns 0 if successful or error code if couldn't do something
ZipDir::ErrorEnum ZipDir::Cache::ReadFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed, const bool decompress/* = true */)
{
	if (!pFileEntry)
		return ZD_ERROR_INVALID_CALL;

	if (pFileEntry->desc.lSizeUncompressed == 0)
	{
		assert (pFileEntry->desc.lSizeCompressed == 0);
		return ZD_ERROR_SUCCESS;
	}

	assert (pFileEntry->desc.lSizeCompressed > 0);

	ErrorEnum nError = Refresh(pFileEntry);
	if (nError != ZD_ERROR_SUCCESS)
		return nError;

	SmartPtr pBufferDestroyer(m_pCacheData->m_pHeap);

	void* pBuffer = pCompressed; // the buffer where the compressed data will go

	if (pFileEntry->nMethod == 0 && pUncompressed)
	{
		// we can directly read into the uncompress buffer
		pBuffer = pUncompressed;
	}

	if (!pBuffer)
	{
		if (!pUncompressed)
		// what's the sense of it - no buffers at all?
			return ZD_ERROR_INVALID_CALL;

		if(decompress)
		{
			pBuffer = m_pCacheData->m_pHeap->Alloc(pFileEntry->desc.lSizeCompressed, "ZipDir::Cache::ReadFile");
			pBufferDestroyer.Attach(pBuffer); // we want it auto-freed once we return
		}
		else	// we can store the data in uncompressed buffer
			pBuffer = pUncompressed;
	}

	{
		PROFILE_DISK_READ(pFileEntry->desc.lSizeCompressed);
		CAutoLock<CryCriticalSection> lock(m_pCacheData->m_csCacheIOLock);	// guarantees that fseek() and fread() will be executed together
		{
			FRAME_PROFILER("ZipDir_Cache_ReadFile_fseek", gEnv->pSystem, PROFILE_SYSTEM);

			if (ZipDir::FSeek(&m_pFile, pFileEntry->nFileDataOffset, SEEK_SET))
				return ZD_ERROR_IO_FAILED;
		}
		{
			assert(pBuffer);
			if(pBuffer==NULL) // return failed if buffer allocation failed
				return ZD_ERROR_NO_MEMORY;

			if (ZipDir::FRead (pBuffer, pFileEntry->desc.lSizeCompressed, 1, &m_pFile) != 1)
				return ZD_ERROR_IO_FAILED;
		}
	}

	// if there's a buffer for uncompressed data, uncompress it to that buffer
	if (pUncompressed && decompress)
	{
		if (pFileEntry->nMethod == 0)
		{
			assert (pBuffer == pUncompressed);
		}
		else
		{
			if (Z_OK != DecompressFile(pFileEntry, pBuffer, pUncompressed, m_pCacheData->m_csCacheIOLock))
				return ZD_ERROR_CORRUPTED_DATA;
		}
	}

	return ZD_ERROR_SUCCESS;
}

// decompress compressed file 
ZipDir::ErrorEnum ZipDir::Cache::DecompressFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed, CryCriticalSection& csDecmopressLock)
{
	FUNCTION_PROFILER( gEnv->pSystem, PROFILE_SYSTEM );
	if(pUncompressed == NULL)
		return ZD_ERROR_INVALID_CALL;

	PROFILE_DISK_DECOMPRESS;
	unsigned long nSizeUncompressed = pFileEntry->desc.lSizeUncompressed;

	SmartPtr pBufferDestroyer(m_pCacheData->m_pHeap);

	void* pBuffer = pCompressed;
	if(pBuffer == pUncompressed && pFileEntry->nMethod != 0)	// it means that we stored compressed data in the uncompressed buffer
	{
		pBuffer = m_pCacheData->m_pHeap->Alloc(pFileEntry->desc.lSizeCompressed, "ZipDir::Cache::DecompressFile");
		pBufferDestroyer.Attach(pBuffer); // we want it auto-freed once we return
		memcpy (pBuffer, pCompressed, pFileEntry->desc.lSizeCompressed);
	}

	AUTO_LOCK_CS(csDecmopressLock);
	if (Z_OK != ZipRawUncompress(m_pCacheData->m_pHeap, pUncompressed, &nSizeUncompressed, pBuffer, pFileEntry->desc.lSizeCompressed))
		return ZD_ERROR_CORRUPTED_DATA;
	return ZD_ERROR_SUCCESS;
}


// loads and unpacks the file into a newly created buffer (that must be subsequently freed with
// Free()) Returns NULL if failed
void* ZipDir::Cache::AllocAndReadFile (FileEntry* pFileEntry)
{
	if (!pFileEntry)
		return NULL;

	void* pData = m_pCacheData->m_pHeap->Alloc(pFileEntry->desc.lSizeUncompressed, "ZipDir::Cache::AllocAndReadFile");
	if (pData)
	{
		if (ZD_ERROR_SUCCESS != ReadFile (pFileEntry, NULL, pData))
		{
			m_pCacheData->m_pHeap->Free (pData);
			pData = NULL;
		}
	}
	return pData;
}

// frees the memory block that was previously allocated by AllocAndReadFile
void ZipDir::Cache::Free (void* pData)
{
	m_pCacheData->m_pHeap->Free(pData);
}

// refreshes information about the given file entry into this file entry
ZipDir::ErrorEnum ZipDir::Cache::Refresh (FileEntry* pFileEntry)
{
	CAutoLock<CryCriticalSection> lock(m_pCacheData->m_csCacheIOLock);	// Not thread safe without this

	if (!pFileEntry)
		return ZD_ERROR_INVALID_CALL;

	if (pFileEntry->nFileDataOffset != pFileEntry->INVALID_DATA_OFFSET)
		return ZD_ERROR_SUCCESS; // the data offset has been successfully read..

	if (!this)
		return ZD_ERROR_INVALID_CALL; // from which cache is this file entry???

	return ZipDir::Refresh(&m_pFile, pFileEntry);
}

//////////////////////////////////////////////////////////////////////////
uint32 ZipDir::Cache::GetFileDataOffset(FileEntry* pFileEntry)
{
	CAutoLock<CryCriticalSection> lock(m_pCacheData->m_csCacheIOLock);	// Not thread safe without this

	if (pFileEntry->nFileDataOffset == pFileEntry->INVALID_DATA_OFFSET)
		ZipDir::Refresh (&m_pFile,pFileEntry);
	return pFileEntry->nFileDataOffset;
}

// returns the size of memory occupied by the instance referred to by this cache
// must be exact, because it's used by CacheRW to reallocate this cache
size_t ZipDir::Cache::GetSize()const
{
	if (this)
		return m_nDataSize + sizeof(Cache) + strlen(GetFilePath());
	else
		return 0;
}


// QUICK check to determine whether the file entry belongs to this object
bool ZipDir::Cache::IsOwnerOf (const FileEntry*pFileEntry)const
{
	if (m_nCacheFactoryFlags & CacheFactory::FLAGS_FILENAMES_AS_CRC32)
	{
		// Assume true, this function only used in asserts
		return true;
	}

	// just check whether the pointer is within the memory block of this cache instance
	return ((ULONG_PTR)pFileEntry >= (ULONG_PTR)(GetRoot()+1)	&&(ULONG_PTR)pFileEntry <= ((ULONG_PTR)GetRoot()) + m_nDataSize - sizeof(FileEntry));
}




bool ZipDir::isFileInMemory(FileExt * f)
{

	return f->m_pData != NULL;
}


int64 ZipDir::FileInMemorySize(FileExt * f)
{
	return f->m_nSize;
}	

int64 ZipDir::FSeek(FileExt * file, int64 origin, int command) 
{
	if (ZipDir::isFileInMemory(file)) {

		int64 retCode = -1;
		int64 newPos;
		switch (command) 
		{
		case SEEK_SET:
			newPos = origin;
			if (newPos <= file->m_nSize) {
				 file->m_nCursor = newPos;
				 retCode = 0;
			}
			break;
		case SEEK_CUR:
			newPos = origin + file->m_nCursor;
			if (newPos <= file->m_nSize) {
				file->m_nCursor =  newPos;
				retCode = 0;
			}
			break;
		case SEEK_END:
			newPos = file->m_nSize - origin;
			if (newPos <= file->m_nSize) {
				file->m_nCursor =  newPos;
				retCode = 0;
			}
			break;
		default:
			// Not valid disk operation!
			assert(0);
		}
		return retCode;
	} 
	else 
	{
#if defined(WIN32) || defined(XENON)
		return _fseeki64 (file->m_file, origin, command);
#elif LINUX
		return fseeko (file->m_file, (off_t)origin, command);
#else
		return fseek (file->m_file, origin, command);
#endif
	}
}

int64 ZipDir::FRead(void * data, size_t elementSize, size_t count, FileExt * file)
{
	if (ZipDir::isFileInMemory(file))
	{
		int64 nRead = count * elementSize;
		int64 nCanBeRead =file->m_nSize - file->m_nCursor;
		if (nRead > nCanBeRead)
			nRead = nCanBeRead;
		memmove(data, (char*)file->m_pData + file->m_nCursor, (size_t)nRead);
		file->m_nCursor += nCanBeRead;
		return nRead / elementSize;
	}
	else
	{
		return fread(data, elementSize, count, file->m_file);
	}
}

int64 ZipDir::FTell(FileExt * file)
{
	if (ZipDir::isFileInMemory(file)) {

		return file->m_nCursor;
	}
	else 
	{
#ifdef WIN32
		__int64 nPos = _ftelli64 (file->m_file);
#elif defined(LINUX)
		off_t nPos = ftello(file->m_file);
#else
		long nPos = ftell (file->m_file);
#endif

		return nPos;
	}
}
