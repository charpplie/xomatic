//==================================================================================================
// Name: CGameEffectsSystem
// Desc: System to handle game effects, render nodes and render elements
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "GameEffectsSystem.h"
#include "Effects/GameEffects/GameEffect.h"
#include "GameCVars.h"

//--------------------------------------------------------------------------------------------------
// Desc: Debug data
//--------------------------------------------------------------------------------------------------
#if DEBUG_GAME_FX_SYSTEM
	int	CGameEffectsSystem::s_currentDebugEffectId = 0;
	PodArray<CGameEffectsSystem::SEffectDebugData> CGameEffectsSystem::s_effectDebugList;

	const char* GAME_FX_DEBUG_VIEW_NAMES[eMAX_GAME_FX_DEBUG_VIEWS] = {	"None",
																																			"Profiling",
																																			"Bounding Box",
																																			"Bounding Sphere",
																																			"Particles" };

#endif
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Desc: Static data
//--------------------------------------------------------------------------------------------------
CGameEffectsSystem*	CGameEffectsSystem::s_singletonInstance = NULL;
int									CGameEffectsSystem::s_postEffectCVarNameOffset = 0;
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: CGameEffectsSystem
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CGameEffectsSystem::CGameEffectsSystem()
{
#if DEBUG_GAME_FX_SYSTEM
	if(gEnv->pInput)
	{
		gEnv->pInput->AddEventListener(this);
	}
#endif

	Reset();
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CGameEffectsSystem
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CGameEffectsSystem::~CGameEffectsSystem()
{
#if DEBUG_GAME_FX_SYSTEM
	if(gEnv->pInput)
	{
		gEnv->pInput->RemoveEventListener(this);
	}
#endif
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Destroy
// Desc: Destroys effects system
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::Destroy()
{
	if(s_singletonInstance)
	{
		CRY_ASSERT_MESSAGE(	(s_singletonInstance->m_effectsToUpdate==NULL) && 
												(s_singletonInstance->m_effectsNotToUpdate==NULL),
												"Game Effects System being destroyed even though game effects still exist!");
	}

	SAFE_DELETE(s_singletonInstance);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Reset
// Desc: Resets effects systems data
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::Reset()
{
	m_isInitialised = false;
	m_effectsToUpdate = NULL;
	m_effectsNotToUpdate = NULL;
	m_nextEffectToUpdate = NULL;
	s_postEffectCVarNameOffset = 0;

#if DEBUG_GAME_FX_SYSTEM
	m_debugView = eGAME_FX_DEBUG_VIEW_None;
#endif
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initialises effects system
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::Initialise()
{
	if(m_isInitialised == false)
	{
		Reset();
		SetPostEffectCVarCallbacks();

		m_isInitialised = true;
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetPostEffectCVarCallbacks
// Desc: Sets Post effect CVar callbacks for testing and tweaking post effect values
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::SetPostEffectCVarCallbacks()
{
#if DEBUG_GAME_FX_SYSTEM
	ICVar* postEffectCvar = NULL;
	const char postEffectNames[][64] = {	"g_postEffect.FilterGrain_Amount", 
																				"g_postEffect.FilterRadialBlurring_Amount",
																				"g_postEffect.FilterRadialBlurring_ScreenPosX",
																				"g_postEffect.FilterRadialBlurring_ScreenPosY",
																				"g_postEffect.FilterRadialBlurring_Radius",
																				"g_postEffect.Global_User_ColorC",
																				"g_postEffect.Global_User_ColorM",
																				"g_postEffect.Global_User_ColorY",
																				"g_postEffect.Global_User_ColorK",
																				"g_postEffect.Global_User_Brightness",
																				"g_postEffect.Global_User_Contrast",
																				"g_postEffect.Global_User_Saturation",
																				"g_postEffect.Global_User_ColorHue" };

	const int postEffectNameCount = sizeof(postEffectNames)/sizeof(*postEffectNames);

	if(postEffectNameCount > 0)
	{
		// Calc name offset
		const char* postEffectName = postEffectNames[0];
		s_postEffectCVarNameOffset = 0;
		while((*postEffectName) != 0)
		{
			s_postEffectCVarNameOffset++;
			if((*postEffectName) == '.')
			{
				break;
			}
			postEffectName++;
		}

		// Set callback functions
		for(int i=0; i<postEffectNameCount; i++)
		{
			postEffectCvar = gEnv->pConsole->GetCVar(postEffectNames[i]);
			if(postEffectCvar)
			{
				postEffectCvar->SetOnChangeCallback(PostEffectCVarCallback);
			}
		}
	}
#endif
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: PostEffectCVarCallback
// Desc: Callback function of post effect cvars to set their values
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::PostEffectCVarCallback(ICVar* cvar)
{
	const char* effectName = cvar->GetName() + s_postEffectCVarNameOffset;
	gEnv->p3DEngine->SetPostEffectParam(effectName,cvar->GetFVal());
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: DeleteRenderNode
// Desc: Use to release render node memory created using CreateRenderNode
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::DeleteRenderNode(IRenderNode** pRenderNode)
{
	CRY_ASSERT_MESSAGE(m_isInitialised,"Game Effects System trying to delete render node without being initialised");

	if(pRenderNode && (*pRenderNode))
	{
		gEnv->p3DEngine->DeleteRenderNode(*pRenderNode);
		(*pRenderNode) = NULL;
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: DeleteEffect
// Desc: Deletes effects created by the CreateEffect function
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::DeleteEffect(IGameEffect** effect)
{
	if(effect && (*effect))
	{
		(*effect)->Release();
		SAFE_DELETE((*effect));
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: RegisterEffect
// Desc: Registers effect with effect system
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::RegisterEffect(IGameEffect* effect)
{
	CRY_ASSERT_MESSAGE(m_isInitialised,"Game Effects System trying to register an effect without being initialised");
	CRY_ASSERT_MESSAGE(effect,"Trying to Register a NULL effect");

	if(effect)
	{
		// If effect is registered, then unregister first
		if(effect->IsFlagSet(GAME_EFFECT_REGISTERED))
		{
			UnRegisterEffect(effect);
		}

		// Add effect to effect list
		IGameEffect** effectList = NULL;
		bool isActive = effect->IsFlagSet(GAME_EFFECT_ACTIVE);
		bool autoUpdatesWhenActive = effect->IsFlagSet(GAME_EFFECT_AUTO_UPDATES_WHEN_ACTIVE);
		bool autoUpdatesWhenNotActive = effect->IsFlagSet(GAME_EFFECT_AUTO_UPDATES_WHEN_NOT_ACTIVE);
		if((isActive && autoUpdatesWhenActive) || 
			((!isActive) && autoUpdatesWhenNotActive))
		{
			effectList = &m_effectsToUpdate;
		}
		else
		{
			effectList = &m_effectsNotToUpdate;
		}

		if(*effectList)
		{
			(*effectList)->SetPrev(effect);
			effect->SetNext(*effectList);
		}
		(*effectList) = effect;

		effect->SetFlag(GAME_EFFECT_REGISTERED,true);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: UnRegisterEffect
// Desc: UnRegisters effect from effect system
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::UnRegisterEffect(IGameEffect* effect)
{
	CRY_ASSERT_MESSAGE(m_isInitialised,"Game Effects System trying to unregister an effect without being initialised");
	CRY_ASSERT_MESSAGE(effect,"Trying to UnRegister a NULL effect");

	if(effect && effect->IsFlagSet(GAME_EFFECT_REGISTERED))
	{
		// If the effect is the next one to be updated, then point m_nextEffectToUpdate to the next effect after it
		if(effect == m_nextEffectToUpdate)
		{
			m_nextEffectToUpdate = m_nextEffectToUpdate->Next();
		}

		if(effect->Prev())
		{
			effect->Prev()->SetNext(effect->Next());
		}
		else
		{
			if(m_effectsToUpdate == effect)
			{
				m_effectsToUpdate = effect->Next();
			}
			else
			{
				CRY_ASSERT_MESSAGE((m_effectsNotToUpdate == effect),"Effect isn't either updating list");
				m_effectsNotToUpdate = effect->Next();
			}
		}

		if(effect->Next())
		{
			effect->Next()->SetPrev(effect->Prev());
		}
	
		effect->SetNext(NULL);
		effect->SetPrev(NULL);

		effect->SetFlag(GAME_EFFECT_REGISTERED,false);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates effects system and any effects registered in it's update list
//--------------------------------------------------------------------------------------------------
void CGameEffectsSystem::Update(float frameTime)
{
	CRY_ASSERT_MESSAGE(m_isInitialised,"Game Effects System trying to update without being initialised");

	if(m_effectsToUpdate)
	{
		IGameEffect* effect = m_effectsToUpdate;
		while(effect)
		{
			m_nextEffectToUpdate = effect->Next();
			effect->Update(frameTime);
			effect = m_nextEffectToUpdate;
		}
	}

	m_nextEffectToUpdate = NULL;

#if DEBUG_GAME_FX_SYSTEM
	DrawDebugDisplay();
#endif
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: DrawDebugDisplay
// Desc: Draws debug display
//--------------------------------------------------------------------------------------------------
#if DEBUG_GAME_FX_SYSTEM
void CGameEffectsSystem::DrawDebugDisplay()
{
	static ColorF textCol(1.0f,1.0f,1.0f,1.0f);
	static ColorF controlCol(0.6f,0.6f,0.6f,1.0f);

	static Vec2 textPos(10.0f,10.0f);
	static float textSize = 1.4f;
	static float textYSpacing = 18.0f;

	static float effectNameXOffset = 100.0f;
	static ColorF effectNameCol(0.0f,1.0f,0.0f,1.0f);

	Vec2 currentTextPos = textPos;

	int debugEffectCount = s_effectDebugList.Size();
	if((g_pGameCVars->g_gameFXSystemDebug) && (debugEffectCount > 0))
	{
		gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Debug view:");
		gEnv->pRenderer->Draw2dLabel(currentTextPos.x+effectNameXOffset,currentTextPos.y,textSize,&effectNameCol.r,false,GAME_FX_DEBUG_VIEW_NAMES[m_debugView]);
		currentTextPos.y += textYSpacing;
		gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&controlCol.r,false,"(Left/Right arrows to change debug view)");
		currentTextPos.y += textYSpacing;
		gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Debug effect:");
		gEnv->pRenderer->Draw2dLabel(currentTextPos.x+effectNameXOffset,currentTextPos.y,textSize,&effectNameCol.r,false,s_effectDebugList[s_currentDebugEffectId].effectName);
		currentTextPos.y += textYSpacing;
		gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&controlCol.r,false,"(NumPad +/- to change effect)");
		currentTextPos.y += textYSpacing;

		if(s_effectDebugList[s_currentDebugEffectId].displayCallback)
		{
			s_effectDebugList[s_currentDebugEffectId].displayCallback(currentTextPos,textSize,textYSpacing);
		}
	}
}
#endif
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: OnInputEvent
// Desc: Handles any debug input for the game effects system to test effects
//--------------------------------------------------------------------------------------------------
bool CGameEffectsSystem::OnInputEvent(const SInputEvent& inputEvent)
{
#if DEBUG_GAME_FX_SYSTEM

	int debugEffectCount = s_effectDebugList.Size();

	if((g_pGameCVars->g_gameFXSystemDebug) && (debugEffectCount > 0))
	{
		if(inputEvent.deviceId == eDI_Keyboard && inputEvent.state == eIS_Pressed)
		{
			switch(inputEvent.keyId)
			{
				case eKI_NP_Add:
				{
					if(s_currentDebugEffectId < (debugEffectCount-1))
					{
						s_currentDebugEffectId++;
					}
					break;
				}
				case eKI_NP_Substract:
				{
					if(s_currentDebugEffectId > 0)
					{
						s_currentDebugEffectId--;
					}
					break;
				}
				case eKI_Left:
				{
					if(m_debugView > 0)
					{
						OnDeActivateDebugView(m_debugView);
						m_debugView--;
						OnActivateDebugView(m_debugView);
					}
					break;
				}
				case eKI_Right:
				{
					if(m_debugView < (eMAX_GAME_FX_DEBUG_VIEWS-1))
					{
						OnDeActivateDebugView(m_debugView);
						m_debugView++;
						OnActivateDebugView(m_debugView);
					}
					break;
				}
			}

			// Send input to current debug effect
			if(s_effectDebugList[s_currentDebugEffectId].inputCallback)
			{
				s_effectDebugList[s_currentDebugEffectId].inputCallback(inputEvent.keyId);
			}
		}
	}
#endif

	return false; // Return false so that other listeners will get this event
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: OnActivateDebugView
// Desc: Called on debug view activation
//--------------------------------------------------------------------------------------------------
#if DEBUG_GAME_FX_SYSTEM
void CGameEffectsSystem::OnActivateDebugView(int debugView)
{
	switch(debugView)
	{
	case eGAME_FX_DEBUG_VIEW_Profiling:
		{
			ICVar* r_displayInfoCVar = gEnv->pConsole->GetCVar("r_DisplayInfo");
			if(r_displayInfoCVar)
			{
				r_displayInfoCVar->Set(1);
			}
			break;
		}
	}
}
#endif
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: OnDeActivateDebugView
// Desc: Called on debug view de-activation
//--------------------------------------------------------------------------------------------------
#if DEBUG_GAME_FX_SYSTEM
void CGameEffectsSystem::OnDeActivateDebugView(int debugView)
{
	switch(debugView)
	{
	case eGAME_FX_DEBUG_VIEW_Profiling:
		{
			ICVar* r_displayInfoCVar = gEnv->pConsole->GetCVar("r_DisplayInfo");
			if(r_displayInfoCVar)
			{
				r_displayInfoCVar->Set(0);
			}
			break;
		}
	}
}
#endif
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: RegisterEffectDebugData
// Desc: Registers effect's debug data with the game effects system, which will then call the 
//			 relevant debug callback functions for the for the effect when its selected using the 
//			 s_currentDebugEffectId
//--------------------------------------------------------------------------------------------------
#if DEBUG_GAME_FX_SYSTEM
void CGameEffectsSystem::RegisterEffectDebugData(	DebugOnInputEventCallback inputEventCallback,
																									DebugDisplayCallback displayCallback,
																									const char* effectName)
{
	s_effectDebugList.push_back(SEffectDebugData(inputEventCallback,displayCallback,effectName));
}
#endif
//--------------------------------------------------------------------------------------------------
