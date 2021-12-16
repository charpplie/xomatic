#ifndef __HUD_TACTICALINFO_H__
#define __HUD_TACTICALINFO_H__

#include "HUDObject.h"

#include "NanoSuitDefs.h"
#include "Audio/AudioSignalPlayer.h"

//////////////////////////////////////////////////////////////////////////


class CHUD_TacticalInfo : public CHUDObject
{

public:

	enum DirtyFlags
	{
		eDF_None						= 0<<0,
		eDF_Info						= BIT(0),
		eDF_Health					= BIT(1),
		eDF_Alertness				= BIT(2),
		eDF_All							= 0xFFFF
	};

	CHUD_TacticalInfo();
	virtual ~CHUD_TacticalInfo();

	virtual void Init();
	virtual void PreDelete();
	virtual void Update	(float frameTime);

	virtual void OnHUDEvent( const SHUDEvent& event );

private:

	void SetTacticalDescription(const char* primary, const char* secondary, const char* tertiary);
	void SetTacticalHealth(const int health);
	void SetTacticalAlertness(const int alertness);
	void SetSuitMode(const ENanoSuitMode mode);

	void Show(const bool show);

	CryFixedStringT<128>	m_secondary;
	CryFixedStringT<64>		m_primary;
	CryFixedStringT<64>		m_tertiary;
	CAudioSignalPlayer		m_audioInfo;
	CAudioSignalPlayer		m_audioHeartbeat;

	int										m_health;
	int										m_alertness;

	IFlashVariableObject*	m_object;

	IFlashVariableObject*	m_objectPrimary;
	IFlashVariableObject*	m_objectSecondary;
	IFlashVariableObject*	m_objectTertiary;
	IFlashVariableObject*	m_objectHealth;
	IFlashVariableObject*	m_objectAlertness;

	ENanoSuitMode					m_suitMode;

	uint16								m_dirty;

	bool									m_show;

};


//////////////////////////////////////////////////////////////////////////

#endif

