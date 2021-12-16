////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SoundBufferXenon.cpp
//  Version:     v1.00
//  Created:     4/4/2005 by Tomas.
//  Compilers:   Visual Studio.NET
//  Description: Xenon Implementation of SoundBuffer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef SOUNDSYSTEM_USE_XENON_XAUDIO

#include ".\soundbufferxenon.h"
#include "SoundSystem.h"
#include "Sound.h"
#include <XAudDefs.h>
#include "XenonUtil/AtgAudio.h"
#include "XenonUtil/AtgApp.h"

//////////////////////////////////////////////////////////////////////////
CSoundBufferXenon::CSoundBufferXenon(const SSoundBufferProps &Props) : CSoundBuffer(Props)
{
}

//////////////////////////////////////////////////////////////////////////
CSoundBufferXenon::~CSoundBufferXenon()
{
	if (m_pReadStream != NULL)
	{
		m_pReadStream->Abort();	
		m_pReadStream = NULL;
	}
	UnloadData(sbUNLOAD_ALL_DATA);

	// Tell Sounds that their soundbuffers are invalid now
	for (TBufferLoadReqVecIt It=m_vecLoadReq.begin(); It!=m_vecLoadReq.end(); ++It) 
		(*It)->OnBufferDelete(); 
}

tAssetHandle CSoundBufferXenon::LoadAsCue(const char *AssetName)
{
  return NULL;
}

//////////////////////////////////////////////////////////////////////////
tAssetHandle CSoundBufferXenon::LoadAsSample(const char *AssetDataPtr, int nLength)
{
	ATG::WaveFileMemory MemWaveFile;
	ATG::WAVEFORMATEXTENSIBLE wfx;
	DWORD dwNewSize;
	void* pSoundData = NULL;

	// Initialize the WaveFile Class and read all the chunks
	MemWaveFile.Init(AssetDataPtr, nLength);
	MemWaveFile.GetFormat(&wfx);

	// Create new memory for the sound data
	MemWaveFile.GetDuration( &dwNewSize );
	pSoundData = m_pSoundAssetManager->GetMemoryChunk(m_Props, dwNewSize);
	
	// out of memory
	if( !pSoundData )
		return (NULL);

	// read the sound data from the file
	MemWaveFile.ReadSample(0, pSoundData, dwNewSize, &dwNewSize);

	// Pass some information to the SoundBuffer
	m_Info.nBaseFreq			= wfx.Format.nSamplesPerSec;
	m_Info.nBitsPerSample = wfx.Format.wBitsPerSample;
	m_Info.nChannels			= wfx.Format.nChannels;
	m_Info.nLengthInBytes	= dwNewSize;

	// Check for embedded loop points
	DWORD dwLoopStart  = 0;
	DWORD dwLoopLength = dwNewSize;
	MemWaveFile.GetLoopRegionBytes( &dwLoopStart, &dwLoopLength );
	// TODO enable to read the loop points from the file and store is somewhere

	if (dwNewSize > 0)
	{
		m_Info.nLengthInMs			= 1000*m_Info.nLengthInBytes/(m_Info.nBaseFreq*m_Info.nChannels*(m_Info.nBitsPerSample/8.0f));
		m_Info.nLengthInSamples = (int32) m_Info.nLengthInMs*m_Info.nBaseFreq/1000.0f;
	}

	return (pSoundData);
}


//////////////////////////////////////////////////////////////////////////
tAssetHandle CSoundBufferXenon::LoadAsStream(const char *AssetName, int nLength)
{
	m_BufferData = NULL;

	return (m_BufferData);
}


// loads a event sound
tAssetHandle CSoundBufferXenon::LoadAsEvent(const char *AssetName)
{
	m_BufferData = NULL;

	return (m_BufferData);
}

//////////////////////////////////////////////////////////////////////////
void CSoundBufferXenon::StreamOnComplete(IReadStream *pStream, unsigned nError)
{
	//GUARD_HEAP;
	FUNCTION_PROFILER( m_pSoundSystem->GetSystem(),PROFILE_SOUND );	

	// Don't need stream anymore.
	m_pReadStream = 0;

	if (nError)
	{
		m_Info.bLoadFailure = true;
		LoadFailed();
		return;
	}

	tAssetHandle pSample = NULL;
	//void* pWavefileData = new int8[ pStream->GetBytesRead() ]; // creating a temporary buffer to save the stream data
	//if( pWavefileData )
	{
		//memcpy(pWavefileData, pStream->GetBuffer(), pStream->GetBytesRead());
		//pSample = LoadAsSample((const char*)pWavefileData, pStream->GetBytesRead()); // this worked
		pSample = LoadAsSample((const char*)pStream->GetBuffer(), pStream->GetBytesRead()); // this worked
		//delete[] pWavefileData; // pWavefileDate can be removed again
	}

	if (!pSample)
	{
		m_pSoundSystem->Log(eSLT_Warning, "Warning: Cannot load sample sound %s\n", m_Props.sName.c_str());
		m_Info.bLoadFailure = true;
		LoadFailed();
		return;
	}		
	//set the sound source
	SetAssetHandle(pSample, btSAMPLE);	
	SoundBufferLoaded();

}

// Gets and Sets Parameter defined in the enumAssetParam list
//////////////////////////////////////////////////////////////////////////
bool CSoundBufferXenon::GetParam(enumAssetParamSemantics eSemantics, ptParam* pParam) const
{
	switch (eSemantics)
	{	
	case apASSETNAME:	
		{
			string sTemp;
			if (!(pParam->GetValue(sTemp))) return (false);
			//sTemp = m_Props.sName;
			pParam->SetValue(sTemp);
		}
		break;
	case apASSETTYPE:	
		{
			int32 nTemp;
			if (!(pParam->GetValue(nTemp))) return (false);
			nTemp = m_Props.eBufferType;
			pParam->SetValue(nTemp);
			break;
		}	
	case apASSETFREQUENCY:	
		{
			int32 nTemp;
			if (!(pParam->GetValue(nTemp))) return (false);
			nTemp = m_Info.nBaseFreq;
			pParam->SetValue(nTemp);
			break;
		}
	case apLENGTHINBYTES:	
		{
			int32 nTemp;
			if (!(pParam->GetValue(nTemp))) return (false);
			switch (m_Props.eBufferType)
			{
			case btSAMPLE:
				nTemp = m_Info.nLengthInBytes;
				pParam->SetValue(nTemp);
				break;
			case btSTREAM: // Stream does not support Byte Length
				nTemp = m_Info.nLengthInBytes;
				pParam->SetValue(nTemp);
				break;
			default:
				return (false);
			}
			break;
		}
	case apLENGTHINMS:	
		{
			int32 nTemp;
			if (!(pParam->GetValue(nTemp))) return (false);
			nTemp = m_Info.nLengthInMs;
			pParam->SetValue(nTemp);
			break;
		}
	case apLENGTHINSAMPLES:	
		{
			int32 nTemp;
			if (!(pParam->GetValue(nTemp))) return (false);
			nTemp = m_Info.nLengthInSamples;
			pParam->SetValue(nTemp);
			break;
		}	
	case apBYTEPOSITION:	
		return (false);
		break;
	case apTIMEPOSITION:	
		return (false);
		break;
	case apLOADINGSTATE:	
		{
			int32 nTemp;
			if (!(pParam->GetValue(nTemp))) return (false);
			nTemp = m_Info.bLoadFailure;
			pParam->SetValue(nTemp);
			break;
		}
	case 	apNUMREFERENCEDSOUNDS:
		{
			int32 nTemp;
			if (!(pParam->GetValue(nTemp))) return (false);
			nTemp = m_vecLoadReq.size();
			pParam->SetValue(nTemp);
		}
	default:	
		return (false);
		break;
	}
	return (true);
}

//////////////////////////////////////////////////////////////////////////
bool CSoundBufferXenon::SetParam(enumAssetParamSemantics eSemantics, ptParam* pParam)
{
	return (true);
}


//////////////////////////////////////////////////////////////////////////
bool CSoundBufferXenon::UnloadData(const eUnloadDataOptions UnloadOption)
{
	//GUARD_HEAP;
	if (m_BufferData)
	{
		//FUNCTION_PROFILER( m_pSoundSystem->GetSystem(),PROFILE_SOUND );
		switch (m_Props.eBufferType)
		{
		case btSAMPLE:	
			m_pSoundAssetManager->FreeMemoryChunk(m_Props);
			SetAssetHandle(NULL, btSAMPLE);	
			break;
		case btSTREAM:	
			SetAssetHandle(NULL, btSTREAM);	
			break;
		}
	}
  return true;
}

#endif