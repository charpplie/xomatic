////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   IPlatformSoundXenon.h
//  Version:     v1.00
//  Created:     12/4/2005 by Tomas
//  Compilers:   Visual Studio.NET
//  Description: Xenon Implementation of a platform dependent Sound
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PLATFORMSOUNDXENONXAUDIO_H__
#define __PLATFORMSOUNDXENONXAUDIO_H__

#ifdef XENON

#pragma once

#include "IPlatformSound.h"
#include <xaudio.h>
#include <xact.h>

// TODO review
// Channel count
const DWORD CHANNELCOUNT = 6;

class CPlatformSoundXenon :	public IPlatformSound
{
public:
	CPlatformSoundXenon(CSound*	pSound);
	~CPlatformSoundXenon();

	//////////////////////////////////////////////////////////////////////////
	// Management
	//////////////////////////////////////////////////////////////////////////
	virtual void	SetSoundPtr(CSound* pSound) {m_pSound = pSound; }
	virtual int32	AddRef();
	virtual int32	Release();
	virtual bool	CreateSound(tAssetHandle AssetHandle, SSoundSettings SoundSettings);
	virtual bool	PlaySound(bool bStartPaused);	// should or could be done through SetParam PlayModes
	virtual bool	StopSound();	// should or could be done through SetParam PlayModes
	virtual bool	FreeSoundHandle();
	virtual bool	Set3DPosition(Vec3* pvPosition, Vec3* pvVelocity, Vec3* pvOrientation);
	virtual void  SetObstruction(const SObstruction *pObstruction) {}
	virtual void  SetPlatformSoundName( const char * sPlatformSoundName) {}
  virtual bool  IsInCategory(const char *) { return false; }

	virtual enumPlatformSoundStates  GetState() { return pssNONE; } 
  virtual enumPlatformSoundClass   GetClass() const { return pscNONE; }
  virtual void Reset(CSound*	pSound, const char *sEventName) {}

	//////////////////////////////////////////////////////////////////////////
	// Information
	//////////////////////////////////////////////////////////////////////////

	// Gets and Sets Parameter defined in the enumAssetParam list
	virtual bool	GetParamByType(enumPlatformSoundParamSemantics eSemantics, ptParam* pParam);
	virtual bool	SetParamByType(enumPlatformSoundParamSemantics eSemantics, ptParam* pParam);

	// Gets and Sets Parameter defined by string and float value, returns the index of that parameter
	virtual int		GetParamByName(const char *sParameter, float *fValue, bool bOutputWarning=true) {return (-1);}
	virtual int 	SetParamByName(const char *sParameter, float fValue, bool bOutputWarning=true) {return (-1);}

	// Gets and Sets Parameter defined by index and float value
	virtual bool	GetParamByIndex(int nIndex, float *fValue, bool bOutputWarning=true) {return (false);}
	virtual bool	SetParamByIndex(int nIndex, float fValue, bool bOutputWarning=true) {return (false);}  

	virtual tSoundHandle	GetSoundHandle() const;	

	// Memory
	virtual void  GetMemoryUsage(ICrySizer* pSizer) const {}

private:
	CSound*									m_pSound;								// Ptr to the sound this implementation belongs to
	IXAudioSourceVoice*     m_pSourceVoice;         // Source voice
	IXACTCue*								m_pSourceCue;
	int32										m_nRefCount;						// Refcounting, although it does not really make sense, but who knows
};

#endif
#endif