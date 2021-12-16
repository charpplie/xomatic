/************************************************************************/
/* New damage indicator UI element, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef __UINEWDAMAGEINDICATOR_H__
#define __UINEWDAMAGEINDICATOR_H__

#include "HUD/UI/UIElement.h"
#include "HUD/ColourFromXML.h"
#include "HUD/HUDEventDispatcher.h"

typedef CUIElement TParent_CUINewDamageIndicator;

class CUINewDamageIndicator : public TParent_CUINewDamageIndicator, IHUDEventListener
{
	// Internally-used definitions
	static const int                kMaxSegments = 16;

	struct SDamageIndicatorSegment
	{
		float m_alpha;
		float m_angleRadians;
		bool m_drawn;
	};

	// Methods
	public:
	CUINewDamageIndicator();
	~CUINewDamageIndicator();

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		TParent_CUINewDamageIndicator::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}

	private:
	virtual void Initialize(const IItemParamsNode* params, IUIElement* parent);
	virtual void Update (float frameTime);
	virtual void Draw (void) const;
	virtual void OnHUDEvent(const SHUDEvent& event);

	void AddDirectionalDamage (float angleRadians);

	// Member variables
	SDamageIndicatorSegment         m_segments[kMaxSegments];
	CColourFromXml                  m_colour;
	float                           m_nonDirectionalDamageAlpha;
	float                           m_maxNonDirectionalAlphaWhenTakingDirectionalDamage;
	float                           m_filteredMaxAlpha;
	int                             m_segmentTextureId, m_nonDirectionalTextureId;
	int                             m_numDamageSegmentsToDisplay;
	bool                            m_drawNonDirectionalDamage;
	bool														m_preventDirectionalDamage;
};

#endif	// __UINEWDAMAGEINDICATOR_H__
