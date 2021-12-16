#pragma once

#include <map>					// STL map


class CTextureFallbackSystem
{
public:
	CTextureFallbackSystem();
	virtual ~CTextureFallbackSystem();

	// this function stores a copy of the provided input data
	// Arguments:
	//   szFilename - must not be 0
	// Returns:
	//   true - entry was added successfully, false - value collided with previous entry
	bool AddEntry( const char *szFilename, unsigned char *pData, const uint32 dwSize );

	// Arguments:
	//   szFilename - must not be 0
	bool Init( const char *szFilename );

	// Returns:
	//   true - saves successful or save was not needed, false - saving failed
	bool SaveIfInitialized();

	bool IsActivated() const;

	uint32 GetEntriesCount() const;

	// only works if Entries contain DDSURFACEDESC2 header
	void DebugDump();

	// for debugging purpose
	// Arguments:
	//   szFilename - must not be 0
	uint64 ComputeCRC( const char *szFilename );
  
private: // ------------------------------------------------------------------
	struct Lock
	{
		const CTextureFallbackSystem& owner;
		Lock(const CTextureFallbackSystem& owner): owner(owner) {WaitForSingleObject(owner.m_hMutex, INFINITE);}
		~Lock() {ReleaseMutex(owner.m_hMutex);}
	};

	struct SMemoryBlock
	{
		SMemoryBlock() 
			:m_pData(0), m_dwSize(0), m_bFresh(false)
		{
		}

		// creates a copy of the input data
		SMemoryBlock( unsigned char *pData, const uint32 dwSize ) 
			:m_pData(0), m_dwSize(0), m_bFresh(true)
		{
			assert(pData);
			assert(dwSize);

			if(Alloc(dwSize))
				memcpy(m_pData,pData,dwSize);
		}

		bool Alloc( const uint32 dwSize ) 
		{
			assert(!m_pData);
			assert(dwSize);

			m_pData = new unsigned char[dwSize];
			
			if(m_pData)
			{
				m_dwSize=dwSize;
				return true;
			}

			return false;
		}
  
		~SMemoryBlock()
		{
			delete m_pData;
		}


		unsigned char *		m_pData;					//
		uint32						m_dwSize;					//
		bool							m_bFresh;					// false=if it was loaded from fall-back file, true=if it was created freshly
	};

	// -------------------------------------------------------------------------

	string														m_sFilename;				//
	std::map<uint64,SMemoryBlock *>		m_Entries;					//

	static const int									m_iVersion=2;				// increase to prevent loading of files with old formats

	// -------------------------------------------------------------------------

	mutable HANDLE										m_hMutex;
};
