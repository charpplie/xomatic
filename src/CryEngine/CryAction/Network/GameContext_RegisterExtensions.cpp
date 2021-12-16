#include "StdAfx.h"
#include "GameContext.h"
#include "VoiceListener.h"

void CGameContext::RegisterExtensions( IGameFramework * pFW )
{
#ifndef OLD_VOICE_SYSTEM_DEPRECATED
	REGISTER_FACTORY(pFW, "VoiceListener", CVoiceListener, false);
#endif
}
