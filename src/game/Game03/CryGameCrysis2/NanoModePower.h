/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Power mode implementation
-------------------------------------------------------------------------
History:
- 4-3-2008: Created by Benito G.R.
- 20-10-2009: Renamed to power mode

*************************************************************************/

#ifndef _NANOMODE_POWER_H_
#define _NANOMODE_POWER_H_

#include "NanoSuitDefs.h"

struct SPlayerStats;


class CNanoModePower: public INanoMode
{

public:

	CNanoModePower(CNanoSuit& nanoSuit);
	virtual ~CNanoModePower();

	//INanoMode
	virtual void Release();

	virtual void Activate(bool activate);
	virtual bool CanActivateSuitPower() const;
	virtual void ActivateSuitPower(bool activate);
	virtual void Update(float frameTime);
	virtual void ProcessEvent(const SNanoSuitEvent& event);
	virtual void OnStateChanged(ENanoSuitState newState);

	virtual void GetMemoryUsage(ICrySizer* s) const;

	virtual ENanoSuitMode GetType() const;
	//~INanoMode

private:

	void ActivateSprinting(bool activate);

	CNanoSuit	&m_nanoSuit;

	bool m_sprintingActive;
	bool m_sprintWasActivatedSinceModeChange;
};

#endif
