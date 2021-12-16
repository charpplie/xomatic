#include "StdAfx.h"
#include "CritSection.h"
#include "RefStreamEngine.h"
#include "RefReadStreamProxy.h"
#include "RefReadStream.h"
#include "IDiskProfiler.h"

// creates a read stream from the file, in the engine, 
// with the given default principal client
// the path is always the real path, it shouldn't undergo MOD path adjustments
CRefReadStream::CRefReadStream (const string& strFileName, CRefStreamEngine* pEngine):
	m_pEngine (pEngine), 
	m_bError (false),    // no error yet because we didn't try to do anything
	m_strFileName (strFileName),
	m_nFileSize (0),
	m_nSectorSize(0),
	m_hFile (INVALID_HANDLE_VALUE),
	m_bOverlapped ( false),
	m_bDirectRead ( false),
	m_pZipEntry (NULL)
{
}

//duplicates the file handle
HANDLE CRefReadStream::DupFile()
{
	HANDLE hFile = CreateFile 
		(m_strFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,	FILE_FLAG_OVERLAPPED,	NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		CCachedFileDataPtr pZipEntry = m_pZipEntry;
		if (pZipEntry == NULL)
			pZipEntry = m_pEngine->GetPak()->GetFileData(m_strFileName.c_str());
		if (pZipEntry)
		{
			//try to open pak file
			const char* szPakFile = pZipEntry->GetZip()->GetFilePath();
			hFile = CreateFile 
				(szPakFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,	FILE_FLAG_OVERLAPPED, NULL);
		}
	}
	return hFile;
}

bool CRefReadStream::IsReqReading()
{
  if (m_strFileName.empty())
    return false;
  return true;
}

// activates: opens the file, gets its size. If failed, returns false
bool CRefReadStream::Activate()
{
	FUNCTION_PROFILER( gEnv->pSystem, PROFILE_SYSTEM );

	m_bDirectRead = false;
	m_bOverlapped = m_pEngine->isOverlappedIoEnabled();
	if (m_pZipEntry == NULL && m_hFile == INVALID_HANDLE_VALUE)
		m_hFile = CreateFile (m_strFileName.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			m_bOverlapped?FILE_FLAG_OVERLAPPED:0,
			NULL);

	if (m_pZipEntry == NULL && m_hFile == INVALID_HANDLE_VALUE)
	{
		// perhaps this is in a zip file
		m_pZipEntry = m_pEngine->GetPak()->GetFileData(m_strFileName.c_str());
		if (m_pZipEntry)
		{
			m_nFileSize = m_pZipEntry->GetFileEntry()->desc.lSizeUncompressed;
			m_bError = false;
			m_bDirectRead = m_pZipEntry->GetFileEntry()->nMethod == ZipFile::METHOD_STORE;

			// try to open the actual file if can be done - this is only worthy
			// if the data isn't yet in the cache AND the actual file is big enough
			// AND it's uncompressed in the zip file
			if (!m_pZipEntry->GetData(false, false)
				&& m_pZipEntry->GetFileEntry()->nMethod == ZipFile::METHOD_STORE
				//&& m_nFileSize > g_nMaxCacheUncompressed
				)
			{
				// try to open the file - this should really be not often the case
				const char* szPakFile = m_pZipEntry->GetZip()->GetFilePath();
				// even if we can't open it, it doesn't matter: we automatically resort to using the cache
				m_hFile = CreateFile (szPakFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
					m_bOverlapped?FILE_FLAG_OVERLAPPED:0,
					NULL);
			}

			if (m_hFile == INVALID_HANDLE_VALUE)
				m_bOverlapped = false;
			return true;
		}
		else
		{
			DWORD dwError = GetLastError();
			m_bError = true;
			m_pEngine->GetPak()->OnMissingFile(m_strFileName.c_str());
			return false;
		}
	}
	else
	{
		if (!m_nFileSize)
		{
			if (m_pZipEntry)
				m_nFileSize = m_pZipEntry->GetFileEntry()->desc.lSizeUncompressed;
			else
				m_nFileSize = ::GetFileSize (m_hFile, NULL);
	
			if (m_nFileSize == INVALID_FILE_SIZE)
			{
				m_bError = true;
				m_nFileSize = 0;
				return false;
			}
			m_bError = false;
		}
		m_bDirectRead = true;
		return true;
	}
}

// deactivates: closes the file
void CRefReadStream::Deactivate()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

// the clients are not allowed to destroy this object directly; only via Release()
CRefReadStream::~CRefReadStream()
{
	Deactivate();
	// carefully destruct shared cached data
	if(m_pZipEntry)
	{
		AutoLockModify<CryReadModifyLock> lock(m_pEngine->GetPak()->m_csCachedFiles);
		m_pZipEntry = NULL;
	}
}

// request to abort comes from the proxy. This doesn't means immediate deallocation.
void CRefReadStream::Abort(CRefReadStreamProxy* pProxy)
{
	if (m_setProxies.size() == 1 && m_hFile != INVALID_HANDLE_VALUE)
	{
		// there's only one proxy that uses this object; so we can safely cancel io
		// on this file
		CancelIo (m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}

// Client (through the given Proxy) has requested priority rise
void CRefReadStream::OnRaisePriority (CRefReadStreamProxy* pProxy, unsigned nPriority)
{
	if (!pProxy->IsIOExecuted())
		m_pEngine->SortIOJobs();
}

// dumps all clients (proxies) - each uses the Dump function of the proxy
string CRefReadStream::Dump()
{
	string strClients;
	for (ProxySet::iterator it = m_setProxies.begin(); it != m_setProxies.end(); ++it)
	{
		if (!strClients.empty())
			strClients += ",";
		strClients += (*it)->Dump();
	}
	return "{"+strClients+"}";
}

// returns the size of allocated memory for this object and all subobjects (Proxies)
size_t CRefReadStream::GetSize()
{
	size_t nSize = sizeof(*this);
	nSize += m_strFileName.capacity();

	for (ProxySet::iterator it = m_setProxies.begin();  it != m_setProxies.end(); ++it)
		nSize += sizeof(ProxySet::value_type) + (*it)->GetSize();
	return nSize;
}

bool CRefReadStream::ReadFileData(const bool wholeFile)
{
	if (m_pZipEntry)
	{
		CryCriticalSection& cs = m_pZipEntry->m_csDecompressLock;
		AUTO_LOCK_CS(cs);
		bool res = m_pZipEntry->GetData(true, false, !wholeFile) != NULL;
		if (res) {
			PROFILE_DISK_SEEK_WITHNAME(m_strFileName);
		}
		return res;
	}
	else
		return false;
}

void* CRefReadStream::GetFileData()
{
	assert(IsDecompressed());
	if(m_pZipEntry) {
		{
			PROFILE_DISK_SEEK_WITHNAME(m_strFileName);
		}
		return m_pZipEntry->GetData(false, false);
	}
	return NULL;
}

void CRefReadStream::DecompressTo(void* pDestination, size_t nBufferSize)
{
	CryCriticalSection& cs = m_pZipEntry->m_csDecompressLock;
	AUTO_LOCK_CS(cs);
	if(IsDecompressed())
		return;
	if(m_pZipEntry->m_pFileEntry->desc.lSizeUncompressed > nBufferSize)
	{
		assert(0);
		gEnv->pLog->LogError("Failed to decompress zip entry: %s, not enough buffer to unzip: %d(required: %d)", m_strFileName.c_str(), (int)nBufferSize, m_pZipEntry->m_pFileEntry->desc.lSizeUncompressed);
		m_bError = true;
		return;
	}
	if(0 != m_pZipEntry->GetZip()->DecompressFile(m_pZipEntry->m_pFileEntry, m_pZipEntry->m_pFileData, pDestination ? pDestination : m_pZipEntry->m_pFileData, m_pZipEntry->m_csDecompressLock))
	{
		gEnv->pLog->LogError("Failed to decompress zip entry: %s", m_strFileName.c_str());
	}
	if(!pDestination)
		m_pZipEntry->m_bDecompressed = true;
}

