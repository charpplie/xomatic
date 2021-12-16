#ifndef _GAME_EFFECTS_SYSTEM_
#define _GAME_EFFECTS_SYSTEM_

#pragma once

// Defines
#define	GAME_FX_SYSTEM					CGameEffectsSystem::Instance()

#ifdef _DEBUG
	#define DEBUG_GAME_FX_SYSTEM	1

	// Register effect's DebugOnInput and DebugDisplay callback functions
	#define REGISTER_EFFECT_DEBUG_DATA(inputEventCallback,debugDisplayCallback,effectName)	\
		static CGameEffectsSystem::SRegisterEffectDebugData effectName(inputEventCallback,debugDisplayCallback,#effectName)

	// Debug views
	enum EGameEffectsSystemDebugView
	{
		eGAME_FX_DEBUG_VIEW_None = 0,
		eGAME_FX_DEBUG_VIEW_Profiling,
		eGAME_FX_DEBUG_VIEW_BoundingBox,
		eGAME_FX_DEBUG_VIEW_BoundingSphere,
		eGAME_FX_DEBUG_VIEW_Particles,
		eMAX_GAME_FX_DEBUG_VIEWS
		// ** If you add/remove a view then remember to update GAME_FX_DEBUG_VIEW_NAMES **
	};

#else
	#define DEBUG_GAME_FX_SYSTEM	0
	#define REGISTER_EFFECT_DEBUG_DATA(inputEventCallback,debugDisplayCallback,effectName)
#endif

// Forward declares
struct IGameEffect;

// Typedefs
typedef void (*DebugOnInputEventCallback)(int);
typedef void (*DebugDisplayCallback)(const Vec2& textStartPos,float textSize,float textYStep);

//==================================================================================================
// Name: CGameEffectsSystem
// Desc: System to handle game effects, game render nodes and game render elements
//			 Game effect: separates out effect logic from game logic
//			 Game render node: handles the render object in 3d space
//			 Game render element: handles the rendering of the object
//			 CVar activation system: system used to have data driven cvars activated in game effects
//			 Post effect activation system: system used to have data driven post effects activated in game effects
// Author: James Chilvers
//==================================================================================================
class CGameEffectsSystem : public IInputEventListener
{
public:
	// ---------- Singleton static functions ----------
	static inline CGameEffectsSystem&		Instance()
	{
		if(s_singletonInstance == NULL)
		{
			s_singletonInstance = new CGameEffectsSystem;
		}
		return *s_singletonInstance;
	}

	static void							Destroy();
	//-------------------------------------------------

	void										Initialise();

	template<class T>
	T*											CreateEffect()  // Use if dynamic memory allocation is required for the game effect
	{																				// Using this function then allows easy changing of memory allocator for all dynamically created effects
		T* newEffect = new T;
		return newEffect;
	}
	void										DeleteEffect(IGameEffect** effect); // Use to release effect memory created using CreateEffect

	void										RegisterEffect(IGameEffect* effect); // Each effect automatically registers and unregisters itself
	void										UnRegisterEffect(IGameEffect* effect);

	void										Update(float frameTime);

	template<class T>
	T*											CreateRenderNode() // Use if dynamic memory allocation is required for the render node
	{																					 // Using this function then allows easy changing of memory allocator for all dynamically created effects
		T* newRenderNode = new T;
		return newRenderNode;
	}
	void										DeleteRenderNode(IRenderNode** pRenderNode); // Use to release render node memory created using CreateRenderNode

	bool										OnInputEvent(const SInputEvent& inputEvent);

#if DEBUG_GAME_FX_SYSTEM
	// Creating a static version of SRegisterEffectDebugData inside an effect cpp registers the effect's debug data with the game effects system
	struct SRegisterEffectDebugData
	{
		SRegisterEffectDebugData(DebugOnInputEventCallback inputEventCallback,DebugDisplayCallback debugDisplayCallback, const char* effectName)
		{
			CGameEffectsSystem::RegisterEffectDebugData(inputEventCallback,debugDisplayCallback,effectName);
		}
	};

	int											GetDebugView() const { return m_debugView; }
#endif

protected:
	CGameEffectsSystem();
	virtual ~CGameEffectsSystem();

private:

	void										Reset();
	void										SetPostEffectCVarCallbacks();
	static void							PostEffectCVarCallback(ICVar* cvar);

#if DEBUG_GAME_FX_SYSTEM
	void										DrawDebugDisplay();
	void										OnActivateDebugView(int debugView);
	void										OnDeActivateDebugView(int debugView);
	static void							RegisterEffectDebugData(	DebugOnInputEventCallback inputEventCallback,
																										DebugDisplayCallback displayCallback,
																										const char* effectName);

	struct SEffectDebugData
	{
		SEffectDebugData(	DebugOnInputEventCallback paramInputCallback,
											DebugDisplayCallback paramDisplayCallback,
											const char* paramEffectName)
		{
			inputCallback = paramInputCallback;
			displayCallback = paramDisplayCallback;
			effectName = paramEffectName;
		}
		DebugOnInputEventCallback inputCallback;
		DebugDisplayCallback			displayCallback;
		const char*								effectName;
	};

	static int												s_currentDebugEffectId;
	static PodArray<SEffectDebugData>	s_effectDebugList;

	int																m_debugView;
#endif

	static CGameEffectsSystem*	s_singletonInstance;
	static int									s_postEffectCVarNameOffset;

	IGameEffect*								m_effectsToUpdate;
	IGameEffect*								m_effectsNotToUpdate;
	IGameEffect*								m_nextEffectToUpdate; // -> If in update loop, this is the next effect to be updated
																										//    this will get changed if the effect is unregistered
	bool												m_isInitialised;
};//------------------------------------------------------------------------------------------------

#endif // _GAME_EFFECTS_SYSTEM_
