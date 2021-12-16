#ifndef __HUD_INTERACTIVE_H__
#define __HUD_INTERACTIVE_H__

#include "Player.h"
#include "HUDObject.h"

//-----------------------------------------------------------------------------------------------------

struct IFlashPlayer;

class CHUD_Interactive : public CHUDObject
{
public:

	enum eReasonsToHide
	{
		eHide_None = 0,
		eHide_SuitMenu = BIT(0),
		eHide_BattleAreaWarning = BIT(1),
	};

	CHUD_Interactive();
	virtual ~CHUD_Interactive();

	virtual void Init();
	virtual void PreDelete();
	virtual void Update	(float frameTime);

	void OnHUDEvent(const SHUDEvent& event);

private:

	struct InteractionDisplay
	{
		InteractionDisplay() :
		m_button(""),
		m_message(L""),
		m_class("None"),
		m_duration(0.0f),
		m_timer(0.0f),
		m_dirty(false)
		{

		}

		void Update(float frameTime)
		{
			if(m_timer>0.0f)
				m_timer -= frameTime;
		}

		bool IsActive()
		{
			return m_timer>0.0f || m_duration==-1.0f;
		}

		CryFixedStringT<64>		m_button;
		CryFixedWStringT<128>	m_message;
		CryFixedStringT<32>		m_class;
		float									m_duration;
		float									m_timer;
		bool									m_dirty;
	};
	

	void										UpdateUsable(EntityId usable, CPlayer::EInteractionType interactionType);
	const char*							GetItemName(const EntityId item) const;
	void										InteractionRequest(const bool activate, const char* message, const char* button, const float duration);

	InteractionDisplay			m_usable;
	InteractionDisplay			m_interaction;

	int               			m_forceDraw;
	bool										m_showing;
	uint16									m_forceHide;

	IFlashVariableObject*		m_objectRoot;
	IFlashVariableObject*		m_objectWeapons;

	IFlashVariableObject*		m_objectRoot2D;
	IFlashVariableObject*		m_objectButton;
};

#endif

