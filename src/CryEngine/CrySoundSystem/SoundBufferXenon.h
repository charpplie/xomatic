////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SoundBufferXenon.h
//  Version:     v1.00
//  Created:     4/4/2005 by Tomas.
//  Compilers:   Visual Studio.NET
//  Description: Xenon Implementation of SoundBuffer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SOUNDBUFFERXENONXAUDIO_H__
#define __SOUNDBUFFERXENONXAUDIO_H__

#ifdef XENON

#pragma once

#include "soundbuffer.h"
#include <xaudio.h>

class CSoundAssetManager;
struct IMicrophoneStream;

class CSoundBufferXenon : public CSoundBuffer
{
public:
	CSoundBufferXenon(const SSoundBufferProps &Props);

protected:
	~CSoundBufferXenon(void);

	//////////////////////////////////////////////////////////////////////////
	// Inherited Method by IStreamCallBack
	//////////////////////////////////////////////////////////////////////////
	virtual void	StreamOnComplete(IReadStream *pStream, unsigned nError);

	//////////////////////////////////////////////////////////////////////////
	//	platform dependent calls
	//////////////////////////////////////////////////////////////////////////

	virtual uint32			GetMemoryUsed() {return 0;}
  virtual uint32			GetMemoryUsage(class ICrySizer* pSizer) { return 0;	}	// compute memory-consumption, returns size in Bytes

	// Loads a file or memory block into memory of the used sound library
	// TODO change interface to (void* Data, void* DataPros) to make it very generic, really?
	
	// Opens a wavebank/soundbank and returns Handle to create a cue
	virtual tAssetHandle LoadAsCue(const char *AssetName);

	// opens a file directly using XAudio and store the data in memory
	virtual tAssetHandle LoadAsSample(const char *AssetDataPtrOrName, int nLength);

	// Opens a file or memory block as a stream 
	virtual tAssetHandle LoadAsStream(const char *AssetName, int nLength);

	// loads a event sound
	virtual tAssetHandle LoadAsEvent(const char *AssetName);

	// loads as microphone stream
	virtual tAssetHandle LoadAsMicro(IMicrophoneStream *pIMicrophoneStream) { return 0; }

	// Gets and Sets Parameter defined in the enumAssetParam list
	virtual bool	GetParam(enumAssetParamSemantics eSemantics, ptParam* pParam) const;
	virtual bool	SetParam(enumAssetParamSemantics eSemantics, ptParam* pParam);

	// Closes down Stream or frees memory of the Sample
	virtual bool	UnloadData(const eUnloadDataOptions UnloadOption);

};

#endif
#endif