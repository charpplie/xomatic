#ifndef __CryScaleformGFx_H__
#define __CryScaleformGFx_H__

#include "IScaleformGfx.h"

class CScaleformGFx
	: public IScaleformGFx
{
public:
	CScaleformGFx() {};
	~CScaleformGFx() {};

	VIRTUAL IFlashPlayer* CreateFlashPlayerInstance() const;
	VIRTUAL void RenderFlashInfo();
	VIRTUAL void GetMemoryUsage(ICrySizer* pSizer);
	VIRTUAL void SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler) const;

};

#endif // ifndef __CryScaleformGFx_H__