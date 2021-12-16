////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   LoaderDBA.cpp
//  Version:     v1.00
//  Created:     31/08/2006 by Alexey Medvedev.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <StringUtils.h>
#include "CryPath.h"

//#include "AnimationBase.h"
#include <CGFContent.h>
#include "LoaderDBA.h"
#include "ChunkFile.h"
#include <crc32.h>

Crc32Gen g_crcGen;

CLoaderDBA::CLoaderDBA()
{
	m_pDatabaseInfo = 0;
	//	m_pChunkFile = 0;
}

CLoaderDBA::~CLoaderDBA()
{
	if (m_pDatabaseInfo)
		delete m_pDatabaseInfo;
}


CInternalDatabaseInfo* CLoaderDBA::LoadDBA(const char *filename)
{
	CChunkFile chunkFile;
	CInternalDatabaseInfo* pDB = LoadDBA( filename, &chunkFile);
	return pDB;
}

CInternalDatabaseInfo* CLoaderDBA::LoadDBA( const char *filename,IChunkFile * pChunkFile)
{
	string strPath = filename;
	CryStringUtils::UnifyFilePath(strPath);
	string fileExt = PathUtil::GetExt(strPath);

	m_Filename = filename;

	m_pChunkFile = pChunkFile;

	if (!pChunkFile->Read( filename ))
	{
		m_LastError = pChunkFile->GetLastError();
		return 0;
	}

	// Load mesh from chunk file.
	if ((pChunkFile->GetFileHeader().Version != ChunkFileVersion) && (pChunkFile->GetFileHeader().Version != ChunkFileVersion_Align))
	{
		m_LastError.Format("Bad DBA file version: %s", m_Filename.c_str() );
		return 0;
	}

	if (pChunkFile->GetFileHeader().FileType != FileType_Geom) 
	{
		if (pChunkFile->GetFileHeader().FileType != FileType_Anim) 
		{
			m_LastError.Format("Illegal File Type for .dba file: %s", m_Filename.c_str() );
			return 0;
		}
	}

	m_pDatabaseInfo = new CInternalDatabaseInfo;

	if (!LoadChunks())
	{
		delete m_pDatabaseInfo;
		m_pDatabaseInfo = 0;
	} 

	return m_pDatabaseInfo;
}

bool CLoaderDBA::LoadChunks()
{
	uint32 numChunck = m_pChunkFile->NumChunks();

	for (uint32 i=0; i<numChunck; i++)
	{

		const CHUNK_HEADER &hdr = m_pChunkFile->GetChunkHeader(i);

		switch (hdr.ChunkType)
		{
			//---------------------------------------------------------
			//---       chunks for CGA-objects  -----------------------
			//---------------------------------------------------------

		case ChunkType_Controller:
			if (!ReadController( m_pChunkFile->GetChunk(i)) )
				return false;
			break;

		//case 
		}
	}

	return true;
}


#define CHECK_POINTER(x, y, z) if ((x) < (y) || (x) > (z)) return false;

bool CLoaderDBA::ReadController (IChunkFile::ChunkDesc *pChunkDesc  )
{
	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0900::VERSION)
	{

	CONTROLLER_CHUNK_DESC_0900* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0900*)pChunkDesc->data;
	SwapEndian(*pCtrlChunk);




	m_pDatabaseInfo->m_AnimationNames.resize(pCtrlChunk->numAnims);
	m_pDatabaseInfo->m_Headers.resize(pCtrlChunk->numAnims);

	m_pDatabaseInfo->m_arrKeyTimes.resize(pCtrlChunk->numKeyTime);
	m_pDatabaseInfo->m_arrPositionTracks.resize(pCtrlChunk->numKeyPos);
	m_pDatabaseInfo->m_arrRotationTracks.resize(pCtrlChunk->numKeyRot);

	std::vector<uint16> sizesKeyTimes;
	std::vector<uint8> formatKeyTimes;

	char *pData = (char*)(pCtrlChunk+1);

	char * pStartChunk = pData;
	char * pEndChunk = pData + pChunkDesc->size;

	uint32 keyTime = pCtrlChunk->numKeyTime;

	sizesKeyTimes.resize(keyTime);
	formatKeyTimes.resize(keyTime);

	// Key times info!
	memcpy(&sizesKeyTimes[0], pData, sizesKeyTimes.size() * sizeof(uint16));
	pData += keyTime * sizeof(uint16);

	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	memcpy(&formatKeyTimes[0], pData, formatKeyTimes.size() * sizeof(uint8));
	pData += keyTime * sizeof(uint8);

	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	// copy raw data
	for(uint32 k = 0; k < pCtrlChunk->numKeyTime; ++k)
	{
		m_pDatabaseInfo->m_arrKeyTimes[k] = ControllerHelper::GetKeyTimesControllerPtr(formatKeyTimes[k]);
		m_pDatabaseInfo->m_arrKeyTimes[k]->ResizeKeyTime(sizesKeyTimes[k]);

		memcpy(m_pDatabaseInfo->m_arrKeyTimes[k]->GetData(), pData, m_pDatabaseInfo->m_arrKeyTimes[k]->GetDataRawSize());
		pData += m_pDatabaseInfo->m_arrKeyTimes[k]->GetDataRawSize();
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
	}

	// Position info!
	std::vector<uint16> sizesPos;
	std::vector<uint8> formatPos;

	uint32 keyPos = pCtrlChunk->numKeyPos;

	sizesPos.resize(keyPos);
	formatPos.resize(keyPos);

	// pos sizes
	memcpy(&sizesPos[0], pData, keyPos * sizeof(uint16));
	pData += keyPos * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	//pos formats
	memcpy(&formatPos[0], pData, keyPos * sizeof(uint8));
	pData += keyPos * sizeof(uint8);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	// copy raw data
	for(uint32 p = 0; p < keyPos; ++p)
	{
		m_pDatabaseInfo->m_arrPositionTracks[p] = ControllerHelper::GetPositionControllerPtr(formatPos[p]);
		m_pDatabaseInfo->m_arrPositionTracks[p]->Resize(sizesPos[p]);


		memcpy(m_pDatabaseInfo->m_arrPositionTracks[p]->GetData(), pData, m_pDatabaseInfo->m_arrPositionTracks[p]->GetDataRawSize());
		pData += m_pDatabaseInfo->m_arrPositionTracks[p]->GetDataRawSize();
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
	}

	//Rotation info!
	std::vector<uint16> sizesRot;
	std::vector<uint8> formatRot;

	uint32 keyRot = pCtrlChunk->numKeyRot;

	sizesRot.resize(keyRot);
	formatRot.resize(keyRot);

	// rot sizes
	memcpy(&sizesRot[0], pData, keyRot * sizeof(uint16));
	pData += sizesRot.size() * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	//rot formats
	memcpy(&formatRot[0], pData, keyRot * sizeof(uint8));
	pData += keyRot * sizeof(uint8);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	// copy raw data
	for(uint32 r = 0; r < keyRot; ++r)
	{
		m_pDatabaseInfo->m_arrRotationTracks[r] = ControllerHelper::GetRotationControllerPtr(formatRot[r]);
		m_pDatabaseInfo->m_arrRotationTracks[r]->Resize(sizesRot[r]);

		memcpy(m_pDatabaseInfo->m_arrRotationTracks[r]->GetData(), pData, m_pDatabaseInfo->m_arrRotationTracks[r]->GetDataRawSize());
		pData += m_pDatabaseInfo->m_arrRotationTracks[r]->GetDataRawSize();
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
	}

	uint32 totalAnims = pCtrlChunk->numAnims;

		//	DynArray<DynArray<ControllerInfo>> controllerInfo(totalAnims);

	m_pDatabaseInfo->m_ControllerInfos.resize(totalAnims);
	for (uint32 i = 0; i < totalAnims; ++i)
	{

		uint16 strSize;// = pData;
		memcpy(&strSize, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
		static char tmp[1024];
		//		m_pDatabaseInfo->m_AnimationNames[i].resize(strSize);//.
		memset(tmp,0,1024);
		memcpy(tmp, pData, strSize);

		m_pDatabaseInfo->m_AnimationNames[i] = string(tmp);

		pData += strSize;
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

			//		m_pDatabaseInfo->m_AnimationNames.InsertValue(tmp, i);
		m_pDatabaseInfo->m_AnimationCRCMap.insert(std::make_pair<uint32, uint32>( g_crcGen.GetCRC32(tmp), i));

		static CStoredSkinningInfo info;

		memcpy(&info, pData, sizeof(info));
		pData += sizeof(info);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		// fill 
		m_pDatabaseInfo->m_Headers[i].m_nTicksPerFrame = info.m_nTicksPerFrame;
		m_pDatabaseInfo->m_Headers[i].m_secsPerTick = info.m_secsPerTick;
		m_pDatabaseInfo->m_Headers[i].m_nStart = info.m_nStart;
		m_pDatabaseInfo->m_Headers[i].m_nEnd = info.m_nEnd;
		m_pDatabaseInfo->m_Headers[i].m_Speed = info.m_Speed;
		m_pDatabaseInfo->m_Headers[i].m_Distance = info.m_Distance;
		m_pDatabaseInfo->m_Headers[i].m_Slope				= info.m_Slope;
		m_pDatabaseInfo->m_Headers[i].m_LHeelStart = info.m_LHeelStart,
		m_pDatabaseInfo->m_Headers[i].m_LHeelEnd = info.m_LHeelEnd;
		m_pDatabaseInfo->m_Headers[i].m_LToe0Start = info.m_LToe0Start;
		m_pDatabaseInfo->m_Headers[i].m_LToe0End = info.m_LToe0End;
		m_pDatabaseInfo->m_Headers[i].m_RHeelStart = info.m_RHeelStart;
		m_pDatabaseInfo->m_Headers[i].m_nAssetFlags = info.m_nAssetFlags;
		m_pDatabaseInfo->m_Headers[i].m_RHeelEnd = info.m_RHeelEnd;
		m_pDatabaseInfo->m_Headers[i].m_RToe0Start = info.m_RToe0Start;
		m_pDatabaseInfo->m_Headers[i].m_RToe0End = info.m_RToe0End;
		m_pDatabaseInfo->m_Headers[i].m_MoveDirection = info.m_MoveDirection; // raw storage
		m_pDatabaseInfo->m_Headers[i].m_nAssetFlags   = info.m_nAssetFlags;

		// footplants
		uint16 footplans;// = m_arrAnimations[i].m_FootPlantBits.size();
		memcpy(&footplans, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		m_pDatabaseInfo->m_Headers[i].m_FootPlantBits.resize(footplans);
		if (footplans)
		{
			memcpy(&(m_pDatabaseInfo->m_Headers[i].m_FootPlantBits[0]), pData, footplans);
			pData += footplans;
			CHECK_POINTER(pData, pStartChunk, pEndChunk);
		}

		// 
		uint16 controllerInfo;// = m_arrAnimations[i].m_arrControlerInfo.size();
		memcpy(&controllerInfo, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		m_pDatabaseInfo->m_ControllerInfos[i].resize(controllerInfo);

		std::vector<CControllerInfo>& controllers = m_pDatabaseInfo->m_ControllerInfos[i];
			//		controllers.resize(controllerInfo);

		if (controllerInfo == 0)
			continue;

		memcpy(&(controllers[0]), pData, controllerInfo * sizeof(CControllerInfo));
		pData += controllerInfo * sizeof(CControllerInfo);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		m_pDatabaseInfo->m_Headers[i].m_pControllers.resize(controllerInfo);
		m_pDatabaseInfo->m_Headers[i].m_arrControllerId.resize(controllerInfo);

		for (uint32  t = 0; t < controllerInfo; ++t)
		{
			uint32 p = controllers[t].m_nPosTrack;
			uint32 r = controllers[t].m_nRotTrack;
			uint32 pk = controllers[t].m_nPosKeyTimeTrack;
			uint32 rk = controllers[t].m_nRotKeyTimeTrack;
			uint32 id = controllers[t].m_nControllerID;

				//			m_pDatabaseInfo->m_arrPositionTracks[p]->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[pk]);
				//			m_pDatabaseInfo->m_arrRotationTracks[r]->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[rk]);
			CController_AutoPtr pController(new CController);
			RotationControllerPtr pRotation;
			PositionControllerPtr pPosition;

			if (r != -1 && rk != -1)
			{
				pRotation =  RotationControllerPtr(new RotationTrackInformation);			
				pRotation->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[rk]);
				pRotation->SetRotationStorage(m_pDatabaseInfo->m_arrRotationTracks[r]);
				pController->SetRotationController(pRotation);
			}

			if (p != -1 && pk != -1)
			{
				pPosition = PositionControllerPtr(new PositionTrackInformation);
				pPosition->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[pk]);
				pPosition->SetPositionStorage(m_pDatabaseInfo->m_arrPositionTracks[p]);
				pController->SetPositionController(pPosition);
			}

			pController->m_nControllerId = id;
			m_pDatabaseInfo->m_Headers[i].m_pControllers[t] = pController;
			m_pDatabaseInfo->m_Headers[i].m_arrControllerId[t] = id;
		}
	}

	}

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0901::VERSION)
	{
		CONTROLLER_CHUNK_DESC_0901* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0901*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk);

		m_pDatabaseInfo->m_arrTimesteps.resize(pCtrlChunk->numTimesteps);

		char *pData = (char*)(pCtrlChunk+1);

		memcpy(&(m_pDatabaseInfo->m_arrTimesteps[0]), pData, pCtrlChunk->numTimesteps* sizeof(DWORD) );
	}

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0902::VERSION)
	{
		CONTROLLER_CHUNK_DESC_0902* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0902*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk);

		m_pDatabaseInfo->m_attStartDirs.resize(pCtrlChunk->numStartDirs);

		char *pData = (char*)(pCtrlChunk+1);

		memcpy(&(m_pDatabaseInfo->m_attStartDirs[0]), pData, pCtrlChunk->numStartDirs* sizeof(QuatT) );
	}


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0903::VERSION)
		return ReadController903(pChunkDesc);

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0904::VERSION)
		return ReadController904(pChunkDesc);

	//	TVectorStartDirs
	return true;
}

bool CLoaderDBA::ReadController903 (IChunkFile::ChunkDesc *pChunkDesc  )
{
	CONTROLLER_CHUNK_DESC_0903* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0903*)pChunkDesc->data;
	SwapEndian(*pCtrlChunk);
	//	m_pDatabaseInfo->m_iTotalControllers = 0;

#ifdef LOADING_STATISTICS
	//char tmp[4096];
	CryModuleMemoryInfo info;
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController903. Init. Memstat %i", info.allocated - info.freed);
#endif

	m_pDatabaseInfo->m_AnimationNames.resize(pCtrlChunk->numAnims);
	m_pDatabaseInfo->m_Headers.resize(pCtrlChunk->numAnims);

	//	m_pDatabaseInfo->m_arrKeyTimes.resize(pCtrlChunk->numKeyTime);
	//	m_pDatabaseInfo->m_arrPositionTracks.resize(pCtrlChunk->numKeyPos);
	//	m_pDatabaseInfo->m_arrRotationTracks.resize(pCtrlChunk->numKeyRot);

#ifdef LOADING_STATISTICS
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController903. TracksResize. Memstat %i", info.allocated - info.freed);
#endif


	char *pData = (char*)(pCtrlChunk+1);

	char * pStartChunk = pData;
	char * pEndChunk = pData + pChunkDesc->size;

	uint32 totalAnims = pCtrlChunk->numAnims;
	uint32 keyTime = pCtrlChunk->numKeyTime;
	uint32 keyPos = pCtrlChunk->numKeyPos;
	uint32 keyRot = pCtrlChunk->numKeyRot;


	// Keytimes pointers
	uint16 * pktSizes = (uint16 *)pData;
	pData += keyTime * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	uint32 * pktFormats = (uint32 *)pData;
	pData += (eBitset + 1) * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	// Positions pointers
	uint16 * ppSizes = (uint16 *)pData;
	pData += keyPos * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	uint32 * ppFormats = (uint32 *)pData;
	pData += eAutomaticQuat * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	//Rotations pointers
	uint16 *prSizes = (uint16 *)pData;
	pData += keyRot * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	uint32 *prFormats = (uint32 *)pData;
	pData += eAutomaticQuat * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	std::vector<uint32> rCount(eAutomaticQuat + 1, 0);
	for (uint32 i = 0; i < eAutomaticQuat; ++i)
	{
		rCount[i+1] = rCount[i] + prFormats[i];
	}

	std::vector<uint32> pCount(eAutomaticQuat + 1, 0);
	for (uint32 i = 0; i < eAutomaticQuat; ++i)
	{
		pCount[i+1] = pCount[i] + ppFormats[i];
	}

	std::vector<uint32> ktCount(eBitset + 2, 0);
	for (uint32 i = 0; i < eBitset + 1; ++i)
	{
		ktCount[i+1] = ktCount[i] + pktFormats[i];
	}

	/*
	uint32 GetPositionsFormatSizeOf(uint32 format);
	uint32 GetRotationFormatSizeOf(uint32 format);
	uint32 GetKeyTimesFormatSizeOf(uint32 format);

	*/
	uint32 totalAlloc= 0;

	if(ktCount[eBitset + 1])
	{
		std::vector<uint32> ktOffsets(keyTime);
		m_pDatabaseInfo->m_arrKeyTimes.resize(keyTime);

		uint32 curFormat = 0;
		while (ktCount[curFormat + 1] == 0)
			++curFormat;
		uint32 curSizeof = ControllerHelper::GetKeyTimesFormatSizeOf(curFormat);

		for (uint32 i = 0; i < keyTime; ++i)
		{
			while( i >= ktCount[curFormat + 1])
			{
				++curFormat;
				curSizeof = ControllerHelper::GetKeyTimesFormatSizeOf(curFormat);
			}

			ktOffsets[i] = totalAlloc;
			totalAlloc += pktSizes[i] * curSizeof;

			m_pDatabaseInfo->m_arrKeyTimes[i] = ControllerHelper::GetKeyTimesControllerPtr(curFormat);
			m_pDatabaseInfo->m_arrKeyTimes[i]->ResizeKeyTime(pktSizes[i]);
			memcpy(m_pDatabaseInfo->m_arrKeyTimes[i]->GetData(), pData, m_pDatabaseInfo->m_arrKeyTimes[i]->GetDataRawSize());
			pData += m_pDatabaseInfo->m_arrKeyTimes[i]->GetDataRawSize();
			CHECK_POINTER(pData, pStartChunk, pEndChunk);
		}
	}


	
	if(pCount[eAutomaticQuat])
	{
		std::vector<uint32> pOffsets(keyPos);
		m_pDatabaseInfo->m_arrPositionTracks.resize(keyPos);

		uint32 curFormat = 0;
		while (pCount[curFormat + 1] == 0)
			++curFormat;

		uint32 curSizeof = ControllerHelper::GetPositionsFormatSizeOf(curFormat);

		for (uint32 i = 0; i < keyPos; ++i)
		{
			while( i >= pCount[curFormat + 1])
			{
				++curFormat;
				curSizeof = ControllerHelper::GetPositionsFormatSizeOf(curFormat);
			}
			pOffsets[i] = totalAlloc;
			totalAlloc += ppSizes[i] * curSizeof;

			m_pDatabaseInfo->m_arrPositionTracks[i] = ControllerHelper::GetPositionControllerPtr(curFormat);
			m_pDatabaseInfo->m_arrPositionTracks[i]->Resize(ppSizes[i]);
			memcpy(m_pDatabaseInfo->m_arrPositionTracks[i]->GetData(), pData, m_pDatabaseInfo->m_arrPositionTracks[i]->GetDataRawSize());
			pData += m_pDatabaseInfo->m_arrPositionTracks[i]->GetDataRawSize();
			CHECK_POINTER(pData, pStartChunk, pEndChunk);

		}
	}


	if(rCount[eAutomaticQuat])
	{
		std::vector<uint32> rOffsets(keyRot);
		m_pDatabaseInfo->m_arrRotationTracks.resize(keyRot);

		uint32 curFormat = 0;
		while (rCount[curFormat + 1] == 0)
			++curFormat;

		uint32 curSizeof = ControllerHelper::GetRotationFormatSizeOf(curFormat);

		for (uint32 i = 0; i < keyRot; ++i)
		{
			while( i >= rCount[curFormat + 1])
			{
				++curFormat;
				curSizeof = ControllerHelper::GetRotationFormatSizeOf(curFormat);
			}
			rOffsets[i] = totalAlloc;
			totalAlloc += prSizes[i] * curSizeof;

			m_pDatabaseInfo->m_arrRotationTracks[i] = ControllerHelper::GetRotationControllerPtr(curFormat);
			m_pDatabaseInfo->m_arrRotationTracks[i]->Resize(prSizes[i]);

			memcpy(m_pDatabaseInfo->m_arrRotationTracks[i]->GetData(), pData, m_pDatabaseInfo->m_arrRotationTracks[i]->GetDataRawSize());
			pData += m_pDatabaseInfo->m_arrRotationTracks[i]->GetDataRawSize();
			CHECK_POINTER(pData, pStartChunk, pEndChunk);
		}
	}

	//FIXME:
	//m_pDatabaseInfo->m_Storage.resize(totalAlloc);
	//memcpy(&(m_pDatabaseInfo->m_Storage[0]), pData, totalAlloc);

	//pData += totalAlloc;

	// set pointers to keytimes, positions, rotations

#ifdef LOADING_STATISTICS
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController903. All dynamic data resize. Memstat %i", info.allocated - info.freed);
#endif



	uint32 rc =0, pc = 0, cc = 0;

	m_pDatabaseInfo->m_ControllerInfos.resize(totalAnims);

	for (uint32 i = 0; i < totalAnims; ++i)
	{

		uint16 strSize;// = pData;
		memcpy(&strSize, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
		char tmp[1024];

		memset(tmp,0,1024);
		memcpy(tmp, pData, strSize);

		m_pDatabaseInfo->m_AnimationNames[i] = tmp;

		pData += strSize;
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
		m_pDatabaseInfo->m_AnimationCRCMap.insert(std::make_pair<uint32, uint32>(g_crcGen.GetCRC32(tmp), i));

		static CStoredSkinningInfo info;

		memcpy(&info, pData, sizeof(info));
		pData += sizeof(info);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		// fill 
		m_pDatabaseInfo->m_Headers[i].m_nTicksPerFrame = info.m_nTicksPerFrame;
		m_pDatabaseInfo->m_Headers[i].m_secsPerTick = info.m_secsPerTick;
		m_pDatabaseInfo->m_Headers[i].m_nStart			= info.m_nStart;
		m_pDatabaseInfo->m_Headers[i].m_nEnd				= info.m_nEnd;
		m_pDatabaseInfo->m_Headers[i].m_Speed				= info.m_Speed;
		m_pDatabaseInfo->m_Headers[i].m_Distance		= info.m_Distance;
		m_pDatabaseInfo->m_Headers[i].m_Slope				= info.m_Slope;
		m_pDatabaseInfo->m_Headers[i].m_LHeelStart	= info.m_LHeelStart,
		m_pDatabaseInfo->m_Headers[i].m_LHeelEnd		= info.m_LHeelEnd;
		m_pDatabaseInfo->m_Headers[i].m_LToe0Start	= info.m_LToe0Start;
		m_pDatabaseInfo->m_Headers[i].m_LToe0End		= info.m_LToe0End;
		m_pDatabaseInfo->m_Headers[i].m_RHeelStart	= info.m_RHeelStart;
		m_pDatabaseInfo->m_Headers[i].m_RHeelEnd		= info.m_RHeelEnd;
		m_pDatabaseInfo->m_Headers[i].m_RToe0Start	= info.m_RToe0Start;
		m_pDatabaseInfo->m_Headers[i].m_RToe0End		= info.m_RToe0End;
		m_pDatabaseInfo->m_Headers[i].m_MoveDirection = info.m_MoveDirection; // raw storage
		m_pDatabaseInfo->m_Headers[i].m_nAssetFlags = info.m_nAssetFlags;

		// footplants
		uint16 footplans;// = m_arrAnimations[i].m_FootPlantBits.size();
		memcpy(&footplans, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		m_pDatabaseInfo->m_Headers[i].m_FootPlantBits.resize(footplans);
		if (footplans)
		{
			memcpy(&(m_pDatabaseInfo->m_Headers[i].m_FootPlantBits[0]), pData, footplans);
			pData += footplans;
			CHECK_POINTER(pData, pStartChunk, pEndChunk);
		}

		// 
		uint16 controllerInfo;// = m_arrAnimations[i].m_arrControlerInfo.size();
		memcpy(&controllerInfo, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		m_pDatabaseInfo->m_ControllerInfos[i].resize(controllerInfo);

		std::vector<CControllerInfo>& controllers = m_pDatabaseInfo->m_ControllerInfos[i];
		//		controllers.resize(controllerInfo);

		if (controllerInfo == 0)
			continue;

		memcpy(&(controllers[0]), pData, controllerInfo * sizeof(CControllerInfo));
		pData += controllerInfo * sizeof(CControllerInfo);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		m_pDatabaseInfo->m_Headers[i].m_pControllers.resize(controllerInfo);
		m_pDatabaseInfo->m_Headers[i].m_arrControllerId.resize(controllerInfo);

		for (uint32  t = 0; t < controllerInfo; ++t)
		{
			uint32 p = controllers[t].m_nPosTrack;
			uint32 r = controllers[t].m_nRotTrack;
			uint32 pk = controllers[t].m_nPosKeyTimeTrack;
			uint32 rk = controllers[t].m_nRotKeyTimeTrack;
			uint32 id = controllers[t].m_nControllerID;

			//			m_pDatabaseInfo->m_arrPositionTracks[p]->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[pk]);
			//			m_pDatabaseInfo->m_arrRotationTracks[r]->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[rk]);
			CController_AutoPtr pController(new CController);
			RotationControllerPtr pRotation;
			PositionControllerPtr pPosition;

			if (r != -1 && rk != -1)
			{
				if(rk >= m_pDatabaseInfo->m_arrKeyTimes.size() || r >= m_pDatabaseInfo->m_arrRotationTracks.size())
				{
					printf("Error reading controllers from DBA - rotation. File %s", tmp);
					return false;
				}
				pRotation =  RotationControllerPtr(new RotationTrackInformation);			
				pRotation->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[rk]);
				pRotation->SetRotationStorage(m_pDatabaseInfo->m_arrRotationTracks[r]);
				pController->SetRotationController(pRotation);
			}

			if (p != -1 && pk != -1)
			{
				if(pk >= m_pDatabaseInfo->m_arrKeyTimes.size() || p >= m_pDatabaseInfo->m_arrPositionTracks.size())
				{
					printf("Error reading controllers from DBA - position. File %s", tmp);
					return false;
				}
				pPosition = PositionControllerPtr(new PositionTrackInformation);
				pPosition->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[pk]);
				pPosition->SetPositionStorage(m_pDatabaseInfo->m_arrPositionTracks[p]);
				pController->SetPositionController(pPosition);
			}

			pController->m_nControllerId = id;
			m_pDatabaseInfo->m_Headers[i].m_pControllers[t] = pController;
			m_pDatabaseInfo->m_Headers[i].m_arrControllerId[t] = id;
		}
	}

#ifdef LOADING_STATISTICS
	//char tmp[4096];
	//		CryModuleMemoryInfo info;
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController903. Finished. Memstat %i", info.allocated - info.freed);
#endif

	return true;
}

bool CLoaderDBA::ReadController904 (IChunkFile::ChunkDesc *pChunkDesc  )
{
	CONTROLLER_CHUNK_DESC_0904* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0904*)pChunkDesc->data;
	//	m_pDatabaseInfo->m_iTotalControllers = 0;

#ifdef LOADING_STATISTICS
	//char tmp[4096];
	CryModuleMemoryInfo info;
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController903. Init. Memstat %i", info.allocated - info.freed);
#endif

	m_pDatabaseInfo->m_AnimationNames.resize(pCtrlChunk->numAnims);
	m_pDatabaseInfo->m_Headers.resize(pCtrlChunk->numAnims);

	//	m_pDatabaseInfo->m_arrKeyTimes.resize(pCtrlChunk->numKeyTime);
	//	m_pDatabaseInfo->m_arrPositionTracks.resize(pCtrlChunk->numKeyPos);
	//	m_pDatabaseInfo->m_arrRotationTracks.resize(pCtrlChunk->numKeyRot);

#ifdef LOADING_STATISTICS
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController904. TracksResize. Memstat %i", info.allocated - info.freed);
#endif


	char *pData = (char*)(pCtrlChunk+1);
	char *pData2 = pData;

	char * pStartChunk = pData;
	char * pEndChunk = pData + pChunkDesc->size;



	uint32 totalAnims = pCtrlChunk->numAnims;
	uint32 keyTime = pCtrlChunk->numKeyTime;
	uint32 keyPos = pCtrlChunk->numKeyPos;
	uint32 keyRot = pCtrlChunk->numKeyRot;


	// Keytimes pointers
	uint16 * pktSizes = (uint16 *)pData;
	pData += keyTime * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	uint32 * pktFormats = (uint32 *)pData;
	pData += (eBitset + 1) * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);


	// Positions pointers
	uint16 * ppSizes = (uint16 *)pData;
	pData += keyPos * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	uint32 * ppFormats = (uint32 *)pData;
	pData += eAutomaticQuat * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	//Rotations pointers
	uint16 *prSizes = (uint16 *)pData;
	pData += keyRot * sizeof(uint16);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	uint32 *prFormats = (uint32 *)pData;
	pData += eAutomaticQuat * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	uint32 * ktOffsets = (uint32 *)pData;
	pData += keyTime * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	uint32 * pOffsets = (uint32 *)pData;
	pData += keyPos * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	uint32 * rOffsets = (uint32 *)pData;
	pData += (keyRot+ 1) * sizeof(uint32);
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	uint32 offset = (eAutomaticQuat + 1) * sizeof(uint32) + keyRot * sizeof(uint16);
	offset += (eAutomaticQuat + 1) * sizeof(uint32) + keyPos * sizeof(uint16);
	offset += (eBitset + 1) * sizeof(uint32) + keyTime * sizeof(uint16);

	offset += (keyRot + keyPos + keyTime + 1) * sizeof(uint32);

	int diff = pData - pData2;
	pData += diff % 4 > 0 ? 4 - diff % 4 : 0;
	CHECK_POINTER(pData, pStartChunk, pEndChunk);
	pData2 = pData;	


	std::vector<uint32> rCount(eAutomaticQuat + 1);
	for (uint32 i = 1; i < eAutomaticQuat + 1; ++i)
	{
		SwapEndianness(rCount[i-1]);
		SwapEndianness(prFormats[i-1]);
	}

	for (uint32 i = 1; i < eAutomaticQuat + 1; ++i)
	{
		rCount[i] += rCount[i-1] + prFormats[i-1];
	}

	std::vector<uint32> pCount(eAutomaticQuat + 1);
	for (uint32 i = 1; i < eAutomaticQuat + 1; ++i)
	{
		SwapEndianness(pCount[i-1]);
		SwapEndianness(ppFormats[i-1]);
	}

	for (uint32 i = 1; i < eAutomaticQuat + 1; ++i)
	{
		pCount[i] += pCount[i-1] + ppFormats[i-1];
	}

	std::vector<uint32> ktCount(eBitset + 2);
	for (uint32 i = 1; i < eBitset + 2; ++i)
	{
		SwapEndianness(ktCount[i-1]);
		SwapEndianness(pktFormats[i-1]);
	}

	for (uint32 i = 1; i < eBitset + 2; ++i)
	{
		ktCount[i] += ktCount[i-1] + pktFormats[i-1];
	}
	/*
	uint32 GetPositionsFormatSizeOf(uint32 format);
	uint32 GetRotationFormatSizeOf(uint32 format);
	uint32 GetKeyTimesFormatSizeOf(uint32 format);

	*/
	uint32 totalAlloc= 0;

	if(ktCount[eBitset + 1])
	{
//		std::vector<uint32> ktOffsets(keyTime);
		m_pDatabaseInfo->m_arrKeyTimes.resize(keyTime);

		uint32 curFormat = 0;
		while (ktCount[curFormat + 1] == 0)
			++curFormat;
		uint32 curSizeof = ControllerHelper::GetKeyTimesFormatSizeOf(curFormat);

		for (uint32 i = 0; i < keyTime; ++i)
		{
			while( i >= ktCount[curFormat + 1])
			{
				++curFormat;
				curSizeof = ControllerHelper::GetKeyTimesFormatSizeOf(curFormat);
			}

//			ktOffsets[i] = totalAlloc;
			SwapEndianness(pktSizes[i]);
			SwapEndianness(ktOffsets[i]);
			totalAlloc += pktSizes[i] * curSizeof;



			m_pDatabaseInfo->m_arrKeyTimes[i] = ControllerHelper::GetKeyTimesControllerPtr(curFormat);
			m_pDatabaseInfo->m_arrKeyTimes[i]->ResizeKeyTime(pktSizes[i]);
			memcpy(m_pDatabaseInfo->m_arrKeyTimes[i]->GetData(), pData + ktOffsets[i], m_pDatabaseInfo->m_arrKeyTimes[i]->GetDataRawSize());
			//pData += m_pDatabaseInfo->m_arrKeyTimes[i]->GetDataRawSize();
			m_pDatabaseInfo->m_arrKeyTimes[i]->SwapBytes();
		}
	}



	if(pCount[eAutomaticQuat])
	{

		m_pDatabaseInfo->m_arrPositionTracks.resize(keyPos);

		uint32 curFormat = 0;
		while (pCount[curFormat + 1] == 0)
			++curFormat;

		uint32 curSizeof = ControllerHelper::GetPositionsFormatSizeOf(curFormat);

		for (uint32 i = 0; i < keyPos; ++i)
		{
			while( i >= pCount[curFormat + 1])
			{
				++curFormat;
				curSizeof = ControllerHelper::GetPositionsFormatSizeOf(curFormat);
			}
			SwapEndianness(ppSizes[i]);
			SwapEndianness(pOffsets[i]);
			totalAlloc += ppSizes[i] * curSizeof;


			m_pDatabaseInfo->m_arrPositionTracks[i] = ControllerHelper::GetPositionControllerPtr(curFormat);
			m_pDatabaseInfo->m_arrPositionTracks[i]->Resize(ppSizes[i]);
			
			memcpy(m_pDatabaseInfo->m_arrPositionTracks[i]->GetData(), pData + pOffsets[i], m_pDatabaseInfo->m_arrPositionTracks[i]->GetDataRawSize());
			m_pDatabaseInfo->m_arrPositionTracks[i]->SwapBytes();
			//pData += m_pDatabaseInfo->m_arrPositionTracks[i]->GetDataRawSize();

		}
	}
 

	if(rCount[eAutomaticQuat])
	{
		m_pDatabaseInfo->m_arrRotationTracks.resize(keyRot);

		uint32 curFormat = 0;
		while (rCount[curFormat + 1] == 0)
			++curFormat;

		uint32 curSizeof = ControllerHelper::GetRotationFormatSizeOf(curFormat);

		for (uint32 i = 0; i < keyRot; ++i)
		{
			while( i >= rCount[curFormat + 1])
			{
				++curFormat;
				curSizeof = ControllerHelper::GetRotationFormatSizeOf(curFormat);
			}

			SwapEndianness(prSizes[i]);
			SwapEndianness(rOffsets[i]);
			totalAlloc += prSizes[i] * curSizeof;

			m_pDatabaseInfo->m_arrRotationTracks[i] = ControllerHelper::GetRotationControllerPtr(curFormat);
			m_pDatabaseInfo->m_arrRotationTracks[i]->Resize(prSizes[i]);

			memcpy(m_pDatabaseInfo->m_arrRotationTracks[i]->GetData(), pData + rOffsets[i], m_pDatabaseInfo->m_arrRotationTracks[i]->GetDataRawSize());
			//pData += m_pDatabaseInfo->m_arrRotationTracks[i]->GetDataRawSize();
			m_pDatabaseInfo->m_arrRotationTracks[i]->SwapBytes();
		}
	}

	//FIXME:
	//m_pDatabaseInfo->m_Storage.resize(totalAlloc);
	//memcpy(&(m_pDatabaseInfo->m_Storage[0]), pData, totalAlloc);

	//pData += totalAlloc;

	// set pointers to keytimes, positions, rotations

#ifdef LOADING_STATISTICS
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController904. All dynamic data resize. Memstat %i", info.allocated - info.freed);
#endif


	totalAlloc = rOffsets[keyRot];
	totalAlloc += totalAlloc % 4 > 0 ? ( 4 - totalAlloc % 4) : 0;
	SwapEndianness(totalAlloc);


	uint32 rc =0, pc = 0, cc = 0;

	pData = pData2 + totalAlloc;
	CHECK_POINTER(pData, pStartChunk, pEndChunk);

	m_pDatabaseInfo->m_ControllerInfos.resize(totalAnims);

	for (uint32 i = 0; i < totalAnims; ++i)
	{

		uint16 strSize;// = pData;
		memcpy(&strSize, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
		char tmp[1024];
		SwapEndianness(strSize);
		memset(tmp,0,1024);
		memcpy(tmp, pData, strSize);

		m_pDatabaseInfo->m_AnimationNames[i] = tmp;

		pData += strSize;
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
		m_pDatabaseInfo->m_AnimationCRCMap.insert(std::make_pair<uint32, uint32>(g_crcGen.GetCRC32(tmp), i));

		static CStoredSkinningInfo info;

		memcpy(&info, pData, sizeof(info));
		pData += sizeof(info);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		// fill 
		m_pDatabaseInfo->m_Headers[i].m_nTicksPerFrame = info.m_nTicksPerFrame;
		m_pDatabaseInfo->m_Headers[i].m_secsPerTick = info.m_secsPerTick;
		m_pDatabaseInfo->m_Headers[i].m_nStart			= info.m_nStart;
		m_pDatabaseInfo->m_Headers[i].m_nEnd				= info.m_nEnd;
		m_pDatabaseInfo->m_Headers[i].m_Speed				= info.m_Speed;
		m_pDatabaseInfo->m_Headers[i].m_Distance		= info.m_Distance;
		m_pDatabaseInfo->m_Headers[i].m_Slope				= info.m_Slope;
		m_pDatabaseInfo->m_Headers[i].m_LHeelStart	= info.m_LHeelStart,
		m_pDatabaseInfo->m_Headers[i].m_LHeelEnd		= info.m_LHeelEnd;
		m_pDatabaseInfo->m_Headers[i].m_LToe0Start	= info.m_LToe0Start;
		m_pDatabaseInfo->m_Headers[i].m_LToe0End		= info.m_LToe0End;
		m_pDatabaseInfo->m_Headers[i].m_RHeelStart	= info.m_RHeelStart;
		m_pDatabaseInfo->m_Headers[i].m_RHeelEnd		= info.m_RHeelEnd;
		m_pDatabaseInfo->m_Headers[i].m_RToe0Start	= info.m_RToe0Start;
		m_pDatabaseInfo->m_Headers[i].m_RToe0End		= info.m_RToe0End;
		m_pDatabaseInfo->m_Headers[i].m_MoveDirection = info.m_MoveDirection; // raw storage
		m_pDatabaseInfo->m_Headers[i].m_nAssetFlags   = info.m_nAssetFlags;

		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_nTicksPerFrame);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_secsPerTick);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_nStart);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_nEnd);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_Speed);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_Distance);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_Slope);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_nAssetFlags);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_LHeelStart);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_LHeelEnd);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_LToe0Start);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_LToe0End);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_RHeelStart);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_RHeelEnd);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_RToe0Start);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_RToe0End);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_MoveDirection.x);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_MoveDirection.y);
		SwapEndianness(m_pDatabaseInfo->m_Headers[i].m_MoveDirection.z);

		// footplants
		uint16 footplans;// = m_arrAnimations[i].m_FootPlantBits.size();
		memcpy(&footplans, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);
		SwapEndianness(footplans);
		m_pDatabaseInfo->m_Headers[i].m_FootPlantBits.resize(footplans);

		if (footplans)
		{
			memcpy(&(m_pDatabaseInfo->m_Headers[i].m_FootPlantBits[0]), pData, footplans);
			pData += footplans;
			CHECK_POINTER(pData, pStartChunk, pEndChunk);
		}

		// 
		uint16 controllerInfo;// = m_arrAnimations[i].m_arrControlerInfo.size();
		memcpy(&controllerInfo, pData, sizeof(uint16));
		pData += sizeof(uint16);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		SwapEndianness(controllerInfo);
		if (controllerInfo == 0)
			continue;


		m_pDatabaseInfo->m_ControllerInfos[i].resize(controllerInfo);

		std::vector<CControllerInfo>& controllers = m_pDatabaseInfo->m_ControllerInfos[i];
		//		controllers.resize(controllerInfo);



		memcpy(&(controllers[0]), pData, controllerInfo * sizeof(CControllerInfo));
		pData += controllerInfo * sizeof(CControllerInfo);
		CHECK_POINTER(pData, pStartChunk, pEndChunk);

		m_pDatabaseInfo->m_Headers[i].m_pControllers.resize(controllerInfo);
		m_pDatabaseInfo->m_Headers[i].m_arrControllerId.resize(controllerInfo);

		for (uint32  t = 0; t < controllerInfo; ++t)
		{

			SwapEndianness(controllers[t].m_nPosTrack);
			SwapEndianness(controllers[t].m_nRotTrack);
			SwapEndianness(controllers[t].m_nPosKeyTimeTrack);
			SwapEndianness(controllers[t].m_nRotKeyTimeTrack);
			SwapEndianness(controllers[t].m_nControllerID);

			uint32 p = controllers[t].m_nPosTrack;
			uint32 r = controllers[t].m_nRotTrack;
			uint32 pk = controllers[t].m_nPosKeyTimeTrack;
			uint32 rk = controllers[t].m_nRotKeyTimeTrack;
			uint32 id = controllers[t].m_nControllerID;

			//			m_pDatabaseInfo->m_arrPositionTracks[p]->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[pk]);
			//			m_pDatabaseInfo->m_arrRotationTracks[r]->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[rk]);
			CController_AutoPtr pController(new CController);
			RotationControllerPtr pRotation;
			PositionControllerPtr pPosition;

			if (r != -1 && rk != -1)
			{
				if(rk >= m_pDatabaseInfo->m_arrKeyTimes.size() || r >= m_pDatabaseInfo->m_arrRotationTracks.size())
				{
					printf("Error reading controllers from DBA - rotation. File %s", tmp);
					return false;
				}
				pRotation =  RotationControllerPtr(new RotationTrackInformation);			
				pRotation->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[rk]);
				pRotation->SetRotationStorage(m_pDatabaseInfo->m_arrRotationTracks[r]);
				pController->SetRotationController(pRotation);
			}

			if (p != -1 && pk != -1)
			{
				if(pk >= m_pDatabaseInfo->m_arrKeyTimes.size() || p >= m_pDatabaseInfo->m_arrPositionTracks.size())
				{
					printf("Error reading controllers from DBA - position. File %s", tmp);
					return false;
				}
				pPosition = PositionControllerPtr(new PositionTrackInformation);
				pPosition->SetKeyTimesInformation(m_pDatabaseInfo->m_arrKeyTimes[pk]);
				pPosition->SetPositionStorage(m_pDatabaseInfo->m_arrPositionTracks[p]);
				pController->SetPositionController(pPosition);
			}

			pController->m_nControllerId = id;
			m_pDatabaseInfo->m_Headers[i].m_pControllers[t] = pController;
			m_pDatabaseInfo->m_Headers[i].m_arrControllerId[t] = id;
		}
	}

#ifdef LOADING_STATISTICS
	//char tmp[4096];
	//		CryModuleMemoryInfo info;
	CryGetMemoryInfoForModule(&info);
	g_pILog->UpdateLoadingScreen("ReadController904. Finished. Memstat %i", info.allocated - info.freed);
#endif

	return true;


	return true;
}



const CCommonSkinningInfo * CLoaderDBA::GetSkinningInfo(const char * filename) const
{
	//int num = m_pDatabaseInfo->m_AnimationNames.GetValue(filename);
	uint32 crc =  g_crcGen.GetCRC32(filename);
	std::map<uint32, uint32>::iterator it = m_pDatabaseInfo->m_AnimationCRCMap.find(crc);
	if ( it == m_pDatabaseInfo->m_AnimationCRCMap.end())
		return 0;

	return &m_pDatabaseInfo->m_Headers[it->second];
}
