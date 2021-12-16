#ifndef __HUD_ENERGYANDSUITMODE_H__
#define __HUD_ENERGYANDSUITMODE_H__

#include "HUDObject.h"
#include "NanoSuitDefs.h"


//////////////////////////////////////////////////////////////////////////


struct IFlashPlayer;


//////////////////////////////////////////////////////////////////////////


class CHUD_EnergyAndSuitMode : public CHUDObject
{
public:

	enum DirtyFlags
	{
		eDF_None						= 0<<0,
		eDF_Energy					= BIT(0),
		eDF_SuitState				= BIT(1),
		eDF_SuitMode				= BIT(2),
		eDF_Health					= BIT(3),
		eDF_Power						= BIT(4),
		eDF_HealthCritical	= BIT(5),
		eDF_All							= 0xFFFF
	};

	CHUD_EnergyAndSuitMode();
	virtual ~CHUD_EnergyAndSuitMode();

	virtual void Update(float frameTime);
	virtual void Init();
	virtual void PreDelete();

	virtual void OnHUDEvent( const SHUDEvent& event );

private:

	void SetEnergy			(float energy);
	void SetHealth			(float health);
	void SetSuitMode		(const ENanoSuitMode mode);
	void SetSuitState		(ENanoSuitState state);
	void SuitPowerActivated(const bool activated);

	IFlashVariableObject*		m_objectRoot;
	IFlashVariableObject*		m_objectEnergy;
	IFlashVariableObject*		m_objectHealth;

	IFlashVariableObject*		m_objectSuitModes[eNanoSuitMode_Last];

	int											m_energy;
	int											m_health;
	ENanoSuitState					m_suitState;
	ENanoSuitMode						m_suitMode;
	ENanoSuitMode						m_lastSuitMode;

	uint16									m_dirty;

	bool										m_powerActive;
	bool										m_healthCritical;

};


//////////////////////////////////////////////////////////////////////////

#endif

