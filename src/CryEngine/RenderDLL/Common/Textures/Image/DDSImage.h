
#ifndef DDSIMAGE_H
#define DDSIMAGE_H

#include "ImageExtensionHelper.h"
#include "CImage.h"
#include <IMemory.h>

/**
 * An ImageFile subclass for reading DDS files.
 */

namespace DDSSplitted
{
	enum
	{ 
		etexNumLastMips = 3,
		etexLowerMipMaxSize = (1<<(etexNumLastMips + 2)),	// + 2 means we drop all the mips that are less than 4x4(two mips: 2x2 and 1x1)
	};

	struct ChunkInfo
	{
		string 	fileName;
		size_t 	m_nOffsetInFile;
		size_t 	size;
		size_t 	m_nOriginalOffset;	// for order
		int			m_nMipLevel;
		const bool operator<(const ChunkInfo& chunk) const { return m_nOriginalOffset < chunk.m_nOriginalOffset; }
	};

	typedef std::vector<ChunkInfo> Chunks;

	string MakeName(const string& sOriginalName, const uint32 nChunk, const uint32 nFlags);

	bool GetFilesToRead(Chunks& files, const string& sOriginalName, 
											const size_t offset, const size_t size, 
											int nWidth, int nHeight, int nDepth, int nSides,
											int nNumMips, int nStartMip, int nEndMip, int nMipsPersistent,
											const ETEX_Format format, const uint32 nFlags);

	bool IsSplitted(const string& sOriginalName);

	size_t LoadMips(const string& sOriginalName, byte* pBuffer, 
									const size_t offset, const size_t size, 
									const int nWidth, const int nHeight, const int nDepth, const int nSides,
									const int nNumMips, const int nStartMip, const int nEndMip, int nMipsPersistent, const ETEX_Format format, const uint32 nFlags);

	int GetNumLastMips(const int nWidth, const int nHeight, const int nNumMips, const int nSides, ETEX_Format eTF, const uint32 nFlags);
};

class CImageDDSFile : public CImageFile
{
  friend class CImageFile;	// For constructor
public:
  CImageDDSFile (const string& filename, uint32 nFlags);
  virtual ~CImageDDSFile ();

protected: // ------------------------------------------------------------------------------
	/// Read the DDS file from the file.
	bool LoadFromSplittedFile(const string& filename, uint32 nFlags);

	/// Read the DDS file from the file.
	bool LoadFromFile(const string& filename, uint32 nFlags);

	/// Read the DDS file from the file.
	bool Load(const string& filename, uint32 nFlags);


	bool SetHeaderFromMemory(byte* pFileStart, uint32 nFlags);

	CImageExtensionHelper::DDS_HEADER	m_DDSHeader;
	
	_smart_ptr<IMemoryBlock> m_pFileMemory;
};

void WriteDDS(byte *dat, int wdt, int hgt, int dpth, int Size, char *name, ETEX_Format eF, int NumMips, ETEX_Type eTT);

#endif


