#ifndef __HUD_DETECTOR_H__
#define __HUD_DETECTOR_H__

#include "HUDObject.h"
#include "Utility/CryFixedArray.h"
#include "NanoSuitDefs.h"

//-----------------------------------------------------------------------------------------------------

class CHUD_Detector : public CHUDObject
{
public:

	struct SDetection
	{
		SDetection(const EntityId id)
			: m_id(id)
			, m_distance(0.0f)
			, m_pEntity(NULL)
		{

		}

		bool operator ==( const SDetection& compare ) const
		{
			return fabs(m_distance - compare.m_distance) < FLT_EPSILON;
		}

		bool operator <( const SDetection& compare ) const
		{
			return (compare.m_distance - m_distance) > FLT_EPSILON;
		}

		float			m_distance;
		EntityId	m_id;
		IEntity*	m_pEntity;	//retrieved every frame to save GetEntity calls
	};

	struct SDetectorIcon
	{
		SDetectorIcon()
			: m_object(NULL)
			, m_class(NULL)
			, m_posX(0.0f)
			, m_posY(0.0f)
			, m_distance(0.0f)
			, m_visible(false)
		{

		}

		IFlashVariableObject*		m_object;
		IEntityClass*						m_class;
		float										m_posX;
		float										m_posY;
		float										m_distance;
		bool										m_visible;
	};

	CHUD_Detector();
	virtual ~CHUD_Detector();

	virtual void Update (float frameTime);
	virtual void Init();
	virtual void PreDelete();

	virtual void OnHUDEvent(const SHUDEvent& event);

private:

	void OnExplosiveSpawned(const EntityId grenade);
	void OnSuitModeChanged(const ENanoSuitMode suitMode);

	const static int k_maxDetectionsSupported = 16;

	typedef std::vector<SDetection> DetectedObjects;
	DetectedObjects m_detected;

	const static int k_maxFlashIconsSupported = 4;

	SDetectorIcon m_icons[k_maxFlashIconsSupported];

	Vec2		m_edge;
	Vec2		m_assetSize;

	float		m_xOffset;

	ENanoSuitMode m_suitMode;
};

#endif

