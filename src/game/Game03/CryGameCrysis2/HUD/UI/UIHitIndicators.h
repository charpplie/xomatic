/************************************************************************/
/* UI class for Hit Indicators, Frank Harrison, 2009													*/
/************************************************************************/

#ifndef ___UIHITINDICATORS_H___
#define ___UIHITINDICATORS_H___

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventDispatcher.h"

typedef CUIElement THitIndicatorsParent;

class UIHitIndicators : public THitIndicatorsParent
                              ,IHUDEventListener
{
private :

	struct SHitIndicatorData
	{
		float angle;
		float depth;
		float timeAdded;
	};

public :

	UIHitIndicators();
	virtual ~UIHitIndicators();

	void Draw() const; // Draw_WarsHitIndicators
	void Update( float ftime ); // Draw_WarsHitIndicators

	void AddHitIndicator( float angle, float damage );
	void ClearHitIndicators( void );

private :

	//IHUDEventListener
	virtual void OnHUDEvent( const SHUDEvent& hudEvent );
	//~IHUDEventListener

	static const int kMaxNumHitIndicators = 10;
	static const float kTimeForIndicatorsToRemainOnScreen;

	int		m_numUsedHitIndicators;
	SHitIndicatorData	m_hitIndicators[kMaxNumHitIndicators];
};

#endif // ___UIHITINDICATORS_H___
