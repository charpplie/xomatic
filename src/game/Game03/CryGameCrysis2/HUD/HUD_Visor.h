#ifndef __HUD_VISOR_H__
#define __HUD_VISOR_H__

#include "Audio/AudioSignalPlayer.h"
#include "HUDObject.h"
#include "CoverAndLean.h"
#include "NanoSuitDefs.h"


class CHUDMissionObjective;

//////////////////////////////////////////////////////////////////////////


class CHUD_Visor : public CHUDObject
{
public:

	CHUD_Visor();
	virtual ~CHUD_Visor();

	enum DirtyFlags
	{
		eDF_None								= 0<<0,
		eDF_SliderPos						= BIT(1),
		eDF_SliderText					= BIT(2),
		eDF_ScanningProgress		= BIT(3),
		eDF_SuitMode						= BIT(4),
		eDF_AIAwareness					= BIT(5),
		eDF_Range								= BIT(6),
		eDF_Speed								= BIT(7),
		eDF_All									= 0xFFFF
	};


	virtual void Init();
	virtual void PreDelete();
	virtual void Update	(float frameTime);

	void OnHUDEvent			(const SHUDEvent& event);

private:

	void								OnZoom(int targetStep, int numSteps);
	void								StartScanning(const float time);
	void								StopScanning();
	void								ShowScanning(const bool show);
	void								SucceedScanning();

	void								UpdateScanningProgress(const float frameTime);
	void								CenterEntityChanged(EntityId entity);

	void								SetSuitMode(int suitModeIdent, const char* suitMode);
	void								SetVisorEnabled(bool enabled);
	void								SetAIAwareness(int level);
	void								OnRangeFinderChanged(const int range);

	void								LeaningChanged(const CCoverAndLean::ELeanMode mode, bool canLean, bool leaning);
	void								ObjectiveChanged(CHUDMissionObjective* pObjective);

	void								UpdateSpeed();

	CryFixedWStringT<64>		m_newObjectiveText;
	CryFixedStringT<64>			m_suitMode;

	CAudioSignalPlayer			m_scanningSound;
	CAudioSignalPlayer			m_scanDoneSound;

	IFlashVariableObject*		m_objectRoot;
	IFlashVariableObject*		m_objectScanning;
	IFlashVariableObject*		m_objectArrowLeft;
	IFlashVariableObject*		m_objectArrowTop;
	IFlashVariableObject*		m_objectArrowRight;
	IFlashVariableObject*		m_objectInfinity;
	IFlashVariableObject*		m_object2DVisor;
	IFlashVariableObject*		m_objectStealthBar;

	float										m_zoomTarget;
	float										m_zoomPos;
	float										m_scanTime;
	float										m_scanTimeLeft;
	float										m_sliderSpeed;
	float										m_currentZoom;

	EntityId								m_currentCenterEntity;
	EntityId								m_currentScanEntity;

	int											m_flashProgress;
	int											m_upperLimit;
	int											m_lowerLimit;
	int											m_range;
	int											m_speed;
	int											m_aiAwareness;

	ENanoSuitMode						m_suitModeIdent;

	uint16									m_dirty;

	bool										m_showScanning;
	bool										m_visorEnabled;
};


//////////////////////////////////////////////////////////////////////////

#endif

