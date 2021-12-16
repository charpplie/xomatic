#ifndef __FRONTENDAUDIO_H__
#define __FRONTENDAUDIO_H__

//////////////////////////////////////////////////////////////////////////

#include "Audio/AudioSignalPlayer.h"


//////////////////////////////////////////////////////////////////////////


class CFrontEndAudio
{
public:

	enum EFrontEndSound
	{
		eFES_Start,
		eFES_Confirm,
		eFES_Cancel,
		eFES_Move,
		eFES_Ambient,
		eFES_Last
	};

	CFrontEndAudio();
	virtual ~CFrontEndAudio();

	void EnterMenu(const bool gameRunning);
	void LeaveMenu();
	void PlaySound(const EFrontEndSound eSound, const bool bPlay);
	bool HandleCommand(const char* sound);

private:
	CAudioSignalPlayer m_soundPlayers[eFES_Last];
	int m_soundListener;
	bool m_musicSerialized;
};


//////////////////////////////////////////////////////////////////////////

#endif
