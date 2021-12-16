#ifndef __HUDWEAPONSELECT_H__
#define __HUDWEAPONSELECT_H__

#include "HUDObject.h"
#include "Weapon.h"


//////////////////////////////////////////////////////////////////////////


class CHUD_WeaponSelect : public CHUDObject
{

public:

	enum DirtyFlags
	{
		eDF_None						= 0<<0,
		eDF_PrimaryWeapon		= BIT(0),
		eDF_SecondaryWeapon	= BIT(1),
		eDF_Attachment			= BIT(2),
		eDF_Fading					= BIT(3),
		eDF_All							= 0xFFFF
	};

	enum WeaponSpots
	{
		eWeapon_Primary		= 0,
		eWeapon_Count			= 1
	};

private:

	typedef	CryFixedWStringT<64>	TWeaponName;

public:

	struct SWeaponInfo
	{
		SWeaponInfo()
			: m_entityId(0)
			, m_pClass(NULL)
			, m_pAmmoClass(NULL)
			, m_weaponObject(NULL)
			, m_ammoObject(NULL)
			, m_displayName(L"")
		{}

		void Clear()
		{
			m_entityId = 0;
			m_pClass = NULL;
			m_pAmmoClass = NULL;
		}

		TWeaponName							m_displayName;
		EntityId								m_entityId;
		IEntityClass*						m_pClass;
		IEntityClass*						m_pOldClass;
		IEntityClass*						m_pAmmoClass;
		IFlashVariableObject*		m_weaponObject;
		IFlashVariableObject*		m_ammoObject;
	};

	CHUD_WeaponSelect();
	virtual ~CHUD_WeaponSelect();

	virtual void		Update(float frameTime);
	virtual void		Init();
	virtual void		PreDelete();

	virtual void		OnHUDEvent(const SHUDEvent& event);

private:

	void									OnAmmoPickup(IEntityClass* pAmmoClass, const int difference);
	void									OnItemSelected(const EntityId itemId);

	bool									UpdateWeaponData(const EntityId id, IEntityClass* pClass, IItem* pItem, SWeaponInfo& outInfo) const;
	void									UpdateWeaponDisplay(const SWeaponInfo& info, IFlashVariableObject* root) const;

	SWeaponInfo						m_infos[eWeapon_Count];

	IFlashVariableObject* m_root;

	float									m_timerDisplay;
	float									m_timerFade;

	uint16								m_dirty;

};


//////////////////////////////////////////////////////////////////////////

#endif

