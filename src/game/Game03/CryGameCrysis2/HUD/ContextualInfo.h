#ifndef __CONTEXTUALINFO_H__
#define __CONTEXTUALINFO_H__

//////////////////////////////////////////////////////////////////////////


#include "NanoSuitDefs.h"

/////////////////////////////////////////////////////////////////////////


struct IHUDAsset;


//////////////////////////////////////////////////////////////////////////


class CContextualInfo
{

public:

	CContextualInfo();
	virtual ~CContextualInfo();

	void				Update(const float x, const float y);
	void				SetTarget(const EntityId entity);

	void				SetAsset(IHUDAsset* pAsset) { m_pAsset = pAsset; }
	void				SetSuitMode(const ENanoSuitMode mode);

private:

	EntityId						m_target;
	float								m_health;
	int									m_alertness;
	uint16							m_dirty;

	IHUDAsset*					m_pAsset;

};


//////////////////////////////////////////////////////////////////////////

#endif

