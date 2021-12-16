#ifndef __HUD_H__
#define __HUD_H__

#include <IGameFramework.h>
#include "GameRulesTypes.h"
#include "HUD/UI/UIElement.h" // TODO: Remove.
#include "HUD/HUDEventDispatcher.h" // For sending events to the HUD.
#include "HUD/HUDSilhouettes.h"
#include "HUD/HUDDefines.h"
#include "IMovieSystem.h"

//////////////////////////////////////////////////////////////////////////

struct IHUDState;
struct IHUDAsset;
struct IFlashVariableObject;
struct IHUDEventListener;

class CHUDObject;
class CHUDEventTranslator;
class CHUDObjectManager;
class CHUDStateManager;
class CUIElementManager;

struct SHUDSizeAndPositionInfo;

class ScreenLayoutManager;
class C2DRenderUtils;

class CHUDCVars;

class CHUD_Impl; // TODO : Remove

struct ITweakMenuController;

#if ENABLE_HUD_TESTS
class HUDTester;
#endif // ENABLE_HUD_TESTS

//////////////////////////////////////////////////////////////////////////


class CHUD : public IGameFrameworkListener, IMovieListener
{
	typedef std::vector<SHUDSizeAndPositionInfo*> TSizeInfoRegistrations;

public:

	                          CHUD();
	virtual                   ~CHUD();

	// Console Commands
#if ENABLE_HUD_TESTS
	static void               CmdTestHud(IConsoleCmdArgs *pArgs);
	static void               CmdTestHudDone(IConsoleCmdArgs *pArgs);
#endif // ENABLE_HUD_TESTS
#if ENABLE_HUD_EXTRA_DEBUG
	static void               CmdForceDrawAll(IConsoleCmdArgs *pArgs);
#endif // ENABLE_HUD_EXTRA_DEBUG

	// IGameFrameworkListener
	virtual void							OnPostUpdate(float fDeltaTime);
	virtual void							OnSaveGame(ISaveGame* pSaveGame);
	virtual void							OnLoadGame(ILoadGame* pLoadGame);
	virtual void							OnLevelEnd(const char* nextLevel) {}
	virtual void							OnActionEvent(const SActionEvent& event) {}
	//~IGameFrameworkListener

	// IMovieListener
	virtual void							OnMovieEvent(IMovieListener::EMovieEvent movieEvent, IAnimSequence* pAnimSequence);
	//~IMovieListener

	static void								AddHUDEventListener(IHUDEventListener* pListener, const char* eventName);
	static void								RemoveHUDEventListener(IHUDEventListener* pListener);
	static void               CallEvent(const SHUDEvent& event);

	void                      ActivateState( const char* stateName );
	void                      ActivateDefaultState( void );
	void											OnAction(const ActionId& action, int activationMode, float value);

	void                      Load();
	void											Reload();
	void											Unload();
	bool											ShouldShowHUD();

	ScreenLayoutManager*      GetLayoutManager( void ) { return m_layoutManager; }

	C2DRenderUtils*           Get2DRenderUtils( void ) const { return m_p2DRenderUtils; }

	CHUDSilhouettes*          GetSilhouettes( void ) const { return m_pSilhouettes; }

	CHUD_Impl*                GetHUDImpl( void ) const { return m_pImpl; }

	CUIElementManager*        GetElementManager( void ) const { return m_elementManager; }

	CHUDObjectManager*				GetObjectManager( void ) const { return m_objectManager; }

	CHUDCVars*								GetCVars( void ) const { return m_pHUDCVars; }

	// StringLocalization
	const wchar_t*            LocalizeStringW( const char *text, const char *arg1 = NULL, const char *arg2 = NULL) const;
	const char*               LocalizeString(const char *text, const char *arg1 = NULL, const char *arg2 = NULL) const;

	void                      RegisterElementsDrawn( TBitfield drawnElements );

	//////////////////////////////////////////////////////////////////////////

	void GetMemoryUsage(ICrySizer *pSizer) const;
private:

	void											InitalizeObjects(const char* xml_path );
	void											InitializeStates(const char* path);


public : // Data members
	static IHUDAsset*					s_pHUDAssetNULL;
	static IFlashVariableObject* s_pHUDFlashvariableObjectNULL;

private : // Data members

	CHUDSilhouettes*					m_pSilhouettes;

	Vec2i                     m_canvasSize;
	TSizeInfoRegistrations    m_sizeObjects;

	TBitfield                 m_lastElementsDrawn;
	TBitfield                 m_elementsDrawn;

	CHUD_Impl*								m_pImpl;

	CHUDEventTranslator*			m_pEventTranslator;
	ITweakMenuController*			m_pTweakMenuController;

	CHUDObjectManager*        m_objectManager;
	CHUDStateManager*         m_stateManager;
	CUIElementManager*        m_elementManager;

	ScreenLayoutManager*      m_layoutManager;

	C2DRenderUtils*           m_p2DRenderUtils;

	CHUDCVars*								m_pHUDCVars;

	bool											m_isLoaded;

#if ENABLE_HUD_TESTS
	static HUDTester*         s_hudTester;
#endif // ENABLE_HUD_TESTS

};

#endif

