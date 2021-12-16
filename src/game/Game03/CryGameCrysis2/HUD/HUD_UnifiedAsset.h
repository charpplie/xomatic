#ifndef __HUDUNIFIEDASSET_H__
#define __HUDUNIFIEDASSET_H__

#include "HUDObject.h"
#include "HUD.h"
#include "Utility/CryFixedArray.h"

//////////////////////////////////////////////////////////////////////////


enum EHUDElement
{
	eHUDElement_objectives,
	eHUDElement_radar,
	eHUDElement_transmissions,
	eHUDElement_tacticalinfo,
	eHUDElement_ammo_suit,
	eHUDElement_incoming,
	eHUDElement_leaning,
	eHUDElement_scanning,
	eHUDElement_interactions,
	eHUDElement_visor_left,
	eHUDElement_visor_right,
	eHUDElement_size
};


enum EHUDPosition
{
	eHUDPosition_center_front,
	eHUDPosition_center_back,
	eHUDPosition_left_front,
	eHUDPosition_left_back,
	eHUDPosition_right_front,
	eHUDPosition_right_back,
};


//////////////////////////////////////////////////////////////////////////


class CHUD_UnifiedAsset : public CHUDObject
{

public:

	typedef uint8 THUDElement;
	typedef uint8 THUDPosition;

	struct SHUDElement
	{
		EntityId			m_entity;
		const char*		m_model;
		THUDElement		m_element;
		THUDPosition	m_position;
		int8					m_visible;
	};

	CHUD_UnifiedAsset();
	virtual ~CHUD_UnifiedAsset();

	virtual void		Update	(float frameTime);
	virtual void		Draw	();
	virtual void		Init	();
	virtual void		OnHUDEvent(const SHUDEvent& event);

	void						Show(void);
	void						Show(const bool show);

	static IHUDAsset*		s_pHUDAssetUnified3D;
	static IHUDAsset*		s_pHUDAssetUnified2D;

private:

	void						UpdateView(SViewParams* pViewParams);
	void						InitObject(const THUDElement element, const EHUDPosition position, const char* model);
	void						LoadObject(SHUDElement& element);
	void						Create3DObjects();
	void						Destroy3DObjects();

	void						ShowElement(const int index, const bool show);
	void						OnImpulse(const Vec3& impulse);

	CryFixedArray<SHUDElement, eHUDElement_size> m_elements;

	Quat						m_previousRotation;
	Vec3						m_impulse;
	Vec3						m_additionalMovement;
	Vec3						m_previousPosition;

	static IEntityClass*		s_HUDObjectClass;

	_smart_ptr<IMaterial>		m_pMaterial;

	float						m_offset3D;

	bool						m_show;
	bool						m_objectsValid;
};


//////////////////////////////////////////////////////////////////////////

#endif

