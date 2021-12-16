#ifndef __HUDAMMO_H__
#define __HUDAMMO_H__

#include "HUDObject.h"
#include "Weapon.h"


//////////////////////////////////////////////////////////////////////////


class CHUD_Ammo : public CHUDObject
{

public:

	enum DirtyFlags
	{
		eDF_None						= 0<<0,
		eDF_Ammo						= BIT(0),
		eDF_ClipSize				= BIT(1),
		eDF_InventoryAmmo		= BIT(2),
		eDF_WeaponName			= BIT(3),
		eDF_LowWarnings			= BIT(4),
		eDF_All							= 0xFFFF
	};


	CHUD_Ammo();
	virtual ~CHUD_Ammo();

	virtual void		Init();
	virtual void		PreDelete();
	virtual void		Update	(float frameTime);

	virtual void		OnHUDEvent(const SHUDEvent& event);

private:

	void								Reset();

	void								OnItemSelected(EntityId itemId);
	void								OnReloaded(IWeapon* pWeapon);
	void								OnSetAmmoCount(IWeapon* pWeapon);
	void								OnShoot(IWeapon* pWeapon);
	void								OnFireModeChanged(IWeapon * pWeapon, int currentFireMode);

	void								UpdateWeaponEnvironment(IWeapon* pWeapon);
	void								UpdateWeaponStats(IWeapon* pWeapon);

	void								UpdateLowAmmoWarning();

	void								UpdateAmmo(IWeapon* pWeapon);

	void								UpdateWeaponName(const EntityId id);


	CryFixedStringT<32>	m_ammoType;

	CryFixedStringT<32>	m_fireMode;

	SFlashDisplayInfo		m_displayInfoLowAmmo;

	int									m_clipSize;

	int									m_weaponAmmo;
	int									m_inventoryAmmo;
	int 								m_forceLowAmmoOn;

	IEntityClass*				m_pWeaponClass;

	IFlashVariableObject*		m_objectRoot;
	IFlashVariableObject*		m_objectAmmo;
	IFlashVariableObject*		m_objectLowAmmo;
	IFlashVariableObject*		m_objectFM;

	uint16							m_dirty;

	bool								m_lowAmmoWarning;
	bool								m_lowClipWarning;
	bool								m_leavingBattleArea;
	bool								m_showingUse;

	bool										m_validAssets;
};


//////////////////////////////////////////////////////////////////////////

#endif

