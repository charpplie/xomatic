#ifndef __IScaleformGFx_H__
#define __IScaleformGFx_H__

#include "IFlashPlayer.h"

struct IFlashLoadMovieHandler;

struct IScaleformGFx
{
	virtual IFlashPlayer* CreateFlashPlayerInstance() const = 0;
	virtual void RenderFlashInfo() = 0;
	virtual void GetMemoryUsage(ICrySizer* pSizer) = 0;
	virtual void SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler) const = 0;
};

#endif // ifndef __IScaleformGFx_H_