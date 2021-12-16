#pragma once

#ifndef _SHOTGUN_H_
#define _SHOTGUN_H_

#include "Single.h"

class CShotgun :
	public CSingle
{
	struct BeginReloadLoop;
	struct SliderBack;
	class PartialReloadAction;
	class ReloadEndAction;
	class ScheduleReload;

public:
	virtual void GetMemoryUsage(ICrySizer * s) const;
	void GetInternalMemoryUsage(ICrySizer * s) const;
	virtual void Activate(bool activate);
	virtual void Reload(int zoomed);
	virtual void StartReload(int zoomed);
	void ReloadShell(int zoomed);
	virtual void EndReload(int zoomed);
	using CSingle::EndReload;
	
	virtual void CancelReload();

	virtual bool CanFire(bool considerAmmo) const;

	virtual bool Shoot(bool resetAnimation, bool autoreload = true , bool isRemote=false );
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int ph);

	virtual const char* GetType() const;
	virtual float GetSpreadForHUD() const;

private:

	bool	m_reload_pump;
	bool	m_load_shell_on_end;				
	bool	m_break_reload;
	bool	m_reload_was_broken;

	int   m_max_shells;
};

#endif
