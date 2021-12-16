#ifndef __HUDEVENTTRANSLATOR_H__
#define __HUDEVENTTRANSLATOR_H__

#include "NanoSuit_v2.h"
#include "IGameRulesSystem.h"
#include "Player.h"
#include "HUD/HUDEventDispatcher.h"

//////////////////////////////////////////////////////////////////////////


class CHUDEventTranslator :
				IHUDEventListener,
				public CNanoSuit::INanoSuitListener,
				IItemSystemListener,
				IHitListener,
				IWeaponEventListener,
				IPlayerEventListener
{

public:

	CHUDEventTranslator();
	virtual ~CHUDEventTranslator();

	void Init( void );
	void Clear( void );

	//CNanoSuit::INanoSuitListener
	virtual void ModeChanged(ENanoSuitMode mode);			
	virtual void OnModeSelected(ENanoSuitMode mode);
	virtual void EnergyChanged(float energy);					
	virtual void OnSuitStateChanged(ENanoSuitState state);
	virtual void OnSuitPowerActivated(const bool activated);
	//~CNanoSuit::INanoSuitListener


	//IItemSystemListener
	virtual void OnSetActorItem(IActor *pActor, IItem *pItem );

	virtual void OnDropActorItem(IActor *pActor, IItem *pItem ) {}
	virtual void OnSetActorAccessory(IActor *pActor, IItem *pItem ) {}
	virtual void OnDropActorAccessory(IActor *pActor, IItem *pItem ) {}
	//~IItemSystemListener


	//IHitListener
	virtual void OnHit(const HitInfo& hitInfo);

	virtual void OnExplosion(const ExplosionInfo&) {}
	virtual void OnServerExplosion(const ExplosionInfo&) {}
	//~IHitListener


	//IWeaponEventListener
	virtual void		OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType);
	virtual void		OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType);
	virtual void		OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId);
	virtual void		OnFireModeChanged(IWeapon *pWeapon, int currentFireMode);

	virtual void		OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel) {}
	virtual void		OnStartFire(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void		OnStopFire(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void		OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType) {}
	virtual void		OnReadyToFire(IWeapon *pWeapon) {}
	virtual void		OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed) {}
	virtual void		OnDropped(IWeapon *pWeapon, EntityId actorId) {}
	virtual void		OnMelee(IWeapon* pWeapon, EntityId shooterId) {}
	virtual void		OnStartTargetting(IWeapon *pWeapon) {}
	virtual void		OnStopTargetting(IWeapon *pWeapon) {}
	virtual void		OnSelected(IWeapon *pWeapon, bool selected) {}
	//~IWeaponEventListener

	//IPlayerEventListener
	virtual void OnDeath(IActor* pActor, bool bIsGod);
	virtual void OnHealthChanged(IActor* pActor, float newHealth);
	//~IPlayerEventListener

	//IHUDEventListener
	virtual void OnHUDEvent( const SHUDEvent& hudEvent );
	//~IHUDEventListener

	void      OnInitGameRules( void );
	void      OnInitPlayer( void );

	ILINE const bool IsSelectMenuOpen( void ) const { return m_select_menu_shown; }
	ILINE const bool IsSelectmapMenuOpen( void ) const { return m_map_menu_shown; }

	void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:

	void			SubscribeWeapon(EntityId weapon);
	void			UnsubscribeWeapon(EntityId weapon);

	EntityId	m_currentWeapon;

	// menus
	bool m_select_menu_shown;
	bool m_map_menu_shown;
};


//////////////////////////////////////////////////////////////////////////

#endif

