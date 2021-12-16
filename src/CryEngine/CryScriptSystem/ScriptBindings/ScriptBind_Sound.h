////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   ScriptBind_Sound.h
//  Version:     v1.00
//  Created:     8/7/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ScriptBind_Sound_h__
#define __ScriptBind_Sound_h__
#pragma once


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <IScriptSystem.h>

struct IMusicSystem;
struct ISoundSystem;
struct ISound;

/*
	<title Sound>
	Syntax: Sound

	In this class are all sound-related script-functions implemented.

	IMPLEMENTATIONS NOTES:
	These function will never be called from C-Code. They're script-exclusive.
*/
class CScriptBind_Sound : public CScriptableBase
{
public:
	CScriptBind_Sound(IScriptSystem *pScriptSystem, ISystem *pSystem);
	virtual ~CScriptBind_Sound();
	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_sCurrentMusicTheme);
		pSizer->AddObject(m_sCurrentMusicMood);
	}

	//////////////////////////////////////////////////////////////////////////
	// Comment on new SoundGroupSystem: DO NOT USE FILE EXTENSIONS ANYMORE
	// example: Sounds/Weapons/Scar:reload
	//////////////////////////////////////////////////////////////////////////
	
	// <title Precache>
	// Syntax: Sound.Precache( const char *sGroupAndSoundName, int nPrecacheFlags )
	// Description:
	//    Precaches the sound data used by a sound definition
	//    Depending on additional flags and current settings may load the sound data into the memory for later usage.
	//    Precaching is optional, sound system can also play not cached sounds, but it can cause small delay before sound playing starts.
	// Arguments:
	//    sGroupAndSoundName - SoundGroup name of the sound definition or direct filename the sound data should be loaded
	//    nPrecacheFlags - Precache flags, specify sound file caching priority.
	int Precache( IFunctionHandler *pH, const char *sSoundOrEventName, uint32 nPrecacheFlags );

	// <title Play>
	// Syntax: Sound.Play( const char *sGroupAndSoundName,Vec3 vPos, int nSoundFlags )
	// Description:
	//    Uses the sound definition of the Handle to play a sound at the position vPos .
	//		It returns a unique SoundID that is valid as long as the sound is played.
	// Arguments:
	//    sGroupAndSoundName -  SoundGroup name of the sound definition or direct filename of a sound
	//    vPos - Position where to play sound.
	//    nSoundFlags - additional Sound flags, specify how to play sound.
	// Return Value:
	//    SoundID.
	int Play( IFunctionHandler *pH, const char *sSoundOrEventName, Vec3 vPos, uint32 nSoundFlags, uint32 nSemantic );

	// <title PlayEx>
	// Syntax: Sound.PlayEx( const char *sGroupAndSoundName,int nSoundFlags, float fVolume, Vec3 pos,float minRadius,float maxRadius )
	// Description:
	//    Play sound file.
	// Arguments:
	//    sGroupAndSoundName -  SoundGroup name of the sound definition or direct filename of a sound
	//    vPos - Position where to play sound.
	//    nSoundFlags - Sound flags, specify how to play sound.
	//    nSemantic - Semantical information what this sound is.
	// Return Value:
	//    SoundID.
	int PlayEx( IFunctionHandler *pH, const char *sSoundOrEventName, Vec3 vPos, uint32 nSoundFlags, float fVolume, float minRadius, float maxRadius, uint32 nSemantic );

	// <title SetParameterValue>
	// Syntax: Sound.SetParameterValue( int SoundID, const char *sParameterName, float fParameterValue )
	// Description:
	//    Changes a parameter value of a specific sound (if supported)
	// Arguments:
	//    SoundID - unique SoundID of the sound
	//    sParameterName - name of the parameter to be changed ("distance"/"pitch")
	//    fParameterValue - value of the parameter to be set, always float
	//    nSemantic - Semantical information what this sound is.
	// Return Value:
	//    bool - returns true if the SoundID was valid and the parameter value could be set
	int SetParameterValue( IFunctionHandler *pH, ScriptHandle SoundID, const char *sParameterName, float fParameterValue );

	// <title Silence>
	// Syntax: Sound.Silence()
	// Description:
	//		Silences the sound.
	// Return Value:
	//		Bool.
	int Silence(IFunctionHandler *pH); 
	// <title DeactivateAudioDevice>
	// Syntax: Sound.DeactivateAudioDevice()
	// Description:
	//		Deactivates the audio device.
	// Return Value:
	//		Bool.
	int DeactivateAudioDevice(IFunctionHandler *pH); 
	// <title ActivateAudioDevice>
	// Syntax: Sound.ActivateAudioDevice()
	// Description:
	//		Activates audio device.
	// Return Value:
	//		Bool.
	int ActivateAudioDevice(IFunctionHandler *pH);
	// <title UnloadProjects>
	// Syntax: Sound.UnloadProjects()
	// Description:
	//		Unloads the projects.
	// Return Value:
	//		Bool.
	int UnloadProjects(IFunctionHandler *pH); 

	// <title IsPlaying>
	// Syntax: Sound.IsPlaying()
	// Description:
	//		Checks if the sound is playing.
	// Return Value:
	//		Integer.
	int IsPlaying(IFunctionHandler *pH);
	// <title IsEvent>
	// Syntax: Sound.IsEvent()
	// Description:
	//		Checks if the sound is an event.
	//		Checks for EVENT flag.
	int IsEvent(IFunctionHandler *pH, ScriptHandle SoundID);
	// <title IsVoice>
	// Syntax: Sound.IsVoice()
	// Description:
	//		Checks if the sound is a voice.
	//		Checks for VOICE flag.
	int IsVoice(IFunctionHandler *pH, ScriptHandle SoundID); 

	// <title LoadSound>
	// Syntax: Sound.LoadSound(const char *sound, uint32 flags)
	// Arguments:
	//		sound - Sound name.
	//		flags - Sound flags.
	// Description:
	//		Loads the specified sound.
	// Return Value:
	//		Integer.
	int LoadSound(IFunctionHandler *pH);
	// <title LoadSound>
	// Syntax: Sound.LoadSound(const char *sound, [optional] uint32 flags, [optional] float volume, [optional] float min, [optional] float clipDistance, [optional] int priority, [optional] int group)
	// Arguments:
	//		sound			- Sound name.
	//		flags			- Sound flags.
	//		volume			- Volume.
	//		min				- Minimum distance.
	//		clipDistance	- Clip distance.
	//		priority		- Priority of the sound.
	//		group			- Group.
	// Description:
	//		Loads a 3D sound.
	// Return Value:
	//		Integer.
	int Load3DSound(IFunctionHandler *pH); 
	// <title LoadStreamSound>
	// Syntax: Sound.LoadStreamSound(const char *sound, uint32 flags)
	// Arguments:
	//		sound			- Sound name.
	//		flags			- Sound flags.
	// Description:
	//		Loads a stream sound.
	// Return Value:
	//		Integer.
	int LoadStreamSound(IFunctionHandler *pH);
	// <title PlaySound>
	// Syntax: Sound.PlaySound(ISound *sound, float volumeScale)
	// Arguments:
	//		sound		- Sound identifier.
	//		volumeScale - Volume scale amount.
	// Description:
	//		Plays a sound.
	int PlaySound(IFunctionHandler *pH);
	// <title SetSoundVolume>
	// Syntax: Sound.SetSoundVolume(ISound* sound, float volume)
	// Arguments:
	//		sound	- Sound identifier.
	//		volume	- Volume amount.
	// Description:
	//		Sets sound volume.
	int SetSoundVolume(IFunctionHandler *pH);
	// <title GetSoundVolume>
	// Syntax: Sound.GetSoundVolume(ISound* sound)
	// Arguments:
	//		sound	- Sound identifier.
	// Description:
	//		Gets sound volume.
	// Return Value:
	//		Float.
	int GetSoundVolume(IFunctionHandler *pH);
	// <title SetSoundLoop>
	// Syntax: Sound.SetSoundLoop(ISound* sound)
	// Arguments:
	//		sound	- Sound identifier.
	//		flags	- Sound flags to activates/deactivates the loop playback.
	// Description:
	//		Sets the loop flag to a sound.
	int SetSoundLoop(IFunctionHandler *pH);
	// <title SetSoundFrequency>
	// Syntax: Sound.SetSoundFrequency(ISound* sound, int frequency)
	// Arguments:
	//		sound		- Sound identifier.
	//		frequency	- Sound frequency.
	// Description:
	//		Sets the sound frequency.
	int SetSoundFrequency(IFunctionHandler *pH);
	// <title SetSoundPitching>
	// Syntax: Sound.SetSoundPitching(ISound* sound, float pitching)
	// Arguments:
	//		sound		- Sound identifier.
	//		pitching	- Sound pitching.
	// Description:
	//		Sets the sound pitching.
	int SetSoundPitching(IFunctionHandler *pH);
	// <title SetSoundPaused>
	// Syntax: Sound.SetSoundPaused(ISound* sound, bool paused)
	// Arguments:
	//		sound		- Sound identifier.
	//		paused		- True to pause the sound, false to resume it.
	// Description:
	//		Pauses/resumes the sound.
	int SetSoundPaused(IFunctionHandler *pH);
	// <title SetSoundRelative>
	// Syntax: Sound.SetSoundRelative(ISound* sound, bool isRelative)
	// Arguments:
	//		sound		- Sound identifier.
	//		isRelative	- True if the pause is relative, false otherwise.
	// Description:
	//		Sets/unsets the RELATIVE Flag.
	//		If unset, a new valid position has to be set, or the sound stays at the last listener position.
	int SetSoundRelative(IFunctionHandler *pH);
	// <title StopSound>
	// Syntax: Sound.StopSound(ISound* sound)
	// Arguments:
	//		sound	- Sound identifier.
	// Description:
	//		Stops the sound.
	//		nSoundHandle can be SoundID or Ptr.
	int StopSound(IFunctionHandler *pH);
	// <title SetSoundPosition>
	// Syntax: Sound.SetSoundPosition(ISound* sound, Vect3 pos)
	// Arguments:
	//		sound	- Sound identifier.
	//		pos		- Position of the sound.
	// Description:
	//		Sets the position of the sound.
	//		nSoundHandle can be SoundID or Ptr.
	int SetSoundPosition(IFunctionHandler *pH);
	// <title SetSoundSpeed>
	// Syntax: Sound.SetSoundSpeed(ISound* sound, Vect3 speed)
	// Arguments:
	//		sound - Sound identifier.
	//		speed - Sound speed.
	// Description:
	//		Sets sound speed.
	int SetSoundSpeed(IFunctionHandler *pH);
	// <title SetMinMaxDistance>
	// Syntax: Sound.SetMinMaxDistance(ISound* sound, float minDist, float maxDist)
	// Arguments:
	//		sound	- Sound identifier.
	//		minDist	- Minimum distance.
	//		maxDist - Maximum distance.
	// Description:
	//		Sets the minimum and the maximum distances for the sound.
	int SetMinMaxDistance(IFunctionHandler *pH);
	//int SetLoopPoints(IFunctionHandler *pH); // int int int (return void)

	// <title SetFadeTime>
	// Syntax: Sound.SetFadeTime(ISound* sound, float fadeGoal, float fadeTimeInMS)
	// Arguments:
	//		sound		 - Sound identifier.
	//		fadeGoal	 - Final value for the sound fading.
	//		fadeTimeInMS - Fading time in millisecond.
	// Description:
	//		Sets the fade time for the sound.
	// Return Value:
	//		Time in milliseconds;
	int SetFadeTime(IFunctionHandler *pH);
	// <title AddSoundFlags>
	// Syntax: Sound.AddSoundFlags()
	// Remarks:
	//		Why is it commented out in.cpp? TN
	int AddSoundFlags(IFunctionHandler *pH); 
	// <title SetMasterVolumeScale>
	// Syntax: Sound.SetMasterVolumeScale(float scale)
	// Arguments:
	//		scale - Master volume scale value for the sound.
	// Description:
	//		Sets the master volume scale.
	int SetMasterVolumeScale(IFunctionHandler *pH);
	// <title SetMasterMusicEffectsVolume>
	// Syntax: Sound.SetMasterMusicEffectsVolume(float newMusicVol)
	// Arguments:
	//		newMusicVol - New master music effect volume value.
	int SetMasterMusicEffectsVolume(IFunctionHandler *pH);

	// <title SetPauseAllPlaying>
	// Syntax: Sound.SetPauseAllPlaying( bool bPause )
	// Arguments:
	//		bPause - True to pause all the playing sounds, false to resume them.
	// Description:
	//		Sets/unsets pause for all the playing sounds.
	int	SetPauseAllPlaying(IFunctionHandler *pH, bool bPause ); 
	

	// group stuff

	// <title AddToScaleGroup>
	// Syntax: Sound.AddToScaleGroup( ISound* sound, int group )
	// Arguments:
	//		sound - Sound identifier.
	//		group - Group number.
	// Description:
	//		Adds the sound to a scale group.
	int AddToScaleGroup(IFunctionHandler *pH);
	// <title RemoveFromScaleGroup>
	// Syntax: Sound.RemoveFromScaleGroup( ISound* sound, int group )
	// Arguments:
	//		sound - Sound identifier.
	//		group - Group number.
	// Description:
	//		Removes the sound from the scale group.
	int RemoveFromScaleGroup(IFunctionHandler *pH);
	// <title SetGroupScale>
	// Syntax: Sound.SetGroupScale( int group, float scale )
	// Arguments:
	//		group - Group number.
	//		scale - Scale value.
	// Description:
	//		Sets values for a group scale.
	// Return Value:
	//		Bool.
	int SetGroupScale(IFunctionHandler *pH);

	// <title RegisterWeightedEaxEnvironment>
	// Syntax: Sound.RegisterWeightedEaxEnvironment( const char *sPresetName, int nFullEffectWhenInside, uint32 nFlags )
	// Arguments:
	//		sPresetName				- Preset name.
	//		nFullEffectWhenInside	- .
	//		nFlags					- .
	// Return Value:
	//		Bool.
	// Description:
	//		Registers a weighted EAX (Environmental Audio Extension) environment.
	int	RegisterWeightedEaxEnvironment(IFunctionHandler *pH);  
	// <title UpdateWeightedEaxEnvironment>
	// Syntax: Sound.UpdateWeightedEaxEnvironment( const char *sPresetName, float fWeight )
	// Arguments:
	//		sPresetName	- Preset name.
	//		fWeight		- .
	// Return Value:
	//		Bool.
	// Description:
	//		Updates a weighted EAX (Environmental Audio Extension) environment.
	int	UpdateWeightedEaxEnvironment(IFunctionHandler *pH); 
	// <title UnregisterWeightedEaxEnvironment>
	// Syntax: Sound.UnregisterWeightedEaxEnvironment( const char *sPresetName, ScriptHandle entityId )
	// Arguments:
	//		sPresetName	- Preset name.
	//		entityId	- Entity identifier.
	// Return Value:
	//			Bool.
	// Description:
	//		Unregisters a weighted EAX (Environmental Audio Extension) environment.
	int	UnregisterWeightedEaxEnvironment(IFunctionHandler *pH); 
	// <title FXEnable>
	// Syntax: Sound.FXEnable( ISound* pSound, int nEffectNumber )
	// Arguments:
	//		pSound			- Sound identifier.
	//		nEffectNumber	- Effect number.
	// Description:
	//		Enables the specified effect number.
	int FXEnable(IFunctionHandler *pH);
	// <title SetFXSetParamEQ>
	// Syntax: Sound.SetFXSetParamEQ( ISound* pSound, float fCenter,float fBandwidth,float fGain )
	// Arguments:
	//		pSound		- Sound identifier.
	//		fCenter		- Sound center.
	//		fBandwidth  - Sound bandwidth.
	//		fGain		- Sound gain.
	// Description:
	//		Sets some parameters to the sound effect.
	int SetFXSetParamEQ(IFunctionHandler *pH);
	// <title SetDirectionalAttenuation>
	// Syntax: Sound.SetDirectionalAttenuation( Vec3 vPos, Vec3 vDir, float fConeInRadians )
	// Arguments:
	//		vPos - Position vector.
	//		vDir - Direction vector for the attenuation.
	//		fConeInRadians - Angle of the cone that represents the attenuation.
	// Description:
	//		Sets a directional attenuation of the sounds.
	int	SetDirectionalAttenuation(IFunctionHandler *pH);
	// <title GetDirectionalAttenuationMaxScale>
	// Syntax: Sound.GetDirectionalAttenuationMaxScale()
	// Description:
	//		Gets the maximum scale of the directional attenuation.
	int	GetDirectionalAttenuationMaxScale(IFunctionHandler *pH);
	// <title SetSoundRatio>
	// Syntax: Sound.SetSoundRatio(ISound* pSound, float fRatio)
	// Arguments:
	//		pSound - Sound identifier.
	//		fRatio - Sound ratio.
	// Description:
	//		Sets the sound ratio.
	int SetSoundRatio(IFunctionHandler *pH);
	// <title GetSoundLength>
	// Syntax: Sound.GetSoundLength(Isound* pSound)
	// Arguments:
	//		pSound - Sound identifier.
	// Description:
	//		Gets the sound length in seconds.
	int GetSoundLength(IFunctionHandler * pH);
	// <title SetWeatherCondition>
	// Syntax: Sound.SetWeatherCondition(float fWeatherTemperatureInCelsius,float fWeatherHumidityAsPercent,float fWeatherInversion)
	// Arguments:
	//		fWeatherTemperatureInCelsius - Weather temperature in Celsius.
	//		fWeatherHumidityAsPercent	 - Weather humidity as percentage.
	//		fWeatherInversion			 - Weather inversion.
	// Description:
	//		Sets the weather condition.
	int SetWeatherCondition(IFunctionHandler * pH);

	// <title RegisterSoundMood>
	// Syntax: Sound.RegisterSoundMood(const char *sSoundMoodName)
	// Arguments:
	//		sSoundMoodName - Sound mood name.
	// Description:
	//		Registers the specified sound mood.
	// Return Value:
	//		Bool.
	int	RegisterSoundMood(IFunctionHandler *pH, const char *sParameterName); 
	// <title RegisterSoundMood>
	// Syntax: Sound.RegisterSoundMood(const char *sSoundMoodName, float fFade, [optional] uint32 nFadeTimeInMS)
	// Arguments:
	//		sSoundMoodName	- Sound mood name.
	//		fFade			- Fade value.
	//		nFadeTimeInMs	- Fade time in milliseconds. Default value 0. 
	// Description:
	//		Updates the specified sound mood.
	// Return Value:
	//		Bool.
	int	UpdateSoundMood(IFunctionHandler *pH, const char *sParameterName, float fFade, uint32 nFadeTimeInMS = 0); 
	// <title GetSoundMoodFade>
	// Syntax: Sound.GetSoundMoodFade(const char *sSoundMoodName)
	// Arguments:
	//		sSoundMoodName	- Sound mood name.
	// Description:
	//		Gets the fade value for the specified sound mood.
	int GetSoundMoodFade(IFunctionHandler *pH, const char *sSoundMoodName);
	// <title UnregisterSoundMood>
	// Syntax: Sound.UnregisterSoundMood(const char *sSoundMoodName)
	// Arguments:
	//		sSoundMoodName	- Sound mood name.
	// Description:
	//		Unregisters the specified sound mood.
	// Return Value:
	//		Bool.
	int	UnregisterSoundMood(IFunctionHandler *pH, const char *sParameterName); 


	// Music

	// <title LoadMusic>
	// Syntax: Sound.LoadMusic(const char *pszFilename)
	// Arguments:
	//		pszFilename - Xml file name containing the music information.
	// Return Value:
	//		Bool.
	int	LoadMusic(IFunctionHandler *pH); 
	// <title UnloadMusic>
	// Syntax: Sound.UnloadMusic()
	// Description:
	//		Unloads the music.
	int	UnloadMusic(IFunctionHandler *pH);
	// <title SetMusicTheme>
	// Syntax: Sound.SetMusicTheme(const char *pszTheme, [optional] bool bForceChange, [optional] bool bKeepMood, [optional] int nDelayInSec)
    // Arguments:
	//		pszTheme		- Theme name.
	//		bForceChange	- True to force the theme change, false otherwise. Default value is false.
	//		bKeepMood		- True to keep the sound mood, false otherwise. Default value is false.
	//		nDelayInSec		- Delay in second to change the theme. Default value is -1.
	// Description:
	//		Sets the music theme.
	int	SetMusicTheme(IFunctionHandler *pH);
	// <title EndMusicTheme>
	// Syntax: Sound.EndMusicTheme([optional] int nThemeFadeType, [optional] int nForceEndLimitInSec, [optional] bool bEndEverything)
	// Arguments:
	//		nThemeFadeType		- Fade type to stop the theme. Default value is 0.
	//		nForceEndLimitInSec	- Ending limit in seconds. Default value is 5.
	//		bEndEverything		- True to end everything, false otherwise. Default value is false.
	int	EndMusicTheme(IFunctionHandler *pH);
	// <title SerializeMusicInternal>
	// Syntax: Sound.SerializeMusicInternal([optional] bool bSave)
	// Arguments:
	//		bSave - True to serialize the music internal, false otherwise. Default value is false.
	// Description:
	//		Serializes the music internal.
	int	SerializeMusicInternal(IFunctionHandler *pH);
	// <title SetMusicMood>
	// Syntax: Sound.SetMusicMood(const char *pszMood, [optional] bool bPlayFromStart, [optional] bool bForceChange)
	// Arguments:
	//		pszMood			- Music mood name.
	//		bPlayFromStart	- True to play from start, false otherwise. Default value is true.
	//		bForceChange	- True to force the music mood change, false otherwise. Default value is false.
	// Description:
	//		Sets the specified music mood.
	int	SetMusicMood(IFunctionHandler *pH);
	// <title SetDefaultMusicMood>
	// Syntax: Sound.SetDefaultMusicMood(const char *pszMood)
	// Arguments:
	//		pszMood			- Music mood name.
	// Description:
	//		Sets the specified music mood as the default mood.
	int	SetDefaultMusicMood(IFunctionHandler *pH);
	// <title GetMusicThemes>
	// Syntax: Sound.GetMusicThemes()
	// Description:
	//		Gets the music themes.
	int	GetMusicThemes(IFunctionHandler *pH);
	// <title GetMusicMoods>
	// Syntax: Sound.GetMusicMoods(const char *pszTheme)
	// Arguments:
	//		pszTheme - Theme name.
	// Description:
	//		Gets the music moods for the specified music theme.
	int	GetMusicMoods(IFunctionHandler *pH);
	// <title AddMusicMoodEvent>
	// Syntax: Sound.AddMusicMoodEvent(const char *pszMood, [optional] float fTimeout)
	// Arguments:
	//		pszMood		- Music mood name.
	//		fTimeout	- Timeout value.
	// Description:
	//		Adds an event for the specified music mood.
	int AddMusicMoodEvent(IFunctionHandler *pH);
	// <title IsInMusicTheme>
	// Syntax: Sound.IsInMusicTheme(const char *pszTheme)
	// Arguments:
	//		pszTheme - Theme name.
	// Description:
	//		Checks if we are currently playing the specified music theme.
	int	IsInMusicTheme(IFunctionHandler *pH);
	// <title IsInMusicMood>
	// Syntax: Sound.IsInMusicMood(const char *pszMood)
	// Arguments:
	//		pszMood		- Music mood name.
	// Description:
	//		Checks if we are currently playing the specified music mood.
	int	IsInMusicMood(IFunctionHandler *pH);
	// <title GetMusicStatus>
	// Syntax: Sound.GetMusicStatus()
	// Description:
	//		Gets the music status and logs the value to the console.
	int GetMusicStatus(IFunctionHandler *pH);
	// <title SetMenuMusic>
	// Syntax: Sound.SetMenuMusic(bool bPlayMenuMusic, const char *pszTheme, const char *pszMood )
	// Arguments:
	//		bPlayMenuMusic	- True to play music in the menu, false otherwise.
	//		pszMood			- Music mood name.
	//		pszTheme		- Theme name.
	int SetMenuMusic(IFunctionHandler *pH);
	// <title PlayStinger>
	// Syntax: Sound.PlayStinger()
	// Description:
	//		Plays stinger.
	int PlayStinger(IFunctionHandler *pH);
	// <title PlayPattern>
	// Syntax: Sound.PlayPattern(const char *pszPattern, bool bStopPrevious, bool bPlaySynched)
	// Arguments:
	//		pszPattern		- Pattern name.
	//		bStopPrevious	- True to stop the previous pattern, false otherwise.
	//		bPlaySynched	- True to play the pattern synched, false otherwise.
	int PlayPattern(IFunctionHandler *pH);

		// Multi Listener

		// <title CreateListener>
		// Syntax: Sound.CreateListener( IFunctionHandler *pH )
		// Description:
		//    Creates a new Listener
		// Arguments:
		// Return Value:
		//    Listener ID.
		int CreateListener( IFunctionHandler *pH );

		// <title RemoveListener>
		// Syntax: Sound.RemoveListener( IFunctionHandler *pH )
		// Description:
		//    Removes a Listener
		// Arguments:
		//		nListenerID - ID of the Listener to be removed
		// Return Value:
		//    bResult - true/false if Listener was removed successfully
		int RemoveListener( IFunctionHandler *pH, int nListenerID );

		// <title SetListener>
		// Syntax: Sound.SetListener( IFunctionHandler *pH )
		// Description:
		//    Sets attributes to a Listener
		// Arguments:
		//		nListenerID - ID of the Listener to be removed
		//		vPos				- Position of the Listener
		//		vForward		- a Lookat vector of the Listener
		//		vTop				- a up vector of the Listener
		//		bActive			- state of Listener (active/unactive)
		//		fRecord			- input signal strength [0.0-1.0] to fade in and out
		//		vVel				- Velocity for Doppler, (0,0,0) to disable or leave out for automatic calculation
		// Return Value:
		//    bResult - true/false if Listener was successfully changed
		int SetListener( IFunctionHandler *pH, int nListenerID, Vec3 vPos, Vec3 vVel , Vec3 vForward, Vec3 vTop, bool bActive, float fRecord);
		//int SetListener( IFunctionHandler *pH); // keep for Debug


private: // -------------------------------------------------------
	ISound* GetSoundPtr(IFunctionHandler *pH,int index);
	SmartScriptTable WrapSoundReturn( IFunctionHandler *pH, ISound * pSound );

	ISystem *								m_pSystem;						//!<
	ISoundSystem *					m_pSoundSystem;				//!< might be 0 (e.g. dedicated server or Init() wasn't successful)
	IMusicSystem *					m_pMusicSystem;				//!< might be 0 (e.g. dedicated server or Init() wasn't successful)

	string m_sCurrentMusicTheme;
	string m_sCurrentMusicMood;

	bool bMenuMusicLoaded;

	static int ReleaseFunction( IFunctionHandler* pH, void *pBuffer, int nSize );
};

#endif // __ScriptBind_Sound_h__