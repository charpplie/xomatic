#ifndef __IUIELEMENT_H__
#define __IUIELEMENT_H__

#include "AutoEnum.h"

class CSubUIElement;
struct IUIInterface;
struct IItemParamsNode;

enum EUISubElementFlag
{
	eSEF_NO_TAINTING = BIT(1),

	eSEF_FADE_OUT = BIT(13),
	eSEF_NORENDER = BIT(14),

	eSEF_LAST,
};

typedef uint16 TUISubElementFlags;

#define UIElementItemList(f)                   \
	f(kHudElementsDrawnFlag_dead)                \
	f(kHudElementsDrawnFlag_respawnCountdown)    \
	f(kHudElementsDrawnFlag_startCountdown)      \
	f(kHudElementsDrawnFlag_suitMenu)            \

AUTOENUM_BUILDFLAGS_WITHZERO(UIElementItemList, kHudElementsDrawnFlag_none);

//---------------------------------------------------------------------------------
// Non-Flash HUDObject interface.
struct IUIElement
{
	explicit IUIElement(){};
	virtual ~IUIElement(){};

	virtual const char* GetName( void ) const = 0;
	//virtual const Vec2& GetPosition( void ) const = 0;
	virtual const float	GetPosX( void ) const = 0;
	virtual const float	GetPosY( void) const = 0;
	//virtual const Vec2& GetSize( void ) const = 0;
	virtual const float	GetWidth( void ) const = 0;
	virtual const float	GetHeight( void ) const = 0;

	virtual void Update( float ftime ) = 0;
	virtual void Draw( void ) const = 0;

	virtual void Initialize( const IItemParamsNode* params, IUIElement* parent ) = 0;

	virtual IUIElement* GetParent( void ) = 0;
	virtual IUIElement* GetSubElement( const char* ) = 0;

	virtual void GetMemoryUsage(ICrySizer *pSizer) const=0;
	
	//virtual void Hide( const bool show ) = 0;
};

#endif //__IUIELEMENT_H__