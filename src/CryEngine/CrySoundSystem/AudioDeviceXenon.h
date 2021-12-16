////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CAudioDeviceXenon.cpp
//  Version:     v1.00
//  Created:     15/4/2005 by Tomas
//  Compilers:   Visual Studio.NET
//  Description: Xenon Implementation of the platform dependent AudioDevice
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AUDIODEVICEXENONXAUDIO_H__
#define __AUDIODEVICEXENONXAUDIO_H__

#ifdef XENON

#pragma once

#include <d3d9.h>
#include <XAudio.h>
#include <X3daudio.h>
#include "ISound.h"
#include <ISystem.h>
#include "SoundSystem.h"
#include "IAudioDevice.h"

// Speed of sound in the sample
const FLOAT SPEEDOFSOUND = 340.29f;

// Speaker channel masks
//const DWORD SPEAKER_FRONT_LEFT      = 0x01;
//const DWORD SPEAKER_FRONT_RIGHT     = 0x02;
//const DWORD SPEAKER_FRONT_CENTER    = 0x04;
//const DWORD SPEAKER_LOW_FREQUENCY   = 0x08;
//const DWORD SPEAKER_BACK_LEFT       = 0x10;
//const DWORD SPEAKER_BACK_RIGHT      = 0x20;

struct X3DListenerXenon
{
  int X3DListenerId;
  X3DAUDIO_LISTENER X3DListener;
};

typedef std::vector<X3DListenerXenon>			SND_X3DListenerVec;
typedef SND_X3DListenerVec::iterator			SND_X3DListenerVecIter;

class CSoundAssetManager;
class CSoundBuffer;

class CAudioDeviceXenon : public IAudioDevice
{
public:
	CAudioDeviceXenon();
	~CAudioDeviceXenon(void);

	// init and shutdown functions
  virtual bool InitDevice(CSoundSystem* pSoundSystem);	// returns true if init went well xenon init
	virtual bool ShutDownDevice(void);										// shut down and free system
	virtual void* GetSoundLibrary(void) {return NULL; }		// returns ptr to the sound library if possible, else NULL
	virtual void* GetEventSystem(void) {return NULL; }		// returns ptr to the event system if possible, else NULL
	virtual void GetOutputHandle( void **pHandle, void **pHandle2, EOutputHandle *HandleType) {*pHandle = NULL; *HandleType = eOUTPUT_MAX; } // returns ptr to the output handle if possible, else NULL and handletype

	virtual void GetInitSettings(AudioDeviceSettings *InitSettings) {}
	virtual void SetInitSettings(AudioDeviceSettings *InitSettings) {}
	
	virtual bool GetCpuUsage(float *fDSP, float *fStream, float* fUpdate, float *fTotal) { return true; } // returns ptr to the output handle if possible, else NULL and handletype

	bool ResetAudio(void);											// major system attrib has been changed del all sounds clean system start
	void PauseSystem(bool status);							// pauses all playing sounds send bool to pause=true to pause
	bool UpdateListeners(void);									// this is called every frame to update all listeners and must be called *before* SubSystemUpdate()
	bool Update(void);													// example would be CS_update()
	void StopAllChannels(void);									// stops all sounds from playing
	bool SetFrequency(int newFreq);							// sets sound system global frequency
	void SetMute(bool mute);										// mutes or unmutes the whole sound system
	int  GetMemoryUsage(class ICrySizer* pSizer); // compute memory-consumption, returns rough estimate in MB
	
	// accesses wavebanks
	virtual IWavebank* GetWavebank(int nIndex) { return NULL; }
	virtual IWavebank* GetWavebank(const char* sWavebankName) { return NULL; }
	virtual int GetWavebankCount() { return 0; }

  // Get listener settings for sound computation
  X3DAUDIO_LISTENER* GetListener(ListenerID id);

	int		GetNumberSoundsPlaying(void);
  
	void	SetMasterVolume(int newVolume);
	int		GetMasterVolume(void);
  int		GetMemoryStats(void);

	// check for multiple record devices and expose them, write name to sName pointer
	virtual bool GetRecordDeviceInfo(const int nRecordDevice, char* sName, const int nNameLength) {return false;}

	CSoundBuffer* CreateSoundBuffer(const SSoundBufferProps &BufferProps);
	IPlatformSound* CreatePlatformSound(CSound* pSound, const char *sEventName);

	// sound system settings
	bool GetParam(enumAudioDeviceParamSemantics enumtype, ptParam* pParam); // get enum for type of info and ptr type of info gotton
	// the bool returns if this info is available on this type
	bool SetParam(enumAudioDeviceParamSemantics enumType, ptParam* pParam);

	// writes output to screen in debug
	virtual void DrawInformation(IRenderer* pRenderer, float xpos, float ypos, int nSoundInfo) { return; } 
  
private:

  // set listener vars or make new one
  bool SetListener(Vec3 *vPos, Vec3 *vVel, Vec3 *vForward, Vec3 *vTop, ListenerID nListenerID);

  // set of listeners
  SND_X3DListenerVec m_VecX3DListener;
  // id number of active listener
  int m_nActiveListener;

  //X3DListenerXenon *m_pTempListXenon;
  //X3DAUDIO_LISTENER *m_pTempListener;


  CSoundSystem *m_pSoundSystem;

  bool  m_bDopplerStatus;
  bool  m_bMuteStatus;
  int   m_nSoftwareChannels; // total voices on xenon
  bool  m_bSystemPaused;
  float m_fpctCpuUsed;
  int   m_nCurrentMemAlloc;
  int   m_nMaxMemAlloc;
  int   m_nCS_MasterVolume; 
  int   m_nSystemFrequency;
};

#endif
#endif