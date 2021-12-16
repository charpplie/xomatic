/************************************************************************/
/* UI PerkIconSet for Component HUD, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef UI_SIMPLE_BAR_H
#define UI_SIMPLE_BAR_H

#include "HUD/UI/UIElement.h"

class CUISimpleBar : public CUIElement
{
	public:

	CUISimpleBar();
	virtual ~CUISimpleBar();

	void Initialize( const IItemParamsNode* params, IUIElement* parent );

	static CUISimpleBar * GetInstanceWithName(const char * name);
	void Set(float f, bool display = true);
	void SetColours( const ColorF& bg, const ColorF& fg, const ColorF& flash1, const ColorF& flash2 );
	void TriggerWarningFlash();

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		CUIElement::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
	private:
	CUISimpleBar * m_next;
	CUISimpleBar * m_prev;

	float m_fractionFull;
	bool m_adjustPosY;
	bool m_setThisFrame;
	bool m_doingWarningFlash;
	float m_alpha;
	float m_warningFlash;
	ColorF m_bgColour;
	ColorF m_fgColour;
	ColorF m_flashColour1;
	ColorF m_flashColour2;

	void Update(float frameTime);
	void Draw( void ) const;
};

#endif	// UI_SIMPLE_BAR_H
