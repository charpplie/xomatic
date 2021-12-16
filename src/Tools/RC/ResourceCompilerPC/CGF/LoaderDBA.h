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


#include "..\CGA\Controller.h"
#include "LoaderCAF.h"
#include "..\CGA\ControllerPQLog.h"
#include "..\CGA\ControllerPQ.h"


typedef std::vector<CCommonSkinningInfo> THeaderVector;
typedef std::vector<string> TNamesVector;
typedef std::vector<std::vector<CControllerInfo>> TControllerInfoVector;
typedef std::vector<DWORD> TFileTimeVector;

struct CInternalDatabaseInfo
{
	TNamesVector m_AnimationNames;
	std::map<uint32, uint32> m_AnimationCRCMap;
	THeaderVector m_Headers;
	TControllerInfoVector m_ControllerInfos;
	TFileTimeVector m_arrTimesteps;
	// hardcode is rules!
	std::vector<QuatT> m_attStartDirs;


	DynArray<KeyTimesInformationPtr> m_arrKeyTimes;
	DynArray<TrackRotationStoragePtr> m_arrRotationTracks;
	DynArray<TrackPositionStoragePtr> m_arrPositionTracks;
/*
	DynArray<KeyTimesInformationPtr> m_arrKeyTimes;
	DynArray<TrackInformationPtr> m_arrRotationTracks;
	DynArray<PositionInformationPtr> m_arrPositionTracks;
*/
};

class CLoaderDBA
{
public:
	CLoaderDBA();
	~CLoaderDBA();
	CInternalDatabaseInfo* LoadDBA(const char *filename);
	CInternalDatabaseInfo* LoadDBA( const char *filename,IChunkFile * pChunkFile);
	const char *GetLastError() const { return m_LastError; }
	const CCommonSkinningInfo * GetSkinningInfo(const char * filename) const;
	bool ReadController903(IChunkFile::ChunkDesc *pChunkDesc);
	bool ReadController904 (IChunkFile::ChunkDesc *pChunkDesc);
private:
	bool LoadChunks();
	bool ReadController(IChunkFile::ChunkDesc *pChunkDesc);

public:
	CInternalDatabaseInfo * m_pDatabaseInfo;
	string m_Filename;

private:
	string m_LastError;
	IChunkFile * m_pChunkFile;
};




#endif