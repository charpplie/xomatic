/************************************************************************/
/* UI class for Hit Indicators, Frank Harrison, 2009													*/
/************************************************************************/

#ifndef ___UIDEATHCAM_H___
#define ___UIDEATHCAM_H___

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventTranslator.h"

#include "RecordingSystem.h"

class C2DRenderUtils;
class ScreenLayoutManager;

typedef CUIElement TDeathCamParent;

class UIDeathCam : public TDeathCamParent
                 , public IHUDEventListener
{
public :
	explicit UIDeathCam();
	virtual ~UIDeathCam();

	void UpdateHUD(void);

	void DrawHUD( C2DRenderUtils* pRenderUtils, ScreenLayoutManager* pLayoutManager ) const;
	void Draw() const;
	void Update( float ftime );

	void OnHUDEvent( const SHUDEvent& event );

private :

	// Font for rendering
	IFFont* m_pFont;

	// Data from recorder.
	CRecordingSystem::TReplayActorMap* m_pReplayActors;

	EntityId m_killerId;
};

#endif // ___UIDEATHCAM_H___
