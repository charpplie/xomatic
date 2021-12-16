/************************************************************************/
/* UI BattleLog for Component HUD, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef UI_BUTTON_PROMPT_REGION_H
#define UI_BUTTON_PROMPT_REGION_H

#include "HUD/UI/UIElement.h"
#include "HUD/HUDControllerInputIcons.h"

class CSingleButtonPromptInfo;
struct SOnScreenMessageDef;

struct CButtonPromptInfoLinkedList
{
	CSingleButtonPromptInfo * m_firstInfo;
	CSingleButtonPromptInfo * m_lastInfo;
};

class CSingleButtonPromptInfo
{
	public:
	CSingleButtonPromptInfo(CButtonPromptInfoLinkedList * list, const char * text, float time, const CControllerInputRenderInfo * inputRenderInfo, TBitfield vanishSettings);
	~CSingleButtonPromptInfo();
	void SetTimeRemaining(float f);

//private:
	CButtonPromptInfoLinkedList * m_list;
	CSingleButtonPromptInfo     * m_next;
	CSingleButtonPromptInfo     * m_prev;

	string												m_string;
	float													m_timeUntilVanish;
	float												  m_visibility;
	bool													m_reduceTime;
	CControllerInputRenderInfo    m_inputRenderInfo;
	TBitfield                     m_vanishSettings;
};

class CUIButtonPromptRegion : public CUIElement
{
	public:
	CUIButtonPromptRegion();
	virtual ~CUIButtonPromptRegion();

	void Initialize(const IItemParamsNode* params, IUIElement* parent);
 
	static CUIButtonPromptRegion * GetInstanceWithName(const char * name);
	void SetOnScreenMessageText(const char * message, const CControllerInputRenderInfo * renderInfo, float timeOut, TBitfield vanishSettings);

	static void SetOnScreenMessageText(const char * queueName, const SOnScreenMessageDef & messageDef, const char * paramA, const char * paramB);
	static void SetOnScreenMessageText(const char * queueName, const char * message, const char * paramA, const char * paramB, const CControllerInputRenderInfo * inputRenderInfo, float timeOut = 0.f, TBitfield vanishSettings = kHudElementsDrawnFlag_none);
	static void SetOnScreenMessageText(const char * queueName, const char * message, const CControllerInputRenderInfo * inputRenderInfo, float timeOut = 0.f, TBitfield vanishSettings = kHudElementsDrawnFlag_none);

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		CUIElement::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
	private:
	CUIButtonPromptRegion * m_next;
	CUIButtonPromptRegion * m_prev;
	CButtonPromptInfoLinkedList m_list;

	int                             m_maxSimultaneous;
	TBitfield                       m_defaultVanishSettings;
	float                           m_inSpeed, m_outSpeed, m_fontSize, m_totalVisibility, m_distanceApartX, m_distanceApartY,
	                                m_inputIconOffsetY, m_inputIconSize, m_captionOffsetY, m_captionSize;

	void Update                       (float frameTime);
	void Draw                         (void) const;
	void ClearAll                     ();
};

#endif	// UI_BUTTON_PROMPT_H
