////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   IPlatformSoundXenon.cpp
//  Version:     v1.00
//  Created:     12/4/2005 by Tomas
//  Compilers:   Visual Studio.NET
//  Description: Xenon Implementation of a platform dependent Sound
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef SOUNDSYSTEM_USE_XENON_XAUDIO

#pragma once

#include ".\platformsoundxenon.h"
#include "SoundBufferXenon.h"
#include "Sound.h"
#include "AudioDeviceXenon.h"
#include <X3DAudio.h>
#include "XenonUtil/AtgAudio.h"
#include "XenonUtil/AtgApp.h"
#include "XenonUtil/AtgUtil.h"


//////////////////////////////////////////////////////////////////////////
CPlatformSoundXenon::CPlatformSoundXenon(CSound*	pSound)
{
	assert(pSound);
	m_pSound				= pSound;
	m_pSourceVoice	= NULL;
	m_nRefCount			= 0;
}

//////////////////////////////////////////////////////////////////////////
CPlatformSoundXenon::~CPlatformSoundXenon()
{
	FreeSoundHandle();																				 
}

//////////////////////////////////////////////////////////////////////////
int32 CPlatformSoundXenon::AddRef()
{
	return (++m_nRefCount);
}

//////////////////////////////////////////////////////////////////////////
int32 CPlatformSoundXenon::Release()
{
	--m_nRefCount;
	if (m_nRefCount <= 0)
	{
		delete this;
		return (0);

	}
	return m_nRefCount;
}

//////////////////////////////////////////////////////////////////////////
tSoundHandle CPlatformSoundXenon::GetSoundHandle() const	
{ 
	if (m_pSourceVoice == NULL) return (NULL);
	return ((tSoundHandle)m_pSourceVoice); 
}

//TODO replace with pointers and ignore NULLs
//////////////////////////////////////////////////////////////////////////
bool CPlatformSoundXenon::Set3DPosition(Vec3* pvPosition, Vec3* pvVelocity, Vec3* pvOrientation)
{
	// essentially this method could be completely empty because of the global way Xenon needs
	// the listener orientation when setting a sound position
	// So this all could/should be done in the AudioDevice.Update()

	CSoundSystem* pSys = m_pSound->GetSoundSystemPtr();
	if (!pSys) return (false);

	CAudioDeviceXenon* pAudioDevice = (CAudioDeviceXenon*) pSys->GetIAudioDevice();
	if (!pAudioDevice) return (false);

	if (!pvPosition || !pvVelocity) return (false);

	// X3DAudio stuff
	X3DAUDIO_DSP_SETTINGS	xDSPSettings;
	X3DAUDIO_EMITTER      xEmitter;
	X3DAUDIO_LISTENER*		pXenonListener = pAudioDevice->GetListener(1);
	
	// invalid Listener ID
	if (!pXenonListener)
		return (false);

	//	X3DAUDIO_CONE           xCone;

	xDSPSettings.SrcChannelCount     = 1;
	xDSPSettings.DstChannelCount     = CHANNELCOUNT;
	xDSPSettings.pMatrixCoefficients = new FLOAT[ CHANNELCOUNT ];
	
	if ( xDSPSettings.pMatrixCoefficients == NULL )
		return (false);

	// Initialize 3D audio parameters
	X3DAUDIO_VECTOR ZeroVector = { 0.0f, 0.0f, 0.0f };
	FLOAT g_EmitterAzimuths [] = { 0.f } ;

	// Set up emitter parameters
	xEmitter.OrientFront.x         = 0.0f;
	xEmitter.OrientFront.y         = 0.0f;
	xEmitter.OrientFront.z         = 1.0f;
	xEmitter.OrientTop.x           = 0.0f;
	xEmitter.OrientTop.y           = 1.0f;
	xEmitter.OrientTop.z           = 0.0f;

	Vec3 vTemp = m_pSound->GetPosition();


	xEmitter.Position.x            = vTemp.x;
	xEmitter.Position.y            = vTemp.z;
	xEmitter.Position.z            = vTemp.y;
	xEmitter.Velocity              = ZeroVector;
	xEmitter.pCone                 = NULL;

	xEmitter.ChannelCount          = 1;
	xEmitter.ChannelRadius         = 0.0f;
	xEmitter.pVolumeCurve          = NULL;
	xEmitter.pLFECurve             = NULL;
	xEmitter.pLPFDirectCurve       = NULL;
	xEmitter.pLPFReverbCurve       = NULL;
	xEmitter.pReverbCurve          = NULL;
	xEmitter.CurveDistanceScaler   = 14.0f;
	xEmitter.DopplerScaler         = 1.0f;
	xEmitter.pChannelAzimuths      = g_EmitterAzimuths;

	if (pvPosition)
	{
		// Sound Settings with axis flip
		xEmitter.Position.x = pvPosition->x;
		xEmitter.Position.z = pvPosition->y;
		xEmitter.Position.y = pvPosition->z;
	}

	if (m_pSound->GetFlags() & ~FLAG_SOUND_3D)
	{
		xEmitter.Position.x = pXenonListener->Position.x;
		xEmitter.Position.y = pXenonListener->Position.y;
		xEmitter.Position.z = pXenonListener->Position.z;
	}

	if (pvVelocity)
	{
		// Sound Settings with axis flip
		xEmitter.Velocity.x = pvVelocity->x;
		xEmitter.Velocity.z = pvVelocity->y;
		xEmitter.Velocity.y = pvVelocity->z;
	}

	// Retrieve 3D Audio parameters
	DWORD dwCalculateFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER |
		X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB |
		X3DAUDIO_CALCULATE_REVERB;

	// There are other flags that control additional X3DAudio
	// functionality that will be illustrated as they become
	// available
  assert(0);
	//X3DAudioCalculate( pXenonListener, &xEmitter, dwCalculateFlags, &xDSPSettings );

	// Pan the voice according to X3DAudio calculation
	XAUDIOVOICEOUTPUTVOLUME      VoiceVolume;
	XAUDIOVOICEOUTPUTVOLUMEENTRY VoiceEntry;
	XAUDIOCHANNELVOLUME          ChannelVolume;
	XAUDIOCHANNELVOLUMEENTRY     ChannelVolumeEntries[ 6 ];

	VoiceVolume.EntryCount = 1;
	VoiceVolume.paEntries  = &VoiceEntry;

	VoiceEntry.OutputVoiceIndex = 0;
	VoiceEntry.pVolume          = &ChannelVolume;

	ChannelVolume.EntryCount = 6;
	ChannelVolume.paEntries  = ChannelVolumeEntries;

	for( DWORD i = 0; i < ChannelVolume.EntryCount; i++ )
	{
		ChannelVolumeEntries[ i ].EntryIndex = (XAUDIOCHANNEL)i;
		ChannelVolumeEntries[ i ].Volume     = xDSPSettings.pMatrixCoefficients[ i ];
	}

	m_pSourceVoice->SetVoiceOutputVolume( &VoiceVolume );

	// Set Pitch (Doppler factor)
	m_pSourceVoice->SetPitch( -1.0f + xDSPSettings.DopplerFactor );

	//if (!CS_3D_SetAttributes(m_nChannel,pos, vel))	return (false);
	return (true);
}

// Gets and Sets Parameter defined in the enumSoundParam list
//////////////////////////////////////////////////////////////////////////
bool CPlatformSoundXenon::GetParamByType(enumPlatformSoundParamSemantics eSemantics, ptParam* pParam)
{
		if (m_pSourceVoice == NULL) return (false);
		//pspNONE,
		//pspSOUNDTYPE,
		//pspSAMPLETYPE,
		//pspFREQUENCY,
		//pspVOLUME,
		//pspISPLAYING,
		//pspLOOPMODE,
		//pspPAUSEMODE,
		//pspSPEAKERPAN,
		//pspSAMPLEPOSITION,
		//pspTIMEPOSITION,
		//psp3DPOSITION,
		//psp3DVELOCITY,
		//pspMINDISTANCE,
		//pspMAXDISTANCE,
		//pspPRIORITY,
		//pspFXEFFECT,
		//pspAMPLITUDE

	//if (m_pSourceVoice == NULL || m_pSourceCue == NULL) return (false);

	switch (eSemantics)
	{
	case pspNONE:
		break;
	case pspSOUNDTYPE:
		break;
	case pspSOUNDHANDLE:
		{
			int32 nTemp = 0;
			if (!(pParam->GetValue(nTemp))) return (false);
			nTemp = (int32) m_pSourceVoice;
			pParam->SetValue(nTemp);
			break;
		}
	case pspSAMPLETYPE:
		{
			int32 nTemp = 0;
			if (!(pParam->GetValue(nTemp))) return (false);
			XAUDIOSOURCEFORMAT Format;
			m_pSourceVoice->GetVoiceFormat(&Format);
			nTemp = Format.SampleType;
			pParam->SetValue(nTemp);
			break;
		}
	case pspFREQUENCY:
		{
			int32 nTemp = 0;
			if (!(pParam->GetValue(nTemp))) return (false);
			XAUDIOSOURCEFORMAT Format;
			m_pSourceVoice->GetVoiceFormat(&Format);
			nTemp = Format.SampleRate;
			pParam->SetValue(nTemp);
			break;
		}
	case pspVOLUME:
		{
			int32 nTemp = 0;
			float fTemp = 0.0f;
			if (!(pParam->GetValue(nTemp))) return (false);
			m_pSourceVoice->GetVolume(&fTemp);
			nTemp = fTemp*255;
			pParam->SetValue(nTemp);
			break;
		}
	case pspISPLAYING:
		{
			bool bTemp = false;
			XAUDIOSOURCESTATE TempState;
			if (!(pParam->GetValue(bTemp))) return (false);
			m_pSourceVoice->GetVoiceState(&TempState);
			bTemp = (TempState & XAUDIOSOURCESTATE_STARTED);
			pParam->SetValue(bTemp);
			break;
		}
	case pspLOOPMODE:
		{
			bool bTemp = false;
			DWORD wTemp = 0;
			if (!(pParam->GetValue(bTemp))) return (false);
			m_pSourceVoice->GetPacketLoopCount(&wTemp);
			bTemp = (wTemp != XAUDIOLOOPCOUNT_MIN);
			pParam->SetValue(bTemp);
			break;		
		}
	case pspPAUSEMODE:
		{
			break;
		}
	case pspSPEAKERPAN:
		break;
	case pspSAMPLEPOSITION:
		{
			break;
		}
		//		TIMEPOSITION
		//CS_Sample_GetDefaults((CS_SAMPLE*)m_pSoundBuffer->GetAssetHandle(), &nFreq, NULL, NULL, NULL);
		//return (unsigned int)((float)CS_GetCurrentPosition(m_nChannel)/(float)nFreq*1000.0f);

	case psp3DPOSITION:
		{
			break;
		}
	case psp3DVELOCITY:
		{
			break;
		}
	case pspPRIORITY:
		break;
	case pspFXEFFECT:
		break;
	case pspAMPLITUDE:
		{
			int32 nTemp = 0;
			XAUDIOVOLUME xTest;
			m_pSourceVoice->GetVolume(&xTest);
			nTemp = xTest*255;
			pParam->SetValue(nTemp);
			break;
		}
	default:
		break;
	}

	return (true);
}

//////////////////////////////////////////////////////////////////////////
bool CPlatformSoundXenon::SetParamByType(enumPlatformSoundParamSemantics eSemantics, ptParam* pParam)
{
	if (m_pSourceVoice == NULL) return (false);

	switch (eSemantics)
	{
	case pspNONE:
		return (false);
		break;
	case pspSOUNDTYPE:
		break;
	case pspSAMPLETYPE:
		break;
	case pspFREQUENCY:
		// TODO Check Pitch and stuff
		{
			return (false);
			break; 
		}
	case pspVOLUME:
		{
			int32	nTemp = 0;
			float	fTemp = 0.0f;
			if (!(pParam->GetValue(nTemp))) return (false);
			fTemp = nTemp/255.0f;
			m_pSourceVoice->SetVolume(fTemp);
			break;
		}
	case pspISPLAYING:
		// dont support that
		break;
	case pspLOOPMODE:
		{
			bool bTemp = false;
			DWORD wTemp = 0;
			if (!(pParam->GetValue(bTemp))) return (false);
			if (bTemp)
				wTemp = XAUDIOLOOPCOUNT_INFINITE;
			else
				wTemp = XAUDIOLOOPCOUNT_MIN;
			m_pSourceVoice->SetPacketLoopCount(wTemp);
			break;		
		}
	case pspPAUSEMODE:
		{
			break;
		}
	case pspSPEAKERPAN:
		break;
	case pspSAMPLEPOSITION:
		{
			break;
		}
	case psp3DPOSITION:
		{
			return (false);
			break;
		}
	case psp3DVELOCITY:
		{
			return (false);
			break;
		}
	case pspPRIORITY:
		break;
	case pspFXEFFECT:
		break;
	case pspAMPLITUDE:
		break;
	default:
		break;
	}
	return (true);
}


//////////////////////////////////////////////////////////////////////////
bool CPlatformSoundXenon::PlaySound(bool bStartPaused)
{
		// TODO Generic approach to Playmodes (Play/Pause/Stop)
		// Stopping a Stream does not stop the streaming itself !

	// Play the source voice
	if (m_pSourceVoice != NULL)
	{
		m_pSourceVoice->Stop(XAUDIOSTOP_IMMEDIATE);
		m_pSourceVoice->Start(0);
		float fVolume = 0.0f;
		//LPXAUDIOVOLUME nVolume = 0;
		m_pSourceVoice->GetVolume(&fVolume);
		//if ( FAILED( hr = m_pSourceVoice->Start() ) )
			//ATG::FatalError( "Error %#X calling Start\n", hr );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPlatformSoundXenon::StopSound()
{
	if (m_pSourceVoice != NULL)
	{
		m_pSourceVoice->Stop(XAUDIOSTOP_IMMEDIATE);
		//CS_StopSound(m_nChannel);
	}

	//	m_fChannelPlayTime = -1;
	m_pSourceVoice = NULL;
	return true;
}


//////////////////////////////////////////////////////////////////////////
bool CPlatformSoundXenon::FreeSoundHandle()
{
	if (m_pSourceVoice != NULL)
	{
		CSoundBuffer* SoundBuf = m_pSound->GetSoundBufferPtr();

		//To actually free a channel FreeSoundHandle has to be called.
		// if it was a stream, or if it was the only ref to the Asset, the Asset will stop.
		StopSound();	

		// TODO Review stuff below
		switch (SoundBuf->GetProps()->eBufferType)
		{
		case btSTREAM:
			// if (m_pSoundBuffer->GetSoundBuffer()) // was that, changed it TN
			if (SoundBuf->Loaded())
				break;
		case btSAMPLE:
			//			CS_StopSound(m_nChannel);
			break;
		}
		SoundBuf->Release();
	}

	m_pSourceVoice = NULL;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPlatformSoundXenon::CreateSound(tAssetHandle AssetHandle, SSoundSettings SoundSettings)
{
	CSoundBuffer* SoundBuf = m_pSound->GetSoundBufferPtr();
	HRESULT hr;

	switch (SoundBuf->GetProps()->eBufferType)
	{
	case btSTREAM:
		if (SoundBuf->Loaded())
		{


		}
//			m_nChannel = CS_Stream_PlayEx(CS_FREE, (CS_STREAM*)AssetHandle, NULL, true);
	//	CS_Stream_Stop((CS_STREAM*)SoundBuf->GetAssetHandle());
		break;

	case btSAMPLE:
		if (SoundBuf->Loaded())
		{
			// Load the wave format from the file
			
			// Initializing a source voice 
			XAUDIOSOURCEVOICEINIT SourceVoiceInit = { 0 };
			if( SoundBuf->GetInfo()->nBitsPerSample == 16 )
				SourceVoiceInit.Format.SampleType = XAUDIOSAMPLETYPE_16BITPCM;
			else if ( SoundBuf->GetInfo()->nBitsPerSample == 8 )
				SourceVoiceInit.Format.SampleType = XAUDIOSAMPLETYPE_8BITPCM;
			else
				ATG::FatalError( "Invalid format\n" );

			// TODO Hack this until this works
			if( SoundBuf->GetInfo()->nChannels > 1)
				SoundBuf->GetInfo()->nChannels = 1;
				//ATG::FatalError( "Now the sample only supports MONO sounds\n" );

			SourceVoiceInit.MaxChannelMapEntryCount = CHANNELCOUNT;    // Set channel map count
			SourceVoiceInit.Format.ChannelCount     = (XAUDIOCHANNEL)SoundBuf->GetInfo()->nChannels;
			SourceVoiceInit.Format.SampleRate       = (XAUDIOSAMPLERATE)SoundBuf->GetInfo()->nBaseFreq;
			SourceVoiceInit.MaxPacketCount          = 1;

			if ( FAILED( hr = XAudioCreateSourceVoice( &SourceVoiceInit, &m_pSourceVoice ) ) )
				ATG::FatalError( "Error %#X calling XAudioCreateSourceVoice\n", hr );


			// Creating a source voice
			XAUDIOSOURCEFORMAT fmt = { 0 };
			if( SoundBuf->GetInfo()->nBitsPerSample == 16 )
				fmt.SampleType = XAUDIOSAMPLETYPE_16BITPCM;
			else if( SoundBuf->GetInfo()->nBitsPerSample == 8 )
				fmt.SampleType = XAUDIOSAMPLETYPE_8BITPCM;
			else
				ATG::FatalError( "Invalid format\n" );

			fmt.ChannelCount = (XAUDIOCHANNEL)SoundBuf->GetInfo()->nChannels;
			fmt.SampleRate   = (XAUDIOSAMPLERATE)SoundBuf->GetInfo()->nBaseFreq;

			// Set up channel map of the source voice
			XAUDIOVOICEOUTPUT      Output;
			XAUDIOVOICEOUTPUTENTRY OutputEntry;
			XAUDIOCHANNELMAP       ChannelMap;
			XAUDIOCHANNELMAPENTRY  ChannelMapEntry[] =
			{
				{ 0, 0, 0.4f },
				{ 0, 1, 0.4f },
				{ 0, 2, 0.4f },
				{ 0, 3, 0.4f },
				{ 0, 4, 0.4f },
				{ 0, 5, 0.4f },
			};
			Output.EntryCount = 1;
			Output.paEntries  = &OutputEntry;
			OutputEntry.pDestVoice  = NULL;
			OutputEntry.pChannelMap = &ChannelMap;
			ChannelMap.EntryCount = CHANNELCOUNT;
			ChannelMap.paEntries  = ChannelMapEntry;

			m_pSourceVoice->SetVoiceFormat( &fmt, &Output, 0 );

			// Find out how big the new sample is
			DWORD dwNewSize = SoundBuf->GetInfo()->nLengthInBytes;
			//awfSound->GetDuration( &dwNewSize );

			// Set up packet
			XAUDIOPACKET Packet = { 0 };
			Packet.pBuffer    = AssetHandle;
			Packet.BufferSize = dwNewSize;
			Packet.LoopCount  = (DWORD)-1;
			Packet.LoopStart  = 0;
			Packet.LoopLength = dwNewSize;
			//Packet.LoopStart  = dwLoopStart;
			//Packet.LoopLength = dwLoopLength;
			// TODO Implement Loopstart and End points

			// Submit packet
			if ( FAILED( hr = m_pSourceVoice->SubmitPacket( &Packet,
				XAUDIOSUBMITPACKET_DISCONTINUITY ) ) )
				ATG::FatalError( "Error %#X calling SubmitPacket\n", hr );

		}

		//m_nChannel = CS_PlaySoundEx(CS_FREE, (CS_SAMPLE*)AssetHandle, NULL, true);
		break;
	}

	return true;
}

#endif
