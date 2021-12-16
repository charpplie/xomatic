// ResFile.cpp : implementation file
//

#include "StdAfx.h"
#include "ResFile.h"


CResFile CResFile::m_Root;
int CResFile::m_nNumOpenResources;
uint32 CResFile::m_nSizeDir;

namespace
{
  CryCriticalSection g_cResLock;
}

void CResFile::mfDeactivate()
{
  AUTO_LOCK(g_cResLock); // Not thread safe without this

  if (m_handle)
  {
    mfFlush(0);
    gEnv->pCryPak->FClose(m_handle);
    m_handle = NULL;
    m_nNumOpenResources--;
  }






  Unlink();
}

bool CResFile::mfActivate(bool bFirstTime)
{
  AUTO_LOCK(g_cResLock); // Not thread safe without this

  Relink(&m_Root);
  if (m_handle)
    return true;
  if (m_nNumOpenResources >= MAX_OPEN_RESFILES)
  {
    CResFile *rf = m_Root.m_Prev;
    assert(rf && rf->m_handle);
    rf->mfDeactivate();
  }

  LOADING_TIME_PROFILE_SECTION(iSystem);
	CDebugAllowFileAccess dafa;

  if (!bFirstTime && m_szAccess[0] == 'w')
  {
    char szAcc[16];
    strcpy(szAcc, m_szAccess);
    szAcc[0] = 'r';
		m_handle = gEnv->pCryPak->FOpen(m_name, szAcc, ICryPak::FOPEN_HINT_DIRECT_OPERATION );
  }
  else
    m_handle = gEnv->pCryPak->FOpen(m_name, m_szAccess, ICryPak::FOPEN_HINT_DIRECT_OPERATION );
  if (!m_handle)
  {
    mfSetError("CResFile::Activate - Can't open resource file <%s>", m_name);
    Unlink();
    return false;
  }
  mfPrepareDir();
  //if (strstr(m_name, "FPVS"))
  {
    //gEnv->pCryPak->FSeek(m_handle, 0, SEEK_END);
    //int length = gEnv->pCryPak->FTell(m_handle);
    //int nnn = 0;
  }
  m_nNumOpenResources++;
  return true;
}

CResFile::CResFile()
{
  m_name[0] = 0;
  m_handle = NULL;
  m_nOffsDir = 0;
	m_bSwapEndianRead = false;
  m_bSwapEndianWrite = false;

  m_Next = NULL;
  m_Prev = NULL;
  m_bDirty = false;
  m_bDirValid = false;
  m_bDirCompressed = false;
  m_pCompressedDir = NULL;
  m_nComprDirSize = 0;
  m_nComprNumFiles = 0;
  m_nOffset = OFFSET_BIG_POSITIVE;

  if (!m_Root.m_Next)
  {
    m_Root.m_Next = &m_Root;
    m_Root.m_Prev = &m_Root;
  }
}

CResFile::CResFile(const char* name)
{
  strcpy_s(m_name, name);
  m_handle = NULL;
  m_nOffsDir = 0;
  m_bSwapEndianRead = false;
  m_bSwapEndianWrite = false;

  m_Next = NULL;
  m_Prev = NULL;
  m_bDirty = false;
  m_bDirValid = false;
  m_bDirCompressed = false;
  m_pCompressedDir = NULL;
  m_nComprDirSize = 0;
  m_nComprNumFiles = 0;
  m_nOffset = OFFSET_BIG_POSITIVE;

  if (!m_Root.m_Next)
  {
    m_Root.m_Next = &m_Root;
    m_Root.m_Prev = &m_Root;
  }
}

CResFile::~CResFile()
{
  mfClose();
  if (this == &m_Root)
  {
    assert(CResFile::m_nNumOpenResources == 0);
  }
}

void CResFile::mfSetError(const char *er, ...)
{
  char buffer[1024];
  va_list args;
  va_start(args, er);
  if (vsnprintf(buffer,sizeof(buffer), er, args) == -1)
    buffer[sizeof(buffer)-1] = 0;
  m_ermes = buffer;
  va_end(args);
}

const char* CResFile::mfGetError(void)
{
  if (m_ermes.size())
    return m_ermes.c_str();
  else
    return NULL;
}

int CResFile::mfGetResourceSize()
{
  if (!m_handle)
    return 0;

  AUTO_LOCK(g_cResLock); // Not thread safe without this

  gEnv->pCryPak->FSeek(m_handle, 0, SEEK_END);
  int length = gEnv->pCryPak->FTell(m_handle);
  gEnv->pCryPak->FSeek(m_handle, 0, SEEK_SET);

  return length;
}

uint64 CResFile::mfGetModifTime()
{
  if (!mfActivate(false))
    return 0;
  return gEnv->pCryPak->GetModificationTime(m_handle);
}

bool CResFile::mfSetModifTime(uint64 MTime)
{
#if !defined(LINUX)
  mfDeactivate();
  DWORD error = 0;
  char szDestBuf[256];
  const char *szDest = gEnv->pCryPak->AdjustFileName(m_name, szDestBuf, 0);




  HANDLE hdst = CreateFile(szDest, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hdst == INVALID_HANDLE_VALUE)
  {
    assert(0);
    /*error = GetLastError();
    LPVOID lpMsgBuf;
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL );
    LocalFree(lpMsgBuf);*/
    return false;
  }
  else
  {
    FILETIME ft = *((FILETIME*)&MTime);
    SetFileTime(hdst,NULL,NULL,&ft);
    CloseHandle(hdst);
  }

  return true;
#else
  return false;
#endif//LINUX
}

bool CResFile::mfFileExist(CCryNameCRC name)
{
  SDirEntry *de = mfGetEntry(name);
  if (!de)
    return false;
  assert(name == de->Name);
  return true;
}

bool CResFile::mfFileExist(const char* name)
{
  return mfFileExist(CCryNameCRC(name));
}

bool CResFile::mfPrepareDir()
{
  if (m_bDirValid)
    return true;
  if (!m_pCompressedDir)
    return false;
  assert(!m_Dir.size());
  SFileDirEntry_name *pFileDir = new SFileDirEntry_name[m_nComprNumFiles];
  if (m_version == RESVERSION_LZSS)
    Decodem(m_pCompressedDir, (byte *)pFileDir, m_nComprDirSize);
  else
  {
    SizeT size = sizeof(SFileDirEntry_name)*m_nComprNumFiles;
    SizeT inSize = m_nComprDirSize;
    int res = Lzma86_Decode((byte *)pFileDir, &size, m_pCompressedDir, &inSize);
  }

  int i;
  m_Dir.resize(m_nComprNumFiles);

  for (i=0; i<m_nComprNumFiles; i++)
  {
    SDirEntry *deS = &m_Dir[i];
    SFileDirEntry_name& fdent = pFileDir[i];
    if (m_bSwapEndianRead)
      SwapEndian(fdent, eBigEndian);
    deS->Name = fdent.cName;
    deS->size = fdent.size;
    deS->offset = fdent.offset;
    deS->curseek = 0;
    deS->flags = fdent.flags;
    deS->pData = NULL;
    //m_dir.insert(ResFilesMapItor::value_type(deS->Name, i));
  }
  m_nSizeDir += m_nComprDirSize*sizeof(SDirEntry);
  m_bDirValid = true;

  return true;
}

void CResFile::mfReleaseDir()
{
  // Never unload directory which wasn't flushed yet
  if (m_bDirty)
    return;

  if (m_bDirValid)
  {
    uint32 i;
    for (i=0; i<m_Dir.size(); i++)
    {
      SDirEntry* de = &m_Dir[i];
      assert(!(de->flags & RF_NOTSAVED));
      SAFE_DELETE_VOID_ARRAY(de->pData);
    }

    m_Dir.clear();
    //m_dir.clear();
    if (m_bDirCompressed)
      m_nSizeDir -= m_nComprDirSize*sizeof(SDirEntry);
    m_bDirValid = false;
  }
  else
  {
    assert(!m_Dir.size());
  }
}

bool CResFile::mfOpen(int type)
{
  SFileResHeader frh;
  SFileDirEntry_name fden;

  PROFILE_FRAME(Resource_Open);

  if (m_name[0] == 0)
  {
    mfSetError("Open - No Resource name");
    return false;
  }
  m_bSwapEndianWrite = (type & RA_ENDIANS) != 0;
  m_bSwapEndianRead = m_bSwapEndianWrite;
  type &= ~RA_ENDIANS;
  if (type == RA_READ)
    m_szAccess = "rb";
  else
  if (type == (RA_WRITE|RA_READ))
    m_szAccess = "r+b";
  else
  if (type & RA_CREATE)
    m_szAccess = "w+b";
  else
  {
    mfSetError("Open - Wrong access mode");
    return false;
  }
  m_typeaccess = type;
  mfActivate(true);

  AUTO_LOCK(g_cResLock); // Not thread safe without this

  if (!m_handle)
  {
    if (type & (RA_WRITE | RA_CREATE))
    {
      char name[256];
      gEnv->pCryPak->AdjustFileName(m_name, name, 0);
      FILE *statusdst = fopen(name, "rb");
      if (statusdst)
      {
/*
#if defined(LINUX)
        struct stat64 st;
#else
        struct __stat64 st;
#endif
        int result = _fstat64(fileno(statusdst), &st);
        if (result == 0)
        {
#if defined(LINUX)
          if (st.st_mode & FILE_ATTRIBUTE_READONLY)
            chmod(name, 0x777);//set to full access
#else
          if (!(st.st_mode & _S_IWRITE))
            _chmod(name, _S_IREAD | _S_IWRITE);
#endif
        }
						*/
        fclose(statusdst);
        m_ermes.clear();

				CrySetFileAttributes( name,FILE_ATTRIBUTE_ARCHIVE );

        mfActivate(true);
      }
    }
    if (!m_handle)
    {
      mfSetError("Open - Can't open resource file <%s>", m_name);
      return false;
    }
  }

  if (type & RA_READ)
  {
		// Detect file endianness automatically.
    if ( gEnv->pCryPak->FReadRaw(&frh, 1, sizeof(frh), m_handle) != sizeof(frh))
    {
      mfSetError("Open - Reading fault");
      return false;
    }
    if (m_bSwapEndianRead)
      SwapEndian(frh, eBigEndian);
    if (frh.hid != IDRESHEADER)
    {
      mfSetError("Open - Wrong header MagicID");
      return false;
    }
    if (frh.ver != RESVERSION_LZSS && frh.ver != RESVERSION_LZMA)
    {
      mfSetError("Open - Wrong version number");
      return false;
    }
    m_version = frh.ver;
    if (!frh.num_files)
    {
      mfSetError("Open - Empty resource file");
      return false;
    }

    m_nOffsDir = frh.ofs_dir;
    gEnv->pCryPak->FSeek(m_handle, frh.ofs_dir, SEEK_SET);
    int nSize = frh.num_files*sizeof(SFileDirEntry_name);
    int nSizeDir = frh.size_dir ? frh.size_dir : nSize;
    m_pCompressedDir = new byte[nSizeDir];
    if (gEnv->pCryPak->FReadRaw(m_pCompressedDir, 1, nSizeDir, m_handle) != nSizeDir)
    {
      mfSetError("Open - Directory reading error");
      return false;
    }
    m_nComprNumFiles = frh.num_files;
    m_nComprDirSize = frh.size_dir;
    m_bDirValid = false;
    if (!frh.size_dir)
    {
      m_bDirValid = true;
      m_bDirCompressed = false;
      int i;
      m_Dir.resize(m_nComprNumFiles);
      SFileDirEntry_name *pFileDir = (SFileDirEntry_name *)m_pCompressedDir;

      for (i=0; i<m_nComprNumFiles; i++)
      {
        SDirEntry *deS = &m_Dir[i];
        SFileDirEntry_name& fdent = pFileDir[i];
        if (m_bSwapEndianRead)
          SwapEndian(fdent, eBigEndian);
        deS->Name = fdent.cName;
        deS->size = fdent.size;
        deS->offset = fdent.offset;
        deS->curseek = 0;
        deS->flags = fdent.flags;
        deS->pData = NULL;
        //m_dir.insert(ResFilesMapItor::value_type(deS->Name, i));
      }
      SAFE_DELETE_ARRAY(m_pCompressedDir);
    }
    else
      m_bDirCompressed = true;
  }
  else
  {
    frh.hid = IDRESHEADER;
#ifdef RES_LZMA
    int ver = RESVERSION_LZMA;
#else
    int ver = RESVERSION_LZSS;
#endif
    frh.ver = ver;
    frh.num_files = 0;
    frh.ofs_dir = -1;
    frh.size_dir = 0;
    m_version = ver;
    m_nOffsDir = sizeof(frh);
    SFileResHeader frhTemp, *pFrh;
    pFrh = &frh;
    if (m_bSwapEndianWrite)
    {
      frhTemp = frh;
      SwapEndian(frhTemp, eBigEndian);
      pFrh = &frhTemp;
    }
    if (gEnv->pCryPak->FWrite(pFrh, 1, sizeof(frh), m_handle) != sizeof(frh))
    {
      mfSetError("Open - Writing fault");
      return false;
    }
    m_nComprDirSize = 0;
    m_nComprNumFiles = 0;
    m_pCompressedDir = NULL;
    m_bDirValid = true;
  }

  return true;
}

bool CResFile::mfClose(void)
{
  AUTO_LOCK(g_cResLock); // Not thread safe without this

  if (m_typeaccess != RA_READ)
    mfFlush(0);

  mfDeactivate();
  assert(!m_bDirty);
  mfReleaseDir();

  SAFE_DELETE_ARRAY(m_pCompressedDir);

  return true;
}

struct ResDirSortByName
{
  bool operator () (const SDirEntry& left, const SDirEntry& right)const
  {
    return left.Name < right.Name;
  }
  bool operator () (const CCryNameCRC left, const SDirEntry& right)const
  {
    return left < right.Name;
  }
  bool operator () (const SDirEntry& left, CCryNameCRC right)const
  {
    return left.Name < right;
  }
};

SDirEntry *CResFile::mfGetEntry(CCryNameCRC name)
{
  ResDirIt it = std::lower_bound(m_Dir.begin(), m_Dir.end(), name, ResDirSortByName());
  if (it != m_Dir.end() && name == (*it).Name)
  {
    assert(m_bDirValid);
    return &(*it);
  }
  return NULL;
}

int CResFile::mfFileClose(SDirEntry *de)
{
  if (!(de->flags & RF_NOTSAVED))
  {
    SAFE_DELETE_VOID_ARRAY(de->pData);
    de->curseek = 0;
  }

  return 0;
}

int CResFile::mfFileRead(SDirEntry *de)
{
  uint32 size = 0;

  if (de->pData)
    return de->size;

  if (!mfActivate(false))
    return 0;

  AUTO_LOCK(g_cResLock); // Not thread safe without this

  //if (strstr(m_name, "FPVS") && m_Dir.size()==3)
  {
    //int nnn = 0;
  }

  if (de->flags & RF_COMPRESS)
  {
    gEnv->pCryPak->FSeek(m_handle, de->offset, SEEK_SET);
    byte* buf = new byte [de->size];
    if (!buf)
    {
      mfSetError("FileRead - Allocation fault");
      return 0;
    }
    if (m_version == RESVERSION_LZSS)
    {
      gEnv->pCryPak->FReadRaw(buf, de->size, 1, m_handle);
      size = *(int32 *)buf;
      if (m_bSwapEndianRead)
        SwapEndian(size, eBigEndian);
      if (size >= 10000000)
        return 0;
      de->pData = new byte [size];
      if (!de->pData)
      {
        mfSetError("FileRead - Allocation fault");
        return 0;
      }
      Decodem(&buf[4], (byte *)de->pData, de->size-4);
    }
    else
    {
      SizeT inSize = de->size;
      gEnv->pCryPak->FReadRaw(buf, inSize, 1, m_handle);
      uint64 outSize64;
      if (Lzma86_GetUnpackSize(buf, inSize, &outSize64) != 0)
      {
        mfSetError("FileRead - data error");
        return 0;
      }
      SizeT outSize = (uint32)outSize64;
      if (outSize != 0)
      {
        uint8 *outBuffer = (uint8 *)MyAlloc(outSize);
        if (outBuffer == 0)
        {
          mfSetError("FileRead - can't allocate");
          return 0;
        }
        SRes res = Lzma86_Decode(outBuffer, &outSize, buf, &inSize);
        if (res != 0)
        {
          mfSetError("FileRead - LzmaDecoder error");
          return 0;
        }
        size = outSize;
        de->pData = outBuffer;
      }
      /*NCompress::NLzma2::CDecoder *pDecoder = new NCompress::NLzma2::CDecoder;
      pDecoder->FinishStream = true;
      if (pDecoder->SetDecoderProperties2(header, kPropertiesSize) != S_OK)
      {
        mfSetError("FileRead - SetDecoderProperties error");
        return 0;
      }
      uint32 fileSize = 0;
      for (int i=0; i<4; i++)
        fileSize |= ((uint32)header[kPropertiesSize + i]) << (8 * i);
      if (pDecoder->Code(inStream, outStream, 0, fileSize == -1 ? 0 : &fileSize, 0) != S_OK)
      {
        mfSetError("FileRead - Decoder error");
        return 0;
      }*/
    }

    delete [] buf;
    return size;
  }

  de->pData = new byte [de->size];
  if (!de->pData)
  {
    mfSetError("FileRead - Allocation fault");
    return 0;
  }
  gEnv->pCryPak->FSeek(m_handle, de->offset, SEEK_SET);
  if (gEnv->pCryPak->FReadRaw(de->pData, 1, de->size, m_handle) != de->size)
  {
    mfSetError("FileRead - Reading fault");
    return 0;
  }

  //if (strstr(m_name, "FPVS") && m_Dir.size()==3)
  {
    //gEnv->pCryPak->FSeek(m_handle, 0, SEEK_END);
    //int nSize = gEnv->pCryPak->FTell(m_handle);
    //int nnn = 0;
  }

  return de->size;
}

byte *CResFile::mfFileReadCompressed(SDirEntry *de, short& nSizeDecomp, short& nSizeComp)
{
  if (!mfActivate(false))
    return NULL;

  if (de->flags & RF_COMPRESS)
  {
    if (de->offset >= 0x10000000)
      return NULL;

    gEnv->pCryPak->FSeek(m_handle, de->offset, SEEK_SET);
    byte* buf = new byte [de->size];
    if (!buf)
    {
      mfSetError("FileRead - Allocation fault");
      return 0;
    }
    if (m_version == RESVERSION_LZSS)
    {
      int nSize = -1;
      gEnv->pCryPak->FReadRaw(&nSize, 1, sizeof(int), m_handle);
      if (m_bSwapEndianRead)
        SwapEndian(nSize, eBigEndian);
      if (nSize > 65536 || nSize <= 0)
      {
        //assert(0);
        SAFE_DELETE_ARRAY(buf);
        return NULL;
      }
      nSizeDecomp = nSize;
      nSizeComp = de->size-4;
      gEnv->pCryPak->FReadRaw(buf, nSizeComp, 1, m_handle);
    }
    else
    {
      uint32 inSize = de->size;
      gEnv->pCryPak->FReadRaw(buf, inSize, 1, m_handle);
      uint64 outSize64;
      if (Lzma86_GetUnpackSize(buf, inSize, &outSize64) != 0)
      {
        mfSetError("FileRead - data error");
        return 0;
      }
      nSizeDecomp = (uint32)outSize64;
      nSizeComp = inSize;
    }
    return buf;
  }

  nSizeComp = nSizeDecomp = de->size;
  byte* buf = new byte [de->size];

  gEnv->pCryPak->FSeek(m_handle, de->offset, SEEK_SET);
  if (gEnv->pCryPak->FReadRaw(buf, 1, de->size, m_handle) != de->size)
  {
    mfSetError("FileRead - Reading fault");
    return 0;
  }
  return buf;
}

int CResFile::mfFileRead(CCryNameCRC name)
{
  SDirEntry *de = mfGetEntry(name);

  if (!de)
  {
    mfSetError("FileRead - Wrong FileId");
    return 0;
  }
  return mfFileRead(de);
}

int CResFile::mfFileRead(const char* name)
{
  return mfFileRead(CCryNameCRC(name));
}

int CResFile::mfFileRead(SDirEntry *de, void* data)
{
  int size;

  if (!data)
  {
    mfSetError("FileRead - Wrong data");
    return 0;
  }

  if (!mfActivate(false))
    return 0;

  AUTO_LOCK(g_cResLock); // Not thread safe without this

  if (de->flags & RF_COMPRESS)
  {
    gEnv->pCryPak->FSeek(m_handle, de->offset, SEEK_SET);
    byte* buf = new byte [de->size];
    if ( !buf )
    {
      mfSetError("FileRead - Allocation fault");
      return 0;
    }
    if (m_version == RESVERSION_LZSS)
    {
      gEnv->pCryPak->FReadRaw(&size, 1, sizeof(int), m_handle);
      if (m_bSwapEndianRead)
        SwapEndian(size, eBigEndian);
      gEnv->pCryPak->FReadRaw(buf, de->size-4, 1, m_handle);
      Decodem(buf, (byte *)data, de->size-4);
    }
    else
    {
      SizeT inSize = de->size;
      gEnv->pCryPak->FReadRaw(buf, inSize, 1, m_handle);
      uint64 outSize64;
      if (Lzma86_GetUnpackSize(buf, inSize, &outSize64) != 0)
      {
        mfSetError("FileRead - data error");
        return 0;
      }
      SizeT outSize = (uint32)outSize64;
      if (outSize != 0)
      {
        uint8 *outBuffer = (uint8 *)MyAlloc(outSize);
        if (outBuffer == 0)
        {
          mfSetError("FileRead - can't allocate");
          return 0;
        }
        SRes res = Lzma86_Decode(outBuffer, &outSize, buf, &inSize);
        if (res != 0)
        {
          mfSetError("FileRead - LzmaDecoder error");
          return 0;
        }
        de->pData = outBuffer;
      }
      size = outSize;
    }
    delete [] buf;
    return size;
  }

  gEnv->pCryPak->FSeek(m_handle, de->offset, SEEK_SET);
  if (gEnv->pCryPak->FReadRaw(data, 1, de->size, m_handle) != de->size)
  {
    mfSetError("FileRead - Reading fault");
    return 0;
  }

  return de->size;
}

int CResFile::mfFileRead(CCryNameCRC name, void* data)
{
  SDirEntry *de = mfGetEntry(name);

  if (!de)
  {
    mfSetError("FileRead - Invalid FileId");
    return 0;
  }
  return mfFileRead(de, data);
}

int CResFile::mfFileRead(const char* name, void* data)
{
  return mfFileRead(CCryNameCRC(name), data);
}

int CResFile::mfFileWrite(CCryNameCRC name, void* data)
{
  SDirEntry *de = mfGetEntry(name);

  if (!de)
  {
    mfSetError("FileWrite - Wrong FileId");
    return 0;
  }
  if (!data)
  {
    mfSetError("FileWrite - Wrong data");
    return 0;
  }

  if (!mfActivate(false))
    return 0;

  if (de->flags & RF_COMPRESS)
  {
    assert(0);
    return 0;
  }

  gEnv->pCryPak->FSeek(m_handle, de->offset, SEEK_SET);
  if (gEnv->pCryPak->FWrite(data, 1, de->size, m_handle) != de->size)
  {
    mfSetError("FileWrite - Writing fault");
    return 0;
  }

  return de->size;
}

void* CResFile::mfFileRead2(SDirEntry *de, int size)
{
  void* buf;

  buf = new byte [size];
  if (!buf)
  {
    mfSetError("FileRead - Allocation fault");
    return NULL;
  }
  if (de->pData)
  {
    memcpy (buf, (byte *)(de->pData)+de->curseek, size);
    de->curseek += size;
    return buf;
  }
  if (!mfActivate(false))
    return false;

  AUTO_LOCK(g_cResLock); // Not thread safe without this

  gEnv->pCryPak->FSeek(m_handle, de->offset+de->curseek, SEEK_SET);
  if (gEnv->pCryPak->FReadRaw(buf, 1, size, m_handle) != size)
  {
    mfSetError("FileRead - Reading fault");
    return NULL;
  }
  de->curseek += size;

  return buf;
}

void* CResFile::mfFileRead2(CCryNameCRC name, int size)
{
  SDirEntry *de = mfGetEntry(name);
  if (!de)
  {
    mfSetError("FileRead - Wrong FileId");
    return NULL;
  }
  return mfFileRead2(de, size);
}

void* CResFile::mfFileRead2(const char* name, int size)
{
  return mfFileRead2(CCryNameCRC(name), size);
}

void CResFile::mfFileRead2(SDirEntry *de, int size, void *buf)
{
  if (!buf)
  {
    mfSetError("FileRead - Wrong data");
    return;
  }
  if (de->pData)
  {
    memcpy (buf, (byte *)(de->pData)+de->curseek, size);
    de->curseek += size;
    return;
  }
  if (!mfActivate(false))
    return;

  gEnv->pCryPak->FSeek(m_handle, de->offset+de->curseek, SEEK_SET);
  if (gEnv->pCryPak->FReadRaw(buf, 1, size, m_handle) != size)
  {
    mfSetError("FileRead - Reading fault");
    return;
  }
  de->curseek += size;

  return;
}

void CResFile::mfFileRead2(CCryNameCRC name, int size, void *buf)
{
  SDirEntry *de = mfGetEntry(name);
  if (!de)
  {
    mfSetError("FileRead2 - wrong file id");
    return;
  }
  return mfFileRead2(de, size, buf);
}

void CResFile::mfFileRead2(const char *szName, int size, void *buf)
{
  mfFileRead2(CCryNameCRC(szName), size, buf);
}

void* CResFile::mfFileGetBuf(SDirEntry *de)
{
  return de->pData;
}

void* CResFile::mfFileGetBuf(CCryNameCRC name)
{
  SDirEntry *de = mfGetEntry(name);
  if (!de)
  {
    mfSetError("FileGetBuf - wrong file id");
    return NULL;
  }
  return mfFileGetBuf(de);
}

void* CResFile::mfFileGetBuf(char* name)
{
  return mfFileGetBuf(CCryNameCRC(name));
}

int CResFile::mfFileSeek(SDirEntry *de, int ofs, int type)
{
  int m;

  mfActivate(false);

  AUTO_LOCK(g_cResLock); // Not thread safe without this

  switch ( type )
  {
    case SEEK_CUR:
      de->curseek += ofs;
      m = gEnv->pCryPak->FSeek(m_handle, de->offset+de->curseek, SEEK_SET);
      break;

    case SEEK_SET:
      m = gEnv->pCryPak->FSeek(m_handle, de->offset+ofs, SEEK_SET);
      de->curseek = ofs;
      break;

    case SEEK_END:
      de->curseek = de->size-ofs;
      m = gEnv->pCryPak->FSeek(m_handle, de->offset+de->curseek, SEEK_SET);
      break;

    default:
    mfSetError("FileSeek - wrong seek type");
    return -1;
  }

  return m;
}

int CResFile::mfFileSeek(CCryNameCRC name, int ofs, int type)
{
  SDirEntry *de = mfGetEntry(name);

  if (!de)
  {
    mfSetError("FileSeek - invalid file id");
    return -1;
  }
  return mfFileSeek(de, ofs, type);
}

int CResFile::mfFileSeek(char* name, int ofs, int type)
{
  return mfFileSeek(CCryNameCRC(name), ofs, type);
}

int CResFile::mfFileLength(SDirEntry *de)
{
  return de->size;
}

int CResFile::mfFileLength(CCryNameCRC name)
{
  SDirEntry *de = mfGetEntry(name);

  if (!de)
  {
    mfSetError("FileLength - invalid file id");
    return -1;
  }
  return mfFileLength(de);
}

int CResFile::mfFileLength(char* name)
{
  return mfFileLength(CCryNameCRC(name));
}


int CResFile::mfFileAdd(SDirEntry* de)
{
  AUTO_LOCK(g_cResLock); // Not thread safe without this

  if (m_typeaccess == RA_READ)
  {
    mfSetError("FileAdd - wrong access mode");
    return -1;
  }
  CCryNameCRC name = de->Name;
  ResDirIt it = std::lower_bound(m_Dir.begin(), m_Dir.end(), name, ResDirSortByName());
  if (it != m_Dir.end() && name == (*it).Name)
    return m_Dir.size();

  if (de->offset == 0)
    de->offset = m_nOffset++;

  if (de->size!=0 && (de->pData || de->offset<0))
  {
    mfPrepareDir();

    SDirEntry newDE;
    newDE = *de;
    newDE.flags |= RF_NOTSAVED;
    m_Dir.insert(it, newDE);
    m_bDirty = true;
  }
  return m_Dir.size();
}

int CResFile::mfFlushDir(long nOffset)
{
  const int nFiles = m_Dir.size();
  int i;
#ifdef _DEBUG
  // Check for sorted array and duplicated values
  std::vector<SDirEntry> Sorted;
  for (i=0; i<nFiles; i++)
  {
    SDirEntry &DE = m_Dir[i];
    ResDirIt it = std::lower_bound(Sorted.begin(), Sorted.end(), DE.Name, ResDirSortByName());
    if (it != Sorted.end() && DE.Name == (*it).Name)
    {
      assert(0);  // Duplicated value
      continue;
    }
    Sorted.insert(it, DE);
  }
  assert(Sorted.size() == m_Dir.size());
  for (i=0; i<nFiles; i++)
  {
    SDirEntry &DE1 = m_Dir[i];
    SDirEntry &DE2 = Sorted[i];
    assert(DE1.Name == DE2.Name);
  }
#endif

  TArray<SFileDirEntry_name> FDir;
  FDir.ReserveNoClear(nFiles);

  for (i=0; i<nFiles; i++)
  {
    SDirEntry *de = &m_Dir[i];
    SFileDirEntry_name& fden = FDir[i];
    fden.cName = de->Name;
    fden.size = de->size;
    assert (de->offset > 0);
    fden.offset = de->offset;
    fden.flags = de->flags;
    if (m_bSwapEndianWrite)
      SwapEndian(fden, eBigEndian);
  }
  gEnv->pCryPak->FSeek(m_handle, nOffset, SEEK_SET);
  int size = FDir.Num()*sizeof(SFileDirEntry_name);
  byte *buf = NULL;
  if (m_bDirCompressed)
  {
  #ifndef RES_LZMA
    buf = new byte [size*2+128];
    if (!buf)
    {
      mfSetError("FlushDir - Allocation fault");
      return false;
    }
    size = Encodem((byte *)&FDir[0], buf, size);
  #else
    // we allocate 105% of original size for output buffer
    uint32 outSize = size / 20 * 21 + (1 << 16);
    buf = (byte *)MyAlloc(outSize);
    if (!buf)
    {
      mfSetError("FlushDir - Allocation fault");
      return false;
    }
    uint32 dict = 1 << 23;
    int res = Lzma86_Encode(buf, &outSize, (byte *)&FDir[0], size, 5, dict, SZ_FILTER_AUTO);
    if (res != 0)
    {
      mfSetError("FlushDir - Encoder error = %d", res);
      return 1;
    }
    size = outSize;
  #endif
  }
  else
  {
    buf = new byte [size];
    memcpy(buf, &FDir[0], size);
  }
  if (gEnv->pCryPak->FWrite(buf, 1, size, m_handle) != size)
  {
    mfSetError("FlushDir - Writing fault");
    return false;
  }
  m_nOffsDir = nOffset;
  m_nComprNumFiles = FDir.Num();
  SAFE_DELETE_ARRAY(m_pCompressedDir);
  if (m_bDirCompressed)
  {
    m_nComprDirSize = size;
    m_pCompressedDir = new byte[size];
    memcpy(m_pCompressedDir, buf, size);
  }
  else
    size = 0;
  SAFE_DELETE_ARRAY(buf);
  m_bDirValid = true;

  return size;
}

int CResFile::mfFlush(uint64 WriteTime, bool bCompressDir)
{
  SFileResHeader frh;

  PROFILE_FRAME(Resource_Flush);

  if (m_typeaccess == RA_READ)
  {		
    mfSetError("Flush - wrong access mode");
    return 0;
  }
  AUTO_LOCK(g_cResLock); // Not thread safe without this

  if (!m_bDirty)
    return true;
  m_bDirty = false;
  if (!mfActivate(false))
    return 0;
  int i;
	uint32 j;

  int nSizeCompr = 0;

  int nUpdate = 0;
  int nSizeUpdate = 0;

  const int nFiles = m_Dir.size();
  //if (strstr(m_name, "FPVS") && nFiles==4)
  {




    //int length = gEnv->pCryPak->FTell(m_handle);
  }

  // Make a list of all references
  TArray<int> *pRefs = new TArray<int>[nFiles];
  for (i=0; i<nFiles; i++)
  {
    SDirEntry *de = &m_Dir[i];
    if (de->offset < 0)
    {
      assert(de->flags & RF_NOTSAVED);
      bool bFound = false;
      for (j=0; j<m_Dir.size(); j++)
      {
        if (i == j)
          continue;
        SDirEntry* d = &m_Dir[j];
        if ((d->offset == -de->offset) || ((de->flags & RF_COMPRESSED) && (d->flags>>8) == -de->offset))
        {
          bFound = true;
          pRefs[j].AddElem(i);
        }
      }
      assert(bFound);
    }
  }

  long nSeek = m_nOffsDir;
  for (i=0; i<nFiles; i++)
  {
    SDirEntry *de = &m_Dir[i];
    if (de->flags & RF_NOTSAVED)
    {
      de->flags &= ~RF_NOTSAVED;
      nUpdate++;
      nSizeUpdate += de->size;

      if (de->offset >= 0)
      {
        assert(de->pData);
        if (!de->pData)
          continue;
        de->offset = nSeek;
        gEnv->pCryPak->FSeek(m_handle, nSeek, SEEK_SET);
        if (de->flags & RF_COMPRESS)
        {
          byte *buf = NULL;
#ifndef RES_LZMA
          if (!(de->flags & RF_COMPRESSED))
          {
            buf = new byte [de->size*2+128];
            if (!buf)
            {
              mfSetError("Flush - Allocation fault");
              return false;
            }
            int sizeEnc = Encodem((byte *)de->pData, &buf[4], de->size);
            int nS = de->size;
            if (m_bSwapEndianWrite)
              SwapEndian(nS, eBigEndian);
            *(int *)buf = nS;
            de->size = sizeEnc+4;
          }
          else
          {
            buf = (byte *)de->pData;
            if (m_bSwapEndianWrite)
            {
              int nS = *(int *)buf;
              SwapEndian(nS, eBigEndian);
              *(int *)buf = nS;
            }
          }
#else
          // we allocate 105% of original size for output buffer
          if (!(de->flags & RF_COMPRESSED))
          {
            uint32 outSize = de->size / 20 * 21 + (1 << 16);
            buf = (byte *)MyAlloc(outSize);
            if (!buf)
            {
              mfSetError("Flush - Allocation fault");
              return false;
            }
            uint32 dict = 1 << 23;
            int res = Lzma86_Encode(buf, &outSize, (byte *)de->pData, de->size, 5, dict, SZ_FILTER_AUTO);
            if (res != 0)
            {
              mfSetError("Flush - Encoder error = %d", res);
              return 1;
            }
            de->size = outSize;
          }
          else
            buf = (byte *)de->pData;
#endif
          if (gEnv->pCryPak->FWrite(buf, 1, de->size, m_handle) != de->size)
            mfSetError("Flush - Writing fault");
          if (!(de->flags & RF_COMPRESSED))
            delete [] buf;
          nSizeCompr += de->size;
        }
        else
        if (!de->pData || (gEnv->pCryPak->FWrite(de->pData, 1, de->size, m_handle) != de->size))
        {
          mfSetError("Flush - Writing fault");
          continue;
        }

        if (de->flags & RF_TEMPDATA)
        {
          char *pPtr = (char*)de->pData;
          delete [] pPtr;
        }
        de->pData = NULL;
        nSeek += de->size;
      }
    }
    // Update reference entries
    for (j=0; j<pRefs[i].Num(); j++)
    {
      nUpdate++;
      SDirEntry *d = &m_Dir[pRefs[i][j]];
      d->offset = de->offset;
      d->size = de->size;
      d->flags = de->flags | RF_REFERENCE;
      d->flags &= ~RF_NOTSAVED;
    }
  }
  SAFE_DELETE_ARRAY(pRefs);

  if (!nUpdate)
    return nSizeCompr;
  m_bDirCompressed = bCompressDir;
  int sizeDir = mfFlushDir(nSeek);

  frh.hid = IDRESHEADER;
#ifndef RES_LZMA
  int ver = RESVERSION_LZSS;
#else
  int ver = RESVERSION_LZMA;
#endif
  frh.ver = ver;
  frh.num_files = m_Dir.size();
  frh.ofs_dir = nSeek;
  frh.size_dir = sizeDir;
  m_version = ver;
  SFileResHeader frhTemp, *pFrh;
  pFrh = &frh;
  if (m_bSwapEndianWrite)
  {
    frhTemp = frh;
    SwapEndian(frhTemp, eBigEndian);
    pFrh = &frhTemp;
  }
  gEnv->pCryPak->FSeek(m_handle, 0, SEEK_SET);
  if (gEnv->pCryPak->FWrite(pFrh, 1, sizeof(frh), m_handle) != sizeof(frh))
  {
    mfSetError("Flush - Writing fault");
    return false;
  }
  gEnv->pCryPak->FFlush(m_handle);
  if (WriteTime != 0)
  {
    mfSetModifTime(WriteTime);
  }
  //if (strstr(m_name, "FPVS") && frh.num_files==4)
  {
    //gEnv->pCryPak->FSeek(m_handle, 0, SEEK_END);
    //int nSize = gEnv->pCryPak->FTell(m_handle);
    //mfDeactivate();
    //int nnn = 0;
  }

  return nSizeCompr;
}

int CResFile::Size()
{
  int nSize = sizeof(CResFile);

  nSize += m_Dir.size() * sizeof(SDirEntry);

  return nSize;
}

void CResFile::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));		
	pSizer->AddObject( m_Dir );
}

std::vector<SDirEntry> *CResFile::mfGetDirectory()
{
  return &m_Dir;
}

//======================================================================

void fpStripExtension (const char *in, char *out)
{
  ptrdiff_t len = strlen(in)-1;

  if(len<=1)
  {
    strcpy(out, in);
    return;
  }

  while (in[len])
  {
    if (in[len]=='.')
    {
      int n = (int)len;
      while(in[n] != 0)
      {
        if (in[n] == '+')
        {
          strcpy(out, in);
          return;
        }
        n++;
      }
      break;
    }
    len--;
    if (!len)
    {
      strcpy(out, in);
      return;
    }
  }
  strncpy(out, in, len);
  out[len] = 0;
}

const char *fpGetExtension (const char *in)
{
  ptrdiff_t len = strlen(in)-1;
  while (len)
  {
    if (in[len]=='.')
      return &in[len];
    len--;
  }
  return NULL;
}

void fpAddExtension (char *path, char *extension)
{
  char    *src;
  src = path + strlen(path) - 1;

  while (*src != '/' && src != path)
  {
    if (*src == '.')
      return;                 // it has an extension
    src--;
  }

  strcat (path, extension);
}

void fpConvertDOSToUnixName( char *dst, const char *src )
{
  while ( *src )
  {
    if ( *src == '\\' )
      *dst = '/';
    else
      *dst = *src;
    dst++; src++;
  }
  *dst = 0;
}

void fpConvertUnixToDosName( char *dst, const char *src )
{
  while ( *src )
  {
    if ( *src == '/' )
      *dst = '\\';
    else
      *dst = *src;
    dst++; src++;
  }
  *dst = 0;
}

void fpUsePath (char *name, char *path, char *dst)
{
  char c;

  if (!path)
  {
    strcpy(dst, name);
    return;
  }

  strcpy(dst, path);
  if ( (c=path[strlen(path)-1]) != '/' && c!='\\')
    strcat(dst, "/");
  strcat(dst, name);
}

#include "TypeInfo_impl.h"
#include "ResFile_info.h"
#ifndef _LIB
	#include "Name_TypeInfo.h"
#endif
