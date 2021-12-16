////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CGFLoader.cpp
//  Version:     v1.00
//  Created:     6/11/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <StringUtils.h>
#include "CryPath.h"
#include <CryTypeInfo.h>
//#include "AnimationBase.h"
//#include <ICryAnimation.h>
//#include <CryTypeInfo.h>
#include "ChunkFile.h"
#include <CGFContent.h>
#include "..\CGA\Controller.h"
#include "..\CGA\ControllerPQ.h"
#include "..\CGA\ControllerPQLog.h"
#include "LoaderCAF.h"


//////////////////////////////////////////////////////////////////////////
CLoaderCAF::CLoaderCAF()
{
	m_pChunkFile = NULL;
	m_bLoadOldChunk = false;
	m_pSkinningInfo = 0;
}

//////////////////////////////////////////////////////////////////////////
CLoaderCAF::~CLoaderCAF()
{
	if (m_pSkinningInfo)
		delete m_pSkinningInfo;
}

//////////////////////////////////////////////////////////////////////////
CInternalSkinningInfo * CLoaderCAF::LoadCAF( const char *filename,ILoaderCAFListener* pListener )
{
	m_pListener = pListener;

//	IChunkFile  * chunkFile = g_pI3DEngine->CreateChunkFile();
	CChunkFile chunkFile;
	//FIXME:
	CInternalSkinningInfo* pInfo = LoadCAF( filename,&chunkFile,pListener );

//	g_pI3DEngine->ReleaseChunkFile(chunkFile);

	m_pListener = 0;

	return pInfo;
}

//////////////////////////////////////////////////////////////////////////
CInternalSkinningInfo * CLoaderCAF::LoadCAF( const char *filename,IChunkFile * pChunkFile,ILoaderCAFListener* pListener )
{
	m_pListener = pListener;

	string strPath = filename;
	CryStringUtils::UnifyFilePath(strPath);
	string fileExt = PathUtil::GetExt(strPath);
//	m_IsCHR = (0 == stricmp(fileExt,"chr"));

	m_filename = filename;
	m_pChunkFile = pChunkFile;

	if (!pChunkFile->Read( filename ))
	{
		const char* errorMsg = pChunkFile->GetLastError();
		if (!errorMsg || !errorMsg[0])
		{
			errorMsg = "UNKNOWN ERROR";
		}
		m_LastError.Format("Chunk file error: %s", errorMsg );
		return 0;
	}

	// Load mesh from chunk file.
	if ((pChunkFile->GetFileHeader().Version != ChunkFileVersion) && (pChunkFile->GetFileHeader().Version != ChunkFileVersion_Align))
	{
		m_LastError.Format("Bad CGF file version: %s", m_filename.c_str() );
		return 0;
	}

	if (pChunkFile->GetFileHeader().FileType != FileType_Geom) 
	{
		if (pChunkFile->GetFileHeader().FileType != FileType_Anim) 
		{
			m_LastError.Format("Illegal File Type for .cgf file: %s", m_filename.c_str() );
			return 0;
		}
	}

	m_pSkinningInfo = new CInternalSkinningInfo;

	uint32 suc = LoadChunks();
	assert(suc);
	if (suc==0)
	{
		delete m_pSkinningInfo;
		m_pSkinningInfo = 0;
		return 0;
	}


	m_pListener = 0;

	return m_pSkinningInfo;
}


bool CLoaderCAF::LoadChunks()
{


	// Load Nodes.	
	uint32 numChunck = m_pChunkFile->NumChunks();

	m_pSkinningInfo->m_arrControllers.resize(numChunck);	
	m_pSkinningInfo->m_numChunks = numChunck;	

	for (uint32 i=0; i<numChunck; i++)
	{

		const CHUNK_HEADER &hdr = m_pChunkFile->GetChunkHeader(i);

		if (hdr.ChunkType == ChunkType_Controller && hdr.ChunkVersion > CONTROLLER_CHUNK_DESC_0828::VERSION)
		{
			m_pSkinningInfo->m_bNewFormat = true;
		}
		else
		{
			if (hdr.ChunkType == ChunkType_Controller && (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION || hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION))
			{
				m_pSkinningInfo->m_bOldFormat = true;
			}
			else
			{
				if (hdr.ChunkType == ChunkType_Controller && hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
				m_pSkinningInfo->m_bTCBFormat = true;
			}
		}
	}


	if (m_bLoadOldChunk && !m_pSkinningInfo->m_bOldFormat)
		m_bLoadOldChunk = false;
		

	// we have only old chunks
	if (!m_pSkinningInfo->m_bNewFormat)
		m_bLoadOldChunk = true;


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

		case ChunkType_Timing:
			if (!ReadTiming( m_pChunkFile->GetChunk(i)) )
				return false;
			break;


		case ChunkType_SpeedInfo:
			if (!ReadSpeedInfo( m_pChunkFile->GetChunk(i)) )
				return false;
			break;

		case ChunkType_FootPlantInfo:
			if (!ReadFootPlantInfo(m_pChunkFile->GetChunk(i)))
				return false;
			break;

		case ChunkType_Timestamp:
			if (!ReadTimestamp(m_pChunkFile->GetChunk(i)))
				return false;
			break;

		}
	}

	return true;
}




//////////////////////////////////////////////////////////////////////////
bool CLoaderCAF::ReadSpeedInfo (  IChunkFile::ChunkDesc *pChunkDesc  )
{



	if (pChunkDesc->hdr.ChunkVersion == SPEED_CHUNK_DESC::VERSION)
	{
		SPEED_CHUNK_DESC* pChunk = (SPEED_CHUNK_DESC*)pChunkDesc->data;
		SwapEndian(*pChunk);
		m_pSkinningInfo->m_Speed	= pChunk->Speed;
		m_pSkinningInfo->m_Distance = pChunk->Distance;
		m_pSkinningInfo->m_Slope = pChunk->Slope;
		m_pSkinningInfo->m_nAssetFlags = pChunk->Looped;

		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == SPEED_CHUNK_DESC_1::VERSION)
	{
		SPEED_CHUNK_DESC_1* pChunk = (SPEED_CHUNK_DESC_1*)pChunkDesc->data;
		SwapEndian(*pChunk);
		m_pSkinningInfo->m_Speed	= pChunk->Speed;
		m_pSkinningInfo->m_Distance = pChunk->Distance;
		m_pSkinningInfo->m_Slope = pChunk->Slope;
		m_pSkinningInfo->m_nAssetFlags = pChunk->Looped;
		m_pSkinningInfo->m_MoveDirection[0] = pChunk->MoveDir[0];
		m_pSkinningInfo->m_MoveDirection[1] = pChunk->MoveDir[1];
		m_pSkinningInfo->m_MoveDirection[2] = pChunk->MoveDir[2];

		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == SPEED_CHUNK_DESC_2::VERSION)
	{
		SPEED_CHUNK_DESC_2* pChunk = (SPEED_CHUNK_DESC_2*)pChunkDesc->data;
		SwapEndian(*pChunk);
		m_pSkinningInfo->m_Speed	= pChunk->Speed;
		m_pSkinningInfo->m_Distance = pChunk->Distance;
		m_pSkinningInfo->m_Slope = pChunk->Slope;
		m_pSkinningInfo->m_nAssetFlags = pChunk->Looped;

		m_pSkinningInfo->m_MoveDirection[0] = pChunk->MoveDir[0];
		m_pSkinningInfo->m_MoveDirection[1] = pChunk->MoveDir[1];
		m_pSkinningInfo->m_MoveDirection[2] = pChunk->MoveDir[2];

		m_pSkinningInfo->m_StartLocation = pChunk->StartPosition;

		return 1;
	}


	return 1;
}

//////////////////////////////////////////////////////////////////////////
bool CLoaderCAF::ReadFootPlantInfo(IChunkFile::ChunkDesc *pChunkDesc)
{

	uint32 version = pChunkDesc->hdr.ChunkVersion;
	uint32 id = pChunkDesc->hdr.ChunkID;
	uint32 nChunkSize = pChunkDesc->size;

		{
			// the old chunk version - fixed-size name list
			FOOTPLANT_INFO* pChunk = (FOOTPLANT_INFO*)(pChunkDesc->data);
			SwapEndian(*pChunk);

			int nPoses = pChunk->nPoses;


			m_pSkinningInfo->m_LHeelStart = pChunk->m_LHeelStart;
			m_pSkinningInfo->m_LHeelEnd = pChunk->m_LHeelEnd;
			m_pSkinningInfo->m_LToe0Start = pChunk->m_LToe0Start;
			m_pSkinningInfo->m_LToe0End = pChunk->m_LToe0End;

			m_pSkinningInfo->m_RHeelStart = pChunk->m_RHeelStart;
			m_pSkinningInfo->m_RHeelEnd = pChunk->m_RHeelEnd;
			m_pSkinningInfo->m_RToe0Start = pChunk->m_RToe0Start;
			m_pSkinningInfo->m_RToe0End = pChunk->m_RToe0End;

			m_pSkinningInfo->m_FootPlantBits.resize(nPoses);
			// just fill in the pointers to the fixed-size string buffers
			const uint8* pFootPlants = (const uint8*)(pChunk+1);

			for (int i = 0; i < nPoses; ++i)
				m_pSkinningInfo->m_FootPlantBits[i] = *(pFootPlants++);
		}

	return 1;
}


//////////////////////////////////////////////////////////////////////////
bool CLoaderCAF::ReadTiming (  IChunkFile::ChunkDesc *pChunkDesc  )
{

	TIMING_CHUNK_DESC* pTimingChunk = (TIMING_CHUNK_DESC*)pChunkDesc->data;
	SwapEndian(*pTimingChunk);

	m_pSkinningInfo->m_nTicksPerFrame	= pTimingChunk->TicksPerFrame;
	m_pSkinningInfo->m_secsPerTick		= pTimingChunk->SecsPerTick;
	m_pSkinningInfo->m_nStart					= pTimingChunk->global_range.start;
	m_pSkinningInfo->m_nEnd						= pTimingChunk->global_range.end;

	return 1;
}

//////////////////////////////////////////////////////////////////////////
bool CLoaderCAF::ReadTimestamp (  IChunkFile::ChunkDesc *pChunkDesc  )
{

	DWORD* pTimeStamp = (DWORD*)pChunkDesc->data;
	SwapEndian(*pTimeStamp);

	m_pSkinningInfo->m_dwTimeStamp	= *pTimeStamp;

	return 1;
}

//////////////////////////////////////////////////////////////////////////
bool CLoaderCAF::ReadController (  IChunkFile::ChunkDesc *pChunkDesc  )
{

	//m_pSkinningInfo->m_arrTracksPQLog.reserve( 200 );
	//m_pSkinningInfo->m_arrTracksTimes.reserve( 200 );
	m_pSkinningInfo->m_TrackVec3.reserve(200);
	m_pSkinningInfo->m_TrackQuat.reserve(200);
	m_pSkinningInfo->m_pControllers.reserve(200);


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
	{
		CONTROLLER_CHUNK_DESC_0826* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0826*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk);

		
		if (pCtrlChunk->type==CTRL_TCB3)
		{
			return true;
		}

		if (pCtrlChunk->type==CTRL_TCBQ)
		{
			DynArray<CryTCBQKey> * pTrack = new DynArray<CryTCBQKey>;
			TCBFlags trackflags;

			CryTCBQKey *pKeys = (CryTCBQKey*)(pCtrlChunk+1);
			uint32 nkeys = pCtrlChunk->nKeys;
			pTrack->resize(nkeys);
			for (uint32 i=0; i<nkeys; i++)
			{
				//track[i] = pKeys[i];// * secsPerTick;
				pTrack->operator [](i) = pKeys[i];
			}

			if (pCtrlChunk->nFlags & CTRL_ORT_CYCLE)
				trackflags.f0=1;
			else if (pCtrlChunk->nFlags & CTRL_ORT_LOOP)
				trackflags.f1=1;;

			uint32 numControllers = m_pSkinningInfo->m_TrackQuat.size();
			m_pSkinningInfo->m_TrackQuatFlags.push_back(trackflags);
			m_pSkinningInfo->m_TrackQuat.push_back(pTrack);
			m_pSkinningInfo->m_arrControllers[pCtrlChunk->chdr.ChunkID].type=0xaa;
			m_pSkinningInfo->m_arrControllers[pCtrlChunk->chdr.ChunkID].index=numControllers;

			m_pSkinningInfo->m_bTCBFormat = true;
			return true;
		}

		if (pCtrlChunk->type==CTRL_CRYBONE)
		{
			m_LastError.Format("animation not loaded: controller-type CTRL_CRYBONE not supported");
			return false;
		}		

		if (pCtrlChunk->type==CTRL_BEZIER3)
		{
			m_LastError.Format("animation not loaded: controller-type CTRL_BEZIER3 not supported");
			return false;
		}		

		m_LastError.Format("animation not loaded: found unsupported controller type in chunk");
		return false;
	}


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION)
	{

		if (!m_bLoadOldChunk)
			return 1;

		CONTROLLER_CHUNK_DESC_0827* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0827*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk);

		//pSkinningInfo->m_nControllerId = pCtrlChunk->nControllerId;
		m_pSkinningInfo->m_arrControllerId.push_back( pCtrlChunk->nControllerId );
		uint32 numKeys = pCtrlChunk->numKeys;
		assert(numKeys<512);

		CryKeyPQLog* pCryKey = (CryKeyPQLog*)(pCtrlChunk+1);
		SwapEndian( pCryKey,numKeys );



		CControllerPQLog * pController = new CControllerPQLog;

		pController->m_nControllerId	= pCtrlChunk->nControllerId;





		pController->m_arrKeys.reserve(numKeys);
		pController->m_arrTimes.reserve(numKeys);

		int32 lasttime = -1; 
		for (uint32 i=0; i<numKeys; ++i)
		{
			int32 curtime = pCryKey[i].nTime /TICKS_CONVERT;
			if (curtime != lasttime)
			{
				pController->m_arrTimes.push_back(curtime);

				PQLog q;
				q.vPos=Vec3(ZERO);
				q.vRotLog=Vec3(ZERO);

		//		if (pCryKey[i].vPos.IsValid())
					q.vPos = pCryKey[i].vPos/100.0f;
		//		if (pCryKey[i].vRotLog.IsValid())
					q.vRotLog = pCryKey[i].vRotLog;

				pController->m_arrKeys.push_back(q);
					//pController->m_arrKeys[i].vPos		= pCryKey[i].vPos/100.0f;
					//pController->m_arrKeys[i].vRotLog	= pCryKey[i].vRotLog;
			}
		}

		m_pSkinningInfo->m_bOldFormat = true;
		m_pSkinningInfo->m_pControllers.push_back(pController);

		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION)
	{

		if (!m_bLoadOldChunk)
			return 1;

		CONTROLLER_CHUNK_DESC_0828* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0828*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk);

		//pSkinningInfo->m_nControllerId = pCtrlChunk->nControllerId;
		m_pSkinningInfo->m_arrControllerId.push_back( pCtrlChunk->nControllerId );
		uint32 numKeys = pCtrlChunk->numKeys;

		CryKeyPQLog* pCryKey = (CryKeyPQLog*)(pCtrlChunk+1);
		SwapEndian( pCryKey,numKeys );


		CControllerPQLog * pController = new CControllerPQLog;

		pController->m_nControllerId	= pCtrlChunk->nControllerId;


		pController->m_arrKeys.reserve(numKeys);
		pController->m_arrTimes.reserve(numKeys);

		int32 lasttime = -1; 
		for (uint32 i=0; i<numKeys; ++i)
		{
			int32 curtime = pCryKey[i].nTime /TICKS_CONVERT;
			if (curtime != lasttime)
			{
				pController->m_arrTimes.push_back(curtime);
				PQLog q;
				q.vPos = pCryKey[i].vPos/100.0f;
				q.vRotLog = pCryKey[i].vRotLog;
				pController->m_arrKeys.push_back(q);
				//pController->m_arrKeys[i].vPos		= pCryKey[i].vPos/100.0f;
				//pController->m_arrKeys[i].vRotLog	= pCryKey[i].vRotLog;
			}
		}

		m_pSkinningInfo->m_pControllers.push_back(pController);
		m_pSkinningInfo->m_bOldFormat = true;

		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0829::VERSION)
	{

		if (m_bLoadOldChunk)
			return 1;


		m_pSkinningInfo->m_bNewFormat = true;
//		return 1;
		//if (m_bLoadOldChunks)
		//	return 1;

		CONTROLLER_CHUNK_DESC_0829* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0829*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk);


		CController * pController = new CController;

		pController->m_nControllerId = pCtrlChunk->nControllerId;
		m_pSkinningInfo->m_arrControllerId.push_back( pCtrlChunk->nControllerId );
		m_pSkinningInfo->m_pControllers.push_back(pController);


		char *pData = (char*)(pCtrlChunk+1);


		KeyTimesInformationPtr pRotTimeKeys;
		KeyTimesInformationPtr pPosTimeKeys;
		KeyTimesInformationPtr pSclTimeKeys;

		RotationControllerPtr pRotation;
		PositionControllerPtr pPosition;
		PositionControllerPtr pScale;

		if (pCtrlChunk->numRotationKeys)
		{
			// we have a rotation info
			pRotation =  RotationControllerPtr(new RotationTrackInformation);

			TrackRotationStoragePtr pStorage = ControllerHelper::GetRotationControllerPtr(pCtrlChunk->RotationFormat);
			pRotation->SetRotationStorage(pStorage);

			if (!pStorage)
				return 0;

			pStorage->Resize(pCtrlChunk->numRotationKeys);

			// its not very safe but its extremely fast!
			memcpy(pStorage->GetData(), pData, pStorage->GetDataRawSize());
			pData+=pStorage->GetDataRawSize();

			pRotTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->RotationTimeFormat);//new F32KeyTimesInformation;//CKeyTimesInformation;
			pRotTimeKeys->ResizeKeyTime(pCtrlChunk->numRotationKeys);

			memcpy(pRotTimeKeys->GetData(), pData, pRotTimeKeys->GetDataRawSize());
			pData += pRotTimeKeys->GetDataRawSize();

			pRotation->SetKeyTimesInformation(pRotTimeKeys);

			pController->SetRotationController(pRotation);

		}

		if (pCtrlChunk->numPositionKeys)
		{
			pPosition = PositionControllerPtr(new PositionTrackInformation);

			TrackPositionStoragePtr pStorage = ControllerHelper::GetPositionControllerPtr(pCtrlChunk->PositionFormat);
			if (!pStorage)
				return 0;

			pPosition->SetPositionStorage(pStorage);

			pStorage->Resize(pCtrlChunk->numPositionKeys);

			memcpy(pStorage->GetData(), pData, pStorage->GetDataRawSize());
			pData += pStorage->GetDataRawSize();

			if (pCtrlChunk->PositionKeysInfo == CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation)
			{
				pPosition->SetKeyTimesInformation(pRotTimeKeys);
			}
			else
			{
				// load from chunk
				pPosTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->PositionTimeFormat);;//PositionTimeFormat;new F32KeyTimesInformation;//CKeyTimesInformation;
				pPosTimeKeys->ResizeKeyTime(pCtrlChunk->numPositionKeys);

				memcpy(pPosTimeKeys->GetData(), pData, pPosTimeKeys->GetDataRawSize());
				pData += pPosTimeKeys->GetDataRawSize();

				pPosition->SetKeyTimesInformation(pPosTimeKeys);
			}

			pController->SetPositionController(pPosition);
		}
		m_pSkinningInfo->m_bNewFormat = true;
		return 1;


	}



	//if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0829::VERSION)
	//{
	//	m_pSkinningInfo->m_bNewFormat = true;
	//	return 1;
	//}

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0830::VERSION)
	{
		m_pSkinningInfo->m_bNewFormat = true;
		return 1;
	}



	return 0;
}


void CLoaderCAF::Warning(const char* szFormat, ...)
{
	if (m_pListener)
	{
		char szBuffer[1024];
		va_list args;
		va_start(args, szFormat);
		vsprintf(szBuffer, szFormat, args);
		m_pListener->Warning(szBuffer);
		va_end(args);
	}
}
