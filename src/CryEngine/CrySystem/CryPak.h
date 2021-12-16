
//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
//	
//	File:CryPak.h
//
//	History:
//	-Feb 2,2001:Created by Honich Andrey
//  -June 12, 2003: Taken over by Sergiy Migdalskiy.
//     Got rid of unzip usage, now using ZipDir for much more effective
//     memory usage (~3-6 times less memory, and no allocator overhead)
//     to keep the directory of the zip file; better overall effectiveness and
//     more readable and manageable code, made the connection to Streaming Engine
//
//////////////////////////////////////////////////////////////////////

#ifndef CRYPAK_H
#define CRYPAK_H

#include <ICryPak.h>
#include <CryThread.h>
#include "IMiniLog.h"
#include "ZipDir.h"
#include "MTSafeAllocator.h"
#include "StlUtils.h"
#include "PakVars.h"
#include "CryPakHandleCache.h"

class CCryPak;







// this is the header in the cache of the file data
struct CCachedFileData : public CMultiThreadRefCount
{
	CCachedFileData (class CCryPak* pPak, ZipDir::Cache*pZip, ZipDir::FileEntry* pFileEntry,const char *szFilename);
	~CCachedFileData();

	// return the data in the file, or NULL if error
	// by default, if bRefreshCache is true, and the data isn't in the cache already,m
	// the cache is refreshed. Otherwise, it returns whatever cache is (NULL if the data isn't cached yet)
	void* GetData(bool bRefreshCache = true, const bool decompress = true, const bool allocateForDecompressed = true);
	// Uncompress file data directly to provided memory.
	bool GetDataTo( void *pFileData,int nDataSize );

	ILINE ZipDir::Cache* GetZip(){return m_pZip;}
	ILINE ZipDir::FileEntry* GetFileEntry() {return m_pFileEntry;}

	/*
	// the memory needs to be allocated out of a MT-safe heap here
	void * __cdecl operator new   (size_t size) { return g_pPakHeap->Alloc(size, "CCachedFileData::new"); }
	void * __cdecl operator new   (size_t size, const std::nothrow_t &nothrow) { return g_pPakHeap->Alloc(size, "CCachedFileData::new(std::nothrow)"); }
	void __cdecl operator delete  (void *p) { g_pPakHeap->Free(p); };
	*/

	unsigned GetFileDataOffset()
	{
		return m_pZip->GetFileDataOffset(m_pFileEntry);
	}

	size_t sizeofThis()const
	{
		return sizeof(*this) + (m_pFileData&&m_pFileEntry?m_pFileEntry->desc.lSizeUncompressed:0);
	}
public:
	void* m_pFileData;

	// the zip file in which this file is opened
	ZipDir::CachePtr m_pZip;
	// the file entry : if this is NULL, the entry is free and all the other fields are meaningless
	ZipDir::FileEntry* m_pFileEntry;

	class CCryPak* m_pPak;

#ifdef _DEBUG
	string m_sDebugFilename;
#endif


	// file I/O guard: guarantees thread-safe decompression operation and safe allocation
	CryCriticalSection m_csDecompressLock;
	volatile bool	m_bDecompressed;
};

TYPEDEF_AUTOPTR(CCachedFileData);
typedef CCachedFileData_AutoPtr CCachedFileDataPtr;

//////////////////////////////////////////////////////////////////////////
struct CCachedFileRawData : public CMultiThreadRefCount
{
	void *m_pCachedData;
	FILE *m_hFile;

	CCachedFileRawData( int nAlloc );
	~CCachedFileRawData();
};


// an (inside zip) emultated open file
struct CZipPseudoFile
{
	CZipPseudoFile()
	{
		Construct();
	}
	~CZipPseudoFile()
	{
	}

	enum
	{
		_O_COMMIT_FLUSH_MODE = 1 << 31,
		_O_DIRECT_OPERATION  = 1 << 30
	};

	// this object must be constructed before usage
	// nFlags is a combination of _O_... flags
	void Construct(CCachedFileData* pFileData = NULL, unsigned nFlags = 0);
	// this object needs to be freed manually when the CryPak shuts down..
	void Destruct();

	CCachedFileData* GetFile() {return m_pFileData;}

	long FTell() {return m_nCurSeek;}
	
	unsigned GetFileSize() { return GetFile()?GetFile()->GetFileEntry()->desc.lSizeUncompressed:0; }

	int FSeek (long nOffset, int nMode);
	size_t FRead (void* pDest, size_t nSize, size_t nCount, FILE* hFile);
	size_t FReadAll (void* pDest, size_t nFileSize, FILE* hFile);
	void*  GetFileData ( size_t &nFileSize, FILE* hFile );
	int FEof();
	int FScanfv (const char* szFormat, va_list args);
	char* FGets(char* pBuf, int n);
	int Getc();
	int Ungetc(int c);

	uint64 GetModificationTime() { return m_pFileData->GetFileEntry()->GetModificationTime(); }
	const char* GetArchivePath() { return m_pFileData->GetZip()->GetFilePath(); }
protected:
  unsigned long m_nCurSeek;
  CCachedFileDataPtr m_pFileData;
	void *m_pCachedData;
	// nFlags is a combination of _O_... flags
  unsigned m_nFlags;
	FILE* m_fDirect;
};


struct CIStringOrder
{
	bool operator () (const string& left, const string& right) const
	{
		return stricmp(left.c_str(), right.c_str()) < 0;
	}
};

class CCryPakFindData: public _reference_target_t
{
public:
	// the directory wildcard must already be adjusted
	CCryPakFindData (class CCryPak*pPak, const char* szDir);
	bool	empty() const;
	bool	Fetch(_finddata_t* pfd);
	void	Scan(CCryPak*pPak, const char* szDir);

	size_t sizeofThis()const;
protected:
	void ScanFS(CCryPak*pPak, const char* szDir);
	void ScanZips(CCryPak*pPak, const char* szDir);

	struct FileDesc
	{
		unsigned nAttrib;
		unsigned nSize;
		time_t tAccess;
		time_t tCreate;
		time_t tWrite;

		FileDesc (struct _finddata_t* fd);
		FileDesc (struct __finddata64_t* fd);

		FileDesc (ZipDir::FileEntry* fe);

		// default initialization is as a directory entry
		FileDesc ();
	};
	typedef std::map<string, FileDesc, CIStringOrder> FileMap;
	FileMap m_mapFiles;
};

TYPEDEF_AUTOPTR(CCryPakFindData);

//////////////////////////////////////////////////////////////////////////
typedef struct  
{
	char	*szName;	// folder or name to be replaced
	int		nLen1;		// string length, for faster operation
	char	*szAlias;	// new folder name
	int		nLen2;		// string length, for faster operation
}tNameAlias;

//////////////////////////////////////////////////////////////////////
class CCryPak : public ICryPak
{
	friend class CRefReadStream;
	// the array of pseudo-files : emulated files in the virtual zip file system
	// the handle to the file is its index inside this array.
	// some of the entries can be free. The entries need to be destructed manually
	CryReadModifyLock m_csOpenFiles;
	typedef std::vector<CZipPseudoFile> ZipPseudoFileArray;
	ZipPseudoFileArray m_arrOpenFiles;

	// the array of file datas; they are relatively self-contained and can
	// read and cache the file data on-demand. It's up to the clients
	// to use caching or access the zips directly
	CryReadModifyLock m_csCachedFiles;
	typedef std::vector<CCachedFileData*> CachedFileDataSet;
	CachedFileDataSet m_setCachedFiles;

	// This is a cached data for the FGetCachedFileData call.
	_smart_ptr<CCachedFileRawData> m_pCachedFileData;

	// The F* emulation functions critical sectio: protects all F* functions
	// that don't have a chance to be called recursively (to avoid deadlocks)
	CryReadModifyLock m_csMain;

	// open zip cache objects that can be reused. They're self-[un]registered
	// they're sorted by the path and 
	typedef std::vector<ICryArchive*> ArchiveArray;
	ArchiveArray m_arrArchives;

	// the array of opened caches - they get destructed by themselves (these are auto-pointers, see the ZipDir::Cache documentation)
	struct PackDesc
	{
		string strBindRoot; // the zip binding root WITH the trailing native slash
		string strFileName; // the zip file name (with path) - very useful for debugging so please don't remove
#if defined(USE_COMMON_PATH_SPEEDUP)
		string strSharedRootPathName; // the zip binding root among the minimum shared path
#endif
		const char* GetFullPath()const {return pZip->GetFilePath();}

		ICryArchive_AutoPtr pArchive;
		ZipDir::CachePtr pZip;
		size_t sizeofThis()
		{
			return strBindRoot.capacity() + strFileName.capacity() + pZip->GetSize();
		}
	};
	typedef std::vector<PackDesc > ZipArray;
	CryReadModifyLock m_csZips;
	ZipArray m_arrZips;
	friend class CCryPakFindData;

	CryReadModifyLock m_csFindData;
	typedef std::set<CCryPakFindData_AutoPtr> CryPakFindDataSet;
	CryPakFindDataSet m_setFindData;

	IMiniLog *m_pLog;

	// the root: "C:\MasterCD\"		  
	string m_strDataRoot;
	string m_strDataRootWithSlash;

	// this is the list of MOD subdirectories that will be prepended to the actual relative file path
	// they all have trailing forward slash. "" means the root dir
	CryReadModifyLock m_csMods;
	std::vector<string> m_arrMods;

	// this is the list of aliases, used to replace certain folder(s) or file name(s).
	CryReadModifyLock m_csAliases;
	typedef std::vector<tNameAlias*> TAliasList;
	TAliasList m_arrAliases;

	//////////////////////////////////////////////////////////////////////////
	// Opened files collector.
	//////////////////////////////////////////////////////////////////////////

	ICryPak::ERecordFileOpenList													m_eRecordFileOpenList;
	typedef std::set<string,stl::less_stricmp<string> >		RecordedFilesSet;
	RecordedFilesSet																			m_recordedFilesSet;

	_smart_ptr<IResourceList>															m_pEngineStartupResourceList;
	_smart_ptr<IResourceList>															m_pLevelResourceList;
	_smart_ptr<IResourceList>															m_pNextLevelResourceList;

	ITimer *																							m_pITimer;
	float																									m_fFileAcessTime;					// Time used to perform file operations
	std::vector<ICryPakFileAcesssSink *>									m_FileAccessSinks;				// useful for gathering file access statistics

	const PakVars*																				m_pPakVars;
	bool																									m_bLvlRes;								// if asset tracking is enabled for GetRecorderdResourceList() - all assets since executable start are recorded
	bool                                                  m_bGameFolderWritable;
	bool																									m_disableRuntimeFileAccess[2];

	CPakHandleCache																				m_PakHandleCache;

	ICVar*																								m_pLogInvalidFileAccess;
	ICVar*																								m_pMessageInvalidFileAccess;

	//threads which we don't want to access files from during the game
	uint32																								m_mainThreadId;
	uint32																								m_renderThreadId;

  bool InitPack(const char *szBasePath, unsigned nFlags = FLAGS_PATH_REAL);

	// Return true if alias was adjusted
	bool AdjustAliases(char *dst);
	const char* AdjustFileNameInternal(const char *src, char dst[g_nMaxPath], unsigned nFlags );

public:
	// given the source relative path, constructs the full path to the file according to the flags
	const char* AdjustFileName (const char *src, char dst[g_nMaxPath], unsigned nFlags );

	// this is the start of indexation of pseudofiles: 
	// to the actual index , this offset is added to get the valid handle
	enum {g_nPseudoFileIdxOffset = 1};

	// this defines which slash will be kept internally
#if defined(PS3) || defined(LINUX)
	enum {g_cNativeSlash = '/', g_cNonNativeSlash = '\\'};
#else
	enum {g_cNativeSlash = '\\', g_cNonNativeSlash = '/'};
#endif
  // makes the path lower-case and removes the duplicate and non native slashes
  // may make some other fool-proof stuff
  // may NOT write beyond the string buffer (may not make it longer)
  // returns: the pointer to the ending terminator \0
  static char* BeautifyPath(char* dst);
	static char* BeautifyPathForWrite(char* dst);

	CCryPak( IMiniLog *pLog,PakVars *pPakVars, const bool bLvlRes );
	~CCryPak();

	const PakVars* GetPakVars()const {return m_pPakVars;}

public: // ---------------------------------------------------------------------------------------

	ICryPak::PakInfo* GetPakInfo();
	void FreePakInfo (PakInfo*);

	// Set from outside if game folder is writable
	void SetGameFolderWritable( bool bWritable );
	void SetLog( IMiniLog *pLog );

	//! adds a mod to the list of mods
	void AddMod(const char* szMod);
	//! removes a mod from the list of mods
	void RemoveMod(const char* szMod);
	//! returns indexed mod path, or NULL if out of range
	VIRTUAL const char* GetMod(int index);

	//! Processes an alias command line containing multiple aliases.
	void ParseAliases(const char* szCommandLine);
	//! adds or removes an alias from the list - if bAdd set to false will remove it
	void SetAlias(const char* szName,const char* szAlias,bool bAdd);
	//! gets an alias from the list, if any exist. 
	//! if bReturnSame==true, it will return the input name if an alias doesn't exist. Otherwise returns NULL
	const char *GetAlias(const char* szName,bool bReturnSame=true);

	// Set and Get "Game" folder (/Game, /Game04, ...)
	void SetGameFolder(const char* szFolder);
	const char* GetGameFolder() const;

	//! Puts the memory statistics into the given sizer object
	//! According to the specifications in interface ICrySizer
	void GetMemoryStatistics(ICrySizer *pSizer);

	// lock all the operations
	VIRTUAL void Lock();
	VIRTUAL void Unlock();

	// open the physical archive file - creates if it doesn't exist
	// returns NULL if it's invalid or can't open the file
	VIRTUAL ICryArchive* OpenArchive (const char* szPath, unsigned int nFlags=0);

	// returns the path to the archive in which the file was opened
	VIRTUAL const char* GetFileArchivePath (FILE* f);

	void Register (CCachedFileData* p)
	{
		// actually, registration may only happen when the set is already locked, but for generality..
		AUTO_MODIFYLOCK(m_csCachedFiles);
		m_setCachedFiles.push_back(p);
	}

	void Unregister (CCachedFileData* p)
	{
		AUTO_MODIFYLOCK(m_csCachedFiles);
		stl::find_and_erase(m_setCachedFiles,p);
	}

	//! Return pointer to pool if available
	VIRTUAL void * PoolMalloc(size_t size);
	//! Free pool
	VIRTUAL void PoolFree(void * p);

	VIRTUAL IMemoryBlock* PoolAllocMemoryBlock( size_t nSize,const char *sUsage );

	// interface ICryPak ---------------------------------------------------------------------------

	VIRTUAL void RegisterFileAccessSink( ICryPakFileAcesssSink *pSink );
	VIRTUAL void UnregisterFileAccessSink( ICryPakFileAcesssSink *pSink );
	VIRTUAL bool GetLvlResStatus() const { return m_bLvlRes; }

  VIRTUAL bool Init (const char *szBasePath);
  virtual void Release();
  VIRTUAL bool OpenPack(const char *pName, unsigned nFlags = 0);
	VIRTUAL bool OpenPack(const char* szBindRoot, const char *pName, unsigned nFlags = 0);
	// after this call, the file will be unlocked and closed, and its contents won't be used to search for files
	VIRTUAL bool ClosePack(const char* pName, unsigned nFlags = 0);
	VIRTUAL bool OpenPacks(const char *pWildcard, unsigned nFlags = 0);
	VIRTUAL bool OpenPacks(const char* szBindRoot, const char *pWildcard, unsigned nFlags = 0);

	bool OpenPackCommon(const char* szBindRoot, const char *pName, unsigned int nPakFlags );
	bool OpenPacksCommon(const char* szDir, const char *pWildcardIn, char *cWork,int nPakFlags);

	// closes pack files by the path and wildcard
	VIRTUAL bool ClosePacks(const char* pWildcard, unsigned nFlags = 0);

	// returns the file modification time
	VIRTUAL uint64 GetModificationTime(FILE* f);


	// this function gets the file data for the given file, if found.
	// The file data object may be created in this function,
	// and it's important that the autoptr is returned: another thread may release the existing
	// cached data before the function returns
	// the path must be absolute normalized lower-case with forward-slashes
	CCachedFileDataPtr GetFileData(const char* szName);

	// tests if the given file path refers to an existing file inside registered (opened) packs
	// the path must be absolute normalized lower-case with forward-slashes
	ZipDir::FileEntry* FindPakFileEntry(const char* szPath,ZipDir::Cache **pZip=0 );

  VIRTUAL FILE *FOpen(const char *pName, const char *mode, unsigned nPathFlags=0);
	VIRTUAL FILE *FOpen(const char *pName, const char *mode,char *szFileGamePath,int nLen);
  VIRTUAL size_t FReadRaw(void *data, size_t length, size_t elems, FILE *handle);
	VIRTUAL size_t FReadRawAll(void *data, size_t nFileSize, FILE *handle);
	VIRTUAL void* FGetCachedFileData( FILE *handle,size_t &nFileSize );
  VIRTUAL size_t FWrite(const void *data, size_t length, size_t elems, FILE *handle);
  VIRTUAL size_t FSeek(FILE *handle, long seek, int mode);
  VIRTUAL long FTell(FILE *handle);
  VIRTUAL int FFlush(FILE *handle);
  VIRTUAL int FClose(FILE *handle);
  VIRTUAL intptr_t FindFirst(const char *pDir, _finddata_t *fd,unsigned int nPathFlags=0);
  VIRTUAL int FindNext(intptr_t handle, _finddata_t *fd);
  VIRTUAL int FindClose(intptr_t handle);
  VIRTUAL int FEof(FILE *handle);
	VIRTUAL int FError(FILE *handle);
	VIRTUAL int FGetErrno() ;
  virtual int FScanf(FILE *, const char *, ...) SCANF_PARAMS(3, 4);
  VIRTUAL char *FGets(char *, int, FILE *);
  VIRTUAL int Getc(FILE *);
  VIRTUAL int Ungetc(int c, FILE *);
  VIRTUAL int FPrintf(FILE *handle, const char *format, ...) PRINTF_PARAMS(3, 4);
	VIRTUAL size_t FGetSize(FILE* f);
	VIRTUAL size_t FGetSize(const char* sFilename);
	VIRTUAL bool IsInPak(FILE *handle);
	VIRTUAL bool RemoveFile(const char* pName); // remove file from FS (if supported)
	VIRTUAL bool RemoveDir(const char* pName, bool bRecurse);  // remove directory from FS (if supported)
	VIRTUAL bool IsAbsPath(const char* pPath);

	VIRTUAL bool IsFileExist( const char *sFilename, EFileSearchLocation fileLocation = eFileLocation_Any );

	// creates a directory
	VIRTUAL bool MakeDir ( const char* szPath,bool bGamePathMapping=false );

	// returns the current game directory, with trailing slash (or empty string if it's right in MasterCD)
	// this is used to support Resource Compiler which doesn't have access to this interface:
	// in case all the contents is located in a subdirectory of MasterCD, this string is the subdirectory name with slash
	//virtual const char* GetGameDir();

	void Register (ICryArchive* pArchive);
	void Unregister (ICryArchive* pArchive);
	ICryArchive* FindArchive (const char* szFullPath);

	// compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
	// returns one of the Z_* errors (Z_OK upon success)
	// MT-safe
	int RawCompress (const void* pUncompressed, unsigned long* pDestSize, void* pCompressed, unsigned long nSrcSize, int nLevel = -1);

	// Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
	// returns one of the Z_* errors (Z_OK upon success)
	// This function just mimics the standard uncompress (with modification taken from unzReadCurrentFile)
	// with 2 differences: there are no 16-bit checks, and 
	// it initializes the inflation to start without waiting for compression method byte, as this is the 
	// way it's stored into zip file
	int RawUncompress (void* pUncompressed, unsigned long* pDestSize, const void* pCompressed, unsigned long nSrcSize) ;

	//////////////////////////////////////////////////////////////////////////
	// Files opening recorder.
	//////////////////////////////////////////////////////////////////////////
	void RecordFileOpen( const ERecordFileOpenList eMode );
	void RecordFile( FILE *in, const char *szFilename );

	VIRTUAL IResourceList* GetRecorderdResourceList( const ERecordFileOpenList eList );
	VIRTUAL ERecordFileOpenList GetRecordFileOpenList() { return m_eRecordFileOpenList; }

	VIRTUAL void Notify( ENotifyEvent event );
	VIRTUAL uint32 ComputeCRC( const char* szPath );
	VIRTUAL bool ComputeMD5(const char* szPath, unsigned char* md5);

	CPakHandleCache& PakHandleCache(){return m_PakHandleCache;}

	void OnMissingFile (const char* szPath);

	VIRTUAL void DisableRuntimeFileAccess( bool status )
	{
		m_disableRuntimeFileAccess[0] = status;
		m_disableRuntimeFileAccess[1] = status;
	}

	VIRTUAL bool DisableRuntimeFileAccess( bool status, uint32 threadId );
	VIRTUAL bool CheckFileAccessDisabled(const char* name, const char * mode);

	VIRTUAL void SetRenderThreadId(uint32 renderThreadId)
	{
		m_renderThreadId = renderThreadId;
	}
	
	// missing file -> count of missing files
	CryCriticalSection m_csMissingFiles;
	typedef std::map<string, uint32, std::less<string> > MissingFileMap;
	MissingFileMap m_mapMissingFiles;

	friend struct SAutoCollectFileAcessTime;

	VIRTUAL void SetCachingState(eCacheState state);

	// gets the current pak priority
	VIRTUAL int GetPakPriority();
#ifdef LINUX
private:
	intptr_t m_HandleSource;
	std::map<intptr_t, CCryPakFindData* > m_Handles;
#endif
};

struct CryArchiveSortByName
{
	bool operator () (const ICryArchive* left, const ICryArchive* right)const
	{
		return stricmp(left->GetFullPath(), right->GetFullPath()) < 0;
	}
	bool operator () (const char* left, const ICryArchive* right)const
	{
		return stricmp(left, right->GetFullPath()) < 0;
	}
	bool operator () (const ICryArchive* left, const char* right)const
	{
		return stricmp(left->GetFullPath(), right) < 0;
	}
};

template <class Cache>
class TCryArchive: public ICryArchive
{			
public:
	TCryArchive (CCryPak*pPak, const string& strBindRoot, Cache* pCache, unsigned nFlags = 0):
		m_pCache(pCache),
		m_strBindRoot (strBindRoot),
		m_pPak(pPak),
		m_nFlags (nFlags)
	{
		pPak->Register(this);
	}

	~TCryArchive()
	{
		m_pPak->Unregister (this);
	}

	// finds the file; you don't have to close the returned handle
	Handle FindFile (const char* szRelativePath)
	{
		char szFullPath[CCryPak::g_nMaxPath];
		const char*pPath = AdjustPath (szRelativePath, szFullPath);
		if (!pPath)
			return NULL;
		return m_pCache->FindFile(pPath);
	}
	
	// returns the size of the file (unpacked) by the handle
	unsigned GetFileSize (Handle h)
	{
		assert (m_pCache->IsOwnerOf((ZipDir::FileEntry*)h));
		return ((ZipDir::FileEntry*)h)->desc.lSizeUncompressed;
	}

	// reads the file into the preallocated buffer (must be at least the size of GetFileSize())
	int ReadFile (Handle h, void* pBuffer)
	{
		assert (m_pCache->IsOwnerOf((ZipDir::FileEntry*)h));
		return m_pCache->ReadFile ((ZipDir::FileEntry*)h, NULL, pBuffer);
	}

	// returns the full path to the archive file
	const char* GetFullPath() const
	{
		return m_pCache->GetFilePath();
	}

	unsigned GetFlags() const {return m_nFlags;}
	bool SetFlags(unsigned nFlagsToSet)
	{
		if (nFlagsToSet & FLAGS_RELATIVE_PATHS_ONLY)
			m_nFlags |= FLAGS_RELATIVE_PATHS_ONLY;

		if (nFlagsToSet & ~(FLAGS_RELATIVE_PATHS_ONLY))
		{
			// we don't support changing of any other flags
			return false;
		}
		return true;
	}

	bool ResetFlags (unsigned nFlagsToReset)
	{
		if (nFlagsToReset & FLAGS_RELATIVE_PATHS_ONLY)
			m_nFlags &= ~FLAGS_RELATIVE_PATHS_ONLY;

		if (nFlagsToReset & ~(FLAGS_RELATIVE_PATHS_ONLY))
		{
			// we don't support changing of any other flags
			return false;
		}
		return true;
	}


	Cache* GetCache() {return m_pCache;}
protected:
	// returns the pointer to the relative file path to be passed
	// to the underlying Cache pointer. Uses the given buffer to construct the path.
	// returns NULL if the file path is invalid
	const char* AdjustPath (const char* szRelativePath, char szFullPathBuf[CCryPak::g_nMaxPath])
	{
		if (!szRelativePath[0])
			return NULL;

		if (m_nFlags & FLAGS_RELATIVE_PATHS_ONLY)
			return szRelativePath;

		if (szRelativePath[1] == ':' || (m_nFlags & FLAGS_ABSOLUTE_PATHS))
		{
			// make the normalized full path and try to match it against the binding root of this object
			const char* szFullPath = m_pPak->AdjustFileName (szRelativePath, szFullPathBuf, ICryPak::FLAGS_PATH_REAL );
			size_t nPathLen = strlen(szFullPath);
			if (nPathLen <= m_strBindRoot.length())
				return NULL;

			// you should access exactly the file under the directly in which the zip is situated
			if (szFullPath[m_strBindRoot.length()] != '/' && szFullPath[m_strBindRoot.length()] != '\\')
				return NULL;
			if (memicmp(szFullPath, m_strBindRoot.c_str(), m_strBindRoot.length()))
				return NULL; // the roots don't match

			return szFullPath + m_strBindRoot.length() + 1;
		}

		return szRelativePath;
	}
protected:
	_smart_ptr<Cache> m_pCache;
	// the binding root may be empty string - in this case, the absolute path binding won't work
	string m_strBindRoot;
	CCryPak* m_pPak;
	unsigned m_nFlags;
};


class CryArchiveRW: public TCryArchive<ZipDir::CacheRW>
{
public:

	CryArchiveRW (CCryPak*pPak, const string& strBindRoot, ZipDir::CacheRW* pCache, unsigned nFlags = 0):
		TCryArchive<ZipDir::CacheRW>(pPak, strBindRoot, pCache, nFlags)
	{
	}

	~CryArchiveRW()
	{
	}

	// Adds a new file to the zip or update an existing one
	// adds a directory (creates several nested directories if needed)
	// compression methods supported are 0 (store) and 8 (deflate) , compression level is 0..9 or -1 for default (like in zlib)
	int UpdateFile (const char* szRelativePath, void* pUncompressed, unsigned nSize, unsigned nCompressionMethod = 0, int nCompressionLevel = -1);

	//   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
	int	StartContinuousFileUpdate( const char* szRelativePath, unsigned nSize );

	// Adds a new file to the zip or update an existing's segment if it is not compressed - just stored 
	// adds a directory (creates several nested directories if needed)
	// Arguments:
	//   nOverwriteSeekPos - 0xffffffff means the seek pos should not be overwritten
	int UpdateFileContinuousSegment (const char* szRelativePath, unsigned nSize, void* pUncompressed, unsigned nSegmentSize, unsigned nOverwriteSeekPos );
	
	virtual int UpdateFileCRC( const char* szRelativePath, const uint32 dwCRC );

	// deletes the file from the archive
	int RemoveFile (const char* szRelativePath);

	// deletes the directory, with all its descendants (files and subdirs)
	int RemoveDir (const char* szRelativePath);

	int RemoveAll();

	enum {gClassId = 1};
	unsigned GetClassId()const {return gClassId;}
};


class CryArchive: public TCryArchive<ZipDir::Cache>
{
public:
	CryArchive (CCryPak* pPak, const string& strBindRoot, ZipDir::Cache* pCache, unsigned nFlags):
		TCryArchive<ZipDir::Cache>(pPak, strBindRoot, pCache, nFlags)
	{	}

	~CryArchive(){}

	// Adds a new file to the zip or update an existing one
	// adds a directory (creates several nested directories if needed)
	// compression methods supported are METHOD_STORE == 0 (store) and
	// METHOD_DEFLATE == METHOD_COMPRESS == 8 (deflate) , compression
	// level is LEVEL_FASTEST == 0 till LEVEL_BEST == 9 or LEVEL_DEFAULT == -1
	// for default (like in zlib)
	int UpdateFile (const char* szRelativePath, void* pUncompressed, unsigned nSize, unsigned nCompressionMethod = 0, int nCompressionLevel = -1) {return ZipDir::ZD_ERROR_INVALID_CALL;}

	//   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
	int	StartContinuousFileUpdate( const char* szRelativePath, unsigned nSize )  {return ZipDir::ZD_ERROR_INVALID_CALL;}

	// Adds a new file to the zip or update an existing's segment if it is not compressed - just stored 
	// adds a directory (creates several nested directories if needed)
	int UpdateFileContinuousSegment (const char* szRelativePath, unsigned nSize, void* pUncompressed, unsigned nSegmentSize, unsigned nOverwriteSeekPos )  {return ZipDir::ZD_ERROR_INVALID_CALL;}

	virtual int UpdateFileCRC( const char* szRelativePath, const uint32 dwCRC ) {return ZipDir::ZD_ERROR_INVALID_CALL;}

	// deletes the file from the archive
	int RemoveFile (const char* szRelativePath) {return ZipDir::ZD_ERROR_INVALID_CALL;}
	int RemoveAll();

	// deletes the directory, with all its descendants (files and subdirs)
	int RemoveDir (const char* szRelativePath) {return ZipDir::ZD_ERROR_INVALID_CALL;}
	enum {gClassId = 2};
	unsigned GetClassId()const {return gClassId;}

};

#endif

