/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description:
- EmpStrike projectile that is used for team perks
- A different class projectile also fixes some networking issues

-------------------------------------------------------------------------
History:
- 12:5:2009   11:15 : Created by Ben Parbury

*************************************************************************/
#ifndef __EMPSTRIKE_H__
#define __EMPSTRIKE_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SatelliteStrike.h"

class CEMPStrike : public CSatelliteStrike
{
public:
	CEMPStrike();
	virtual ~CEMPStrike() {}

	virtual bool Init(IGameObject *pGameObject);

protected:
	virtual void UpdateEffect();

	static SStrikeParams s_empStrikeParams;
};

#endif // __SATELLITESTRIKE_H__
