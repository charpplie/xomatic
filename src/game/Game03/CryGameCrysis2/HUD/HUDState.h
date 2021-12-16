#ifndef __HUDSTATE_H__
#define __HUDSTATE_H__

#include "HUDObject.h"

#include "HUD/HUD.h"

//////////////////////////////////////////////////////////////////////////

struct IFlashPlayer;
class CHUDStateManager;
class CHUDObjectManager;
class CUIElementManager;
struct IHUDState;
struct IUIElement;
struct SHUDStateGameModeAddon;

//////////////////////////////////////////////////////////////////////////

typedef std::vector<CHUDObject*> THUDObjects;
typedef std::vector<IHUDState*>  TRequiredStates;
typedef std::vector<IUIElement*> TUIElements;
typedef std::vector<SHUDStateGameModeAddon*> TModeAddonsList;

#if ENABLE_HUD_TESTS
typedef std::map<const CHUDObject*,THUDObjects> THUDObjectsOkToOverlap;
#endif // ENABLE_HUD_TESTS

//////////////////////////////////////////////////////////////////////////


struct IHUDState
{

	virtual ~IHUDState() {}

	virtual void			Initialize		(const IItemParamsNode * object, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager) = 0;
	virtual void			Update				(float frameTime) = 0;
	virtual void			Draw					() = 0;

	virtual void			SetActive(bool active) = 0;
	virtual bool			IsActive() const = 0;

	virtual const THUDObjects* GetObjects() const = 0;
	virtual const TRequiredStates* GetDepStates() const = 0;
	virtual const TUIElements* GetElements() const = 0; // Gets the Non-Flash elements for this state.
	virtual const TModeAddonsList* GetModeAddons() const = 0;
	//virtual void      PrintObjectInfo(  CHUDObjectManager* pObjectManager ) const = 0;

#if ENABLE_HUD_TESTS
	virtual TRequiredStates GetDepAncestralStates() const = 0;
	virtual const THUDObjectsOkToOverlap* GetAllowedObjectObjectOverlaps( void ) const = 0;
	virtual bool IsObjectAnOverlay( const CHUDObject* pObject ) const = 0;
	virtual bool IsObjectObjectOverlapAllowed( const CHUDObject* pObject, const CHUDObject* pObject2 ) const = 0;
#endif // ENABLE_HUD_TESTS

	virtual void GetMemoryUsage(ICrySizer *pSizer) const=0;
};


//////////////////////////////////////////////////////////////////////////


class CHUDState : public IHUDState
{
public:

	CHUDState();
	virtual ~CHUDState();

	void			Initialize		(const IItemParamsNode * object, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager);
	void			Update				(float frameTime);
	void			Draw					();

	void			SetActive(bool active);
	bool			IsActive() const { return m_active; }

	const THUDObjects* GetObjects() const {return &m_objects;};
	const TRequiredStates* GetDepStates() const { return &m_otherRequiredStates; }
	const TUIElements* GetElements() const { return &m_uiElements; };
	const TModeAddonsList* GetModeAddons() const { return &m_modeAddons; };

#if ENABLE_HUD_TESTS
	TRequiredStates GetDepAncestralStates() const;
	const THUDObjectsOkToOverlap* GetAllowedObjectObjectOverlaps( void ) const { return &m_allowedOverlaps; }
	bool IsObjectAnOverlay( const CHUDObject* pObject ) const;
	bool IsObjectObjectOverlapAllowed( const CHUDObject* pObject, const CHUDObject* pObject2 ) const;
#endif // ENABLE_HUD_TESTS

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this,sizeof(*this));
		pSizer->AddObject(m_objects);		
		pSizer->AddObject(m_uiElements);		
	}
private:
	
	THUDObjects m_objects;
	TUIElements m_uiElements;
	TModeAddonsList m_modeAddons;

	bool		    m_active;

	TRequiredStates m_otherRequiredStates;

#if ENABLE_HUD_TESTS
	CHUDStateManager* m_stateManager;
	CHUDObjectManager* m_objectManager;
	THUDObjects m_overlayObjects;
	THUDObjectsOkToOverlap m_allowedOverlaps;
#endif // ENABLE_HUD_TESTS

};


//////////////////////////////////////////////////////////////////////////


class CHUDStateNull : public IHUDState
{

public:

	CHUDStateNull() {}
	virtual ~CHUDStateNull() {}

	virtual void			Initialize		(const IItemParamsNode * object, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager) {}
	virtual void			Update				(float frameTime) {}
	virtual void			Draw					() {}

	void			SetActive(bool active) {}
	bool			IsActive() const { return false; }

	const THUDObjects*     GetObjects() const {return NULL;};
	const TRequiredStates* GetDepStates() const { return NULL; }
	const TUIElements* GetElements() const { return NULL; };
	const TModeAddonsList* GetModeAddons() const { return NULL; };

#if ENABLE_HUD_TESTS
	TRequiredStates GetDepAncestralStates() const { TRequiredStates ret; return ret; }
	const THUDObjectsOkToOverlap* GetAllowedObjectObjectOverlaps( void ) const { return NULL; }
	bool IsObjectAnOverlay( const CHUDObject* pObject ) const { return false; };
	bool IsObjectObjectOverlapAllowed( const CHUDObject* pObject, const CHUDObject* pObject2 ) const { return false; };
#endif // ENABLE_HUD_TESTS

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this,sizeof(*this));
	}
};




//////////////////////////////////////////////////////////////////////////

#endif

