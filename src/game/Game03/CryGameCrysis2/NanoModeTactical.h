/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Tactical mode implementation
-------------------------------------------------------------------------
History:
- 3-2-2009: Created by Benito G.R.

*************************************************************************/

#ifndef _NANOMODE_TACTICAL_H_
#define _NANOMODE_TACTICAL_H_

#include "NanoSuitDefs.h"


class CNanoModeTactical: public INanoMode
{

public:

	CNanoModeTactical(CNanoSuit& nanoSuit);
	virtual ~CNanoModeTactical();

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

	void ActivateSuitVisor(bool activate);

	CNanoSuit&	m_nanoSuit;

};

#endif
