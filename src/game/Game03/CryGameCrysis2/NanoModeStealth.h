/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Stealth mode implementation
-------------------------------------------------------------------------
History:
- 10-3-2008: Created by Benito G.R.
- 20-10-2009: Renamed to stealth mode

*************************************************************************/

#ifndef _NANOMODE_STEALTH_H_
#define _NANOMODE_STEALTH_H_

#include "NanoSuitDefs.h"


class CNanoModeStealth: public INanoMode
{

public:

	CNanoModeStealth(CNanoSuit& nanoSuit);
	virtual ~CNanoModeStealth();

	//INanoMode
	virtual void Release();

	virtual void Activate(bool activate);
	virtual bool CanActivateSuitPower() const;
	virtual void ActivateSuitPower(bool activate);
	virtual void Update(float frameTime);
	virtual void ProcessEvent(const SNanoSuitEvent& event);
	virtual void OnStateChanged(ENanoSuitState newState);
	inline void SetInterference(bool on) { m_interference = on; }

	virtual void GetMemoryUsage(ICrySizer* s) const;

	virtual ENanoSuitMode GetType() const;
	//~INanoMode

private:

	void SetCloakLayer(bool set);
	void SetCloak(bool set);
	void ActivateCloakFlickering(bool activate, float timeOut = -1.0f);
	void UpdateCloakFlickering(float frameTime);

	CNanoSuit&	m_nanoSuit;

	float		m_cloakPulseTimer;
	float		m_flickeringTimer;
	float		m_flickeringTimeOut;

	bool		m_cloakLayerActive;
	bool		m_interference;
	bool		m_isFlickering;
};

#endif
