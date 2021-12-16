#ifndef __RESFILE_H__
#define __RESFILE_H__

#include "CryName.h"

#define IDRESHEADER		(('K'<<24)+('C'<<16)+('P'<<8)+'C')
#define RESVERSION_LZSS  4 // LZSS
#define RESVERSION_LZMA  5 // LZMA

//#define RES_LZMA 1

#include "LZSS.H"
//#include "Lzma/Compress/Lzma2Decoder.h"
//#include "Lzma/Compress/Lzma2Encoder.h"
#include "Lzma/Lzma86.h"

// Resource files flags
#define RF_NOTSAVED 1
#define RF_REFERENCE 2
#define RF_COMPRESS 4
#define RF_TEMPDATA 8
#define RF_COMPRESSED 0x80
#define RF_RES_$HEAD 0x10
#define RF_RES_$MAP  0x20
#define RF_RES_$NAME 0x40
#define RF_RES_$ (RF_RES_$HEAD | RF_RES_$MAP | RF_RES_$NAME)

// Resource header
struct SFileResHeader
{
  uint32 hid;
  int  ver;
  int num_files;
  uint32 ofs_dir;
  uint32 size_dir;
	AUTO_STRUCT_INFO
};

#define MAX_FILE_NAME 256

#define OFFSET_BIG_POSITIVE 0x20000000

// Each file entry in resource
struct SFileDirEntry_name
{
  int size;
  int32 offset;
  uint32 flags;     // RF_
  CCryNameCRC cName;
	AUTO_STRUCT_INFO
};

// Internal file entry
struct SDirEntry
{
  CCryNameCRC Name;
  int size;
  int32 offset;
  int32 curseek;
  void *pData;
  uint32 flags;     // RF_

  SDirEntry()
  {
    memset(this, 0, sizeof(SDirEntry));
  }

	void GetMemoryUsage(ICrySizer *pSizer) const{}
};

//typedef std::map<CCryNameCRC, int> ResFilesMap;
//typedef ResFilesMap::iterator ResFilesMapItor;

// Resource access types
#define RA_READ   1
#define RA_WRITE  2
#define RA_CREATE 4
#define RA_ENDIANS 8


// Resource optimize flags
#define RO_HEADERS_IN_BEGIN 1
#define RO_HEADERS_IN_END 2
#define RO_HEADER_FILE 4
#define RO_SORT_ALPHA_ASC 8
#define RO_SORT_ALPHA_DESC 0x10




#define MAX_OPEN_RESFILES 32

#define MAX_DIR_SIZE      (2*1024*1024)   // Directory size in memory (consoles only)

typedef std::vector<SDirEntry> ResDir;
typedef ResDir::iterator ResDirIt;

class CResFile
{
private:
  char m_name[1024];
  char *m_szAccess;
  FILE* m_handle;
  ResDir m_Dir;
  byte *m_pCompressedDir;
  int m_typeaccess;
  uint32 m_nOffsDir;
  uint32 m_nComprNumFiles;
  uint32 m_nComprDirSize;
  int32  m_nOffset;
	bool m_bSwapEndianRead;
  bool m_bSwapEndianWrite;
  bool m_bDirty;
  bool m_bDirValid;
  bool m_bDirCompressed;
  string m_ermes;
  int m_version;

  static CResFile m_Root;
  static int m_nNumOpenResources;
  static uint32 m_nSizeDir;
  CResFile *m_Next;
  CResFile *m_Prev;

  bool mfActivate(bool bFirstTime);
  void mfDeactivate();

  _inline void Relink(CResFile* Before)
  {
    if (m_Next && m_Prev)
    {
      m_Next->m_Prev = m_Prev;
      m_Prev->m_Next = m_Next;
    }
    m_Next = Before->m_Next;
    Before->m_Next->m_Prev = this;
    Before->m_Next = this;
    m_Prev = Before;
  }
  _inline void Unlink()
  {
    if (!m_Next || !m_Prev)
      return;
    m_Next->m_Prev = m_Prev;
    m_Prev->m_Next = m_Next;
    m_Next = m_Prev = NULL;
  }
  _inline void Link(CResFile* Before)
  {
    if (m_Next || m_Prev)
      return;
    m_Next = Before->m_Next;
    Before->m_Next->m_Prev = this;
    Before->m_Next = this;
    m_Prev = Before;
  }

public:
  CResFile(const char* name);
  CResFile();
  ~CResFile();

  const char* mfGetError(void);
  void mfSetError(const char *er, ...);
  const char *mfGetFileName() {return m_name;}
  int mfGetVersion() { return m_version; }

  bool mfOpen(int type);
  bool mfClose();
  int mfFlush(uint64 WriteTime=0, bool bCompressDir=false);
  int mfFlushDir(long nSeek);
  bool mfPrepareDir();
  void mfReleaseDir();

  int mfGetNumFiles() { return m_Dir.size(); }

  byte *mfFileReadCompressed(SDirEntry *de, short& nSizeDecomp, short& nSizeComp);

  int mfFileRead(SDirEntry *de);
  int mfFileRead(const char* name);
  int mfFileRead(CCryNameCRC name);

  int mfFileRead(SDirEntry *de, void* data);
  int mfFileRead(CCryNameCRC name, void* data);
  int mfFileRead(const char* name, void* data);

  int mfFileWrite(CCryNameCRC name, void* data);

  void* mfFileRead2(SDirEntry *de, int size);
  void* mfFileRead2(CCryNameCRC name, int size);
  void* mfFileRead2(const char* name, int size);

  void  mfFileRead2(SDirEntry *de, int size, void *buf);
  void  mfFileRead2(CCryNameCRC name, int size, void *buf);
  void  mfFileRead2(const char *szName, int size, void *buf);

  void* mfFileGetBuf(SDirEntry *de);
  void* mfFileGetBuf(CCryNameCRC name);
  void* mfFileGetBuf(char* name);

  int mfFileSeek(SDirEntry *de, int offs, int type);
  int mfFileSeek(CCryNameCRC name, int offs, int type);
  int mfFileSeek(char* name, int offs, int type);

  int mfFileLength(SDirEntry *de);
  int mfFileLength(CCryNameCRC name);
  int mfFileLength(char* name);

  int mfFileAdd(SDirEntry* de);

  bool mfIsDirty() { return m_bDirty; }

  //int mfFileDelete(SDirEntry *de);
  //int mfFileDelete(CCryName name);
  //int mfFileDelete(char* name);

  bool mfFileExist(CCryNameCRC name);
  bool mfFileExist(const char* name);

  int mfFileClose(SDirEntry *de);
  SDirEntry *mfGetEntry(CCryNameCRC name);
  std::vector<SDirEntry> *mfGetDirectory();

  FILE *mfGetHandle() { return m_handle; }
  int mfGetResourceSize();

  uint64 mfGetModifTime();
  bool mfSetModifTime(uint64 MTime);

  int Size();
	void GetMemoryUsage( ICrySizer *pSizer ) const;
};

#endif //  __RESFILE_H__
