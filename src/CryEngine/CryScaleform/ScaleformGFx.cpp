// CryScaleform.cpp : Defines the exported functions for the DLL application.
//

#include "StdAfx.h"
#include "ScaleformGFx.h"

#include "Scaleform/FlashPlayerInstance.h"
#include "Scaleform/GAllocatorCryMem.h"

IFlashPlayer* CScaleformGFx::CreateFlashPlayerInstance() const
{
#ifndef EXCLUDE_SCALEFORM_SDK
	return new CFlashPlayer;
#else
	return 0;
#endif
}

void CScaleformGFx::RenderFlashInfo()
{
#ifndef EXCLUDE_SCALEFORM_SDK
	CFlashPlayer::RenderFlashInfo();
#endif
}

void CScaleformGFx::GetMemoryUsage(ICrySizer* pSizer)
{
#ifndef EXCLUDE_SCALEFORM_SDK
	CSharedFlashPlayerResources::GetAccess().GetMemoryUsage(pSizer);
#endif
}

void CScaleformGFx::SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler) const
{
#ifndef EXCLUDE_SCALEFORM_SDK
	CFlashPlayer::SetFlashLoadMovieHandler(pHandler);
#endif
}
