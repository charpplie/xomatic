#ifndef ___HUD_MESSAGE___
#define ___HUD_MESSAGE___

#include "HUD/HUDObject.h"


class CHUD_Message : public CHUDObject
{
public :
	CHUD_Message();

	void Init( void );

	void Draw( void );

	void OnHUDEvent(const SHUDEvent& event);

	void SetMessage( const char* pString );

private :
	IHUDAsset* m_gameStateMessage;
};

#endif // ___HUD_MESSAGE___