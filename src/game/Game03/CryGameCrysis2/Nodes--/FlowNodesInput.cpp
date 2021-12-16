/************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

-------------------------------------------------------------------------
History:
- 10:3:2009  : Created by Jan Müller, integrated from G04

*************************************************************************/


#include "StdAfx.h"
#include "Nodes/G2FlowBaseNode.h"

// 360 keys enum
enum EXBoxKey
{
	// Invalid - Not an XBox key
	eXBK_Invalid = -1,

	// Keep in same order as EKeyID (KI_XINPUT_BASE+value)
	eXBK_DPadUp = 0,
	eXBK_DPadDown,
	eXBK_DPadLeft,
	eXBK_DPadRight,
	eXBK_Start,
	eXBK_Back,
	eXBK_ThumbL,
	eXBK_ThumbR,
	eXBK_ShoulderL,
	eXBK_ShoulderR,
	eXBK_A,
	eXBK_B,
	eXBK_X,
	eXBK_Y,
	eXBK_TriggerL,
	eXBK_TriggerR,
	eXBK_ThumbLX,
	eXBK_ThumbLY,
	eXBK_ThumbLUp,
	eXBK_ThumbLDown,
	eXBK_ThumbLLeft,
	eXBK_ThumbLRight,
	eXBK_ThumbRX,
	eXBK_ThumbRY,
	eXBK_ThumbRUp,
	eXBK_ThumbRDown,
	eXBK_ThumbRLeft,
	eXBK_ThumbRRight,
	eXBK_TriggerLBtn,
	eXBK_TriggerRBtn,
};

#if defined(PS3)
static int PS3KeyTable[]=
{
	eKI_PS3_Up,			// 0:DPadUp
	eKI_PS3_Down,		// 1:DPadDown
	eKI_PS3_Left,		// 2:DPadLeft
	eKI_PS3_Right,		// 3:DPadRight
	eKI_PS3_Start,		// 4:Start
	eKI_PS3_Select,		// 5:Back
	eKI_PS3_StickLX,	// 6:ThumbL
	eKI_PS3_StickRY,	// 7:ThumbR
	eKI_PS3_L1,			// 8:ShoulderL
	eKI_PS3_R1,			// 9:ShoulderR
	eKI_PS3_Cross,		// 10:A
	eKI_PS3_Circle,		// 11:B
	eKI_PS3_Square,		// 12:X
	eKI_PS3_Triangle,	// 13:Y
	eKI_PS3_L2,			// 14
	eKI_PS3_R2,			// 15
	-1,					// 16
	-1,					// 17
	-1,					// 18
	-1,					// 19
	-1,					// 20
	-1,					// 21
	-1,					// 22
	-1,					// 23
	-1,					// 26
	-1,					// 27
	eKI_PS3_L3,			// 28:TriggerL
	eKI_PS3_R3,			// 29:TriggerR
};

#endif

#define XBoxKeyEnum "enum_int:DPadUp=0,DPadDown=1,DPadLeft=2,DPadRight=3,Start=4,Back=5,ThumbL=6,ThumbR=7,ShoulderL=8,ShoulderR=9,A=10,B=11,X=12,Y=13,TriggerL=28,TriggerR=29"
#define XBoxAnalogEnum "enum_int:ThumbL=6,ThumbR=7,TriggerL=14,TriggerR=15"

//////////////////////////////////////////////////////////////////////////
class CG4FlowNode_XBoxKey : public CFlowBaseNode, public IInputEventListener
{
public:
	CG4FlowNode_XBoxKey(SActivationInfo * pActInfo)
	{
	}

	~CG4FlowNode_XBoxKey()
	{
		Register(false);
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CG4FlowNode_XBoxKey(pActInfo);
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		ser.Value("m_bActive", m_bActive);
		if (ser.IsReading())
		{
			m_actInfo = *pActInfo;
			Register(m_bActive);
		}
	}

	enum EInputPorts
	{
		EIP_Enable = 0,
		EIP_Disable,
		EIP_Key,
		EIP_NonDevMode,
	};

	enum EOutputPorts
	{
		EOP_Pressed = 0,
		EOP_Released,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Enable", _HELP("Enable reporting")),
			InputPortConfig_Void("Disable", _HELP("Disable reporting")),
			InputPortConfig<int>("Key", 0, _HELP("XBox 360 controller key"), NULL, _UICONFIG(XBoxKeyEnum)),
			InputPortConfig<bool>("NonDevMode", false, _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("Pressed", _HELP("Called when key is pressed")),
			OutputPortConfig_Void("Released", _HELP("Called when key is released")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Get input from XBox 360 controller. EntityInput is used in multiplayer");
		config.SetCategory(EFLN_DEBUG);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (InputEntityIsLocalPlayer( pActInfo )) 
		{     
			switch (event)
			{
				case eFE_Initialize:
				{
					m_actInfo = *pActInfo;
					Register(true);
				}
				break;

				case eFE_Activate:
				{
					m_actInfo = *pActInfo;
					if (IsPortActive(pActInfo, EIP_Enable))
					{
						Register(true);
					}
					if (IsPortActive(pActInfo, EIP_Disable))
					{
						Register(false);
					}
				}
				break;
			}
		}
	}

	void Register(bool bRegister)
	{
		if (IInput *pInput = gEnv->pInput)
		{
			if (true == bRegister)
			{
				const bool bAllowedInNonDevMode = GetPortBool(&m_actInfo, EIP_NonDevMode);
				if (gEnv->pSystem->IsDevMode() || bAllowedInNonDevMode)
					pInput->AddEventListener(this);
			}
			else
				pInput->RemoveEventListener(this);

			m_bActive = bRegister;
		}
		else
			m_bActive = false;
	}

	int TranslateKey(int nKeyId)
	{
		if (nKeyId >= eKI_XI_DPadUp && nKeyId <= eKI_XI_Disconnect)
			return nKeyId-KI_XINPUT_BASE;
		return eXBK_Invalid;
	}

	// ~IInputEventListener
	virtual bool OnInputEvent( const SInputEvent &event )
	{
		if (true == m_bActive)
		{
#if defined(PS3)
			int nThisKey = event.keyId;
			int nKey = -1;
			int nInput = GetPortInt(&m_actInfo, EIP_Key);
			int tableSize = sizeof(PS3KeyTable)/sizeof(PS3KeyTable[0]);
			if ( nInput>=0 && nInput<tableSize )
				nKey = PS3KeyTable[nInput];
#else
			// Translate key, check value
			const int nThisKey = TranslateKey(event.keyId);
			const int nKey = GetPortInt(&m_actInfo, EIP_Key);
#endif
			if (nKey == nThisKey)
			{
				// Return based on state
				if (eIS_Pressed == event.state)
					ActivateOutput(&m_actInfo, EOP_Pressed, true);
				else if (eIS_Released == event.state)
					ActivateOutput(&m_actInfo, EOP_Released, true);
			}
		}

		// Let other listeners handle it
		return false;
	}

private:
	bool m_bActive; // TRUE when node is enabled
	SActivationInfo m_actInfo; // Activation info instance
};

//////////////////////////////////////////////////////////////////////////
class CG4FlowNode_XBoxAnalog : public CFlowBaseNode, public IInputEventListener
{
public:
	CG4FlowNode_XBoxAnalog(SActivationInfo * pActInfo)
	{
	}

	~CG4FlowNode_XBoxAnalog()
	{
		Register(false);
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CG4FlowNode_XBoxAnalog(pActInfo);
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		ser.Value("m_bActive", m_bActive);
		if (ser.IsReading())
		{
			m_actInfo = *pActInfo;
			Register(m_bActive);
		}
	}

	enum EInputPorts
	{
		EIP_Enable = 0,
		EIP_Disable,
		EIP_Key,
		EIP_NonDevMode,
	};

	enum EOutputPorts
	{
		EOP_ChangedX = 0,
		EOP_ChangedY,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Enable", _HELP("Enable reporting")),
			InputPortConfig_Void("Disable", _HELP("Disable reporting")),
			InputPortConfig<int>("Key", 0, _HELP("XBox 360 controller key"), NULL, _UICONFIG(XBoxAnalogEnum)),
			InputPortConfig<bool>("NonDevMode", false, _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<float>("ChangedX", _HELP("Called when analog changes in X (trigger info sent out here as well)")),
			OutputPortConfig<float>("ChangedY", _HELP("Called when analog changes in Y")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Get analog input from XBox 360 controller. Note: Expensive!  Note2: entity input is used in multiplayer");
		config.SetCategory(EFLN_DEBUG);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (InputEntityIsLocalPlayer( pActInfo )) 
		{     
			switch (event)
			{
			case eFE_Initialize:
				{
					m_actInfo = *pActInfo;
					Register(true);
				}
				break;

			case eFE_Activate:
				{
					m_actInfo = *pActInfo;
					if (IsPortActive(pActInfo, EIP_Enable))
					{
						Register(true);
					}
					if (IsPortActive(pActInfo, EIP_Disable))
					{
						Register(false);
					}
				}
				break;
			}
		}
	}

	void Register(bool bRegister)
	{
		if (IInput *pInput = gEnv->pInput)
		{
			if (true == bRegister)
			{
				const bool bAllowedInNonDevMode = GetPortBool(&m_actInfo, EIP_NonDevMode);
				if (gEnv->pSystem->IsDevMode() || bAllowedInNonDevMode)
					pInput->AddEventListener(this);
			}
			else
				pInput->RemoveEventListener(this);

			m_bActive = bRegister;
		}
		else
			m_bActive = false;
	}

	int TranslateKey(int nKeyId)
	{
		switch (nKeyId)
		{
		case eKI_XI_ThumbLX:
		case eKI_XI_ThumbLY:
			return eXBK_ThumbL;
			break;

		case eKI_XI_ThumbRX:
		case eKI_XI_ThumbRY:
			return eXBK_ThumbR;
			break;

		case eKI_XI_TriggerL:
			return eXBK_TriggerL;
			break;

		case eKI_XI_TriggerR:
			return eXBK_TriggerR;
			break;
		}
		return eXBK_Invalid;
	}

	// ~IInputEventListener
	virtual bool OnInputEvent( const SInputEvent &event )
	{
		if (true == m_bActive)
		{

#if defined(PS3)
			int nThisKey = event.keyId;
			int nKey = -1;
			int nInput = GetPortInt(&m_actInfo, EIP_Key);
			int tableSize = sizeof(PS3KeyTable)/sizeof(PS3KeyTable[0]);
			if ( nInput>=0 && nInput<tableSize )
				nKey = PS3KeyTable[nInput];
			if ( ( eIS_Changed == event.state || eIS_Pressed == event.state ) && nKey == nThisKey)
			{
				// Triggers go out in X as well
				if (
					eKI_PS3_StickLX == nKey ||
					eKI_PS3_StickRX == nKey ||
					eKI_PS3_L2 == nKey		||
					eKI_PS3_R2 == nKey
					)
					ActivateOutput(&m_actInfo, EOP_ChangedX, event.value);
				else
					ActivateOutput(&m_actInfo, EOP_ChangedY, event.value);
			}
#else
			// Translate key, check value
			const int nThisKey = TranslateKey(event.keyId);
			const int nKey = GetPortInt(&m_actInfo, EIP_Key);
			if (eIS_Changed == event.state && nKey == nThisKey)
			{
				// Triggers go out in X as well
				if (
					eKI_XI_TriggerL == event.keyId ||
					eKI_XI_TriggerR == event.keyId ||
					eKI_XI_ThumbLX == event.keyId ||
					eKI_XI_ThumbRX == event.keyId
					)
					ActivateOutput(&m_actInfo, EOP_ChangedX, event.value);
				else
					ActivateOutput(&m_actInfo, EOP_ChangedY, event.value);
			}
#endif
		}

		// Let other listeners handle it
		return false;
	}

private:
	bool m_bActive; // TRUE when node is enabled
	SActivationInfo m_actInfo; // Activation info instance
};


//////////////////////////////////////////////////////////////////////////
// Register nodes

REGISTER_FLOW_NODE("Input:XBoxKey", CG4FlowNode_XBoxKey);
REGISTER_FLOW_NODE("Input:XBoxAnalog", CG4FlowNode_XBoxAnalog);
