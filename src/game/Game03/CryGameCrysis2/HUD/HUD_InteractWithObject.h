/************************************************************************/
/* Hud callback used to interact with object via "scanning"             */
/* created by Jan Mueller, 02/2010																			*/
/************************************************************************/


#ifndef __HUDINTERACTWITHOBJECT_H__
#define __HUDINTERACTWITHOBJECT_H__

#include "HUD/HUDObject.h"

class CHUD_InteractWithObject : public CHUDObject
{
public:
	CHUD_InteractWithObject();
	virtual ~CHUD_InteractWithObject();

	// IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);
	// ~IHUDEventListener

	//can this entity be used ?
	static bool IsUsableEntity(const IEntity *pEntity);

private:

	//deal with actually usable entity
	void UseInteractiveEntity(IEntity *pEntity) const;

	static IEntityClass *m_pInteractiveEntityClass;
};

#endif