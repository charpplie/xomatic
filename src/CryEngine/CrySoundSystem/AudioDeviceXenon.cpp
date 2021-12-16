////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CAudioDeviceXenon.h
//  Version:     v1.00
//  Created:     15/4/2005 by Tomas
//  Compilers:   Visual Studio.NET
//  Description: Xenon Implementation of the platform dependent AudioDevice
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef SOUNDSYSTEM_USE_XENON_XAUDIO

#include <XAudDefs.h>
#include <XAudio.h>
#include <X3DAudio.h>
#include <CrySizer.h>
#include "XenonUtil/AtgApp.h"
#include "XenonUtil/AtgUtil.h"
#include "AudioDeviceXenon.h"
#include "SoundSystem.h"
#include "Sound.h"
#include "SoundBufferXenon.h"
#include "PlatformSoundXenon.h"



//////////////////////////////////////////////////////////////////////////
CAudioDeviceXenon::CAudioDeviceXenon()
{
	m_pSoundSystem = NULL;
}

//////////////////////////////////////////////////////////////////////////
CAudioDeviceXenon::~CAudioDeviceXenon(void)
{
}


//////////////////////////////////////////////////////////////////////////
bool CAudioDeviceXenon::InitDevice(CSoundSystem* pSoundSystem)
{
  // by now soundsystem made get init started
  // software channels is not used here

  //m_pSoundSystem = pSoundSystem;	

  // get intial value from console or system.cfg
  //m_bDopplerStatus = (m_pSoundSystem->m_pCVarDopplerEnable->GetFVal() ? true : false);
  //m_nSystemFrequency = m_pSoundSystem->m_pCVarSampleRate->GetIVal();
	
	if (!pSoundSystem) // check for valid system
		return (false);

	m_pSoundSystem = pSoundSystem;

	// Initialize Xenon Listeners
	if (!UpdateListeners())
		return false;

  //m_nActiveListener = 1;
  //Vec3 pTempVec(0.0, 0.0, 0.0);
	//Vec3 pTempTopVec(0.0, 1.0, 0.0);
	//Vec3 pTempForwardVec(0.0, 0.0, 1.0);
  //Set up first or default xenon Listener init with 0.0 values
  //SetListener(&pTempVec, &pTempVec, &pTempForwardVec,&pTempTopVec, m_nActiveListener);

	// Set up initialize parameters of XAudio Engine
	XAUDIOENGINEINIT EngineInit = { 0 };
	EngineInit.pEffectTable = &XAudioDefaultEffectTable;
	//EngineInit.MaxVoiceChannelCount = xx;
	
	// Initialize the XAudio Engine
	HRESULT hr = XAudioInitialize( &EngineInit );
	
	// link error
	//if ( FAILED( hr ) )
		//ATG::FatalError( "Error %#X calling XAudioInitialize\n", hr );

	// Initialize the X3DAudio
	//  Speaker geometry configuration on the final mix, specifies assignment of channels
	//  to speaker positions, defined as per WAVEFORMATEXTENSIBLE.dwChannelMask
	//
	//  SpeedOfSound - speed of sound in user-defined world units/second, used
	//  only for doppler calculations, it must be >= FLT_MIN
	
	//X3DAudioInitialize( SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|
	//	SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT,
	//	SPEEDOFSOUND );
  assert(0);

	// Initialize 3D audio parameters
	X3DAUDIO_VECTOR ZeroVector = { 0.0f, 0.0f, 0.0f };


	return (true);
}

//////////////////////////////////////////////////////////////////////////
bool CAudioDeviceXenon::ShutDownDevice(void)
{
	// shut down and free system

	// Initialize the XAudio Engine
	HRESULT hr = XAudioShutDown( );
	return true;
}

bool CAudioDeviceXenon::ResetAudio(void)
{
  XAudioDumpMemoryUsage(false);
	// todo make it return bool
	return true;
}


void CAudioDeviceXenon::PauseSystem(bool status)
{

	// todo make it return bool
	//return true;
}

// if id = 0 then get the active listener
X3DAUDIO_LISTENER* CAudioDeviceXenon::GetListener(ListenerID nListenerID)
{
  for (SND_X3DListenerVecIter It = m_VecX3DListener.begin(); It != m_VecX3DListener.end(); ++It)
    {
    if ((*It).X3DListenerId == nListenerID)
      return (&((*It).X3DListener));
    }
  // returns a null
  return (NULL);
}
	
// this is called every frame to update all listeners and must be called *before* SubSystemUpdate()
bool CAudioDeviceXenon::UpdateListeners(void)
{

	bool bResult = false;
	//uint32 nNumActiveListeners = m_pSoundSystem->GetNumActiveListeners();

	IListener* pListener = NULL;
	ListenerID nListenerID = LISTENERID_INVALID;
	do 
	{
		pListener = m_pSoundSystem->GetNextListener(nListenerID);
		if (pListener)
		{
			nListenerID = pListener->GetID();
			if (pListener->GetActive())
			{
				Vec3 vPos			= pListener->GetPosition();
				Vec3 vVel			= pListener->GetVelocity();
				Vec3 vTop			= pListener->GetTop();
				Vec3 vForward	= pListener->GetForward();

				SetListener(&vPos, &vVel, &vForward, &vTop, nListenerID);
			}
			else
			{
				SND_X3DListenerVecIter IterEnd = m_VecX3DListener.end();
				for (SND_X3DListenerVecIter Iter = m_VecX3DListener.begin(); Iter != IterEnd; ++Iter)
				{
					if ((*Iter).X3DListenerId == nListenerID)
					{
						m_VecX3DListener.erase(Iter);
						break;
					}
				}
			}
		}

	}	while (pListener);

	return true;
}

bool CAudioDeviceXenon::Update(void)
{
	if (!m_pSoundSystem)
		return false;

// Right now just 1 listener active
  SND_X3DListenerVecIter It = m_VecX3DListener.begin();

	IListener* pListener = m_pSoundSystem->GetListener(LISTENERID_STANDARD);
	if (!pListener)
		return false;

	Vec3 tempPos(pListener->GetPosition());
  Vec3 tempFor(pListener->GetForward());
  Vec3 tempTop(pListener->GetTop());
  
	// TODO Verify coordinate system
  (*It).X3DListener.Position.x = tempPos.x;
  (*It).X3DListener.Position.y = tempPos.z;
  (*It).X3DListener.Position.z = tempPos.y;

  (*It).X3DListener.OrientFront.x = tempFor.x;
  (*It).X3DListener.OrientFront.y = tempFor.z;
  (*It).X3DListener.OrientFront.z = tempFor.y;

  (*It).X3DListener.OrientTop.x = tempTop.x;
  (*It).X3DListener.OrientTop.y = tempTop.z;
  (*It).X3DListener.OrientTop.z = tempTop.y;

  return (true);	
}

void CAudioDeviceXenon::StopAllChannels(void)
{

	// todo make it return bool
	//return true;
}

bool CAudioDeviceXenon::SetFrequency(int newFreq)
{

	// todo make it return bool
	return true;
}

void CAudioDeviceXenon::SetMute(bool mute)
{

	// todo make it return bool
	//return true;
}

int CAudioDeviceXenon::GetNumberSoundsPlaying(void)
{

	// todo make it return bool
	//return true;
	return 0;
}
int CAudioDeviceXenon::GetMemoryStats(void)
{

	return 0;
}
// set or create new listener returns true if created a new listener or false if just updated
// an existing listener
bool CAudioDeviceXenon::SetListener(Vec3 *vPos, Vec3 *vVel, Vec3 *vForward, Vec3 *vTop, int nListenerID) 
{
	bool bListenerFound = false;
	SND_X3DListenerVecIter FoundListenerIt;

	for (SND_X3DListenerVecIter It = m_VecX3DListener.begin(); It != m_VecX3DListener.end(); ++It)
	{
		// is there already a listener who needs to be updated
		if ((*It).X3DListenerId == nListenerID)
		{
			FoundListenerIt = It;
			bListenerFound	= true;
			//pTempListener = &((*It)->X3DListener); // we found it dont need to create new one
			break;
		}
	}

	if (!bListenerFound)
	{
		X3DListenerXenon NewXenonListener;
		X3DAUDIO_VECTOR ZeroVector = { 0.0f, 0.0f, 0.0f };

		NewXenonListener.X3DListenerId = nListenerID;
		NewXenonListener.X3DListener.Position			= ZeroVector;
		NewXenonListener.X3DListener.Velocity			= ZeroVector;
		NewXenonListener.X3DListener.OrientFront.x	= 0.0f;
		NewXenonListener.X3DListener.OrientFront.y  = 0.0f;
		NewXenonListener.X3DListener.OrientFront.z  = 1.0f;
		NewXenonListener.X3DListener.OrientTop.x  = 0.0f;
		NewXenonListener.X3DListener.OrientTop.y  = 1.0f;
		NewXenonListener.X3DListener.OrientTop.z  = 0.0f;

		m_VecX3DListener.push_back(NewXenonListener);

		for (SND_X3DListenerVecIter It = m_VecX3DListener.begin(); It != m_VecX3DListener.end(); ++It)
		{
			// is there already a listener who needs to be updated
			if ((*It).X3DListenerId == nListenerID)
			{
				FoundListenerIt = It;
				bListenerFound	= true;
				//pTempListener = &((*It)->X3DListener); // we found it dont need to create new one
				break;
			}
		}
	}

	if (vPos != NULL)
	{
		(*FoundListenerIt).X3DListener.Position.x = vPos->x;
		(*FoundListenerIt).X3DListener.Position.y = vPos->z;
		(*FoundListenerIt).X3DListener.Position.z = vPos->y;
	}

	if (vVel != NULL)
	{
		(*FoundListenerIt).X3DListener.Velocity.x = vVel->x;
		(*FoundListenerIt).X3DListener.Velocity.y = vVel->z;
		(*FoundListenerIt).X3DListener.Velocity.z = vVel->y;
	}
	if (vForward != NULL)
	{
		(*FoundListenerIt).X3DListener.OrientFront.x = vForward->x;
		(*FoundListenerIt).X3DListener.OrientFront.y = vForward->y;
		(*FoundListenerIt).X3DListener.OrientFront.z = vForward->z;
	}

	if (vTop != NULL)
	{
		(*FoundListenerIt).X3DListener.OrientTop.x = vTop->x;
		(*FoundListenerIt).X3DListener.OrientTop.y = vTop->y;
		(*FoundListenerIt).X3DListener.OrientTop.z = vTop->z;
	}

	return true;
}
void	CAudioDeviceXenon::SetMasterVolume(int newVolume) 
{
	m_nCS_MasterVolume = newVolume;
}
int		CAudioDeviceXenon::GetMasterVolume(void) 
{
	return m_nCS_MasterVolume;
}

// compute memory-consumption, returns rough estimate in MB
int CAudioDeviceXenon::GetMemoryUsage(class ICrySizer* pSizer)
{
	// TODO MAJOR REVIEW HERE !
	if (!pSizer->Add(*this))
		return 0;
	//CSoundBuffer *pBuffer=m_pSoundBuffer;
	//pSizer->Add(*pBuffer);
  return 0;
}

// creates a new platform dependent SoundBuffer
CSoundBuffer* CAudioDeviceXenon::CreateSoundBuffer(const SSoundBufferProps &BufferProps)
{
	CSoundBuffer* pBuf = NULL;
	pBuf = new CSoundBufferXenon(BufferProps);
	return pBuf;
}

// creates a new platform dependent PlatformSound
IPlatformSound* CAudioDeviceXenon::CreatePlatformSound(CSound* pSound, const char *sEventName)
{
	IPlatformSound* pPlatform = NULL;
	pPlatform = new CPlatformSoundXenon(pSound);
	return pPlatform;
}

/*
adpSOFTWARECHANNELS, // number of software channels
adpHARDWARECHANNELS,
adpMIN_HARDWARECHANNELS,
adpMAX_HARDWARECHANNELS,
adpTOTALCHANNELS,
adpEAX_STATUS,
adpMUTE_STATUS,
adpLISTENER_POSITION,
adpLISTENER_VELOCITY,
adpLISTENER_FORWARD,   // the 4 3d listener attributes
adpLISTENER_TOP,
adpWINDOW_HANDLE,
adpSYSTEM_FREQUENCY,  // mixing rate
adpMASTER_VOLUME,// for fmod value is 0-1, 1 being max volume
adpSPEAKER_MODE,
adpPAUSED_STATUS,
adpCPU_USED_PCT,
adpSNDSYSTEM_MEM_USAGE_NOW,
adpSNDSYSTEM_MEM_USAGE_HIGHEST,
adpSNDSYSTEM_OUTPUTTYPE,
adpSNDSYSTEM_DRIVER,
adpDOPPLER,            // turns doppler on and off
adpMIXING_BUFFER_SIZE,   // fmod sets a default value this changes that value in millesecons
adpSTOPALL_CHANNELS,
adpCHANNELS_PLAYING,
*/
bool CAudioDeviceXenon::GetParam(enumAudioDeviceParamSemantics enumtype, ptParam* pParam)
{

  int   nTemp;
  float fTemp;
  bool  bTemp;
  Vec3	vTemp;
  string sTemp;  
//  void * vpTemp;
  float tempVect[3];
  bool result=true;


  switch(enumtype) 
    {
    // FMOD ONLY****
    case adpSNDSYSTEM_DRIVER:// sets a string name of driver
    case adpSNDSYSTEM_OUTPUTTYPE:// returns a enum in int form of output type
    case adpMIXING_BUFFER_SIZE:// ****** OPTIONAL ****** if set returns mixing buffer size in milliseconds (fmod does automaticly)

    //** There is no EAX on zenon
    case adpEAX_STATUS:// is eax available on this machine

    // there are no hardware channels on XENON
    //case adpMIN_HARDWARECHANNELS:
    case adpTOTALCHANNELS:// total number of channels available

      // there are no output windows in XENON
    case adpWINDOW_HANDLE:// returns handle of active window which is void * type
    //case adpMAX_HARDWARECHANNELS:
    case adpSPEAKER_MODE:// gets active speaker mode
    case adpHARDWARECHANNELS:// number of hardware channels available
      result = false;
      break;
    case adpDOPPLER: // turn doppler on or off with a bool
      if (!(pParam->GetValue(bTemp))) return (false);
      bTemp = m_bDopplerStatus;
      pParam->SetValue(nTemp);
      break;
    /*case adpSOFTWARECHANNELS:// # of software channels available ***** or VOICES FOR XENON
      if (!(pParam->GetValue(nTemp))) return (false);
      nTemp = m_nSoftwareChannels;
      pParam->SetValue(nTemp);
      break;*/
    case adpMUTE_STATUS:// returns bool telling caller if system wide mute on=true or off=false
      if (!(pParam->GetValue(bTemp))) return (false);
      bTemp = m_bMuteStatus;
      pParam->SetValue(bTemp);
      break;
    case adpLISTENER_POSITION:// gets active Listeners position
      if (!(pParam->GetValue(vTemp))) return (false);
      tempVect[0] = tempVect[1] = tempVect[2] = 0.0f;
      //CS_3D_Listener_GetAttributes(tempVect, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      vTemp(tempVect[0], tempVect[2], tempVect[1]);
      pParam->SetValue(vTemp);
      break;
    case adpLISTENER_VELOCITY:// gets active Listeners velocity
      if (!(pParam->GetValue(vTemp))) return (false);
      tempVect[0] = tempVect[1] = tempVect[2] = 0.0f;
      //3D_Listener_GetAttributes(NULL, tempVect, NULL, NULL, NULL, NULL, NULL, NULL);
      vTemp(tempVect[0], tempVect[2], tempVect[1]);
      pParam->SetValue(vTemp);
      break;
    case adpLISTENER_FORWARD:// gets active Listeners direction or forward
      if (!(pParam->GetValue(vTemp))) return (false);
      tempVect[0] = tempVect[1] = tempVect[2] = 0.0f;
      //3D_Listener_GetAttributes(NULL, NULL, &tempVect[0], &tempVect[1], &tempVect[2], NULL, NULL, NULL);
      vTemp(tempVect[0], tempVect[2], tempVect[1]);
      pParam->SetValue(vTemp);
      break;
    case adpLISTENER_TOP:// gets active Listeners top
      if (!(pParam->GetValue(vTemp))) return (false);
      tempVect[0] = tempVect[1] = tempVect[2] = 0.0f;
      //3D_Listener_GetAttributes(NULL, NULL, NULL, NULL, NULL, &tempVect[0], &tempVect[1], &tempVect[2]);
      vTemp(tempVect[0], tempVect[2], tempVect[1]);
      pParam->SetValue(vTemp);
      break;
    case adpSYSTEM_FREQUENCY:// gets the mixing speed
      if (!(pParam->GetValue(nTemp))) return (false);
      //nTemp = CS_GetFrequency(CS_ALL);
      pParam->SetValue(nTemp);
      break;
    case adpMASTER_VOLUME: // value from 0-255 255 being max fmod master volume
      if (!(pParam->GetValue(nTemp))) return (false);
      //nTemp = CS_GetSFXMasterVolume();
      pParam->SetValue(nTemp);
      break;
    case adpPAUSED_STATUS:// is fmod paused or not
      if (!(pParam->GetValue(bTemp))) return (false);
      bTemp = m_bSystemPaused;
      pParam->SetValue(bTemp);
      break;
    case adpCPU_USED_PCT:// returns a value from 0% to 100% in the 0.0 to 1.0 float format 1.0 = 100%
      if (!(pParam->GetValue(fTemp))) return (false);
      //fTemp = CS_GetCPUUsage();
      m_fpctCpuUsed = fTemp;
      pParam->SetValue(fTemp);
      break;
    case adpSNDSYSTEM_MEM_USAGE_NOW:// returns how much memory fmod is using RIGHT NOW
      if (!(pParam->GetValue(nTemp))) return (false);
      GetMemoryStats();
      nTemp = m_nCurrentMemAlloc;
      pParam->SetValue(nTemp);
      break;
    case adpSNDSYSTEM_MEM_USAGE_HIGHEST:// returns the amount of memory fmod used at its PEEK
      if (!(pParam->GetValue(nTemp))) return (false);
      GetMemoryStats();
      nTemp = m_nMaxMemAlloc;
      pParam->SetValue(nTemp);
      break;
    case adpCHANNELS_PLAYING:// returns number of channels being used
      if (!(pParam->GetValue(nTemp))) return (false);
      //nTemp = CS_GetChannelsPlaying();
      pParam->SetValue(nTemp);
      break;
    case adpSTOPALL_CHANNELS:// ******* NOT ACTIVE !!!!!
      break;
    default:
      break;
    }
	return result;
}
/*
adpSOFTWARECHANNELS, // number of software channels
adpHARDWARECHANNELS,
adpMIN_HARDWARECHANNELS,
adpMAX_HARDWARECHANNELS,
adpTOTALCHANNELS,
adpEAX_STATUS,
adpMUTE_STATUS,
adpLISTENER_POSITION,
adpLISTENER_VELOCITY,
adpLISTENER_FORWARD,   // the 4 3d listener attributes
adpLISTENER_TOP,
adpWINDOW_HANDLE,
adpSYSTEM_FREQUENCY,  // mixing rate
adpMASTER_VOLUME,// for fmod value is 0-255 255 being max volume
adpSPEAKER_MODE,
adpPAUSED_STATUS,
adpCPU_USED_PCT,
adpSNDSYSTEM_MEM_USAGE_NOW,
adpSNDSYSTEM_MEM_USAGE_HIGHEST,
adpSNDSYSTEM_OUTPUTTYPE,
adpSNDSYSTEM_DRIVER,
adpDOPPLER,            // turns doppler on and off
adpMIXING_BUFFER_SIZE,   // fmod sets a default value this changes that value in millesecons
adpSTOPALL_CHANNELS,
adpCHANNELS_PLAYING,
*/
bool CAudioDeviceXenon::SetParam(enumAudioDeviceParamSemantics enumtype, ptParam* pParam)
{
  int   nTemp;
  bool  bTemp;
  Vec3	vTemp;
  string sTemp;  
//  void * vpTemp;
  bool result = true;

  switch(enumtype) 
    {
    case adpHARDWARECHANNELS:
    case adpTOTALCHANNELS:
    case adpEAX_STATUS:          // these values are not used at this time
    case adpCHANNELS_PLAYING:
    case adpSNDSYSTEM_OUTPUTTYPE:// **** OPTIONAL ***** (-1 lets fmod pick best one) sets the fmod mixer driver type 
    case adpSNDSYSTEM_DRIVER:   
    case adpCPU_USED_PCT:
    case adpSNDSYSTEM_MEM_USAGE_NOW:
    case adpSNDSYSTEM_MEM_USAGE_HIGHEST:
    //case adpMIN_HARDWARECHANNELS:// needed for fmod init
    //case adpMAX_HARDWARECHANNELS:// needed for fmod init
    case adpWINDOW_HANDLE:// sets fmod window output handle
    case adpSPEAKER_MODE:// sets fmod speaker enum
    case adpMIXING_BUFFER_SIZE:// ****OPTIONAL**** set the size of the mixing buffer in milliSeconds
      result = false;
        break;
    /*case adpSOFTWARECHANNELS:
      if (!(pParam->GetValue(nTemp))) return (false);
      m_nSoftwareChannels = nTemp;
        break;*/
    case adpMUTE_STATUS:// changes the system mute status ,bTemp true mutes the system false reactivates sound system
      if (!(pParam->GetValue(bTemp))) return (false);
      m_bMuteStatus = bTemp;
      //result = (CS_SetMute(CS_ALL, (signed char)bTemp) ? true : false);
      break;

		case adpLISTENER_POSITION:// sets listener position
			return (false);

			break;
		case adpLISTENER_VELOCITY:// sets 3d listener velocity
			return (false);

			break;
		case adpLISTENER_FORWARD:// sets 3d listener forward value
			return (false);

			break;
		case adpLISTENER_TOP:// sets 3d listener top value
			return (false);

      break;
    case adpSYSTEM_FREQUENCY:// sets fmod global mixing speed
      if (!(pParam->GetValue(nTemp))) return (false);
      m_nSystemFrequency = nTemp;
      //result = (CS_SetFrequency(CS_ALL, nTemp) ? true : false);
      break;
    case adpMASTER_VOLUME:// sets the fmod master volume 0 - 255 255 being highest volume
      if (!(pParam->GetValue(nTemp))) 
        return (false);

      if (nTemp < 0)
        nTemp = 0;
      else if (nTemp > 255)
        nTemp = 255;

      m_nCS_MasterVolume = nTemp;

      // 0 - 255 255 being max volume
      //CS_SetSFXMasterVolume(nTemp);  
      break;
    case adpPAUSED_STATUS:// pauses or un pauses system
      if (!(pParam->GetValue(bTemp))) return (false);
      m_bSystemPaused = bTemp;
      //result = (CS_SetPaused(CS_ALL, bTemp) ? true : false);
      break;
    case adpSTOPALL_CHANNELS:// stops all channels
      //result = (CS_StopSound(CS_ALL) ? true : false);  
      break;

    case adpDOPPLER: // turn doppler on or off with a bool
      if (!(pParam->GetValue(bTemp))) return (false);
      m_bDopplerStatus = bTemp;
      break;

    default:
      break;
    }
	return result;
}

#endif