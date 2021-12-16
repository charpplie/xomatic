/*************************************************************************
Crytek Source File.
Copyright (C) Crytek Studios 2001-2004.
-------------------------------------------------------------------------
$Id: FM_TunnellingGrenade.h$
$DateTime$
Description: Fire mode for Tunnelling Grenade. 
Basically just the Single firemode with additional HUD info.

-------------------------------------------------------------------------
History:
- 07/05/2009 12:56:48: Created by tombe

*************************************************************************/

#ifndef __FM_TUNNELLINGGRENADE_H__
#define __FM_TUNNELLINGGRENADE_H__


#include "Single.h"

class CTunnellingGrenadeFM : public CSingle
{
public:
	CTunnellingGrenadeFM();
	~CTunnellingGrenadeFM();

	void Update(float frameTime, uint32 frameId);

	virtual const char *GetType() const { return "Tun_Gren"; };
	virtual void Activate(bool activate);

	virtual void ResetParams(const struct IItemParamsNode *params);
	void ResetDrillerParams();

	bool IsBlocked() const 
	{
		return m_blocked;
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));	
		CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
	}

private:

	typedef CSingle BaseClass; 

	float		m_drillerPenetration;
	float   m_blockedTime;
	bool	  m_blocked;
};


#endif //!__FM_TUNNELLINGGRENADE_H__
