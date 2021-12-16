/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Armor mode implementation
-------------------------------------------------------------------------
History:
- 10-3-2008: Created by Benito G.R.
- 20-10-2009: Renamed to armor mode

*************************************************************************/

#ifndef _NANOMODE_ARMOR_H_
#define _NANOMODE_ARMOR_H_

#include "NanoSuitDefs.h"


class CNanoModeArmor: public INanoMode
{

public:

	CNanoModeArmor(CNanoSuit& nanoSuit);
	virtual ~CNanoModeArmor();

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

	CNanoSuit&	m_nanoSuit;
};

#endif
