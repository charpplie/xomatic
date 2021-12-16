/************************************************************************/
/* UI class for Spectator Cam, Frank Harrison, 2009									  */
/************************************************************************/

#ifndef ___UISPECTATORCAM_H___
#define ___UISPECTATORCAM_H___

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventTranslator.h"

typedef CUIElement TSpectatorCamParent;

class UISpectatorCam : public TSpectatorCamParent
                     , public IHUDEventListener
{
public :
	explicit UISpectatorCam();
	virtual ~UISpectatorCam();

	void Draw() const;
	void Update( float ftime );

	void OnHUDEvent( const SHUDEvent& event );

private :
	void Draw_CGameRulesMPSpawningBase( void ) const;
	void Draw_CGameRulesMPSpectator( void ) const;


	CRY_TODO( 07, 01, 2009, "Convert localised HUD strings to wide. /FH");
	static CryFixedStringT<128> s_strBuffer;
};
#endif // ___UISPECTATORCAM_H___