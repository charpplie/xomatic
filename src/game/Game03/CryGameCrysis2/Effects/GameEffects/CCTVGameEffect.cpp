//==================================================================================================
// Name: CCCTVGameEffect
// Desc: CCTV game effect
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "CCTVGameEffect.h"
#include "Effects/RenderNodes/ScreenFilterRenderNode.h"
#include "IItemSystem.h"
#include "GameCVars.h"
#include "Player.h"

REGISTER_EFFECT_DEBUG_DATA(CCCTVGameEffect::DebugOnInputEvent,CCCTVGameEffect::DebugDisplay,CCTV);

//--------------------------------------------------------------------------------------------------
// Name: CCCTVNoiseControlData
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CCCTVNoiseControlData::CCCTVNoiseControlData()
{
	m_minValue = 0.0f;
	m_maxValue = 0.0f;
	m_currentValue = 0.0f;

	m_minTime = 0.0f;
	m_maxTime = 0.0f;
	m_currentTime = 0.0f;

	m_minAudio = 0.0f;
	m_maxAudio = 0.0f;
	m_currentAudio = 0.0f;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initialises noise control data
//--------------------------------------------------------------------------------------------------
void CCCTVNoiseControlData::Initialise(const IItemParamsNode* xmlNode)
{
	CRY_ASSERT_MESSAGE(xmlNode,"Missing XML node for noise in CCTV effect");
	
	if(xmlNode)
	{
		xmlNode->GetAttribute("minValue",m_minValue);
		xmlNode->GetAttribute("maxValue",m_maxValue);
		xmlNode->GetAttribute("minTime",m_minTime);
		xmlNode->GetAttribute("maxTime",m_maxTime);
		xmlNode->GetAttribute("minAudio",m_minAudio);
		xmlNode->GetAttribute("maxAudio",m_maxAudio);
	}

	RandomiseNoise();
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: RandomiseNoise
// Desc: Randomise noise data
//--------------------------------------------------------------------------------------------------
void CCCTVNoiseControlData::RandomiseNoise()
{
	m_currentValue = Random(m_minValue,m_maxValue);
	m_currentTime = Random(m_minTime,m_maxTime);

	// Use the m_currentValue to interpolate the audio value
	m_currentAudio = LERP(m_minAudio,m_maxAudio,((m_currentValue-m_minValue)/(m_maxValue-m_minValue)));
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: CCCTVGameEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CCCTVGameEffect::CCCTVGameEffect()
{
	m_renderNode = NULL;
	m_countDownTimer = 0.0f;
	m_noiseState = eCCTV_Noise_Standard;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CCCTVGameEffect
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CCCTVGameEffect::~CCCTVGameEffect()
{
	
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initializes game effect
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::Initialise(const SGameEffectParams* gameEffectParams)
{
	CGameEffect::Initialise(gameEffectParams);
	
	m_renderNode = GAME_FX_SYSTEM.CreateRenderNode<CScreenFilterRenderNode>();

	SCCTVGameEffectParams cctvGameEffectParams;
	if(gameEffectParams)
	{
		cctvGameEffectParams = *(SCCTVGameEffectParams*)(gameEffectParams);
	}

	CRY_ASSERT_MESSAGE(cctvGameEffectParams.xmlNode,"No xml data for CCTV game effect");

	if(cctvGameEffectParams.xmlNode)
	{
		// Initialise screen filter render node
		const IItemParamsNode* screenFilterXmlNode = cctvGameEffectParams.xmlNode->GetChild("ScreenFilter");
		const char* materialName = NULL;
		if(screenFilterXmlNode)
		{
			materialName = screenFilterXmlNode->GetAttribute("material");
		}
		CRY_ASSERT_MESSAGE(materialName,"Failed to find material name for cctv screen filter in player data");

		SScreenFilterRenderNodeParams renderNodeParams;
		renderNodeParams.material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialName,false);
		CRY_ASSERT_MESSAGE(renderNodeParams.material,"CCTV effect initialised without a material");
		m_renderNode->SetParams(renderNodeParams);

		m_renderNode->SetRndFlags(ERF_HIDDEN,true);

		// Initialise HUD
		m_CCTVHudListener.Initialise(cctvGameEffectParams.xmlNode->GetChild("Hud"));

		// Initialise CVars
		m_cvarActivationSystem.Initialise(cctvGameEffectParams.xmlNode->GetChild("CVars"));

		// Initialise post effects
		m_postEffectActivationSystem.Initialise(cctvGameEffectParams.xmlNode->GetChild("PostEffects"));

		// Initialise noise
		const IItemParamsNode* noiseXmlNode = cctvGameEffectParams.xmlNode->GetChild("NoiseStates");
		int noiseStateCount = noiseXmlNode->GetChildCount();
		CRY_ASSERT_MESSAGE(noiseStateCount == eCCTV_Noise_State_Count,"XML CCTV noise state count differs from code state count");

		for(int i=0; i<noiseStateCount; i++)
		{
			m_noiseControlData[i].Initialise(noiseXmlNode->GetChild(i));
		}

		m_countDownTimer = m_noiseControlData[eCCTV_Noise_Standard].GetTime();

		// Initialise sound FX
		m_noiseAudio.SetSignal("CCTV_Loop");
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Release
// Desc: Releases game effect
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::Release()
{
	CGameEffect::Release();
	
	GAME_FX_SYSTEM.DeleteRenderNode((IRenderNode**)&m_renderNode);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates game effect
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::Update(float frameTime)
{
	CGameEffect::Update(frameTime);

	// Update render node
	if(m_renderNode)
	{
		m_renderNode->UpdatePosFromCamera();
	}

	// Update noise
	m_countDownTimer -= frameTime;
	if(m_countDownTimer <= 0.0f)
	{
		switch(m_noiseState)
		{
			case eCCTV_Noise_Standard:	{ m_noiseState = eCCTV_Noise_Intense; break; }
			case eCCTV_Noise_Intense:		{ m_noiseState = eCCTV_Noise_Standard; break; }
		}

		EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
		m_noiseControlData[m_noiseState].RandomiseNoise();
		m_countDownTimer = m_noiseControlData[m_noiseState].GetTime();
		gEnv->p3DEngine->SetPostEffectParam( "FilterGrain_Amount", m_noiseControlData[m_noiseState].GetValue() );
		m_noiseAudio.SetParam(localClientId, "static", m_noiseControlData[m_noiseState].GetAudioLevel());
	}

	// Set cvars and post effects on every frame due to other things changing them after effect activation
#if DEBUG_GAME_FX_SYSTEM
	static bool setVarsEveryFrame = true;
	if(setVarsEveryFrame)
#endif
	{
		m_cvarActivationSystem.SetCVarsActive(true);
		m_postEffectActivationSystem.SetPostEffectsActive(true);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Start
// Desc: Starts effect
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::Start()
{
	if(IsFlagSet(GAME_EFFECT_ACTIVE) == false)
	{
		m_CCTVHudListener.Register();

		if(m_renderNode)
		{
			m_renderNode->SetRndFlags(ERF_HIDDEN,false);
		}

		// Set CVars
		m_cvarActivationSystem.StoreCurrentValues();
		m_cvarActivationSystem.SetCVarsActive(true);
		
		// Set post effects
		m_postEffectActivationSystem.SetPostEffectsActive(true);
		gEnv->p3DEngine->SetPostEffectParam( "FilterGrain_Amount", m_noiseControlData[m_noiseState].GetValue() );

		// Sound FX
		EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
		CAudioSignalPlayer::JustPlay("CCTV_Enable",localClientId);

		m_noiseAudio.Play(localClientId);
		m_noiseAudio.SetParam(localClientId, "static", 0.5f);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Stop
// Desc: Stops effect
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::Stop()
{
	if(IsFlagSet(GAME_EFFECT_ACTIVE))
	{
		m_CCTVHudListener.UnRegister();

		if(m_renderNode)
		{
			m_renderNode->SetRndFlags(ERF_HIDDEN,true);
		}

		// Set CVars back to original values
		m_cvarActivationSystem.SetCVarsActive(false);

		// Set post effect back
		m_postEffectActivationSystem.SetPostEffectsActive(false);
		gEnv->p3DEngine->SetPostEffectParam( "FilterGrain_Amount", 0.0f );

		// Sound FX
		EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
		CAudioSignalPlayer::JustPlay("CCTV_Disable",localClientId);
		m_noiseAudio.Stop(localClientId);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetActive
// Desc: Sets active status of effect
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::SetActive(bool isActive)
{
	if(isActive)
	{
		Start();
	}
	else
	{
		Stop();
	}
	CGameEffect::SetActive(isActive);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetCameraEntityId
// Desc: Sets camera entity Id and updates camera name
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::SetCameraEntityId(EntityId cameraEntityId)
{
	m_CCTVHudListener.SetCameraEntityId(cameraEntityId);
}//-------------------------------------------------------------------------------------------------

#if DEBUG_GAME_FX_SYSTEM
//--------------------------------------------------------------------------------------------------
// Name: DebugOnInputEvent
// Desc: Called when input events happen in debug builds
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::DebugOnInputEvent(int keyId)
{
	EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	CPlayer* player = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(localClientId));

	switch(keyId)
	{
		case eKI_NP_1:
		{
			player->GetCCTVScreenEffect()->SetActive(true);
			break;
		}
		case eKI_NP_2:
		{
			player->GetCCTVScreenEffect()->SetActive(false);
			break;
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: DebugDisplay
// Desc: Display when this effect is selected to debug through the game effects system
//--------------------------------------------------------------------------------------------------
void CCCTVGameEffect::DebugDisplay(const Vec2& textStartPos,float textSize,float textYStep)
{
	ColorF textCol(1.0f,1.0f,0.0f,1.0f);
	Vec2 currentTextPos = textStartPos;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Turn on: NumPad 1");
	currentTextPos.y += textYStep;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Turn off: NumPad 2");
}//-------------------------------------------------------------------------------------------------
#endif