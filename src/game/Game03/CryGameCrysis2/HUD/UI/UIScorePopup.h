/************************************************************************/
/* UI class for Score popups, Frank Harrison, 2009											*/
/* Contributions from Jan Muller, Tim Furnish      											*/
/************************************************************************/

#ifndef ___UISCOREPOPUP_H___
#define ___UISCOREPOPUP_H___

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventDispatcher.h"
#include "PlayerProgression.h"

struct SGameRulesScoreInfo;

typedef CUIElement TScorePopupParent;

class UIScorePopup : public TScorePopupParent
                           ,IHUDEventListener
{
private :

public :

	UIScorePopup();
	virtual ~UIScorePopup();

	void Draw() const {}; // taken care of by CUIButtonPromptRegion.
	void Update( float ftime ) {}; // taken care of by CUIButtonPromptRegion.

	void InitScoreElement( void );

	void NewScoreElement(const SGameRulesScoreInfo& score_info );

	void NewXPElement(const EPPType type, const int xp);

	static void ScoreCmd(IConsoleCmdArgs* cmdArgs);

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));		
		TScorePopupParent::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
private :

	//IHUDEventListener
	virtual void OnHUDEvent( const SHUDEvent& hudEvent );
	//~IHUDEventListener

};

#endif // ___UISCOREPOPUP_H___
