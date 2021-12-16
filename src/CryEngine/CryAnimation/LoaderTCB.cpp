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
#include "ControllerPQ.h"
#include <CGFContent.h>
#include "LoaderTCB.h"




//////////////////////////////////////////////////////////////////////////
CLoaderTCB::CLoaderTCB()
{
	m_pSkinningInfo = 0;
	m_bHasNewControllers = false;
	m_bLoadOldChunks = false;
	m_bHasOldControllers = false;
	m_bLoadOnlyCommon = false;
}

//////////////////////////////////////////////////////////////////////////
CLoaderTCB::~CLoaderTCB()
{
	if (m_pSkinningInfo)
	{
		delete m_pSkinningInfo;
		m_pSkinningInfo = 0;
	}
}


//////////////////////////////////////////////////////////////////////////
CHeaderTCB * CLoaderTCB::LoadTCB( const char *filename,ILoaderCAFListener* pListener )
{
	IChunkFile  * chunkFile = g_pI3DEngine->CreateChunkFile(true);
	CHeaderTCB* pInfo = LoadTCB( filename,chunkFile,pListener );
	chunkFile->Release();
	return pInfo;
}
//////////////////////////////////////////////////////////////////////////
CHeaderTCB * CLoaderTCB::LoadTCB( const char *filename,IChunkFile * pChunkFile,ILoaderCAFListener* pListener )
{
	stack_string strPath = filename;
	CryStringUtils::UnifyFilePath(strPath);

	m_filename = filename;

	if (!pChunkFile->Read( filename ))
	{
		m_LastError = pChunkFile->GetLastError();
		return 0;
	}

	// Load mesh from chunk file.
	if ((pChunkFile->GetFileHeader().Version != ChunkFileVersion) && (pChunkFile->GetFileHeader().Version != ChunkFileVersion_Align))
	{
		m_LastError.Format("Bad CAF file version: %s", m_filename.c_str() );
		return 0;
	}

	if (pChunkFile->GetFileHeader().FileType != FileType_Geom) 
	{
		if (pChunkFile->GetFileHeader().FileType != FileType_Anim) 
		{
			m_LastError.Format("Illegal File Type for .caf file: %s", m_filename.c_str() );
			return 0;
		}
	}

	m_pSkinningInfo = new CHeaderTCB;

	if (!LoadChunksTCB(pChunkFile))
	{
		delete m_pSkinningInfo;
		m_pSkinningInfo = 0;
	}

	return m_pSkinningInfo;
}


bool CLoaderTCB::LoadChunksTCB(IChunkFile* pChunkFile)
{
	// Load Nodes.	
	uint32 numChunck = pChunkFile->NumChunks();

	uint32 statTCB(0),statNewChunck(0), statOldChunck(0);

	for (uint32 i=0; i<numChunck; i++)
	{
		const CHUNK_HEADER &hdr = pChunkFile->GetChunkHeader(i);
		if (hdr.ChunkType == ChunkType_Controller && hdr.ChunkVersion > CONTROLLER_CHUNK_DESC_0828::VERSION)
		{
			CryFatalError("CONTROLLER_CHUNK_DESC_0828" );
			m_bHasNewControllers = true;
			++statNewChunck;
		}
		else
		{
			if (hdr.ChunkType == ChunkType_Controller && (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION || hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION))
			{
				CryFatalError("CONTROLLER_CHUNK_DESC_0828" );
				m_bHasOldControllers = true;
				++statOldChunck;
			}
			else
			{
				if (hdr.ChunkType == ChunkType_Controller && hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
					++statTCB;
			}
		}
	}

	if (!m_bHasNewControllers)
		m_bLoadOldChunks = true;

	if (m_bLoadOnlyCommon==0) 
	{
		// Only old data will be loaded. 
		if (m_bLoadOldChunks)
		{
			m_pSkinningInfo->m_arrControllerId.reserve(statOldChunck);
			m_pSkinningInfo->m_pControllers.reserve(statOldChunck);
		}
		else
		{
			m_pSkinningInfo->m_arrControllerId.reserve(statNewChunck);
			m_pSkinningInfo->m_pControllers.reserve(statNewChunck);

		}

		// We have a TCB data. Reserve memory 
		if (statTCB > 0)
		{
			m_pSkinningInfo->m_arrControllersTCB.resize(numChunck);
			m_pSkinningInfo->m_TrackVec3QQQ.reserve(statTCB);
			m_pSkinningInfo->m_TrackQuat.reserve(statTCB);
		}
	}

	for (uint32 i=0; i<numChunck; i++)
	{
		const CHUNK_HEADER &hdr = pChunkFile->GetChunkHeader(i);
		switch (hdr.ChunkType)
		{
		case ChunkType_Controller:
			if (!ReadController( pChunkFile->GetChunk(i)) )
				return false;
			break;

		case ChunkType_Timing:
			if (!ReadTiming( pChunkFile->GetChunk(i)) )
				return false;
			break;

		case ChunkType_MotionParameters:
			if (!ReadMotionParameters( pChunkFile->GetChunk(i)) )
				return false;
			break;
		}
	}

	return true;
}










//////////////////////////////////////////////////////////////////////////
bool CLoaderTCB::ReadMotionParameters (  IChunkFile::ChunkDesc *pChunkDesc  )
{
	CryFatalError("ReadMotionParameters" );
	if (pChunkDesc->hdr.ChunkVersion == SPEED_CHUNK_DESC_2::VERSION)
	{
		//	CryFatalError("SPEED_CHUNK_DESC_2 not supported any more");
		SPEED_CHUNK_DESC_2* pChunk = (SPEED_CHUNK_DESC_2*)pChunkDesc->data;
		SwapEndian(*pChunk, pChunkDesc->bSwapEndian);
		m_pSkinningInfo->m_nAnimFlags = pChunk->AnimFlags;
		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == CHUNK_MOTION_PARAMETERS::VERSION)
	{
		CHUNK_MOTION_PARAMETERS* pChunk = (CHUNK_MOTION_PARAMETERS*)pChunkDesc->data;
#if (defined(XENON) || defined(PS3) || defined(CAFE))
		if ((pChunk->mp.m_nAssetFlags&CA_ASSET_BIG_ENDIAN)==0)
			CryFatalError("Data Error in Console Build: CAF file must have Big Endian format");
#endif
#if !(defined(XENON) || defined(PS3))
		if ((pChunk->mp.m_nAssetFlags&CA_ASSET_BIG_ENDIAN))
			CryFatalError("Data Error in Win32 Build: CAF file must have Little Endian format");
#endif

		m_pSkinningInfo->m_nAnimFlags			= pChunk->mp.m_nAssetFlags;
		m_pSkinningInfo->m_nCompression		= pChunk->mp.m_nCompression;

		m_pSkinningInfo->m_nTicksPerFrame	= pChunk->mp.m_nTicksPerFrame;
		m_pSkinningInfo->m_secsPerTick		= pChunk->mp.m_fSecsPerTick;
		m_pSkinningInfo->m_nStart					= pChunk->mp.m_nStart;
		m_pSkinningInfo->m_nEnd						= pChunk->mp.m_nEnd;
		return 1;
	}


	return 1;
}



//////////////////////////////////////////////////////////////////////////
bool CLoaderTCB::ReadTiming (  IChunkFile::ChunkDesc *pChunkDesc  )
{

	TIMING_CHUNK_DESC* pTimingChunk = (TIMING_CHUNK_DESC*)pChunkDesc->data;
	SwapEndian(*pTimingChunk, pChunkDesc->bSwapEndian);

	m_pSkinningInfo->m_nTicksPerFrame	= pTimingChunk->m_TicksPerFrame;
	m_pSkinningInfo->m_secsPerTick		= pTimingChunk->m_SecsPerTick;
	m_pSkinningInfo->m_nStart					= pTimingChunk->global_range.start;
	m_pSkinningInfo->m_nEnd						= pTimingChunk->global_range.end;

	return 1;
}

//////////////////////////////////////////////////////////////////////////
bool CLoaderTCB::ReadController (  IChunkFile::ChunkDesc *pChunkDesc  )
{
	if (m_bLoadOnlyCommon)
		return true;

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
	{
		CONTROLLER_CHUNK_DESC_0826* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0826*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk, pChunkDesc->bSwapEndian);

		if (pCtrlChunk->type==CTRL_TCB3)
		{
			DynArray<CryTCB3Key> * pTrack = new DynArray<CryTCB3Key>;
			TCBFlags trackflags;

			CryTCB3Key* pKeys = (CryTCB3Key*)(pCtrlChunk+1);
			uint32 nkeys = pCtrlChunk->nKeys;
			SwapEndian(pKeys, nkeys, pChunkDesc->bSwapEndian);
			pTrack->resize(nkeys);

			for (uint32 i=0; i<nkeys; i++)
			{
				(*pTrack)[i] = pKeys[i];
			}

			if (pCtrlChunk->nFlags & CTRL_ORT_CYCLE)
				trackflags.f0=1;
			else if (pCtrlChunk->nFlags & CTRL_ORT_LOOP)
				trackflags.f1=1;

			uint32 numControllers = m_pSkinningInfo->m_TrackVec3QQQ.size();
			m_pSkinningInfo->m_TrackVec3FlagsQQQ.push_back(trackflags);
			m_pSkinningInfo->m_TrackVec3QQQ.push_back(pTrack);

			int32 num=m_pSkinningInfo->m_arrControllersTCB.size();
			if( pCtrlChunk->chdr.ChunkID < num )
			{
				m_pSkinningInfo->m_arrControllersTCB[pCtrlChunk->chdr.ChunkID].m_controllertype=POSCHANNEL;
				m_pSkinningInfo->m_arrControllersTCB[pCtrlChunk->chdr.ChunkID].m_index=numControllers;
			}
			return 1;
		}

		if (pCtrlChunk->type==CTRL_TCBQ)
		{
			DynArray<CryTCBQKey> * pTrack = new DynArray<CryTCBQKey>;
			TCBFlags trackflags;

			CryTCBQKey *pKeys = (CryTCBQKey*)(pCtrlChunk+1);
			uint32 nkeys = pCtrlChunk->nKeys;
			SwapEndian(pKeys, nkeys, pChunkDesc->bSwapEndian);
			pTrack->resize(nkeys);

			for (uint32 i=0; i<nkeys; i++)
			{
				(*pTrack)[i] = pKeys[i];
			}

			if (pCtrlChunk->nFlags & CTRL_ORT_CYCLE)
				trackflags.f0=1;
			else if (pCtrlChunk->nFlags & CTRL_ORT_LOOP)
				trackflags.f1=1;;

			uint32 numControllers = m_pSkinningInfo->m_TrackQuat.size();
			m_pSkinningInfo->m_TrackQuatFlagsQQQ.push_back(trackflags);
			m_pSkinningInfo->m_TrackQuat.push_back(pTrack);
			
			int32 num=m_pSkinningInfo->m_arrControllersTCB.size();
			if( pCtrlChunk->chdr.ChunkID < num )
			{
				m_pSkinningInfo->m_arrControllersTCB[pCtrlChunk->chdr.ChunkID].m_controllertype=ROTCHANNEL;
				m_pSkinningInfo->m_arrControllersTCB[pCtrlChunk->chdr.ChunkID].m_index=numControllers;
			}
			return 1;
		}

		if (pCtrlChunk->type==CTRL_CRYBONE)
		{
			m_LastError.Format("animation not loaded: controller-type CTRL_CRYBONE not supported");
			return 1;
		}		

		if (pCtrlChunk->type==CTRL_BEZIER3)
		{
			m_LastError.Format("animation not loaded: controller-type CTRL_BEZIER3 not supported");
			return 1;
		}		

		m_LastError.Format("animation not loaded: found unsupported controller type in chunk");
		return 0;	
	}


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION)
	{

		if (m_bLoadOldChunks)
		{
			CONTROLLER_CHUNK_DESC_0827* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0827*)pChunkDesc->data;
			SwapEndian(*pCtrlChunk, pChunkDesc->bSwapEndian);

			uint32 numKeys = pCtrlChunk->numKeys;

			CryKeyPQLog* pCryKey = (CryKeyPQLog*)(pCtrlChunk+1);
			SwapEndian(pCryKey, numKeys, pChunkDesc->bSwapEndian);



			CControllerPQLog * pController = new CControllerPQLog;

			pController->m_nControllerId	= pCtrlChunk->nControllerId;


			pController->m_arrKeys.resize(numKeys);
			pController->m_arrTimes.resize(numKeys);

			for (uint32 i=0; i<numKeys; ++i)
			{
				pController->m_arrTimes[i]	= pCryKey[i].nTime/TICKS_CONVERT; 
				pController->m_arrKeys[i].vPos		= pCryKey[i].vPos/100.0f;
				pController->m_arrKeys[i].vRotLog	= pCryKey[i].vRotLog;
			}
			m_pSkinningInfo->m_arrControllerId.push_back( pCtrlChunk->nControllerId );
			m_pSkinningInfo->m_pControllers.push_back(pController);
		}

		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION)
	{
		if (m_bLoadOldChunks)
		{

			CONTROLLER_CHUNK_DESC_0828* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0828*)pChunkDesc->data;
			SwapEndian(*pCtrlChunk, pChunkDesc->bSwapEndian);

			//pSkinningInfo->m_nControllerId = pCtrlChunk->nControllerId;

			uint32 numKeys = pCtrlChunk->numKeys;

			CryKeyPQLog* pCryKey = (CryKeyPQLog*)(pCtrlChunk+1);
			SwapEndian(pCryKey, numKeys, pChunkDesc->bSwapEndian);


			CControllerPQLog * pController = new CControllerPQLog;

			pController->m_nControllerId	= pCtrlChunk->nControllerId;

			pController->m_arrKeys.resize(numKeys);
			pController->m_arrTimes.resize(numKeys);

			uint32 told=~0;
			for (uint32 i=0; i<numKeys; ++i)
			{
				assert(pCryKey[i].nTime-told<160);
				told=pCryKey[i].nTime;

				pController->m_arrTimes[i]	= pCryKey[i].nTime/TICKS_CONVERT;
				pController->m_arrKeys[i].vPos		= pCryKey[i].vPos/100.0f;
				pController->m_arrKeys[i].vRotLog	= pCryKey[i].vRotLog;
			}
			(void)told;

			m_pSkinningInfo->m_arrControllerId.push_back( pCtrlChunk->nControllerId );
			m_pSkinningInfo->m_pControllers.push_back(pController);
		}
		return 1;
	}

	/*
	enum {VERSION = 0x0829};

	enum { eKeyTimeRotation = 0, eKeyTimePosition = 1, eKeyTimeScale = 2 };
	CHUNK_HEADER	chdr;

	unsigned nControllerId;

	// Bitset with information of rotation info
	uint32 RotationFormat;
	uint32 numRotationKeys;
	// if we have rotation information we ALWAYS have a keytimes info
	uint32 PositionFormat;
	uint32 numPositionKeys;
	uint32 PositionKeysInfo;

	uint32 ScaleFormat;
	uint32 numScaleKeys;
	uint32 ScaleKeysInfo; 

	*/


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0829::VERSION)
	{

		if (m_bLoadOldChunks)
			return 1;

		CONTROLLER_CHUNK_DESC_0829* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0829*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk, pChunkDesc->bSwapEndian);


		CController * pController = new CController;

		pController->m_nControllerId = pCtrlChunk->nControllerId;

		m_pSkinningInfo->m_arrControllerId.push_back( pController->m_nControllerId );
		m_pSkinningInfo->m_pControllers.push_back(pController);


		char *pData = (char*)(pCtrlChunk+1);


		_smart_ptr<IKeyTimesInformation> pRotTimeKeys;
		_smart_ptr<IKeyTimesInformation> pPosTimeKeys;
		//		KeyTimesInformationPtr pSclTimeKeys;

		RotationControllerPtr pRotation;
		PositionControllerPtr pPosition;
		//		PositionControllerPtr pScale;

		if (pCtrlChunk->numRotationKeys)
		{
			// we have a rotation info
			pRotation =  RotationControllerPtr(new RotationTrackInformation);

			ITrackRotationStorage* pStorage = ControllerHelper::GetRotationControllerPtr(pCtrlChunk->RotationFormat);
			if (!pStorage)
				return false;

			pRotation->SetRotationStorage(pStorage);  

			pData = (char*)pStorage->CopyValues(pData, pCtrlChunk->numRotationKeys, pChunkDesc->bSwapEndian);

			pRotTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->RotationTimeFormat);//new F32KeyTimesInformation;//CKeyTimesInformation;
			if (!pRotTimeKeys)
				return false;

			pData = (char*)pRotTimeKeys->CopyValues(pData, pCtrlChunk->numRotationKeys, pChunkDesc->bSwapEndian);

			pRotation->SetKeyTimesInformation(pRotTimeKeys);

			pController->SetRotationController(pRotation);
		}

		if (pCtrlChunk->numPositionKeys)
		{
			pPosition = PositionControllerPtr(new PositionTrackInformation);

			TrackPositionStoragePtr pStorage = ControllerHelper::GetPositionControllerPtr(pCtrlChunk->PositionFormat);
			if (!pStorage)
				return false;

			pPosition->SetPositionStorage(pStorage);

			pData = (char*)pStorage->CopyValues(pData, pCtrlChunk->numPositionKeys, pChunkDesc->bSwapEndian);

			if (pCtrlChunk->PositionKeysInfo == CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation)
			{
				pPosition->SetKeyTimesInformation(pRotTimeKeys);
			}
			else
			{
				// load from chunk
				pPosTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->PositionTimeFormat);;//PositionTimeFormat;new F32KeyTimesInformation;//CKeyTimesInformation;
				pData = (char*)pPosTimeKeys->CopyValues(pData, pCtrlChunk->numPositionKeys, pChunkDesc->bSwapEndian);

				pPosition->SetKeyTimesInformation(pPosTimeKeys);
			}

			pController->SetPositionController(pPosition);
		}


		/*
		if (pCtrlChunk->numScaleKeys)
		{
		pScale = ControllerHelper::GetPositionControllerPtr(pCtrlChunk->ScaleFormat);

		pScale->Resize(pCtrlChunk->numScaleKeys);

		cryMemcpy(pScale->GetData(), pData, pScale->GetDataRawSize());
		pData += pScale->GetDataRawSize();

		if (pCtrlChunk->ScaleKeysInfo == CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation)
		{
		pScale->SetKeyTimesInformation(pRotTimeKeys);
		}
		else
		if(pCtrlChunk->ScaleKeysInfo == CONTROLLER_CHUNK_DESC_0829::eKeyTimePosition)
		{
		pScale->SetKeyTimesInformation(pPosTimeKeys);
		}
		else
		{
		pSclTimeKeys = new F32KeyTimesInformation;//CKeyTimesInformation;
		pSclTimeKeys->ResizeKeyTime(pCtrlChunk->numScaleKeys);

		cryMemcpy(pSclTimeKeys->GetData(), pData, pSclTimeKeys->GetDataRawSize());
		pData += pSclTimeKeys->GetDataRawSize();

		pScale->SetKeyTimesInformation(pSclTimeKeys);


		}

		pController->SetScaleController(pScale);

		}
		*/

		return 1;
	}


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0831::VERSION)
	{
		return 1;
	}

	//if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0900::VERSION)
	//{
	//	// load tracks database
	//}
	return 0;
}


