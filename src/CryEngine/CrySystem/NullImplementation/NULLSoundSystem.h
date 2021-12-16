
//////////////////////////////////////////////////////////////////////
//
//  CrySound Source Code
//
//  File: SoundSystemNULL.h
//  Description: NULL interface implementation.
//
//  History:
//	-June 06,2001:Implemented by Marco Corbetta
//	-Nov  20,2008:Renamed, refactored and moved to CrySystem by Tomas Neumann
//
//////////////////////////////////////////////////////////////////////

#ifndef _NULLSOUNDSYSTEM_H_
#define _NULLSOUNDSYSTEM_H_

#include "ISound.h"
#include "IReverbManager.h"
#include "ISoundMoodManager.h"
#include <IAudioDevice.h>
#include "NULLMusicSystem.h"
#include "../CrySoundSystem/SoundSystemCommon.h" // remove??
//#include "IReverbManager.h"
//#include "IRenderer.h"

// forward declaration
struct CRYSOUND_REVERB_PROPERTIES;
class CSoundSystem;
struct AudioDeviceSettings;
class CSoundBuffer;
struct SSoundBufferProps;
struct IPlatformSound;
class CSound;
enum enumAudioDeviceParamSemantics;

class CNULLAudioDevice : public IAudioDevice
{
	// this initializes the AudioDevice
	virtual bool InitDevice(CSoundSystem* pSoundSystem) { return true; }

	// loads a project (FEV file) and returns handle if Project is available 
	virtual ProjectIndex LoadProjectFile(const string &sProjectPath, const string &sProjectName, bool bAsync) { return (ProjectIndex) 1; }

	// releases the data of a project but keeps it known to the system 
	virtual bool UnloadProjectFile(const ProjectIndex nProjectIndex) { return true; }

	// retrieves the project entry by the project index 
	virtual SProjectFile* GetProjectFile(const ProjectIndex nProjectIndex) { return NULL; }

	// shut down and free audio device
	virtual bool ShutDownDevice(void) { return true; }

	// returns ptr to the sound library if possible, else NULL
	virtual void* GetSoundLibrary(void) { return NULL; }

	// returns ptr to the event system if possible, else NULL
	virtual void* GetEventSystem(void) { return NULL; }

	// returns the output handle if possible, else NULL
	// HandleType will specify if this a pointer to DirectX LPDIRECTSOUND or a WINMM handle
	// HandleType will be eOUTPUT_MAX if invalid
	virtual void GetOutputHandle( void **pHandle, void **pHandle2, EOutputHandle *HandleType) {}

	virtual void GetInitSettings(AudioDeviceSettings *InitSettings) {}
	virtual void SetInitSettings(AudioDeviceSettings *InitSettings) {}

	// major system attrib has been changed del all sounds clean system start
	virtual bool ResetAudio(void) { return true; }

	// this is called every frame to update all listeners and must be called *before* SubSystemUpdate()
	virtual bool UpdateListeners(void) { return true; }

	// this is called every frame to update audio device
	virtual bool Update(void) { return true; }

	// return current Mem_Usage
	virtual void GetMemoryStats(enumAudioDeviceMemoryQuery eMemoryQuery, unsigned int *nMemoryUsed) {}

	// returns percent of cpu usage 1.0 is 100%
	virtual bool GetCpuUsage(float *fDSP, float *fStream, float *fGeometry, float *fUpdate, float *fTotal) { return true; }

	// sets sound system global frequency this is the mixing value like fmod is 44100
	virtual bool SetFrequency(int newFreq) { return true; }

	// this sets up info for how many 2d and 3d hardware channels are setup
	virtual int  GetNumberSoundsPlaying(void) { return 0; }

	// compute memory-consumption, returns rough estimate in MB
	virtual int GetMemoryUsage(class ICrySizer* pSizer) const { return 0; }

	// returns number of open file handles opened by the AudioDevice
	virtual int GetNumOpenFileHandle() { return 0; }

	// accesses wavebanks
	virtual IWavebank* GetWavebank(int nIndex) { return NULL; }
	virtual IWavebank* GetWavebank(const char* sWavebankName) { return NULL; }
	virtual int GetWavebankCount() { return 0; }

	// this creates a platform dependent Soundbuffer
	virtual CSoundBuffer* CreateSoundBuffer(const SSoundBufferProps &BufferProps) { return NULL; }
	// this creates a platform dependent PlatformSound
	virtual IPlatformSound* CreatePlatformSound(CSound* pSound, const char *sEventName) { return NULL; }

	// check for multiple record devices and expose them, write name to sName pointer
	virtual bool GetRecordDeviceInfo(const int nRecordDevice, char* sName, const int nNameLength) { return true; }

	// sound system settings *** this is main io for setting and getting all audio parameters
	virtual bool GetParam(enumAudioDeviceParamSemantics enumtype, ptParam* pParam) { return true; } // get enum for type of info and ptr type of info gotton
	// the bool returns false if this info is available on this type
	virtual bool SetParam(enumAudioDeviceParamSemantics enumType, ptParam* pParam) { return true; }

	// writes output to screen in debug
	virtual void DrawInformation(IRenderer* pRenderer, float xpos, float ypos, int nSoundInfo) {}
};

class CNULLReverbManager : public IReverbManager
{
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const {}
	//////////////////////////////////////////////////////////////////////////
	// Initialization
	//////////////////////////////////////////////////////////////////////////
	virtual void Init(IAudioDevice *pAudioDevice, int nInstanceNumber, CSoundSystem* pSoundSystem) {}
	virtual bool SelectReverb(int nReverbType) { return true; }

	virtual void Reset() {}
	virtual void Release() {}

	virtual bool SetData( SReverbInfo::Data *pReverbData ) { return true; }

	//////////////////////////////////////////////////////////////////////////
	// Information
	//////////////////////////////////////////////////////////////////////////

	// writes output to screen in debug
	virtual void DrawInformation(IRenderer* pRenderer, float xpos, float ypos) {}

	// Draws the ray cast lines plus spheres at the collision points
	virtual void	DrawDynamicReverbDebugInfo(IRenderer* const pRenderer) {}

	//////////////////////////////////////////////////////////////////////////
	// Management
	//////////////////////////////////////////////////////////////////////////

	// needs to be called regularly
	virtual bool Update(bool bInside) { return true; }

	virtual bool SetListenerReverb(SOUND_REVERB_PRESETS nPreset, CRYSOUND_REVERB_PROPERTIES *tpProps=NULL, uint32 nFlags=0) { return true; }

	// Preset management
	virtual bool RegisterReverbPreset(char const* const pcPresetName=NULL, CRYSOUND_REVERB_PROPERTIES const* const pReverbProperties=NULL, CRYSOUND_ECHO_PROPERTIES const* const pEchoProperties=NULL) { return true; }
	virtual bool UnregisterReverbPreset(const char *sPresetName=NULL) { return true; }

	//! Registers an EAX Preset Area with the current blending weight (0-1) as being active
	//! morphing of several EAX Preset Areas is done internally
	//! Registering the same Preset multiple time will only overwrite the old one
	virtual bool RegisterWeightedReverbEnvironment(const char *sPreset=NULL, EntityId RegID=0, bool bFullEffectWhenInside=false, uint32 nFlags=0) { return true; }
	//virtual bool RegisterWeightedReverbEnvironment(const char *sPreset=NULL, CRYSOUND_REVERB_PROPERTIES *pProps=NULL, bool bFullEffectWhenInside=false, uint32 nFlags=0) = 0;
	//virtual bool RegisterWeightedReverbEnvironment(const char *sPreset=NULL, SOUND_REVERB_PRESETS nPreset, bool bFullEffectWhenInside=false, uint32 nFlags=0) = 0;

	//! Updates an EAX Preset Area with the current blending ratio (0-1)
	virtual bool UpdateWeightedReverbEnvironment(const char *sPreset=NULL, EntityId RegID=0, float fWeight=0.0) { return true; }

	//! Unregisters an active EAX Preset Area 
	virtual bool UnregisterWeightedReverbEnvironment(const char *sPreset=NULL, EntityId RegID=0) { return true; }

	//! Gets current EAX listener environment.
	virtual bool GetCurrentReverbEnvironment(SOUND_REVERB_PRESETS &nPreset, CRYSOUND_REVERB_PROPERTIES &Props) { return true; }

	//////////////////////////////////////////////////////////////////////////
	// Dependency with loading a sound
	//////////////////////////////////////////////////////////////////////////
	//! returns boolean if hardware reverb (EAX) is used or not
	virtual bool UseHardwareVoices() { return false; }


	virtual float GetEnvironment(ISound *pSound) { return 0.0f; }

	//virtual bool RegisterSound(ISound *pSound) { return true; }
	//virtual int  GetReverbInstance(ISound *pSound) const { return 0; }

	//virtual bool RegisterPlayer(EntityId PlayerID) { return true; }
	//virtual bool UnregisterPlayer(EntityId PlayerID) { return true; }

	virtual void SerializeState(TSerialize ser) {}
};


class CNULLSoundMoodManager : public ISoundMoodManager
{
	//////////////////////////////////////////////////////////////////////////
	// Initialization
	//////////////////////////////////////////////////////////////////////////

	virtual bool RefreshCategories(IMood *pMood) { return true; }
	virtual bool RefreshPlatformCategories() { return true; }
	virtual void Release() {}

	virtual void Reset() {} // removes all soundmoods except "default" and resets the effect

	//////////////////////////////////////////////////////////////////////////
	// Memory Usage
	//////////////////////////////////////////////////////////////////////////

	// Returns current Memory Usage
	virtual uint32 GetMemUsage(void) const { return 0; }

	//////////////////////////////////////////////////////////////////////////
	// Information
	//////////////////////////////////////////////////////////////////////////

	virtual uint32 GetNumberSoundsLoaded(void) const { return 0; }

	// writes output to screen in debug
	virtual void DrawInformation(IRenderer* pRenderer, float &xpos, float &ypos, int nSoundInfo) {}

	virtual void GetMemoryUsage(ICrySizer *pSizer) const{}
	//////////////////////////////////////////////////////////////////////////
	// Management
	//////////////////////////////////////////////////////////////////////////

	// adds/removes a Mood to the MoodManager database
	virtual IMood*			AddMood(const char *sMoodName) { return NULL; }
	virtual bool				RemoveMood(const char *sMoodName){ return true; }

	//////////////////////////////////////////////////////////////////////////
	// Real Time Manipulation
	//////////////////////////////////////////////////////////////////////////

	// needs to be updated regularly
	virtual bool Update() { return true; }

	// registers a Mood to be actively processes (starts with 0 fade value)
	virtual bool	RegisterSoundMood(const char *sMoodName) { return true; }

	// updates the fade value of a registered mood
	virtual bool	UpdateSoundMood(const char *sMoodName, float fFade, uint32 nFadeTimeInMS, bool bUnregistedOnFadeOut=true) { return true; }

	// get current fade value of a registered mood
	virtual float	GetSoundMoodFade(const char *sMoodName) { return 0.0f; }

	// unregisters a Mood and removes it from the active ones
	virtual bool	UnregisterSoundMood(const char *sMoodName) { return true; }

	//////////////////////////////////////////////////////////////////////////
	// Access
	//////////////////////////////////////////////////////////////////////////

	virtual IMood*			GetMoodPtr(uint32 nGroupCount) const { return NULL; }
	virtual IMood*			GetMoodPtr(const char *sMoodName) const { return NULL; }
	virtual IMood*			GetMixMoodPtr() const { return NULL; }

	//////////////////////////////////////////////////////////////////////////
	// Import/Export
	//////////////////////////////////////////////////////////////////////////

	virtual bool Serialize(XmlNodeRef &node, bool bLoading) { return true; }
	virtual void SerializeState(TSerialize ser) {}

	//Other
	virtual bool RefreshCategories() { return true; }
};


class CNULLListener : public IListener
{
private:

	CNULLListener(const CNULLListener&) {}

public:
	Matrix34	  TransformationNewest;
	Matrix34	  TransformationSet;

	float				fUnderwater;
	IVisArea*		pVisArea;
	Vec3				vVelocity;
	ListenerID	nListenerID;	
	EntityId    nEntityID;
	PodArray<IVisArea*>* pVisAreas;
	float				fRecordLevel;	
	bool				bActive;	
	bool				bMoved;
	bool				bRotated;
	bool				bDirty;
	bool				bInside;


	// Constructor
	CNULLListener() 
	{
		nListenerID		= LISTENERID_INVALID;
		vVelocity			= Vec3(0,0,0);
		fRecordLevel	= 0.0f;
		bActive				= true;
		bMoved				= false;
		bRotated			= false;
		bDirty				= true;
		bInside				= false;
		fUnderwater   = 1.0f;
		nEntityID     = 0;
		pVisArea			= NULL;
		TransformationSet.SetIdentity();
		TransformationNewest.SetIdentity();

		pVisAreas			= new (PodArray<IVisArea*>);
		//VisAreas.PreAllocate(MAX_VIS_AREAS);
	}
	~CNULLListener()
	{
		if (pVisAreas)
		{
			pVisAreas->clear();
			delete pVisAreas;
		}
	}

	virtual ListenerID GetID() const
	{
		return nListenerID;
	}

	virtual EntityId GetEntityID() const
	{
		return nEntityID;
	}

	virtual bool GetActive() const
	{
		return bActive;
	}

	virtual void SetActive(bool bNewActive)
	{
		bActive = bNewActive;
	}

	virtual void SetRecordLevel(float fRecord)
	{
		fRecordLevel = fRecord;
	}

	virtual float GetRecordLevel()
	{
		return fRecordLevel;
	}

	virtual Vec3 GetPosition() const
	{
		return TransformationSet.GetColumn3();
	}

	virtual void SetPosition(const Vec3 Position)
	{
		TransformationNewest.SetColumn(3, Position);
		bDirty = true;
	}

	virtual Vec3 GetForward() const
	{ 
		Vec3 ForwardVec(0,1,0); // Forward.
		ForwardVec = TransformationSet.TransformVector(ForwardVec);
		ForwardVec.Normalize();
		return ForwardVec;
	}

	virtual Vec3 GetTop() const
	{
		Vec3 TopVec(0,0,1); // Up.
		TopVec = TransformationSet.TransformVector(TopVec);
		TopVec.Normalize();
		return TopVec;
	}

	virtual Vec3 GetVelocity() const
	{
		return vVelocity;
	}

	virtual void SetVelocity(Vec3 vVel)
	{
		vVelocity = vVel;
	}

	virtual void SetMatrix(const Matrix34 newTransformation)
	{
		TransformationNewest = newTransformation;
	}

	virtual Matrix34 GetMatrix() const
	{
		return TransformationSet;
	}

	virtual float GetUnderwater() const
	{
		return fUnderwater;
	}

	virtual void SetUnderwater(const float fUnder)
	{
		fUnderwater = fUnder;
	}

	virtual IVisArea* GetVisArea() const
	{
		return pVisArea;
	}

	virtual void SetVisArea(IVisArea* pVArea)
	{
		pVisArea = pVArea;
	}
};
 
/////////////////////////////////////////////////////////////////
class CNULLSound : public ISound_Extended, public ISound_Deprecated
{
public:
	CNULLSound()
	{		
		m_refCount = 0;
		m_mTransformation.SetIdentity();
	}

	~CNULLSound(){}
	
	//// ISound //////////////////////////////////////////////////////

	// CORE FUNCTIONALITY
	virtual void	SetSemantic(const ESoundSemantic eSemantic) {} 
	virtual const ESoundSemantic	GetSemantic() const { return eSoundSemantic_None; }
	void AddEventListener( ISoundEventListener *pListener, const char *sWho ) {};
	void RemoveEventListener( ISoundEventListener *pListener ) {};	
	
	// Add/remove sounds. // This is used in the smart_ptr
	int		AddRef() { return ++m_refCount; };
	int		Release()
	{
		int ref = --m_refCount;
		return ref;
	};
	
	// PLAYBACK CONTROL
	void	Play(const float fVolumeScale=1.0f, const bool bForceActiveState=true, const bool bSetRatio=true, IEntitySoundProxy *pEntitySoundProxy=NULL) {}
	void	Stop(const ESoundStopMode eStopMode=ESoundStopMode_EventFade) {}
	void	SetPaused(const bool bPaused) {}
	const bool GetPaused() const {return false;}
	//! Fading In/Out - 0.0f is Out 1.0f is In
	virtual void				SetFade(const float fFadeGoal, const int nFadeTimeInMS) {}
	virtual EFadeState	GetFadeState() const { return eFadeState_None; }

	// SPATIALIZATION
	void	SetPosition(const Vec3 &pos){}
	Vec3  GetPosition() const { return Vec3(0); }
	virtual void			SetMatrix(const Matrix34 newTransformation) { m_mTransformation = newTransformation; }
	virtual Matrix34	GetMatrix() const { return m_mTransformation; }
	virtual void			SetDirection(const Vec3 &vDir) {}
	virtual Vec3			GetForward() const {return (Vec3(0,1,0));}	

	// MANIPULATION

	// Gets and Sets Parameter defined in the enumAssetParam list
	virtual bool	GetParam(enumSoundParamSemantics eSemantics, ptParam* pParam) const {return (false);}
	virtual bool	SetParam(enumSoundParamSemantics eSemantics, ptParam* pParam) {return (false);}

	// Gets and Sets Parameter defined by string and float value, returns the index of that parameter
	virtual int		GetParam(const char *sParameter, float *fValue, bool bOutputWarning) const {return (-1);}
	virtual int		SetParam(const char *sParameter, float fValue, bool bOutputWarning) {return (-1);}

	// Gets and Sets Parameter defined by index and float value
	virtual bool	GetParam(int nIndex, float* const pfValue, float* const pfRangeMin=NULL, float* const pfRangeMax=NULL, char const** const ppcParameterName=NULL, bool const bOutputWarning=true) const {return false;}
	virtual bool	SetParam(int nIndex, float fValue, bool bOutputWarning) {return (false);}
	
	void  SetDistanceMultiplier(const float) {}

	// INFORMATION
	const tSoundID	GetId() const {return INVALID_SOUNDID; }
	const char* GetName() {return "dummy sound";}
	uint32	GetFlags() const			{ return 0; };
	bool	IsPlaying() const { return false; }
	bool	IsPlayingVirtual() const { return false; }
	bool	IsLoading() const { return false; }
	bool	IsLoaded() const { return true; }
	int GetLengthMs() const { return(0); } //! returns the size of the stream in ms
	int GetLengthInBytes() const { return(0); } //! returns the size of the stream in bytes
	float GetMaxDistance() const { return 1.0f; }
	bool  IsRelative() const { return false; };

	// EXTENDED CONTROL
	virtual void SetLineSpec(const Vec3 &vStart, const Vec3 &vEnd) {} 
	virtual bool GetLineSpec(  Vec3 &vStart,   Vec3 &vEnd) { vStart = Vec3(0); vEnd = Vec3(0); return false;}
	virtual void SetSphereSpec(const float fRadius) {}
	void SetPhysicsToBeSkipObstruction(EntityId *pSkipEnts,int nSkipEnts) {}
	bool  UnloadBuffer(eUnloadDataOptions UnloadOption) {return true;}

	// INTERFACES
	virtual ISound_Extended* GetInterfaceExtended() {return this;}
	virtual ISound_Deprecated* GetInterfaceDeprecated() {return this;}

	//// ISOUND EXTENDED //////////////////////////////////////////////////////

	// PLAYBACK CONTROL
	void	SetVolume(const float) {}
	float	GetVolume() const { return(0); }
	void		SetFlags(uint32 nFlags)	{};
	void  SetPitch(int nPitch) {}
	void	SetVelocity(const Vec3 &vVel) {}
	Vec3	GetVelocity() const {return (Vec3(0,0,0));}
	void	SetRatio(float fRatio){}
	
	// MANIPULATION

	//! set the currently played sample-pos in bytes or milliseconds
	void SetCurrentSamplePos(unsigned int nPos,bool bMilliSeconds) {}

	//! retrieves the currently played sample-pos, in milliseconds or bytes
	unsigned int GetCurrentSamplePos(bool bMilliSeconds=false) const { return (0);}

	void	SetSoundPriority(int nSoundPriority) {} ;

	void	SetConeAngles(const float, const float) {}
	void	GetConeAngles(float&, float&) {}

	bool  Preload() { return true; }

	//! Set panning values (-1.0 left, 0.0 center, 1.0 right)
	virtual void	SetPan(float fPan) {}
	virtual float GetPan() const { return 0.0f; }

	//! Set 3d panning values (0.0 no directional, 1.0 full 3d directional effect)
	virtual void	Set3DPan(float f3DPan) {}
	virtual float	Get3DPan() const { return 0.0f; }

	// INFORMATION
	SObstruction* GetObstruction( void ) { return NULL; }		//! Get sound obstruction struct.
	virtual bool IsInCategory(const char* sCategory) { return false; }

	//// ISOUND DEPRECATED //////////////////////////////////////////////////////
	void	SetPitching(float fPitching){}
	void	SetMinMaxDistance(float fMinDist, float fMaxDist){}
	void	FXEnable(int nEffectNumber) {}
	void	FXSetParamEQ(float fCenter,float fBandwidth,float fGain) {}
	void	AddToScaleGroup(int nGroup) {}
	void	RemoveFromScaleGroup(int nGroup) {}
	void	SetScaleGroup(unsigned int nGroupBits) {}
	void	SetLoopMode(bool bLoop){}
	int		GetFrequency() const { return (0);}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	Vec3 m_vDummy;	
	int		m_refCount;	
	Matrix34 m_mTransformation;
};


/////////////////////////////////////////////////////////////////
class CNULLSoundSystem :	public ISoundSystem_Extended,
													public ISoundSystem_Deprecated,
													public ISoundSystem_WorkInProgress
{
public:
	CNULLSoundSystem()
	{
		m_NULLListener.bActive = false;
	}

	~CNULLSoundSystem()
  {
  }

	bool IsOK() { return true; }

	//// ISoundSystem ///////////////////////////////////////////////
	bool    Init(){ return true; }
	void		Release(){}
	void		Update(ESoundUpdateMode UpdateMode) {}
	ESoundSystemErrorCode GetError() { return eSoundSystemErrorCode_None; }

	IMusicSystem* CreateMusicSystem() { return &m_NULLMusicSystem; }
	IAudioDevice*		GetIAudioDevice()				const { return (IAudioDevice*) &m_NULLAudioDevice; }
	ISoundMoodManager* GetIMoodManager()		const { return (ISoundMoodManager*)&m_NULLSoundMoodManager; }
	IReverbManager* GetIReverbManager()			const { return (IReverbManager*)&m_NULLReverbManager; }

	// Register listener to the sound.
	virtual void AddEventListener( ISoundSystemEventListener *pListener, bool bOnlyVoiceSounds ) {}
	virtual void RemoveEventListener( ISoundSystemEventListener *pListener ) {}

	//ISound* LoadSound(const char *szFile, uint32 nFlags) { return &m_sndDummy; }
	void		RemoveSound(int nSoundID){}
	ISound* GetSound(tSoundID nSoundID) const					{ return (ISound*) &m_NULLSound; }
	void SetMasterVolume(float fVol)					{ }
	void SetMasterVolumeScale(float fScale, bool bForceRecalc=false){}
	virtual float GetSFXVolume() { return 1.0f; }
	virtual void SetMovieFadeoutVolume(const float movieFadeoutVolume) {}
	virtual float GetMovieFadeoutVolume() const {return 1.0f;}
	void SetSoundActiveState(ISound *pSound, ESoundActiveState State) {}
	void SetMasterPitch(float fPitch)					{ }
	void AddSoundFlags(int nSoundID, uint32 nFlags){}

	void	GetOutputHandle( void **pHandle, void **pHandle2, EOutputHandle *HandleType) const
	{
			*pHandle = NULL;
			*pHandle2 = NULL;
			*HandleType = eOUTPUT_MAX;
	}
	//void		PlaySound(tSoundID nSoundID) {}

	EPrecacheResult Precache( const char *sGroupAndSoundName, uint32 nSoundFlags, uint32 nPrecacheFlags ) {return ePrecacheResult_OK;}
	virtual ISound*	CreateSound( const char *sGroupAndSoundName, uint32 nFlags )		{return &m_NULLSound; }
	virtual ISound* CreateLineSound		( const char *sGroupAndSoundName, uint32 nFlags, const Vec3 &vStart, const Vec3 &VEnd ) {return &m_NULLSound; }
	virtual ISound* CreateSphereSound	( const char *sGroupAndSoundName, uint32 nFlags, const float fRadius ) {return &m_NULLSound; }


	bool				SetListener(const ListenerID nListenerID, const Matrix34 &matOrientation, const Vec3 &vVel, bool bActive, float fRecordLevel) { return true; }
	virtual void SetListenerEntity( ListenerID nListenerID, EntityId nEntityID ) {}
	ListenerID	CreateListener() { return 	LISTENERID_INVALID; }
	bool				RemoveListener(ListenerID nListenerID)					{ return true; }
	ListenerID	GetClosestActiveListener(Vec3 vPosition) const	{ return LISTENERID_INVALID; }
	IListener*	GetListener(ListenerID nListenerID)							{ return &m_NULLListener; }
	IListener*	GetListener(EntityId nEntityID)									{ return &m_NULLListener; }
	IListener*	GetNextListener(ListenerID nListenerID)					{ return NULL; }
	uint32			GetNumActiveListeners() const										{ return 1; }


	bool	PlayMusic(const char *szFileName) { return (false); }
	void	StopMusic() {}
	bool	Silence(bool bStopLoopingSounds, bool bUnloadData) {return true;}	
	bool DeactivateAudioDevice() {return true;}
	bool ActivateAudioDevice() {return true;}

	virtual void	Pause(bool bPause, bool bResetVolume=false, unsigned int nFadeOutInMS=0) {}
	virtual bool  IsPaused() {return (false); }
	void	Mute(bool bMute) {}
	
	//! Check for EAX support.
	bool IsEAX(void) { return(false); }

	//! Set EAX listener environment.
	bool SetEaxListenerEnvironment(int nPreset,CRYSOUND_REVERB_PROPERTIES *pProps=NULL, uint32 nFlags=0) { return(false); }

	// added by Tomas
	//! Registers an EAX Preset Area as being active
	//! morphing of several EAX Preset Areas is done internally
	//! Registering the same Preset multiple time will only overwrite the old one
	//bool RegisterWeightedEaxEnvironment(const char *sPreset=NULL, CRYSOUND_REVERB_PROPERTIES *pProps=NULL, bool bFullEffectWhenInside=false, int nFlags=0) { return(false); }

	//! Updates an EAX Preset Area with the current blending ratio (0-1)
	//bool UpdateWeightedEaxEnvironment(const char *sPreset=NULL, float fWeight=0.0) { return(false); }

	//! Unregisters an active EAX Preset Area 
	//bool UnregisterWeightedEaxEnvironment(const char *sPreset=NULL) { return(false); }

	//! Gets current EAX listener environment.
	//bool GetCurrentEaxEnvironment(int &nPreset, CRYSOUND_REVERB_PROPERTIES &Props) { nPreset=0;return (true);}

	//! sets the weather condition that affect acoustics
	bool SetWeatherCondition(float fWeatherTemperatureInCelsius, float fWeatherHumidityAsPercent, float fWeatherInversion) { return(false); }

	//! gets the weather condition that affect acoustics
	bool GetWeatherCondition(float &fWeatherTemperatureInCelsius, float &fWeatherHumidityAsPercent, float &fWeatherInversion) { return(false); }

	//! Sets the options for a group of sounds to animate its behavior
	//bool SetSoundGroupProperties(size_t eSoundGroup, SSoundGroupProperties *pSoundGroupProperties) {return false;}

	//! Sets the options for a group of sounds to animate its behavior
	//bool GetSoundGroupProperties(size_t eSoundGroup, SSoundGroupProperties &pSoundGroupProperties) {return false;}

	bool SetGroupScale(int nGroup, float fScale) { return true; }
	void RecomputeSoundOcclusion(bool bRecomputeListener, bool bForceRecompute, bool bReset) {}

	//! get memory usage info
	void	GetSoundMemoryUsageInfo(int *nCurrentMemory,int *nMaxMemory) const { *nCurrentMemory = *nMaxMemory=0;	}

  void SetMusicEffectsVolume(float) {}

	int	GetUsedVoices() const { return 0; }
	float	GetCPUUsage() const { return 0.0f; }
	float GetMusicVolume() const { return 1.0f; }
	void CalcDirectionalAttenuation(const Vec3 &Pos, const Vec3 &Dir, const float fConeInRadians) {}
	float GetDirectionalAttenuationMaxScale() { return 0.0f; }
	bool UsingDirectionalAttenuation() { return false; }
	virtual void GetMemoryUsage(class ICrySizer* pSizer) const {}
	virtual int GetMemoryUsageInMB(bool) { return 0; }

	//! get the current area the listener is in
	//virtual IVisArea	*GetListenerArea() { return(NULL); }

	virtual bool DebuggingSound() { return false; }

	int SetMinSoundPriority( int nPriority ) { return 0; };
	const char* MakeDialogKey(const char* sDialogFileName) { return sDialogFileName; } // TODO real implementation
	
	void LockResources() {};
	void UnlockResources() {};
	void SetGlobalParameter( const char *sGlobalParameter, const float fValue) {};

	void TraceMemoryUsage(int nMemUsage) {};

	//! Profiling Sounds
	virtual ISoundProfileInfo* GetSoundInfo(int nIndex) { return NULL; }
	virtual ISoundProfileInfo* GetSoundInfo(const char* sSoundName) { return NULL; }
	virtual int GetSoundInfoCount() { return 0; }

	virtual bool GetRecordDeviceInfo(const int nRecordDevice, char* sName, int nNameLength) { return false; }
	virtual IMicrophone* CreateMicrophone(
		const unsigned int nRecordDevice,
		const unsigned int nBitsPerSample, 
		const unsigned int nSamplesPerSecond,
		const unsigned int nBufferSizeInSamples) { return NULL; }
	virtual bool RemoveMicrophone( IMicrophone *pMicrophone ) {return false;}
	virtual ISound* CreateNetworkSound(	INetworkSoundListener *pNetworkSoundListener,
		const unsigned int nBitsPerSample, 
		const unsigned int nSamplesPerSecond,
		const unsigned int nBufferSizeInSamples,
		EntityId PlayerID) { return NULL; }
	virtual void RemoveNetworkSound (ISound *pSound) {}

	virtual void Serialize(TSerialize ser) {}

	virtual bool IsNullImplementation() const { return true; }

	// Debuggin
	virtual void Log( const ESoundLogType eType, const char *szFormat,... )
	{

		if (!szFormat)
			return;

		char szBuffer[MAX_WARNING_LENGTH];
		va_list	ArgList;
		va_start(ArgList, szFormat);
		int count = vsnprintf_s(szBuffer,sizeof(szBuffer),sizeof(szBuffer)-1,szFormat, ArgList);
		szBuffer[sizeof(szBuffer)-1] = '\0';
		va_end(ArgList);

		char *szString = szBuffer;

		if (gEnv->pLog)
			gEnv->pLog->Log("[NULLSoundSystem] %s", szString);
	}

	//////////////////////////////////////////////////////////////////////////
	// Interfaces
	//////////////////////////////////////////////////////////////////////////

	virtual ISoundSystem_Extended* GetInterfaceExtended() {return this;}
	virtual ISoundSystem_Deprecated* GetInterfaceDeprecated() {return this;}
	virtual ISoundSystem_WorkInProgress* GetInterfaceWorkInProgress() {return this;}

private:
	CNULLSound m_NULLSound;
	CNULLListener m_NULLListener;
	CNULLAudioDevice m_NULLAudioDevice;
	CNULLMusicSystem m_NULLMusicSystem;
	CNULLReverbManager m_NULLReverbManager;
	CNULLSoundMoodManager m_NULLSoundMoodManager;
};


#endif //_NULLSOUNDSYSTEM_H_
