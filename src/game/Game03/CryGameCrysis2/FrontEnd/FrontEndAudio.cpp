#include "StdAfx.h"

#include "FrontEndAudio.h"
#include "IMusicSystem.h"

//////////////////////////////////////////////////////////////////////////


static const char* s_frontendAudioSignalNames[ CFrontEndAudio::eFES_Last ] =
{
	"FrontEnd_Start",
	"FrontEnd_Confirm",
	"FrontEnd_Cancel",
	"FrontEnd_Move",
	"FrontEnd_Ambient"
};


CFrontEndAudio::CFrontEndAudio()
: m_musicSerialized(false)
, m_soundListener(LISTENERID_INVALID)
{
	for(int i=0; i<eFES_Last; ++i)
		m_soundPlayers[i].SetSignal( s_frontendAudioSignalNames[i] );
}



CFrontEndAudio::~CFrontEndAudio()
{
}



void CFrontEndAudio::EnterMenu(const bool gameRunning)
{
	IMusicSystem* pMusicSystem = gEnv->pMusicSystem;
	if(pMusicSystem)
	{
		pMusicSystem->LoadFromXML("music/cxp.xml",true, false);
		if(gameRunning)
		{
			pMusicSystem->SerializeInternal(true);
			m_musicSerialized = true;
		}
		pMusicSystem->EndTheme(EThemeFade_FadeOut, 0, true);
		pMusicSystem->SetTheme("menu", true, false);

		if(gameRunning)
			pMusicSystem->SetDefaultMood("pause_menu");
		else
			pMusicSystem->SetDefaultMood("main_menu");
	}

	if(!gameRunning && m_soundListener == LISTENERID_INVALID)
	{
		m_soundListener = gEnv->pSoundSystem->CreateListener();
		IListener *pListener = gEnv->pSoundSystem->GetListener(m_soundListener);
		pListener->SetRecordLevel(1.0f);
		pListener->SetActive(true);
	}

	PlaySound(eFES_Ambient, true);
}



void CFrontEndAudio::LeaveMenu()
{
	PlaySound(eFES_Ambient, false);
	IMusicSystem* pMusicSystem = gEnv->pMusicSystem;
	if(pMusicSystem)
	{
		pMusicSystem->EndTheme(EThemeFade_FadeOut, 0, true);
		if(m_musicSerialized)
		{
			pMusicSystem->SerializeInternal(false);
			m_musicSerialized = false;
		}
	}
	if(m_soundListener != LISTENERID_INVALID)
	{
		gEnv->pSoundSystem->RemoveListener(m_soundListener);
		m_soundListener = LISTENERID_INVALID;
	}
}



void CFrontEndAudio::PlaySound(const EFrontEndSound eSound, const bool bPlay)
{
	assert( eSound<eFES_Last );

	m_soundPlayers[eSound].Stop();

	if (bPlay)
		m_soundPlayers[eSound].Play();
}



bool CFrontEndAudio::HandleCommand(const char* sound)
{
	for(int i=0; i<eFES_Last; ++i)
	{
		if(!stricmp(s_frontendAudioSignalNames[i], sound))
		{
			PlaySound((EFrontEndSound)i, true);
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////