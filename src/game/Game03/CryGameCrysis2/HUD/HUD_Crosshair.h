#ifndef __HUD_CROSSHAIR_H__
#define __HUD_CROSSHAIR_H__

#include "HUDObject.h"

#include "HUD/HUD.h"


//////////////////////////////////////////////////////////////////////////


struct IFlashPlayer;


//////////////////////////////////////////////////////////////////////////


class CHUD_Crosshair : public CHUDObject
{
public:

	enum eHCH_Types {
		eHCH_None = 0,
		eHCH_Normal = 1,
		eHCH_Shotgun = 2,
	};

	static const int k_numberCrosshairs = 2;

	enum DirtyFlags
	{
		eDF_None						= 0<<0,
		eDF_Spread					= BIT(0),
		eDF_Opacity					= BIT(1),
		eDF_Crosshair				= BIT(2),
		eDF_Target					= BIT(3),
		eDF_DontFire				= BIT(4),
		eDF_HitIndicator		= BIT(5),
		eDF_SetDisplayInfo	= BIT(6), // Should only be set by update
		eDF_Visibility			= BIT(7),
		eDF_All							= 0xFFFF
	};


	enum TargetType
	{
		eTT_None		= 1,
		eTT_Enemy		= 2,
		eTT_Friend	= 3
	};

public :

	CHUD_Crosshair();
	virtual ~CHUD_Crosshair();

	//CHUDObject
	virtual void Update	(float frameTime);
	virtual void Draw		( void );
	void Init           ( void );
	void OnHUDEvent			( const SHUDEvent& event );
	void PreDelete      ( void );
	//~CHUDObject

	static TargetType	GetLookAtType(EntityId target);

private:

	void												UpdateWeaponEnvironment(float frameTime);
	void												SelectCrosshair(eHCH_Types crosshair);
	float												GetScaledSpread(float base);
	void												SetLookAtTarget(EntityId target);
	void 												OnHitTarget(const SHUDEvent& theEvent);

private :

	IFlashVariableObject* m_rootObj;
	IFlashVariableObject* m_crosshairObj[k_numberCrosshairs];
	//IFlashVariableObject* m_hitObj[k_numberCrosshairs];
	IFlashVariableObject* m_friendlyFireObj;
	uint16							m_dirty;

	bool								m_visible;
	bool								m_reloading;
	bool								m_dontFire;
	float								m_spread;
	float								m_opacity;
	float								m_targetHitTimer;
	float								m_targetHitTime;
	int									m_hitIndicatorFade;
	float								m_hitIndicatorTime[eHUDEventHT_numTypes];
	eHCH_Types          m_crosshair;
	TargetType					m_targetType;
	EntityId						m_enemyToLockOnTo;
	float   						m_timeLockedOnEnemy;

#if ENABLE_HUD_EXTRA_DEBUG
	float								m_weaponSpread; // none smoothed spread value, m_spread's target.
#endif

	SFlashDisplayInfo		m_rootDI;
	SFlashDisplayInfo		m_crosshairDI[k_numberCrosshairs];
	//SFlashDisplayInfo		m_hitDI[k_numberCrosshairs];
	SFlashDisplayInfo		m_friendlyFireDI;
};


//////////////////////////////////////////////////////////////////////////

#endif

