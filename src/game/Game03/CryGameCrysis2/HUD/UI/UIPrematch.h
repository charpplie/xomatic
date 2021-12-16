/************************************************************************/
/* UI BattleLog for Component HUD, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef UI_PREMATCH_H
#define UI_PREMATCH_H

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventDispatcher.h"

typedef CUIElement TPrematchParent;

class UIPrematch : public TPrematchParent
                 , public IHUDEventListener
{
private :
	enum EPrematchStates{
		  ePMS_NONE
		, ePMS_GameStartCountDown
		, ePMS_WaitingForPlayers
		, ePMS_WaitingForTeamPlayers
		, ePMS_GameResumeCountDown
	};
public :

	UIPrematch();
	~UIPrematch( void );

	virtual void Initialize( const IItemParamsNode* element, IUIElement* parent );

	virtual void Update(float frameTime);
	virtual void Draw(void) const;

	virtual void OnHUDEvent(const SHUDEvent& event);

private :

	void Reinit( void );

private :

	EPrematchStates m_state;

	int m_required_team_players;
	int m_required_players;
	int m_countdown_timer;
	int m_joinGameButtonTexture;

	CRY_TODO( 07, 01, 2009, "Convert localised HUD strings to wide. /FH");
	static string /*CryFixedStringT<128>*/ s_strBuffer;
};


#endif // UI_PREMATCH_H
