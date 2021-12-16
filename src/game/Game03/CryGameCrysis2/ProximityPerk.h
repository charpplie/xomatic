#ifndef __PROXIMITYPERK_H__
#define __PROXIMITYPERK_H__

#include "IPerk.h"

class ProximityPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(ProximityPerk);

	ProximityPerk(const float *pMaxRange, const float *pScanSpeed1, const float *pScanSpeed2, const float *pScanSpeed3);
	virtual ~ProximityPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual void Update(const float dt);
	virtual void Leave();

protected:
	bool findClosestTarget(float* detectedDist);
	void updateHud(float range, bool detectedUpdate, float detectedRange);
	void audioFeedback(float noramlizedRange, bool detected);

	float m_range;
	float m_lastDetectedRange;
	const float *m_pMaxRange;
	const float *m_pScanSpeed[eTierMax];
	tSoundID m_sound;

};

#endif
