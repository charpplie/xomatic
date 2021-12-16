//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimationManager.cpp
//  Implementation of GlobalAnimationHeaderCAF.cpp
//
//	History:
//	Mai 25, 2010: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"


//////////////////////////////////////////////////////////////////////////
uint32 GlobalAnimationHeaderCAF::LoadCAF( uint32 nRootCRC32 )
{
	LOADING_TIME_PROFILE_SECTION(GetISystem());
	const char* pname = m_FilePath.c_str();
	if (pname[0]!='a')
	{
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,GetFilePath(),	"Animation-asset not in Animation-Folder" );
#if (defined(XENON) || defined(PS3))
	//	return 0; //we can't load un-processed assets on consoles
#endif
	}

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "CAF Animation %s",pname);

	m_nFlags = 0;
	OnAssetNotFound();

	_smart_ptr<IChunkFile> pChunkFile = g_pI3DEngine->CreateChunkFile(true);
	if ( !pChunkFile->Read(m_FilePath) )
	{
		if (Console::GetInst().ca_AnimWarningLevel > 0)
			g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,GetFilePath(),"CAF-File Not Found: %s",GetFilePath() );
		return 0;
	}

	bool bLoadOldChunks = false;
	ParseChunkHeaders(pChunkFile, bLoadOldChunks);
	uint32 numChunks = pChunkFile->NumChunks();
	bool result = ParseChunkRange(pChunkFile, 0, numChunks, bLoadOldChunks);

	if (result==0)
		return 0;

	//---> file loaded successfully
	m_FilePathDBACRC32	=	0; //if 0, then this is a streamable CAF file
	ControllerInit( nRootCRC32 );
#ifndef _RELEASE	
	if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
		g_pCharacterManager->GetAnimationManager().DebugAnimUsage(0);
#endif
	return result;
}


void GlobalAnimationHeaderCAF::LoadControllersCAF(uint32 nRootCRC32)
{
	uint32 OnDemand=IsAssetOnDemand();
	if (OnDemand)
		LoadCAF(nRootCRC32);
	else
		LoadDBA();
}

//----------------------------------------------------------------------------------------

void GlobalAnimationHeaderCAF::LoadDBA()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Animation DBA");

	if (m_nControllers==0 && m_FilePathDBACRC32)
	{
		size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
		for (uint32 d=0; d<numDBA_Files; d++)
		{
			CGlobalHeaderDBA& pGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[d];
			if (m_FilePathDBACRC32!=pGlobalHeaderDBA.m_FilePathDBACRC32)
				continue;
			pGlobalHeaderDBA.m_nTCount=0;
			if (pGlobalHeaderDBA.m_pDatabaseInfo==0)
				pGlobalHeaderDBA.LoadDatabaseDBA( "" );

			break;
		}
	}
#ifndef _RELEASE	
	if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
		g_pCharacterManager->GetAnimationManager().DebugAnimUsage(0);
#endif
}






//////////////////////////////////////////////////////////////////////////
void GlobalAnimationHeaderCAF::StartStreamingCAF()
{
	//LoadCAF( 0 );
	//return;

	if ( IsAssetCreated()==0 )
	{
		if (Console::GetInst().ca_UseIMG_CAF)
		{
			//data-mismatch between IMG file and Animation-PAK. Most likely an issue with the build-process 
#ifndef _RELEASE 
			uint32 num=g_DataMismatch.size();
			for (uint32 i=0; i<num; i++)
			{
				if (g_DataMismatch[i]==GetFilePath())
					return;
			}
			g_DataMismatch.push_back(GetFilePath());
			g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,GetFilePath(),	"GlobalAnimationHeaderCAF is not created. This is a data-corruption issue!  m_nControllers2: %08x  m_FilePathDBACRC32: %08x", m_nControllers2,m_FilePathDBACRC32 );
#endif
			return;
		}
		else
		{
			m_nControllers=0;
			m_nControllers2=0;
			return;
		}
	}

	uint32 IsLoaded    = IsAssetLoaded();
	if (IsLoaded)
		return;
	uint32 IsRequested = IsAssetRequested();
	if (IsRequested)
		return;

	if (IsAssetOnDemand())
	{
		OnAssetRequested();
		// start streaming
		StreamReadParams params;



		params.dwUserData = eLoadFullData;

		params.nSize = 0;
		params.pBuffer = NULL;
		params.nLoadTime = 10000;
		params.nMaxLoadTime = 1000;
		g_pISystem->GetStreamEngine()->StartRead(eStreamTaskTypeAnimation, GetFilePath(), this, &params);
	} 
	else
	{
		size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
		for (uint32 d=0; d<numDBA_Files; d++)
		{
			CGlobalHeaderDBA& pGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[d];
			if (m_FilePathDBACRC32 != pGlobalHeaderDBA.m_FilePathDBACRC32)
				continue;
			pGlobalHeaderDBA.m_nTCount=0;
			if (pGlobalHeaderDBA.m_pDatabaseInfo==0)
				pGlobalHeaderDBA.StartStreamingDBA(false);
			else 
				m_nControllers  = m_nControllers2;

			break;
		}
	}
}


void GlobalAnimationHeaderCAF::StreamAsyncOnComplete(IReadStream* pStream, unsigned nError)
{
	if (pStream->IsError())
	{
		return;
	}

	_smart_ptr<IChunkFile> pChunkFile = g_pI3DEngine->CreateChunkFile(true);
	if ( !pChunkFile->ReadFromMemBlock(pStream->GetBuffer(), pStream->GetBytesRead()) )
	{
		pStream->FreeTemporaryMemory();
		return;
	}

	bool bLoadOldChunks = false;
	ParseChunkHeaders(pChunkFile, bLoadOldChunks);
	uint32 numChunks = pChunkFile->NumChunks();
	bool result = ParseChunkRange(pChunkFile, 0, numChunks, bLoadOldChunks);

	if (!result)
	{
		pStream->FreeTemporaryMemory();
		return;
	}
}

void GlobalAnimationHeaderCAF::StreamOnComplete(IReadStream* pStream, unsigned nError)
{
	DEFINE_PROFILER_FUNCTION();
	LOADING_TIME_PROFILE_SECTION(g_pISystem);

	if(pStream->IsError())
	{ 
		/*
		ERROR_UNKNOWN_ERROR = 0xF0000000,
		ERROR_UNEXPECTED_DESTRUCTION = 0xF0000001,
		ERROR_INVALID_CALL = 0xF0000002,
		ERROR_CANT_OPEN_FILE = 0xF0000003,
		ERROR_REFSTREAM_ERROR = 0xF0000004,
		ERROR_OFFSET_OUT_OF_RANGE = 0xF0000005,
		ERROR_REGION_OUT_OF_RANGE = 0xF0000006,
		ERROR_SIZE_OUT_OF_RANGE = 0xF0000007,
		ERROR_CANT_START_READING = 0xF0000008,
		ERROR_OUT_OF_MEMORY = 0xF0000009,
		ERROR_ABORTED_ON_SHUTDOWN = 0xF000000A,
		ERROR_OUT_OF_MEMORY_QUOTA = 0xF000000B,
		ERROR_ZIP_CACHE_FAILURE = 0xF000000C,
		ERROR_USER_ABORT = 0xF000000D
		*/

		// file was not loaded successfully
		FinishLoading(false);
		return;
	}


	//---> file loaded successfully
	m_FilePathDBACRC32	=	0; //if 0, then this is a streamable CAF file
	FinishLoading(true);
}

void GlobalAnimationHeaderCAF::FinishLoading(bool success)
{
	if (success==0)
	{
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE, GetFilePath(),	"Failed to parse CAF-file" );
		m_nFlags = 0;
		OnAssetNotFound();
		return;  //error
	}

	//----------------------------------------------------------------------------------------------

	ControllerInit( 0 );
	if (g_pCharacterManager->m_pStreamingListener)
	{
		int32 globalID = g_AnimationManager.GetGlobalIDbyFilePath_CAF(GetFilePath());
		g_pCharacterManager->m_pStreamingListener->NotifyAnimLoaded(globalID);
	}
#ifndef _RELEASE	
	if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
		g_pCharacterManager->GetAnimationManager().DebugAnimUsage(0);
#endif
}


void GlobalAnimationHeaderCAF::ClearControllers() 
{
	if (m_FilePathDBACRC32==0 || m_FilePathDBACRC32==-1) 
	{
		ClearAssetRequested();
		m_nControllers = 0;
		m_arrController.clear();
		m_arrControllerLookupVector.clear();
#ifndef _RELEASE	
		if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
			g_pCharacterManager->GetAnimationManager().DebugAnimUsage(0);
#endif
	}
}

bool GlobalAnimationHeaderCAF::ParseChunkRange(IChunkFile* pChunkFile, uint32 min, uint32 max, bool bLoadOldChunks)
{
	assert((int)max <= pChunkFile->NumChunks());
	
	//first we initialize the controllers
	for (uint32 i=min; i<max; i++)
	{
		const CHUNK_HEADER &hdr = pChunkFile->GetChunkHeader(i);
		switch (hdr.ChunkType)
		{
		case ChunkType_MotionParameters:
			if (!ReadMotionParameters( pChunkFile->GetChunk(i) ) )
				return false;
			break;

		case ChunkType_Timing:
			if (!ReadTiming( pChunkFile->GetChunk(i)) )
				return false;
			break;

		case ChunkType_Controller:
			if (!ReadController( pChunkFile->GetChunk(i),bLoadOldChunks) )
				return false;
			break;
		}
	}
	return true;
}


uint32 GlobalAnimationHeaderCAF::DoesExistCAF()
{
	LOADING_TIME_PROFILE_SECTION(GetISystem());
	_smart_ptr<IChunkFile> pChunkFile = g_pI3DEngine->CreateChunkFile(true);
	if ( !pChunkFile->Read(m_FilePath) )
		return 0;

	return 1;
}


void GlobalAnimationHeaderCAF::ControllerInit( uint32 nRootCRC32 )
{
	uint32 numController = m_arrController.size();
	assert(numController);
	std::sort(m_arrController.begin(),	m_arrController.end(), AnimCtrlSortPred()	);
	InitControllerLookup(numController);
	m_nControllers  = numController;
	m_nControllers2 = numController;
	if (m_nControllers2==0)
		CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "CryAnimation CAF: Assets has no controllers. Probably compressed to death: %s",GetFilePath());

	ClearAssetRequested();
	ClearAssetNotFound();
	OnAssetCreated();
	if (m_EndLocation.q.w==-1.0f)
	{
		//backward compatibility: initialize values that are not stored in the chunk
		IController* pController = GetControllerByJointCRC32(nRootCRC32);
		if (pController)
		{
			QuatT fkey;	pController->GetOP( NTime2KTime(0.0f), fkey.q, fkey.t);
			QuatT mkey;	pController->GetOP( NTime2KTime(0.5f), mkey.q, mkey.t);
			QuatT lkey;	pController->GetOP( NTime2KTime(1.0f), lkey.q, lkey.t);

			m_EndLocation = m_StartLocation*lkey;

			const Vec3 startDir		= fkey.q.GetColumn1();
			const Vec3 middleDir	= mkey.q.GetColumn1();
			const Vec3 endDir			= lkey.q.GetColumn1();
			const f32 leftAngle		= Ang3::CreateRadZ(startDir, middleDir);
			const f32 rightAngle	= Ang3::CreateRadZ(middleDir, endDir);
			f32 angle		 = leftAngle + rightAngle;
			m_fAssetTurn = angle;
			m_fTurnSpeed = angle;
			if (m_fTotalDuration)
				m_fTurnSpeed = angle / m_fTotalDuration;
		}
	}
}

uint32 GlobalAnimationHeaderCAF::ParseChunkHeaders(IChunkFile* pChunkFile, bool& bLoadOldChunksOut)
{
	//Load mesh from chunk file.
	if ((pChunkFile->GetFileHeader().Version != ChunkFileVersion) && (pChunkFile->GetFileHeader().Version != ChunkFileVersion_Align))
	{
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE, GetFilePath(),	"Bad CAF file version" );
		return 0;
	}

	if (pChunkFile->GetFileHeader().FileType != FileType_Geom) 
	{
		if (pChunkFile->GetFileHeader().FileType != FileType_Anim) 
		{
			g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE, GetFilePath(),	"Illegal File Type for .caf file" );
			return 0;
		}
	}

	// Load Nodes.	
	uint32 numChunck = pChunkFile->NumChunks();
	uint32 numOldControllers=0;
	uint32 numNewControllers=0;
	uint32 bLoadOldChunks = 0;

	for (uint32 i=0; i<numChunck; i++)
	{
		const CHUNK_HEADER &hdr = pChunkFile->GetChunkHeader(i);

		if (hdr.ChunkType == ChunkType_Controller)
		{
			if (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
			{
				CryFatalError("TCB chunks" );
				continue;
			}
			if (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION)
			{
				numOldControllers++;
				continue;
			}
			if (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION)
			{
				numOldControllers++;
				continue;
			}


			if (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0829::VERSION)
			{
				numNewControllers++;
				continue;
			}
			if (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0830::VERSION)
			{
				CryFatalError("CONTROLLER_CHUNK_DESC_0830" );
				numNewControllers++;
				continue;
			}
			if (hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0831::VERSION)
			{
				CryFatalError("CONTROLLER_CHUNK_DESC_0831" );
				numNewControllers++;
				continue;
			}
			CryFatalError("weird controller" );
		}
	}

	if (numNewControllers==0)
		bLoadOldChunks = true;
	if (numOldControllers)
		//CryFatalError("Old Chunks" );
	
		if (Console::GetInst().ca_AnimWarningLevel > 2 && numOldControllers)
		{
			if ( Console::GetInst().ca_UseIMG_CAF )
				g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,GetFilePath(),	"Animation file has uncompressed data" );
		}
		if (Console::GetInst().ca_AnimWarningLevel > 2 && numNewControllers==0)
		{
			if ( Console::GetInst().ca_UseIMG_CAF )
				g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,GetFilePath(),	"Animation file has no compressed data" );
		}


	// Only old data will be loaded. 
	uint32 numController = 0;
	if (bLoadOldChunks)
		numController = numOldControllers;
	else
		numController = numNewControllers;

	m_arrControllerLookupVector.resize(0);
	m_arrController.reserve(numController);
	m_arrController.resize(0);


	bLoadOldChunksOut = bLoadOldChunks != 0;

	return numController;
}




//========================================
//SpeedInfo Header
//========================================
struct SPEED_CHUNK_DESC
{
	enum {VERSION = 0x0920};
	CHUNK_HEADER chdr;
	float			Speed;
	float			Distance;
	float			Slope;
	int       Looped;

	AUTO_STRUCT_INFO
};

struct SPEED_CHUNK_DESC_1
{
	enum {VERSION = 0x0921};
	CHUNK_HEADER chdr;
	float			Speed;
	float			Distance;
	float			Slope;
	int       Looped;
	f32				MoveDir[3];
	AUTO_STRUCT_INFO
};

//////////////////////////////////////////////////////////////////////////
bool GlobalAnimationHeaderCAF::ReadTiming (  IChunkFile::ChunkDesc *pChunkDesc  )
{
	TIMING_CHUNK_DESC* pChunk = (TIMING_CHUNK_DESC*)pChunkDesc->data;
	SwapEndian(*pChunk, pChunkDesc->bSwapEndian);
	int32 nStartKey		= pChunk->global_range.start;
	int32 nEndKey			= pChunk->global_range.end;
	int32 fTicksPerFrame  = TICKS_PER_FRAME;
	f32		fSecsPerTick		= SECONDS_PER_TICK;
	f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
	m_fStartSec = nStartKey * fSecsPerFrame;
	m_fEndSec   = nEndKey   * fSecsPerFrame;
	if(m_fEndSec<=m_fStartSec)
		m_fEndSec  = m_fStartSec; //+(1.0f/30.0f);
	m_fTotalDuration = m_fEndSec - m_fStartSec;

	return 1;
}


//////////////////////////////////////////////////////////////////////////
bool GlobalAnimationHeaderCAF::ReadMotionParameters (  IChunkFile::ChunkDesc *pChunkDesc )
{
	if (pChunkDesc->hdr.ChunkVersion == SPEED_CHUNK_DESC::VERSION)
	{
		CryFatalError("SPEED_CHUNK_DESC not supported any more");
		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == SPEED_CHUNK_DESC_1::VERSION)
	{
		CryFatalError("SPEED_CHUNK_DESC_1 not supported any more");
		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == SPEED_CHUNK_DESC_2::VERSION)
	{
		//	CryFatalError("SPEED_CHUNK_DESC_2 not supported any more");
		SPEED_CHUNK_DESC_2* pChunk = (SPEED_CHUNK_DESC_2*)pChunkDesc->data;
		SwapEndian(*pChunk, pChunkDesc->bSwapEndian);
		m_nFlags = FlagsSanityFilter(pChunk->AnimFlags) | (m_nFlags & (CA_ASSET_REQUESTED));
		m_FilePathDBACRC32	=	0; //if 0, then this is a streamable CAF file
		m_fMoveSpeed			= pChunk->Speed;
		m_fDistance		= pChunk->Distance;
		m_fSlope			= -pChunk->Slope;

		m_StartLocation = pChunk->StartPosition;
		return 1;
	}


	if (pChunkDesc->hdr.ChunkVersion == CHUNK_MOTION_PARAMETERS::VERSION)
	{
		CHUNK_MOTION_PARAMETERS* pChunk = (CHUNK_MOTION_PARAMETERS*)pChunkDesc->data;
#if (defined(XENON) || defined(PS3) || defined(CAFE))
		if ((pChunk->mp.m_nAssetFlags&CA_ASSET_BIG_ENDIAN)==0)
			CryFatalError("Data Error in Console Build: CAF file must have Big Endian format");
#endif
#if !(defined(XENON) || defined(PS3) || defined(CAFE))
		if ((pChunk->mp.m_nAssetFlags&CA_ASSET_BIG_ENDIAN))
			CryFatalError("Data Error in Win32 Build: CAF file must have Little Endian format");
#endif

		m_nFlags			= FlagsSanityFilter(pChunk->mp.m_nAssetFlags) | (m_nFlags & (CA_ASSET_REQUESTED));

		// this had to be done for DBAs as well, apparently some 
		// assets dont have the CREATED flag set...
		OnAssetCreated();

		m_FilePathDBACRC32	=	0; //if 0, then this is a streamable CAF file
		//m_nCompression		= pChunk->mp.m_nCompression;
		//m_nTicksPerFrame	= pChunk->mp.m_nTicksPerFrame;
		//m_secsPerTick		= pChunk->mp.m_fSecsPerTick;
		int32 nStartKey		= pChunk->mp.m_nStart;
		int32 nEndKey			= pChunk->mp.m_nEnd;
		if (pChunk->mp.m_nAssetFlags & CA_ASSET_ADDITIVE)
			nStartKey++;

		int32 fTicksPerFrame  = TICKS_PER_FRAME;
		f32		fSecsPerTick		= SECONDS_PER_TICK;
		f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
		m_fStartSec = nStartKey * fSecsPerFrame;
		m_fEndSec   = nEndKey   * fSecsPerFrame;
		if(m_fEndSec<=m_fStartSec)
			m_fEndSec  = m_fStartSec; //+(1.0f/30.0f);
		m_fTotalDuration = m_fEndSec - m_fStartSec;

		m_fMoveSpeed				=	 pChunk->mp.m_fMoveSpeed;
		m_fTurnSpeed		= pChunk->mp.m_fTurnSpeed; //radians per second
		m_fAssetTurn		= pChunk->mp.m_fAssetTurn; //total turn-angle in radians
		m_fDistance			= pChunk->mp.m_fDistance;
		m_fSlope				= -pChunk->mp.m_fSlope;

		m_StartLocation = pChunk->mp.m_StartLocation;
		m_EndLocation   = pChunk->mp.m_EndLocation;

		/*
		m_LHeelStart = pChunk->mp.m_LHeelStart;
		m_LHeelEnd		= pChunk->mp.m_LHeelEnd;
		m_LToe0Start = pChunk->mp.m_LToe0Start;
		m_LToe0End		= pChunk->mp.m_LToe0End;
		m_RHeelStart = pChunk->mp.m_RHeelStart;
		m_RHeelEnd		= pChunk->mp.m_RHeelEnd;
		m_RToe0Start = pChunk->mp.m_RToe0Start;
		m_RToe0End		= pChunk->mp.m_RToe0End;
		*/
		return 1;
	}


	return 1;
}



//////////////////////////////////////////////////////////////////////////
bool GlobalAnimationHeaderCAF::ReadController (  IChunkFile::ChunkDesc *pChunkDesc, uint32 bLoadOldChunks  )
{
	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
	{
		CryFatalError("CONTROLLER_CHUNK_DESC_0826" );
		return 0;
	}


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION)
	{
		if (bLoadOldChunks)
		{
		//	CryFatalError("CONTROLLER_CHUNK_DESC_0827" );
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
			m_arrController.push_back(pController);
		}

		return 1;
	}

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION)
	{
		if (bLoadOldChunks)
		{
			//CryFatalError("CONTROLLER_CHUNK_DESC_0828" );

			CONTROLLER_CHUNK_DESC_0828* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0828*)pChunkDesc->data;
			SwapEndian(*pCtrlChunk, pChunkDesc->bSwapEndian);


			uint32 numKeys = pCtrlChunk->numKeys;

			CryKeyPQLog* pCryKey = (CryKeyPQLog*)(pCtrlChunk+1);
			SwapEndian(pCryKey, numKeys, pChunkDesc->bSwapEndian);

			CControllerPQLog* pController = new CControllerPQLog;

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

			m_arrController.push_back(pController);
		}
		return 1;
	}


//--------------------------------------------------------------

	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0829::VERSION)
	{
		if (bLoadOldChunks)
			return 1;

		CONTROLLER_CHUNK_DESC_0829* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0829*)pChunkDesc->data;
		SwapEndian(*pCtrlChunk, pChunkDesc->bSwapEndian);

		CController * pController = new CController;
		pController->m_nControllerId = pCtrlChunk->nControllerId;
		m_arrController.push_back(pController);

		char *pData = (char*)(pCtrlChunk+1);

		_smart_ptr<IKeyTimesInformation> pRotTimeKeys;
		_smart_ptr<IKeyTimesInformation> pPosTimeKeys;

		RotationControllerPtr pRotation;
		PositionControllerPtr pPosition;

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
				pPosTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->PositionTimeFormat);
				pData = (char*)pPosTimeKeys->CopyValues(pData, pCtrlChunk->numPositionKeys, pChunkDesc->bSwapEndian);

				pPosition->SetKeyTimesInformation(pPosTimeKeys);
			}

			pController->SetPositionController(pPosition);
		}

		return 1;
	}


	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0831::VERSION)
	{
		CryFatalError("CONTROLLER_CHUNK_DESC_0831" );
		return 1;
	}

	return 0;
}

uint32 GlobalAnimationHeaderCAF::FlagsSanityFilter( uint32 flags )
{
	// sanity check
	uint32 validFlags =  (CA_ASSET_BIG_ENDIAN|CA_ASSET_CREATED|CA_ASSET_ONDEMAND|CA_ASSET_LOADED|CA_ASSET_ADDITIVE|CA_ASSET_CYCLE|CA_ASSET_NOT_FOUND);
	if (flags & ~validFlags)
	{
		CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Badly exported animation-asset: flags: %08x %s",flags,this->GetFilePath());
		flags &= validFlags;
	}
	return flags;
}

uint32 GlobalAnimationHeaderCAF::GetTotalPosKeys() const
{
	uint32 pos=0;
	for (uint32 i=0; i<m_nControllers; i++)
		pos += (m_arrController[i]->GetPositionKeysNum()!=0);
	return pos;
}

uint32 GlobalAnimationHeaderCAF::GetTotalRotKeys() const
{
	uint32 rot = 0;
	for (uint32 i=0; i<m_nControllers; i++)
		rot += (m_arrController[i]->GetRotationKeysNum()!=0);
	return rot;
}


bool GlobalAnimationHeaderCAF::Export2HTR(const char* szAnimationName, const char* savePath, const CCharacterModel* pModel)
{
	std::vector<string> jointNameArray;
	std::vector<string> jointParentArray;


	const CModelSkeleton* pModelSkeleton = &pModel->m_ModelSkeleton;
	const QuatT* parrDefJoints = &pModelSkeleton->m_poseData.m_pJointsRelative[0];
	uint32 numJoints = pModelSkeleton->m_arrModelJoints.size();
	for(uint32 j=0; j<numJoints; j++)
	{
		const CModelJoint* pJoint	= &pModelSkeleton->m_arrModelJoints[j];
		assert(pJoint);
		jointNameArray.push_back( pJoint->GetJointName() );

		int16 parentID = pJoint->m_idxParent;
		if(parentID == -1)
			jointParentArray.push_back("INVALID");
		else
		{
			const CModelJoint* parentJoint	= &pModelSkeleton->m_arrModelJoints[parentID];
			if(parentJoint)
				jointParentArray.push_back(parentJoint->GetJointName());
		}		
	}


	//fetch animation
	std::vector< std::vector<QuatT> > arrAnimation;
	f32 fFrames = m_fTotalDuration/SECONDS_PER_TICK;
	uint32 nFrames = uint32(fFrames+1.5f); //roundup
	f32 timestep = 1.0f/f32(nFrames-1);

	arrAnimation.resize(numJoints);
	for(uint32 j=0; j<numJoints; j++)
		arrAnimation[j].resize(nFrames);

	for(uint32 j=0; j<numJoints; j++)
	{
		const CModelJoint* pJoint	= &pModelSkeleton->m_arrModelJoints[j];
		assert(pJoint);
		IController* pController = GetControllerByJointCRC32(pJoint->m_nJointCRC32);
		f32 t=0.0f;	
		for (uint32 k=0; k<nFrames; k++)
		{
			QuatT qt=parrDefJoints[j];
			if(pController)
				pController->GetOP( NTime2KTime(t), qt.q,qt.t);
			arrAnimation[j][k]=qt;
			t += timestep;
		}
	}

	bool htr = SaveHTR(szAnimationName, savePath, jointNameArray, jointParentArray, arrAnimation, parrDefJoints);
	bool caf = SaveCAF(szAnimationName, savePath, jointNameArray, arrAnimation); 
	return htr;
}



// replace ' ' with '_'
void ChangeBoneName(const char* name, char newName[])
{
	int lenName = strlen(name);
	memcpy(newName, name, lenName*sizeof(char));
	for(int i=0; i< lenName; ++i)
	{
		if( newName[i] == ' ')
			newName[i] = '_';
	}
	newName[lenName] = '\0';
}

bool GlobalAnimationHeaderCAF::SaveHTR(const char* szAnimationName, const char* savePath, const std::vector<string>& jointNameArray, const std::vector<string>& jointParentArray, const std::vector< std::vector<QuatT> >& arrAnimation, const QuatT* parrDefJoints)
{
	if(jointNameArray.empty() || arrAnimation.empty())
		return false;

	/*CFileDialog dlg(FALSE, "htr", NULL, OFN_OVERWRITEPROMPT, "HTR (*.htr)|*.htr|CAF(*.caf)|*.caf||");

	if(dlg.DoModal() != IDOK)
		return;

	string path = dlg.GetPathName().GetBuffer();*/

	ICryPak* g_pIPak = gEnv->pCryPak;

	string saveName = string(savePath) + szAnimationName + string(".htr");
	FILE* fExport =  g_pIPak->FOpen(saveName.c_str(), "wt");
	if (fExport==0)
	{
		return false;
	}

	uint32 numFrames = arrAnimation[0].size();
	uint32 numBipedJoints = jointNameArray.size();

	//------------------------------------------------------------------------------
	// Export header

	g_pIPak->FPrintf(fExport,"#Comment line ignore any data following # character \n");
	g_pIPak->FPrintf(fExport,"#Hierarchical Translation and Rotation (.htr) file \n");
	g_pIPak->FPrintf(fExport,"[Header]	\n");	
	g_pIPak->FPrintf(fExport,"#Header keywords are followed by a single value \n");
	g_pIPak->FPrintf(fExport,"FileType HTR		#Single word string\n");
	g_pIPak->FPrintf(fExport,"DataType HTRS		#Translation followed by rotation and scale data\n");
	g_pIPak->FPrintf(fExport,"FileVersion 1		#integer\n");

	g_pIPak->FPrintf(fExport,"NumSegments %d		#integer\n",numBipedJoints);

	g_pIPak->FPrintf(fExport,"NumFrames %d		#integer\n",numFrames);

	int32 nFrameRate= 30; // ivo are you reading framrate from Vicon? //Xiaomao: make sure if we need to fix it.
	g_pIPak->FPrintf(fExport,"DataFrameRate %d		#integer, data frame rate in this file\n",nFrameRate);
	g_pIPak->FPrintf(fExport,"EulerRotationOrder ZYX\n");

	// ivo i cuoldnt find how to specify data in meters in HTR format (since you already divided by 1000).
	// So i will leave mm (millimiters) here and specify 1000 for scale. Result should be the same.
	g_pIPak->FPrintf(fExport,"CalibrationUnits mm\n");
	g_pIPak->FPrintf(fExport,"RotationUnits Degrees\n");
	g_pIPak->FPrintf(fExport,"GlobalAxisofGravity Y\n");
	g_pIPak->FPrintf(fExport,"BoneLengthAxis Y\n");
	g_pIPak->FPrintf(fExport,"ScaleFactor 1.00\n"); // ivo see previous note
	g_pIPak->FPrintf(fExport,"[SegmentNames&Hierarchy]\n");
	g_pIPak->FPrintf(fExport,"#CHILD	PARENT\n");

	// export the hierarchy 
	char newName[128], newNameParent[128];
	for(uint32 v=0; v<numBipedJoints; v++)
	{
		const char* pBonename = jointNameArray[v];

		ChangeBoneName(pBonename, newName); //replace " " with "_"
		const char* pParentBonename = jointParentArray[v];
		if ( string(jointParentArray[v]) != string("INVALID") )
		{
			ChangeBoneName(jointParentArray[v], newNameParent);
		}
	
		bool bParentValid = (string(pParentBonename) != string("INVALID") );
		string parentName = "GLOBAL";
		if(bParentValid)
			parentName = newNameParent;
		g_pIPak->FPrintf(fExport,"%s %s\n",newName,parentName.c_str());

	} //v


	g_pIPak->FPrintf(fExport,"[BasePosition]\n");
	g_pIPak->FPrintf(fExport,"#SegmentName Tx, Ty, Tz, Rx, Ry, Rz, BoneLength\n");

	//------------------------------------------------------------------------------
	// Export base pose

	//ivo Each line in this section indicates how each bone is initially orientated within
	//it’s own local coordinate system.
	for(uint32 v=0; v<numBipedJoints; v++)
	{
		const char* pBonename = jointNameArray[v];

		ChangeBoneName(pBonename, newName);

		Quat q= parrDefJoints[v].q;
		Vec3 t= parrDefJoints[v].t;

		// ivo do you have bone lenght somewhere after conversion?
		float fBoneLen=0.0f;
		if (jointParentArray[v] != string("INVALID"))
		{
			fBoneLen = t.GetLength();
		}

		Ang3 Ang = RAD2DEG(Ang3::GetAnglesXYZ(q));						

		t.Set(0,0,0);
		Ang.Set(0,0,0);
		if (v==0)
			Ang.Set(-90,0,0);
		fBoneLen=10.0f;

		g_pIPak->FPrintf(fExport,"%s %f %f %f %f %f %f %f \n",newName,t.x,t.y,t.z,Ang.x,Ang.y,Ang.z,fBoneLen);

	} //v

	//------------------------------------------------------------------------------
	// Export motion data joint by joint

	g_pIPak->FPrintf(fExport,"#Beginning of Data. Separated by tabs\n");
	g_pIPak->FPrintf(fExport,"#Fr	Tx	Ty	Tz	Rx	Ry	Rz	SF\n");

	for(uint32 v=0; v<numBipedJoints; v++)
	{
		const char* pBonename = jointNameArray[v];

		ChangeBoneName(pBonename, newName);

		g_pIPak->FPrintf(fExport,"[%s]\n",newName);
		for (uint32 k=0; k<numFrames; k++ )
		{				
			//Ivo do you have bone length somewhere after conversion?
			Quat q = arrAnimation[v][k].q;
			Vec3 t = arrAnimation[v][k].t*1000.0f;
			float fBoneLen= t.GetLength();
			Ang3 Ang = RAD2DEG(Ang3::GetAnglesXYZ(q));
			g_pIPak->FPrintf(fExport,"%d %f %f %f %f %f %f %f\n",k+1,  t.x,t.y,t.z, Ang.x,Ang.y,Ang.z,  fBoneLen);
		}
	}
	g_pIPak->FClose(fExport);

	return 0;
}

//-------------------------------------------------------------------------------------------------

bool GlobalAnimationHeaderCAF::SaveCAF( const char* szAnimationName, const char* savePath, const std::vector<string>& arrJointNames, const std::vector< std::vector<QuatT> >& arrAnimation ) 
{
	ICryPak* g_pIPak = gEnv->pCryPak;

	string saveName = string(savePath) + szAnimationName + string(".caf");
	FILE* fExport =  g_pIPak->FOpen(saveName.c_str(), "w+b");
	if (fExport==0)
		return 0; //fail

	//------------------------------------
	//---  write the file-header       ---
	//------------------------------------
	FILE_HEADER fh;
	fh.Signature[0]='C';
	fh.Signature[1]='r';
	fh.Signature[2]='y';
	fh.Signature[3]='T';
	fh.Signature[4]='e';
	fh.Signature[5]='k';
	fh.Signature[6]=0;
	fh._Pad_[0]=0;
	fh.FileType=FileType_Anim;
	fh.Version=ChunkFileVersion;
	fh.ChunkTableOffset= sizeof(FILE_HEADER);	
	g_pIPak->FWrite( &fh, sizeof(uint8), sizeof(FILE_HEADER), fExport );


	//-------------------------------------
	//---  write the number of chunks   ---
	//-------------------------------------
	uint32 numBipedJoints = arrAnimation.size();
	g_pIPak->FWrite( &numBipedJoints, sizeof(uint8), sizeof(uint32), fExport );  


	//-------------------------------------
	//---  write the chunk headers      ---
	//-------------------------------------
	CHUNK_HEADER ch[0x101]; 
	ch[0].ChunkType			= ChunkType_Timing; //Xiaomao: enum-4 bytes
	ch[0].ChunkVersion	= 0x0918;
	ch[0].FileOffset		= numBipedJoints*sizeof(CHUNK_HEADER) + 0x28;//sizeof(FILE_HEADER);
	ch[0].ChunkID				= 0;

	uint32 nControllerOffset = ch[0].FileOffset+sizeof(TIMING_CHUNK_DESC_0918);
	size_t numKeys = arrAnimation[0].size();//m_frameCount / m_SamplingRatio;
	for (uint32 x=0; x<numBipedJoints; x++)
	{
		ch[1+x].ChunkType			= ChunkType_Controller;
		ch[1+x].ChunkVersion	= 0x0827;
		ch[1+x].FileOffset		= nControllerOffset;
		ch[1+x].ChunkID				=	x;
		nControllerOffset += sizeof(CONTROLLER_CHUNK_DESC_0827);
		nControllerOffset += uint32(numKeys*sizeof(CryKeyPQLog));
	}
	g_pIPak->FWrite( &ch[0], sizeof(uint8), (1+numBipedJoints)*sizeof(CHUNK_HEADER), fExport );  


	//-------------------------------------
	//---  write the timing             ---
	//-------------------------------------
	TIMING_CHUNK_DESC_0918 Timing;
	Timing.global_range.start	=	0;
	Timing.global_range.end		=	uint32(numKeys-1);
	g_pIPak->FWrite( &Timing, sizeof(uint8), sizeof(TIMING_CHUNK_DESC_0918), fExport );  


	//-------------------------------------
	//---  write the controller header  ---
	//-------------------------------------
	for(uint32 j=0; j<numBipedJoints; j++)
	{
		const char* pBonename = arrJointNames[j];

		CONTROLLER_CHUNK_DESC_0827 controller;
		controller.numKeys				=	uint32(numKeys);
		controller.nControllerId	=	g_pCrc32Gen->GetCRC32(pBonename); 
 		g_pIPak->FWrite( &controller, sizeof(uint8), sizeof(CONTROLLER_CHUNK_DESC_0827), fExport );  

		std::vector<CryKeyPQLog> arrPQLogs;	
		arrPQLogs.resize(numKeys);
		uint32 time=0;
		for (uint32 k=0; k<numKeys; k++ )
		{				
			Quat q = arrAnimation[j][k].q;
			Vec3 t = arrAnimation[j][k].t*100.0f;
			arrPQLogs[k].nTime	=time;
			arrPQLogs[k].vRotLog=Quat::log(!q);
			arrPQLogs[k].vPos		=t;
			time += 0xa0;
		}
		g_pIPak->FWrite( &arrPQLogs[0], sizeof(uint8), numKeys*sizeof(CryKeyPQLog), fExport );  
	}

	g_pIPak->FClose( fExport );
	return 0;
}


void GlobalAnimationHeaderCAF::ConnectCAFandDBA()
{
	if (m_FilePathDBACRC32==0)
		return; //its a normal CAF file
	if (m_nControllers)
		return; //this CAF belongs to a DBA and its already connected

	size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
	for (uint32 d=0; d<numDBA_Files; d++)
	{
		CGlobalHeaderDBA& pGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[d];
		if (pGlobalHeaderDBA.m_pDatabaseInfo==0)
			continue;
		if (m_FilePathDBACRC32!=pGlobalHeaderDBA.m_FilePathDBACRC32)
			continue;
		pGlobalHeaderDBA.m_nTCount=0; //DBA in use. Reset unload count-down
		break;
	}
}


//--------------------------------------------------------------------------------------

size_t GlobalAnimationHeaderCAF::SizeOfCAF(const bool bForceControllerCalcu) const
{
	size_t nSize = 0; //sizeof(*this)

	size_t nTemp00 = m_FilePath.capacity();		nSize += nTemp00;
	size_t nTemp07 = m_AnimEventsCAF.get_alloc_size();							nSize += nTemp07;
	uint32 numEvents = m_AnimEventsCAF.size();
	for (uint32 i=0; i<numEvents; i++)
	{
		nSize += m_AnimEventsCAF[i].m_strModelName.capacity();
		nSize += m_AnimEventsCAF[i].m_strEventName.capacity();
		nSize += m_AnimEventsCAF[i].m_strCustomParameter.capacity();
		nSize += m_AnimEventsCAF[i].m_strBoneName.capacity();
	}

	size_t nTemp08 = m_arrControllerLookupVector.get_alloc_size();	nSize += nTemp08;
	size_t nTemp09 = m_arrController.get_alloc_size();							nSize += nTemp09;

	if (m_FilePathDBACRC32 && m_FilePathDBACRC32!=-1)
	{
		//it's part of a DBA file
		bool InMem=g_AnimationManager.IsDatabaseInMemory(m_FilePathDBACRC32);
		if (InMem && bForceControllerCalcu)
		{
			for (uint16 i=0; i<m_nControllers; ++i)
				nSize += m_arrController[i]->ApproximateSizeOfThis();
		}
	} 
	else
	{
		//it's a CAF or an ANM file
		for (uint16 i=0; i<m_nControllers; ++i)
			nSize += m_arrController[i]->SizeOfController();
	}

	return nSize;
}



void GlobalAnimationHeaderCAF::GetMemoryUsage(ICrySizer *pSizer) const
{

#ifdef USE_SELECTION_PROPERTIES
	pSizer->AddObject( m_pSelectionProperties );
	pSizer->AddObject( g_Alloc_AnimSelectProps );
#endif

	pSizer->AddObject( m_FilePath );
	pSizer->AddObject( m_AnimEventsCAF );
	pSizer->AddObject( m_arrControllerLookupVector );		
	if( m_arrController.size() )
		pSizer->AddObject( m_arrController );

	for( int i = 0 ; i < m_nControllers ; ++i )
	{
		assert(m_arrController.size());
		pSizer->AddObject( m_arrController[i].get() );  //Bolti, is this REALLY ok????
	}
}
