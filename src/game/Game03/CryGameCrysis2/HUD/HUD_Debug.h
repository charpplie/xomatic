#ifndef __HUDDEBUG_H__
#define __HUDDEBUG_H__

#include "HUDObject.h"


//////////////////////////////////////////////////////////////////////////


struct IWeapon;


//////////////////////////////////////////////////////////////////////////


class CHUD_Debug : public CHUDObject
{
#ifndef _RELEASE

public:

	enum ETemporaryTextFlags
	{
		eTTF_FireModeName,
		eTTF_WeaponName,
		eTTF_ClipSize,
		eTTF_WeaponAmmo,
		eTTF_InventoryAmmo,
		eTTF_SuitMode,
		eTTF_Health,
		eTTF_Energy,
		eTTF_F1DebugKey,
		eTTF_F2DebugKey,
		eTTF_F3DebugKey,
		eTTF_F4DebugKey,
		eTTF_F5DebugKey,
		eTTF_F6DebugKey,
		eTTF_F10DebugKey,
		eTTF_F11DebugKey
	};


	CHUD_Debug();
	virtual ~CHUD_Debug();

	virtual void		Update	(float frameTime);
	virtual void		Draw	();
	void						Reload();

	virtual void		OnHUDEvent( const SHUDEvent& event );

protected:

	struct STextDisplay
	{

		STextDisplay(ETemporaryTextFlags id, const char* text, float posX, float posY, float timer, bool temporary)
		{
			m_id = id;
			m_text = text;
			m_position.x = posX;
			m_position.y = posY;
			m_timer = timer;
			m_temporary = temporary;
		}

		int									m_id;
		float								m_timer;
		float								m_intensity;
		CryFixedStringT<64>	m_text;
		Vec2								m_position;
		bool								m_temporary;
	};

private:

	void								CreateTextDisplay(ETemporaryTextFlags id, float posX, float posY, bool temporary);
	void								SetTextDisplay(ETemporaryTextFlags id, const char* text, float timer = 0.0f, float intensity = 1.0f);

	void								UpdateFKeys();

	void								ClearWeapon();

	void								OnItemSelected(EntityId itemId);
	void								OnReloaded(IWeapon* pWeapon);
	void								OnSetAmmoCount(IWeapon* pWeapon);
	void								OnShoot(IWeapon* pWeapon);
	void								OnFireModeChanged(IWeapon * pWeapon, int currentFireMode);

	void								UpdateWeaponEnvironment(IWeapon* pWeapon);
	void								UpdateWeaponStats(IWeapon* pWeapon);

	typedef std::vector<STextDisplay> Displays;
	Displays						m_displays;

	CryFixedStringT<32>	m_ammoType;
	int									m_clipSize;

#endif //_RELEASE
};


//////////////////////////////////////////////////////////////////////////

#endif

