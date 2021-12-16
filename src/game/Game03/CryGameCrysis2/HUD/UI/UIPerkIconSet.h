/************************************************************************/
/* UI PerkIconSet for Component HUD, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef UI_PERK_ICON_SET_H
#define UI_PERK_ICON_SET_H

#include "HUD/UI/UIElement.h"
#include "HUD/ColourFromXML.h"

#define PERK_ICONS_DO_BUTTON_PROMPTS 1

class CUIPerkIconSet : public CUIElement
{
public:

	CUIPerkIconSet();

	void Initialize( const IItemParamsNode* params, IUIElement* parent );

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		CUIElement::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
private:

	void Draw(void) const;

protected :

	float						m_distanceApart;
	float						m_pulseSize;
	float						m_alignHorizontal;
	float						m_alignVertical;

	TBitfield       m_onlyShowPerksWithFlags, m_skipPerksWithFlags;

	CColourFromXml	m_activeBrightColour;
	CColourFromXml	m_activeDrainedColour;
	CColourFromXml	m_inactiveBrightColour;
	CColourFromXml	m_inactiveDrainedColour;
	CColourFromXml	m_availableColour;
	CColourFromXml	m_unavailableColour;
};

#endif	// UI_PERK_ICON_SET_H
