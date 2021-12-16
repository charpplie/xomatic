#ifndef __FlashHelper_H__
#define __FlashHelper_H__

#include "IFlashPlayer.h"

struct SFlashHelper
{
	enum EFlashPositionType
	{
		eFPT_Lower,
		eFPT_Mid,
		eFPT_Upper,
	};

	static void updateViewPort(IFlashPlayer* pFlashPlayer, bool bScale, EFlashPositionType px, EFlashPositionType py);
	static std::vector<string> SliceArguments(const char* args);
};

#endif // #ifndef __FlashHelper_H__