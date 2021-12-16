#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameActions.h"


#include "HUD/HUDSilhouettes.h"
#include "HUD/HUD.h"
#include "HUD/HUD_Interactive.h"
#include "Weapon.h"
#include "Nodes/G2FlowBaseNode.h"
#include "HUDNodes.h"
#include "PlayerNavPath.h"

#include <StringUtils.h>
#include "Graphics/2DRenderUtils.h"

class CFlowNode_HUDSilhouettes : public CFlowBaseNode 
{
	enum INPUTS
	{
		EIP_Activate = 0,
		EIP_Deactivate,
		EIP_Color,
	};

public:
	CFlowNode_HUDSilhouettes( SActivationInfo * pActInfo ) { }

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void  ( "Activate", _HELP("Trigger to activate. This sets a permanent silhouette (until removed), overwriting automated ones." )),
			InputPortConfig_Void  ( "Deactivate", _HELP("Trigger to deactivate" )),
			InputPortConfig<Vec3>  ( "Color", Vec3(1.0f,0.0f,0.0f), _HELP("Color"), 0, _UICONFIG("dt=clr")),
			{0}
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.sDescription = _HELP("HUD Silhouette Shader");
		config.SetCategory(EFLN_APPROVED);
	}

	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}

	//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Activate == event)
		{
			if(!pActInfo->pEntity)
				return;

			const bool activate = IsPortActive(pActInfo, EIP_Activate);
			const bool deactivate = IsPortActive(pActInfo, EIP_Deactivate);
			if (!activate && !deactivate)
				return;

			if (deactivate)
				g_pGame->GetHUD()->GetSilhouettes()->ResetFlowGraphSilhouette(pActInfo->pEntity->GetId());
				
			if (activate)
			{
				const Vec3& color = GetPortVec3(pActInfo, EIP_Color);
				g_pGame->GetHUD()->GetSilhouettes()->SetFlowGraphSilhouette(pActInfo->pEntity, color.x, color.y, color.z, 1.0f, -1.0f);
			}
		}
	}
};

CRY_FIXME(7, 7, 2009, "Remove this Flownode!")
class CFlowNode_HUDNightVision : public CFlowBaseNode
{
	enum INPUTS
	{
		EIP_Activate = 0,
		EIP_Deactivate,
	};

	enum OUTPUTS
	{
		EOP_Activated = 0,
		EOP_Deactivated,
	};

public:
	CFlowNode_HUDNightVision( SActivationInfo * pActInfo )
	{
	}

	~CFlowNode_HUDNightVision()
	{
		SAFE_HUD_FUNC(UnRegisterListener(this));
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_HUDNightVision(pActInfo);
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void( "Activate",  _HELP("Activate NightVision"  )),
			InputPortConfig_Void( "Deactivate", _HELP("Deactivate NightVision" )),
			{0}
		};
		static const SOutputPortConfig out_ports[] = 
		{
			OutputPortConfig_Void ( "Activated",  _HELP("Triggered when NightVision is activated" )),
			OutputPortConfig_Void ( "Deactivated",  _HELP("Triggered when NightVision is deactivated" )),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("HUD NightVision\rIf an entity is not provided, the local player will be used instead");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
				SAFE_HUD_FUNC(RegisterListener(this));
			}
			break;
		case eFE_Activate:
			{
				if (InputEntityIsLocalPlayer( pActInfo )) 
				{
					const CGameActions &rGameActions = g_pGame->Actions();
					if (IsPortActive(pActInfo, EIP_Activate))
					{
						SAFE_HUD_FUNC(OnAction(rGameActions.hud_night_vision, eIS_Pressed, 0.0f));
					}
					if (IsPortActive(pActInfo, EIP_Deactivate))
					{
						SAFE_HUD_FUNC(OnAction(rGameActions.hud_night_vision, eIS_Released, 0.0f));
					}
				}
			}
			break;
		}
	}

protected:
	virtual void OnNightVision(bool bEnabled)
	{
		ActivateOutput(&m_actInfo, bEnabled ? EOP_Activated : EOP_Deactivated, true);
	}

	SActivationInfo m_actInfo;
};

class CFlowNode_HUDObjectives : public CFlowBaseNode
{
	enum INPUTS
	{
		EIP_SetGoal = 0,
		EIP_Goal,
		EIP_SetMain,
		EIP_Main,
	};

public:
	CFlowNode_HUDObjectives( SActivationInfo * pActInfo )
	{
	}

	~CFlowNode_HUDObjectives()
	{
	}

	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}

	//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void   ( "SetGoal", _HELP("Set Goal Objective" )),
			InputPortConfig<string> ("mission_Goal", _HELP("Goal Objective")),
			InputPortConfig_Void   ( "SetMain", _HELP("Set Main Objective"), _HELP("SetMainObjective") ),
			InputPortConfig<string> ("mission_Main", _HELP("Main Objective"), _HELP("MainObjective") ),
			{0}
		};
		config.pInputPorts = in_ports;
		config.pOutputPorts = 0;
		config.sDescription = _HELP("HUD Objective support for Goal and MainObjective");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event != eFE_Activate)
			return;

		if (InputEntityIsLocalPlayer( pActInfo )) 
		{
			if (IsPortActive(pActInfo, EIP_SetMain))
			{
				const string& v = GetPortString(pActInfo, EIP_Main);
				SAFE_HUD_FUNC(SetMainObjective(v.c_str(), false));
			}
			if (IsPortActive(pActInfo, EIP_SetGoal))
			{
				const string& v = GetPortString(pActInfo, EIP_Goal);
				SAFE_HUD_FUNC(SetMainObjective(v.c_str(), true));
			}
		}
	}
};

class CFlowNode_HUDTutorial : public CFlowBaseNode
{
	enum INPUTS
	{
		EIP_Show = 0,
		EIP_Hide,
		EIP_Message,
		EIP_Position,
		EIP_Mode,
		EIP_Timeout,
	};

	enum OUTPUTS
	{
		EOP_Done = 0
	};

public:
	CFlowNode_HUDTutorial( SActivationInfo * pActInfo )
	{
	}

	~CFlowNode_HUDTutorial()
	{
	}

	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}

	//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void   ( "Show", _HELP("Show the message in the specified MessageBox" )),
			InputPortConfig_Void   ( "Hide", _HELP("Hide the MessageBox" )),
			InputPortConfig<string>( "Message", _HELP("Message to show"), 0, _UICONFIG("dt=text")),
			InputPortConfig<int>   ( "Position", 0, _HELP("Which message box"), 0, _UICONFIG("enum_int:Left=0,Right=1")),
			InputPortConfig<int>   ( "Mode", 0, _HELP("Mode: Add or Replace Message"), 0, _UICONFIG("enum_int:Add=0,Replace=1")),
			InputPortConfig<float> ( "Timeout", 0.0f, _HELP("How long to show message. 0.0 = Until hidden")),
			{0}
		};
		static const SOutputPortConfig out_ports[] =
		{
			OutputPortConfig_Void( "Done", _HELP("Triggered when Done")),
			{0}
		};
		config.pInputPorts = in_ports;
		config.pOutputPorts = out_ports;
		config.sDescription = _HELP("Show Tutorial Messages on the HUD");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event != eFE_Activate)
			return;
	}
};


class CFlowNode_HUDInteractives : public CFlowBaseNode 
{
	enum INPUTS
	{
		EIP_Show = 0,
		EIP_Button,
		EIP_Text,
		EIP_Duration
	};

public:
	CFlowNode_HUDInteractives( SActivationInfo * pActInfo ) { }

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void  ( "Show", _HELP("Trigger to show the hint." )),
			InputPortConfig<string> ("Button", _HELP("Button that should be displayed with the hint."), 0, _UICONFIG("enum_string:X=X,Y=Y,A=A,B=B,RightBumper=Bumper,LeftBumper=Bumper,RightTrigger=Trigger,LeftTrigger=Trigger")),
			InputPortConfig<string> ("Text", _HELP("Text that should be shown as a hint.")),
			InputPortConfig<float> ("Duration", _HELP("Hint will be displayed for the specified time.")),
			{0}
		};

		config.pInputPorts = in_ports;
		config.sDescription = _HELP("HUD interactive hint display");
		config.SetCategory(EFLN_APPROVED);
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(eFE_Activate == event)
		{
			if(!IsPortActive(pActInfo, EIP_Show))
				return;

			const string& button = GetPortString(pActInfo, EIP_Button);
			const string& text = GetPortString(pActInfo, EIP_Text);
			const float& duration = GetPortFloat(pActInfo, EIP_Duration);

			//			g_pGame->GetHUD()->Interactivity()->SetInteraction(text.c_str(), button.c_str(), duration);
		}
	}
};


REGISTER_FLOW_NODE("HUD:SilhouetteOutline",						 CFlowNode_HUDSilhouettes);
REGISTER_FLOW_NODE("HUD:NightVision",									 CFlowNode_HUDNightVision);
REGISTER_FLOW_NODE_SINGLETON("HUD:Objectives",				 CFlowNode_HUDObjectives);
REGISTER_FLOW_NODE_SINGLETON("HUD:InteractiveHints",	 CFlowNode_HUDInteractives);




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// temporary use of an HUD fader object until the support is properly done in the HUD code

CHUDFaderObject CHUDFaderObject::m_instance;
CHUDFaderObject* g_pHUDFader = CHUDFaderObject::GetHUDFaderObject();

//////////////////////////////////////////////////////////////////////////
CHUDFader::CHUDFader() :	
m_currentColor(Col_Black),
m_targetColor(Col_Black), 
m_drawColor(Col_Black), 
m_pTexture(0), 
m_bActive(false), 
m_fDuration(0.0f), 
m_fCurTime(0.0f), 
m_iDirection(0), 
m_iTicket(0), 
m_fTopLeftX(0.0f), 
m_fTopLeftY(0.0f), 
m_fWidth(800.0f), 
m_fHeight(600.0f),
m_fRotation(0.0f),
m_fAlpha(1.0f),
m_iZLevel(0),
m_bIsFading(false),
m_bDrawBeforeHUD(false)
{
	m_pRenderer = gEnv->pRenderer;
	CRY_ASSERT (m_pRenderer != 0);
}

//////////////////////////////////////////////////////////////////////////
CHUDFader::~CHUDFader()
{
	// FIXME: Crash on close. Store a textureID instead.
	//SAFE_RELEASE(m_pTexture);
}

//////////////////////////////////////////////////////////////////////////
const char* CHUDFader::GetDebugName() const
{
	if (m_pTexture)
		return m_pTexture->GetName();
	return "<no texture>";
}

//////////////////////////////////////////////////////////////////////////
void CHUDFader::Stop()
{
	m_bActive = false;
}

//////////////////////////////////////////////////////////////////////////
void CHUDFader::Reset()
{
	m_iTicket = 0;
	m_bActive = false;
}

//////////////////////////////////////////////////////////////////////////
ColorF CHUDFader::GetCurrentColor() const
{
	float curVal = 1.0;
	if (m_fDuration != 0.0f)
	{
		curVal = m_fCurTime / m_fDuration;
		curVal = clamp(curVal, 0.0f, 1.0f);
	}
	ColorF col;
	col.lerpFloat(m_currentColor, m_targetColor, curVal);
	return col;
}

//////////////////////////////////////////////////////////////////////////
int CHUDFader::FadeIn(const ColorF& targetColor, float fDuration, bool bUseCurrentColor)
{
	if (m_iTicket == 0 || bUseCurrentColor == false)
	{
		m_currentColor = targetColor;
		m_currentColor.a = 1.0f;
	}
	else
		m_currentColor = GetCurrentColor();
	m_targetColor = targetColor;
	m_targetColor.a = 0.0f;
	m_fDuration = max(fDuration,0.0f);
	m_fCurTime = 0.0f;
	m_iDirection = -1;
	m_bActive = true;
	m_bIsFading = true;
	return ++m_iTicket;
}

//////////////////////////////////////////////////////////////////////////
int CHUDFader::FadeOut(const ColorF& targetColor, float fDuration, const char* textureName, bool bUseCurrentColor)
{
	SetTexture(textureName);
	if (m_iTicket == 0 || bUseCurrentColor == false)
	{
		m_currentColor = targetColor;
		m_currentColor.a = 0.0f;
	}
	else
		m_currentColor = GetCurrentColor();

	m_targetColor = targetColor;
	m_targetColor.a = 1.0f;
	m_fDuration = max(fDuration,0.0f);
	m_fCurTime = 0.0f;
	m_iDirection = 1;
	m_bActive = true;
	m_bIsFading = true;
	return ++m_iTicket;
}

//////////////////////////////////////////////////////////////////////////
void CHUDFader::Update(float fDeltaTime)
{
	if (m_bActive == false)
		return;

	if (m_fCurTime >= m_fDuration)
	{
		m_bIsFading = false;
		// when we're logically supposed to fade 'in' (meaning, to go away), do so
		if (m_iDirection < 0)
		{
			m_bActive = false;
			m_currentColor = m_targetColor;
		}
		m_currentColor = m_targetColor;
		m_iTicket = 0;
	}
	m_drawColor = GetCurrentColor();
	m_fCurTime+=fDeltaTime;
}

//////////////////////////////////////////////////////////////////////////
void CHUDFader::Draw()
{
	if (m_bActive == false)
		return;

	m_pRenderer->Draw2dImage(m_fTopLeftX, m_fTopLeftY, m_fWidth, m_fHeight,
		m_pTexture ? m_pTexture->GetTextureID() : 0,
		0.0f,1.0f,1.0f,0.0f, // tex coords
		m_fRotation, // angle
		m_drawColor.r, m_drawColor.g, m_drawColor.b, m_drawColor.a * m_fAlpha, 0
		);
}

//////////////////////////////////////////////////////////////////////////
ITexture* CHUDFader::LoadTexture(const char* textureName)
{
	if (gEnv->pRenderer)
		return gEnv->pRenderer->EF_LoadTexture(textureName,FT_DONT_RESIZE|FT_DONT_STREAM|FT_STATE_CLAMP,eTT_2D);
	else
		return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CHUDFader::SetTexture(const char* textureName)
{
	if (m_pTexture && textureName)
	{
		const char* ptr = strstr(textureName,".tif");
		if (ptr)
		{
			// a.tif and a.dds is the same 
			if ( strncmp(textureName, m_pTexture->GetName(), ptr - textureName) == 0 )
				return;
		}
		else
		{
			if ( strcmp(textureName, m_pTexture->GetName()) == 0 )
				return;
		}
	}

	SAFE_RELEASE(m_pTexture);
	if (textureName == 0 || *textureName == 0)
		return;

	m_pTexture = LoadTexture(textureName);
	if (m_pTexture != 0)
		m_pTexture->SetClamp(true);
}

//////////////////////////////////////////////////////////////////////////

CHUDFaderObject::CHUDFaderObject() : IHUDEventListener() //: CHUDObject()
{
	m_bRegistered = false;
}

//////////////////////////////////////////////////////////////////////////
CHUDFaderObject::~CHUDFaderObject()
{
	UnRegister();
	std::list<CHUDFader*>::iterator it = m_listHUDFader.begin();
	std::list<CHUDFader*>::iterator itend = m_listHUDFader.end();
	for(; it != itend; ++it)
	{
		SAFE_DELETE(*it);
	}
}

//////////////////////////////////////////////////////////////////////////
CHUDFader* CHUDFaderObject::GetHUDFader(int iZLevel)
{
	if (!m_bRegistered)
		Register();

	CHUDFader* pHUDFader = NULL;
	std::list<CHUDFader*>::iterator it = m_listHUDFader.begin();
	std::list<CHUDFader*>::iterator itend = m_listHUDFader.end();
	for(; it != itend; ++it)
	{
		if(iZLevel == (*it)->GetZLevel())
		{
			pHUDFader = *it;
			break;
		}
		if(iZLevel < (*it)->GetZLevel())
		{
			pHUDFader = new CHUDFader;
			pHUDFader->SetZLevel(iZLevel);
			m_listHUDFader.insert(it,pHUDFader);
			break;
		}
	}

	if(pHUDFader == NULL)
	{
		pHUDFader = new CHUDFader;
		pHUDFader->SetZLevel(iZLevel);
		m_listHUDFader.push_back(pHUDFader);
	}

	return pHUDFader;
}


//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::OnHUDEvent(const SHUDEvent& event)
{
	switch(event.eventType)
	{
		case eHUDEvent_OnHUDUnload:
		{
			m_bRegistered = false;
			break;
		}
		case eHUDEvent_OnPostHUDDraw:
		{
			PostHUDDraw();
			break;
		}
		default:
		{
			Update( event.eventFloatData );
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::Update( float fDeltaTime )
{
	int nActive = 0;

	std::list<CHUDFader*>::iterator it = m_listHUDFader.begin();
	std::list<CHUDFader*>::iterator itend = m_listHUDFader.end();
	for(; it != itend; ++it)
	{
		(*it)->Update(fDeltaTime);
		if((*it)->IsDrawnBeforeHUD())
		{
			(*it)->Draw();
		}
	}

	if (g_pGameCVars->hud_faderDebug != 0)
	{
		for(it = m_listHUDFader.begin(); it != itend; ++it)
		{
			if ((*it)->IsActive())
				++nActive;
		}
		const float fX = 5.0f;
		float fY = 115.0f;
		float white[] = {1,1,1,1};
		gEnv->pRenderer->Draw2dLabel( fX, fY, 1.2f, white, false, "HUDFader Active: %d", nActive );
		fY+=12.0f;
		if (nActive > 0)
		{
			for(it = m_listHUDFader.begin(); it != itend; ++it)
			{
				if ((*it)->IsActive())
				{
					const ColorF colF = (*it)->GetCurrentColor();
					const ColorB colB (colF);
					gEnv->pRenderer->Draw2dLabel( fX, fY, 1.2f, white, false, " Fader : ColRGBA=%d,%d,%d,%d Texture='%s'", colB.r, colB.g, colB.b, colB.a, (*it)->GetDebugName());

				}
				fY+=10.0f;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::PostHUDDraw()
{
	g_pGame->GetHUD()->Get2DRenderUtils()->PreRender();

	std::list<CHUDFader*>::iterator it = m_listHUDFader.begin();
	std::list<CHUDFader*>::iterator itend = m_listHUDFader.end();
	for(; it != itend; ++it)
	{
		if((*it)->IsDrawnBeforeHUD() == false)
		{
			(*it)->Draw();
		}
	}

	g_pGame->GetHUD()->Get2DRenderUtils()->PostRender();
}

//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::OnHUDToBeDestroyed()
{
	m_bRegistered = false;
	Reset();
}

//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::Serialize(TSerialize ser)
{
	if (ser.IsReading())
	{
		Reset();
	}
}

//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::Register()
{
	if (m_bRegistered == false)
	{
		CHUD* pHUD = g_pGame->GetHUD();
		if (pHUD)
		{
			pHUD->AddHUDEventListener(this, "OnUpdate");
			pHUD->AddHUDEventListener(this, "OnPostHUDDraw");
			pHUD->AddHUDEventListener(this, "OnHUDUnload");
			m_bRegistered = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::UnRegister()
{
	if (m_bRegistered == true)
	{
		CHUD* pHUD = g_pGame->GetHUD();
		if (pHUD)
		{

			pHUD->RemoveHUDEventListener(this);
			m_bRegistered = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CHUDFaderObject::Reset()
{
	std::list<CHUDFader*>::iterator it = m_listHUDFader.begin();
	std::list<CHUDFader*>::iterator itend = m_listHUDFader.end();
	for(; it != itend; ++it)
	{
		(*it)->Reset();
	}
}


//////////////////////////////////////////////////////////////////////////

class CFlowNode_ScreenFader : public CFlowBaseNode
{
	CHUDFader* GetFader(SActivationInfo * pActInfo)
	{
		int group = GetPortInt(pActInfo, EIP_FadeGroup);
		group+=m_nFaderOffset;
		return g_pHUDFader->GetHUDFader(group);
	}

public:
	CFlowNode_ScreenFader( SActivationInfo * pActInfo ) : m_bPlaying(false), m_bNeedFaderStop(false), m_direction(0), m_ticket(0), m_postSerializeTrigger(0)
	{
		//HACK HACK HACK HACK HACK HACK HACK HACK
		m_pTexture = gEnv->pRenderer->EF_LoadTexture("textures/sprites/hud/blood2_bullet.dds", FT_DONT_RESIZE|FT_DONT_STREAM|FT_STATE_CLAMP,eTT_2D);
	}

	~CFlowNode_ScreenFader()
	{
		SAFE_RELEASE(m_pTexture);
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowNode_ScreenFader(pActInfo);
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		ser.Value("m_bPlaying", m_bPlaying);
		ser.Value("m_direction", m_direction);
		ser.Value("m_postSerializeTrigger", m_postSerializeTrigger);

		if (ser.IsReading())
		{
			// in case we were playing before the fader is stopped on load,
			// but still we need to activate outputs. this MUST NOT be done in serialize
			// but in ProcessEvent
			if (m_bPlaying)
			{
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				m_postSerializeTrigger = m_direction;
			}
			m_bPlaying = false;
			m_bNeedFaderStop = false;
			m_ticket = 0;
			m_direction = 0;
		}
	}

	void StopFader(SActivationInfo * pActInfo)
	{
		CHUDFader* pFader = GetFader(pActInfo);
		if (pFader)
			pFader->Stop();
		m_ticket = 0;
		m_bNeedFaderStop = false;
	}

	enum EInputPorts
	{
		EIP_FadeGroup = 0,
		EIP_FadeIn,
		EIP_FadeOut,
		EIP_UseCurrentColor,
		EIP_InTime,
		EIP_OutTime,
		EIP_Color,
		EIP_TextureName
	};

	enum EOutputPorts
	{
		EOP_FadedIn = 0,
		EOP_FadedOut,
		EOP_FadeColor
	};

	void StartFader(SActivationInfo * pActInfo)
	{
		CHUDFader* pFader = GetFader(pActInfo);
		if (pFader != 0)
		{
			pFader->SetPosition(0.0f,0.0f);
			pFader->SetDimension(800.0f,600.0f); // the HUDFader uses a 800x600 coordinate system
			pFader->SetRotation(0.0f);
			const float fDuration = GetPortFloat(pActInfo, m_direction < 0 ? EIP_InTime : EIP_OutTime);
			ColorF col;
			const Vec3 fadeColor = GetPortVec3(pActInfo, EIP_Color);
			const bool bUseCurColor = GetPortBool(pActInfo, EIP_UseCurrentColor);
			col.r = fadeColor[0];
			col.g = fadeColor[1];
			col.b = fadeColor[2];
			col.a = m_direction < 0 ? 0.0f : 1.0f;
			if (m_direction < 0)
				m_ticket = pFader->FadeIn(col, fDuration, bUseCurColor);
			else
				m_ticket =pFader->FadeOut(col, fDuration, GetPortString(pActInfo, EIP_TextureName), bUseCurColor);
			m_bNeedFaderStop = true;
		}
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig<int> ("FadeGroup", 0, _HELP("Fade Group [0-3]"), 0, _UICONFIG("enum_int:0=0,1=1,2=2,3=3")),
			InputPortConfig_Void("FadeIn", _HELP("Fade back from the specified color back to normal screen")),
			InputPortConfig_Void("FadeOut", _HELP("Fade the screen to the specified color")),
			InputPortConfig<bool> ("UseCurColor", true, _HELP("If checked, use the current color as Source color. Otherwise use [FadeColor] as Source color and Target color.")),
			InputPortConfig<float>("FadeInTime", 2.0f, _HELP("Duration of fade in")),
			InputPortConfig<float>("FadeOutTime", 2.0f, _HELP("Duration of fade out")),
			InputPortConfig<Vec3> ("color_FadeColor", _HELP("Target Color to fade to")),
			InputPortConfig<string> ("tex_TextureName", _HELP("Texture Name")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("FadedIn", _HELP("FadedIn")),
			OutputPortConfig_Void("FadedOut", _HELP("FadedOut")),
			OutputPortConfig<Vec3> ("CurColor", _HELP("Current Faded Color")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Controls Screen Fading.");
		config.SetCategory(EFLN_ADVANCED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(gEnv->bMultiplayer)
		{
			CRY_FIXME(04, 12, 2009, "Aborted use of CFlowNode_ScreenFader in multiplayer to avoid synchronous loading stall");
			return;
		}

		switch (event)
		{
		case eFE_Initialize:
			if (pActInfo->pGraph->GetGraphEntity(0) == 0 && pActInfo->pGraph->GetGraphEntity(1) == 0)
				m_nFaderOffset = MFX_FADER_OFFSET;
			else
				m_nFaderOffset = GAME_FADER_OFFSET;

			//if (gEnv->pCryPak->GetLvlResStatus())
/*
			{
				const string& texName = GetPortString(pActInfo, EIP_TextureName);
				if (texName.empty() == false)
				{
					ITexture* pTexture = CHUDFader::LoadTexture(texName.c_str());
					if (pTexture)
						pTexture->Release();

				}
			}*/


			if (m_bNeedFaderStop)
			{
				StopFader(pActInfo);
				m_bPlaying = false;
				m_bNeedFaderStop = false;
				m_ticket = 0;
			}
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
			break;
		case eFE_Activate:
			if (InputEntityIsLocalPlayer( pActInfo )) 
			{
				if (IsPortActive(pActInfo, EIP_FadeIn))
				{
					StopFader(pActInfo);
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
					m_direction = -1;
					StartFader(pActInfo);
					m_bPlaying = true;
					m_bNeedFaderStop = true;
					m_postSerializeTrigger = 0;
				}
				if (IsPortActive(pActInfo, EIP_FadeOut))
				{
					StopFader(pActInfo);
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
					m_direction = 1;
					StartFader(pActInfo);
					m_bPlaying = true;
					m_bNeedFaderStop = true;
					m_postSerializeTrigger = 0;
				}
			}
			break;
		case eFE_Update:
			if (InputEntityIsLocalPlayer( pActInfo )) 
			{
				if (m_postSerializeTrigger)
				{
					ActivateOutput(pActInfo, m_postSerializeTrigger < 0 ? EOP_FadedIn : EOP_FadedOut, true);
					m_postSerializeTrigger = 0;
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					return;
				}

				CHUDFader* pFader = GetFader(pActInfo);
				if (pFader == 0 || m_bPlaying == false)
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);	
					m_bPlaying = false;
					m_ticket = 0;
					return;
				}

				const ColorF &col = pFader->GetCurrentColor();
				const Vec3 vCol (col.r,col.g,col.b);
				ActivateOutput(pActInfo, EOP_FadeColor, vCol);
				if (pFader->IsPlaying(m_ticket) == false)
				{
					if (m_direction < 0.0f)
					{
						ActivateOutput(pActInfo, EOP_FadedIn, true);
						pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);					
						m_bNeedFaderStop = false; 
					}
					else
					{
						ActivateOutput(pActInfo, EOP_FadedOut, true);
						pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);	
						m_bNeedFaderStop = true; // but needs a stop, if we're faded out (fader is still active then!)
					}
					m_bPlaying = false;
					m_ticket = 0;
				}
			}
			break;
		}
	}
protected:
	ITexture  *m_pTexture;
	int				m_ticket;
	int				m_direction;
	int				m_nFaderOffset;
	int				m_postSerializeTrigger;
	bool			m_bPlaying;
	bool			m_bNeedFaderStop;
};



//////////////////////////////////////////////////////////////////////////

class CFlowNode_DrawSprite : public CFlowBaseNode
{
	enum EInputPorts
	{
		eIN_FadeIn,
		eIN_FadeOut,
		eIN_InTime,
		eIN_DrawTime,
		eIN_OutTime,
		eIN_TextureName,
		eIN_TopLeftPosX,
		eIN_TopLeftPosY,
		eIN_Width,
		eIN_Height,
		eIN_Rotation,
		eIN_Alpha,
		eIN_ZLevel,
		eIn_DrawBeforeHUD,
	};
	enum EOutputPorts
	{
		eOUT_FadedIn,
		eOUT_FadedOut,
	};
	enum EFade
	{
		eFADE_In,
		eFADE_Out,
	};

public:
	CFlowNode_DrawSprite( SActivationInfo * pActInfo ) : 
			m_direction(0), 
				m_fDrawTime(0.0f),
				m_bDrawTimerActive(false),
				m_fTimeLeft(0.0f),
				m_pHUDFader(NULL)
			{
			}

			~CFlowNode_DrawSprite()
			{
			}

			virtual void GetConfiguration(SFlowNodeConfig& config)
			{
				static const SInputPortConfig inputs[] = {
					InputPortConfig_Void("FadeIn", _HELP("Fade the sprite in")),
					InputPortConfig_Void("FadeOut", _HELP("Fade the sprite out")),
					InputPortConfig<float>("FadeInTime", 0.0f, _HELP("Duration of fade in")),
					InputPortConfig<float>("DrawTime", 0.0f, _HELP("Duration to draw, zero or negative number draws until fadeout is triggered")),
					InputPortConfig<float>("FadeOutTime", 0.0f, _HELP("Duration of fade out")),
					InputPortConfig<string>("tex_TextureName", _HELP("Texture Name")),
					InputPortConfig<float>("TopLeftPosX", 0.0f, _HELP("Top left corner of texture in 800x600 virtual coordinates")),
					InputPortConfig<float>("TopLeftPosY", 0.0f, _HELP("Top left corner of texture in 800x600 virtual coordinates")),
					InputPortConfig<float>("Width", 800.0f, _HELP("Width of texture in 800x600 virtual coordinates")),
					InputPortConfig<float>("Height", 600.0f, _HELP("Height of texture in 800x600 virtual coordinates")),
					InputPortConfig<float>("Rotation", 0.0f, _HELP("Clockwise rotation in degrees")),
					InputPortConfig<float>("Alpha", 1.0f, _HELP("Alpha of texture between 0.0 and 1.0")),
					InputPortConfig<int>("ZLevel", -10, _HELP("The sprites will ordered according to this value (it can be negative). The ones with the higher values will be drawn on top. ZLevels 0 to 3 are also used on the ScreenFader Node")),
					InputPortConfig<bool>("DrawBeforeHUD", false, _HELP("Toggle drawing sprite before HUD")),
					{0}
				};
				static const SOutputPortConfig outputs[] = {
					OutputPortConfig_Void("FadedIn", _HELP("FadedIn")),
					OutputPortConfig_Void("FadedOut", _HELP("FadedOut")),
					{0}
				};
				config.nFlags |= EFLN_TARGET_ENTITY;
				config.pInputPorts = inputs;
				config.pOutputPorts = outputs;
				config.sDescription = _HELP("Draws a sprite on the screen. Changes to the values will only take effect when the FadeIn/FadeOut ports are activated.");
				config.SetCategory(EFLN_APPROVED);
			}

			virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
			{
				switch (event)
				{
				case eFE_Initialize:
					{
						pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
						m_pHUDFader = GetFader(pActInfo);
						CRY_ASSERT(m_pHUDFader != NULL);
						m_pHUDFader->Stop();
					}
					break;

				case eFE_Activate:
					{
						if (InputEntityIsLocalPlayer( pActInfo )) 
						{	
							if (IsPortActive(pActInfo, eIN_FadeIn))
							{
								pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
								m_direction = eFADE_In;
								m_bDrawTimerActive = false;
								m_fDrawTime = GetPortFloat(pActInfo, eIN_DrawTime);
								StartFader(pActInfo);
							}
							if (IsPortActive(pActInfo, eIN_FadeOut))
							{
								pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
								m_direction = eFADE_Out;
								m_bDrawTimerActive = false;
								StartFader(pActInfo);
							}
						}
					}
					break;

				case eFE_Update:
					{
						if(m_bDrawTimerActive)
						{
							float fFrameTime = gEnv->pTimer->GetFrameTime();
							m_fTimeLeft -= fFrameTime;
							if(m_fTimeLeft <= 0.0f)
							{
								// Starts fading out when the time is up.
								m_direction = eFADE_Out;
								StartFader(pActInfo);
								m_bDrawTimerActive = false;
							}
						}
						else
						{
							// Activate Output ports
							if(!m_pHUDFader->IsFading())
							{
								if(m_direction == eFADE_In)
								{
									// Start the timer if DrawTime is greater than zero.
									if(m_fDrawTime > 0.0f)
									{
										m_bDrawTimerActive = true;
										m_fTimeLeft = m_fDrawTime;
									}
									ActivateOutput(pActInfo, eOUT_FadedIn, true);
								}
								else
								{
									ActivateOutput(pActInfo, eOUT_FadedOut, true);
									pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);	
								}
							}
						}
					}
					break;

				default:
					CFlowBaseNode::ProcessEvent(event, pActInfo);
					break;
				}
			}

			CHUDFader* GetFader(SActivationInfo * pActInfo)
			{
				return g_pHUDFader->GetHUDFader(GetPortInt(pActInfo, eIN_ZLevel));
			}

			void StartFader(SActivationInfo * pActInfo)
			{
				// Stop the previous fader and start a new one
				m_pHUDFader->Stop();

				m_pHUDFader = GetFader(pActInfo);
				CRY_ASSERT(m_pHUDFader != NULL);

				m_pHUDFader->SetPosition(GetPortFloat(pActInfo, eIN_TopLeftPosX),GetPortFloat(pActInfo, eIN_TopLeftPosY));
				m_pHUDFader->SetDimension(GetPortFloat(pActInfo, eIN_Width),GetPortFloat(pActInfo, eIN_Height));
				m_pHUDFader->SetRotation(GetPortFloat(pActInfo, eIN_Rotation));
				m_pHUDFader->SetDrawBeforeHUD(GetPortBool(pActInfo, eIn_DrawBeforeHUD));
				m_pHUDFader->SetAlpha(GetPortFloat(pActInfo, eIN_Alpha));
				SetTexture(pActInfo);

				const float fDuration = GetPortFloat(pActInfo, m_direction == eFADE_In ? eIN_InTime : eIN_OutTime);
				ColorF color = Col_White;
				color.a = m_direction == eFADE_In ? 0.0f : 1.0f;

				// In HUDFader, FadeOut starts fading a texture into the screen, and FadeIn starts fading a texture out of the screen
				if (m_direction == eFADE_In)
					m_pHUDFader->FadeOut(color, fDuration, GetPortString(pActInfo, eIN_TextureName), true);
				else
					m_pHUDFader->FadeIn(color, fDuration, true);
			}

			void SetTexture( SActivationInfo *pActInfo )
			{
				const string& sTexName = GetPortString(pActInfo, eIN_TextureName);
				if (!sTexName.empty())
				{
					m_pHUDFader->SetTexture(sTexName);
				}
			}

			//virtual void GetMemoryUsage(ICrySizer * s) const
			//{
			//	s->Add(*this);
			//}

	virtual void GetMemoryStatistics(ICrySizer * s) 
	{ 
		s->Add(*this); 
	}


protected:

	int			m_direction;
	float		m_fDrawTime;
	bool		m_bDrawTimerActive;
	float		m_fTimeLeft;
	CHUDFader*	m_pHUDFader;

};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_HUDNavPath : public CFlowBaseNode 
{
	enum INPUTS
	{
		EIP_Set = 0,
		EIP_Clear,
		EIP_Pos,
		EIP_Entity,
		EIP_PlaceMarker,
	};

public:
	CFlowNode_HUDNavPath( SActivationInfo * pActInfo ) { }

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void("Set", _HELP("Set player nav path")),
			InputPortConfig_Void("Clear", _HELP("clear player nav path")),
			InputPortConfig<Vec3>("Pos", _HELP("Position to set target to")),
			InputPortConfig<EntityId>("Entity", _HELP("entity id to set target to")),
			InputPortConfig<bool>("PlaceMarker", _HELP("If true, world marker will also be set")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = 0;
		config.sDescription = _HELP("Manipulate HUD nav path");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(InputEntityIsLocalPlayer( pActInfo ) && eFE_Activate == event)
		{
			CPlayer *pClient = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			CRY_ASSERT(pClient);
			if(!pClient)
				return;

			CPlayerNavPath* pPath = pClient->GetNavPath();
			CRY_ASSERT(pPath);
			if(!pPath)
				return;

			CGameRules *pRules = g_pGame->GetGameRules();
			CRY_ASSERT(pRules);
			if (!pRules)
				return;

			if (IsPortActive(pActInfo, EIP_Clear))
			{
				pPath->Clear();
			}
			else if (IsPortActive(pActInfo, EIP_Set))
			{
				const EntityId targetId = GetPortEntityId(pActInfo, EIP_Entity);

				if (targetId > 0)
				{
					pPath->SetTarget(targetId);
				}
				else
				{
					const Vec3 vPos = GetPortVec3(pActInfo, EIP_Pos);
					pPath->SetTarget(vPos);
				}
			}
		}
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}


};


//////////////////////////////////////////////////////////////////////////
class CFlowNode_PlayerNavPath : public CFlowBaseNode 
{
	enum INPUTS
	{
		EIP_Activate = 0,
		EIP_Deactivate,
		EIP_Entity,
	};

public:
	CFlowNode_PlayerNavPath( SActivationInfo * pActInfo ) { }

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_ports[] = 
		{
			InputPortConfig_Void("Activate", _HELP("Activates the Player Nav Path feature.")),
			InputPortConfig_Void("Deactivate", _HELP("Deactivates the Player Nav Path feature.")),
			InputPortConfig<EntityId>("Start", _HELP("Tagpoint entity at which the Tagpoint net starts.")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_ports;
		config.pOutputPorts = 0;
		config.sDescription = _HELP("Manipulate player nav path");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if(InputEntityIsLocalPlayer( pActInfo ) && eFE_Activate == event)
		{
			CPlayer *pClient = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			CRY_ASSERT(pClient);
			if(!pClient)
				return;

			CPlayerNavPath* pPath = pClient->GetNavPath();
			CRY_ASSERT(pPath);
			if(!pPath)
				return;

			CGameRules *pRules = g_pGame->GetGameRules();
			CRY_ASSERT(pRules);
			if (!pRules)
				return;

			if (IsPortActive(pActInfo, EIP_Deactivate))
			{
				pPath->Clear();
			}
			else if (IsPortActive(pActInfo, EIP_Activate))
			{
				const EntityId startId = GetPortEntityId(pActInfo, EIP_Entity);

				if (startId > 0)
				{
					pPath->SetTagPointStart(startId);
				}
			}
		}
	}


//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

};


REGISTER_FLOW_NODE("CrysisFX:ScreenFader",	CFlowNode_ScreenFader);
REGISTER_FLOW_NODE("HUD:DrawSprite",				CFlowNode_DrawSprite);
REGISTER_FLOW_NODE_SINGLETON("HUD:TagPointNavPath",	CFlowNode_PlayerNavPath);
REGISTER_FLOW_NODE_SINGLETON("HUD:NavPath",	CFlowNode_HUDNavPath);


