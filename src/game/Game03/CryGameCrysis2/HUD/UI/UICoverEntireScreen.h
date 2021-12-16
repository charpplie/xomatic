/************************************************************************/
/* UI PerkIconSet for Component HUD, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef __UI_COVERENTIRESCREEN_H__
#define __UI_COVERENTIRESCREEN_H__

#include "HUD/UI/UIElement.h"

class CUICoverEntireScreen : public CUIElement
{
	public:

	CUICoverEntireScreen();
	virtual ~CUICoverEntireScreen();

	void Initialize(const IItemParamsNode* param, IUIElement* parent);

	static CUICoverEntireScreen * GetInstanceWithName(const char * name);
	void Set(float f);

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		CUIElement::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}

	private:
	CUICoverEntireScreen * m_next;
	CUICoverEntireScreen * m_prev;

	float m_red, m_green, m_blue, m_maxAlpha;
	float m_targetVisibility;
	float m_filteredVisibility;
	float m_filterRateAppear, m_filterRateVanish;
	int m_textureID;

	void Update(float frameTime);
	void Draw(void) const;
};

#endif	// __UI_COVERENTIRESCREEN_H__
