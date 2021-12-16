////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   LoaderDBA.h
//  Version:     v1.00
//  Created:     31/08/2006 by Alexey Medvedev.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __DBALoader_h__
#define __DBALoader_h__
#pragma once

#include "LoaderCAF.h"
#include "ControllerPQ.h"


struct CInternalDatabaseInfo
{
	DynArray<string> m_AnimationNames;
	std::map<uint32, uint32> m_AnimationCRCMap;
	DynArray<CCommonSkinningInfo*> m_Headers;

	DynArray<QuatT> m_arrStartDirs;

	DynArray<char> m_Storage;

	DynArray<IKeyTimesInformation*> m_arrKeyTimesArray;
	DynArray<ITrackRotationStorage*> m_arrRotationTracksArray;
	DynArray<ITrackPositionStorage*> m_arrPositionTracksArray;

	DynArray<KeyTimesInformationPtr> m_arrKeyTimes;
	DynArray<TrackRotationStoragePtr> m_arrRotationTracks;
	DynArray<TrackPositionStoragePtr> m_arrPositionTracks;

	int m_iTotalControllers;

	//	CControllerInplace * m_pControllers;
	//	RotationTrackInformation * m_pRots;
	//	PositionTrackInformation * m_pPoss;

	CInternalDatabaseInfo()/* : m_pControllers(0), m_pPoss(0), m_pRots(0)*/ 
	{
		//		m_arrKeyTimesArray = 0;
		//		m_arrRotationTracksArray = 0;
	};

	~CInternalDatabaseInfo()
	{
		for (std::size_t i =0, end =m_Headers.size(); i < end; ++i )
		{
			if (m_Headers[i])
				delete m_Headers[i];
		}

		Clear();
	}

	void Clear()
	{
		for (std::size_t i =0, end =m_arrRotationTracksArray.size(); i < end; ++i )
		{
			if (m_arrRotationTracksArray[i])
				ControllerHelper::DeleteRotationControllerPtrArray(m_arrRotationTracksArray[i]);
		}

		for (std::size_t i =0, end =m_arrKeyTimesArray.size(); i < end; ++i )
		{
			if (m_arrKeyTimesArray[i])
				ControllerHelper::DeleteTimesControllerPtrArray(m_arrKeyTimesArray[i]);
		}
		for (std::size_t i =0, end =m_arrPositionTracksArray.size(); i < end; ++i )
		{

			if (m_arrPositionTracksArray[i])
				ControllerHelper::DeletePositionControllerPtrArray(m_arrPositionTracksArray[i]);
		}

		//SAFE_DELETE_ARRAY(m_pControllers);
		//SAFE_DELETE_ARRAY(m_pPoss);
		//SAFE_DELETE_ARRAY(m_pRots);

	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_AnimationNames);
		pSizer->AddObject(m_AnimationCRCMap);
		pSizer->AddObject(m_Headers);
		pSizer->AddObject(m_arrStartDirs);
		pSizer->AddObject(m_Storage);
		
		pSizer->AddObject(m_arrKeyTimesArray);
		pSizer->AddObject(m_arrRotationTracksArray);
		pSizer->AddObject(m_arrPositionTracksArray);
		pSizer->AddObject(m_arrKeyTimes);
		pSizer->AddObject(m_arrRotationTracks);
		pSizer->AddObject(m_arrPositionTracks);
	}
};

struct CGlobalHeaderDBA
{
	friend class CAnimationManager;


	CGlobalHeaderDBA()
	{
		m_pDatabaseInfo = 0;
		m_nTCount=0;
	}

	~CGlobalHeaderDBA()
	{
		if (m_pDatabaseInfo)
			delete m_pDatabaseInfo;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const;
	const char *GetLastError() const { return m_LastError; }

	bool RemoveSkinningInfoDBA(const char * filename);

	bool LoadChunks(IChunkFile* pChunkFile);

	bool ReadControllers(IChunkFile::ChunkDesc* pChunkDesc);
	bool ReadController900(IChunkFile::ChunkDesc* pChunkDesc);
	bool ReadController902 (IChunkFile::ChunkDesc* pChunkDesc);
	bool ReadController903 (IChunkFile::ChunkDesc* pChunkDesc);
	bool ReadController904 (IChunkFile::ChunkDesc* pChunkDesc);
	int32 FindFormat(uint32 num, std::vector<uint32>& vec)
	{
		for (size_t i =0; i < vec.size(); ++i)
		{
			if ( num < vec[i+1])
				return (int)i;
		}
		return -1;
	}

	void CreateDatabaseDBA( const char* filename );
	void StreamDatabaseDBA();
	const CCommonSkinningInfo* GetSkinningInfoDBA(const char* filename) const;
	void DeleteDatabaseDBA( );
	const size_t SizeOf_DBA() const;

	CInternalDatabaseInfo* m_pDatabaseInfo;
	string m_strFilePathDBA;
	uint32 m_FilePathDBACRC32;
	uint32 m_nUsedAnimations;
	uint32 m_nTCount;
	bool InMemory();
	int m_iRefCounter;
	string m_LastError;
};

#endif
