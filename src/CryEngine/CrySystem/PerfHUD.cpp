////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   PerfHUD.cpp
//  Created:     26/08/2009 by Timur.
//  Description: Button implementation in the MiniGUI
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include <StdAfx.h>
#include "PerfHUD.h"
#include "MiniGUI/MiniInfoBox.h"
#include "MiniGUI/MiniMenu.h"
#include "MiniGUI/MiniTable.h"
#include <IConsole.h>
#include <I3DEngine.h>
#include <ITimer.h>
#include <IParticles.h>

#include <CryExtension/CryCreateClassInstance.h>

using namespace minigui;





const float CPerfHUD::OVERSCAN_X = 15;
const float CPerfHUD::OVERSCAN_Y = 15;

	
const ColorB CPerfHUD::COL_NORM = ColorB(255,255,255,255);
const ColorB CPerfHUD::COL_WARN = ColorB(255,255,0,255);
const ColorB CPerfHUD::COL_ERROR = ColorB(255,0,0,255);
	
const float CPerfHUD::TEXT_SIZE_NORM = 18.f;
const float CPerfHUD::TEXT_SIZE_WARN = 22.f;
const float CPerfHUD::TEXT_SIZE_ERROR = 26.f;

const float CPerfHUD::ACTIVATE_TIME = 1.f;

CRYREGISTER_CLASS(CPerfHUD)
CPerfHUD::CPerfHUD() :
m_menuStartX(OVERSCAN_X),
m_menuStartY(OVERSCAN_Y),
m_hudCreated(false),
m_L1Pressed(false),
m_L2Pressed(false),
m_R1Pressed(false),
m_R2Pressed(false),
m_changingState(false),
m_hwMouseEnabled(false),
m_triggersDownStartTime(-1.f),
m_hudState(eHudOff),
m_hudLastState(eHudOff)
{
	m_widgets.reserve(ICryPerfHUDWidget::eWidget_Num);
}

CPerfHUD::~CPerfHUD() {}

int CPerfHUD::m_sys_perfhud = 0;

void CPerfHUD::CVarChangeCallback(ICVar *pCvar)
{
	ICryPerfHUD *pPerfHud = gEnv->pSystem->GetPerfHUD();

	if(pPerfHud)
	{
		int val = pCvar->GetIVal();
		//Check for invalid value
		if(val>=0 && val<eHudNumStates)
		{
			pPerfHud->SetState((EHudState)val);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
void CPerfHUD::Init()
{
	m_sys_perfhud_prev = 0;

	if(gEnv->pConsole)
	{
		gEnv->pConsole->Register("sys_perfhud", &m_sys_perfhud, 0, VF_CHEAT, CVARHELP("PerfHUD 0:off, 1:In focus, 2:Out of focus"), CVarChangeCallback );
	}

	if(gEnv->pInput)
	{
		gEnv->pInput->AddEventListener(this);
	}
	
	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		InitUI(pGUI.get());
	}
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::LoadBudgets()
{
	XmlNodeRef budgets = gEnv->pSystem->LoadXmlFile( "PerfHud.xml" );

	if(budgets)
	{
		const int nWidgets = m_widgets.size();

		for(int i=0; i<nWidgets; i++)
		{
			m_widgets[i]->LoadBudgets(budgets);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::SaveStats(const char* filename)
{
	XmlNodeRef rootNode = GetISystem()->CreateXmlNode("PerfHudStats");

	if(rootNode)
	{
		const int nWidgets = m_widgets.size();

		for(int i=0; i<nWidgets; i++)
		{
			m_widgets[i]->SaveStats(rootNode);
		}

		if (!filename)
			filename = "PerfHudStats.xml";
		rootNode->saveToFile(filename, 15*1024);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::ResetWidgets()
{
	std::vector< _smart_ptr<ICryPerfHUDWidget> >::iterator itWidget;
	for (itWidget = m_widgets.begin(); itWidget != m_widgets.end(); itWidget++)
	{
		(*itWidget)->Reset();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::Done()
{
	if(gEnv->pInput)
	{
		gEnv->pInput->RemoveEventListener(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::Draw()
{
  FUNCTION_PROFILER_FAST( GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled );

	if (m_hudState != m_hudLastState)
	{
		Show();
		m_hudLastState = m_hudState;
	}

	if(m_hudState!=eHudOff)
	{
		const int nWidgets = m_widgets.size();

		for(int i=0; i<nWidgets; i++)
		{
			if(m_widgets[i]->ShouldUpdate())
			{
				m_widgets[i]->Update();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::Show()
{
	cryshared_ptr<IMiniGUI> pGUI;
	if (!CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
		return;

	switch (m_hudState)
	{
	case eHudInFocus:
		{
			pGUI->SetEventListener( this );

			if(!m_hudCreated)
			{
				InitUI(pGUI.get());
			}
			else
			{
				pGUI->RestoreState();
			}
			pGUI->SetEnabled(true);
			pGUI->SetInFocus(true);
			
			int nRootMenus = m_rootMenus.size();
			for(int i=0; i<nRootMenus; i++)
			{
				m_rootMenus[i]->SetVisible(1);
			}
		}
		break;

	case eHudOutOfFocus:
		{
			int nRootMenus = m_rootMenus.size();
			for(int i=0; i<nRootMenus; i++)
			{
				m_rootMenus[i]->SetVisible(0);
			}
		
			pGUI->SetEventListener( 0 );
			pGUI->SetEnabled(true);
			pGUI->SetInFocus(false);
		}
		break;

	case eHudOff:
		{
			pGUI->SaveState();
			pGUI->Reset();
			pGUI->SetEnabled(false);
		}
		break;

	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::InitUI( IMiniGUI *pGUI )
{
	assert(m_hudCreated==false);

	IMiniCtrl* pMenu;
	XmlNodeRef perfXML = gEnv->pSystem->LoadXmlFile( "PerfHud.xml" );

	//
	// RENDERING MENU
	//
	pMenu = CreateMenu("Rendering");
	
	IMiniCtrl* pDebugMenu = CreateMenu("Debug", pMenu);
	CreateCVarMenuItem(pDebugMenu, "Wireframe", "r_wireframe", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Overdraw", "r_MeasureOverdrawScale", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Freeze Camera", "e_CameraFreeze", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Post Effects", "r_PostProcessEffects", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "HDR", "r_HDRRendering", 0, 2 );
	CreateCVarMenuItem(pDebugMenu, "Shadows", "e_Shadows", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Ocean", "e_WaterOcean", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Particles", "e_Particles", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Terrain", "e_Terrain", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Brushes", "e_Brushes", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Vegetation", "e_Vegetation", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "Coverage Buffer", "e_CoverageBuffer", 0, 1 );
	CreateCVarMenuItem(pDebugMenu, "MFX visual debug", "mfx_DebugVisual", 0, 1 );

	IMiniCtrl* pStatsMenu = CreateMenu("Stats", pMenu);
	//Render Info
	CRenderStatsWidget *pRenderStats = new CRenderStatsWidget(pStatsMenu, this);
	
	if(pRenderStats)
	{
		pRenderStats->LoadBudgets(perfXML);
		m_widgets.push_back(pRenderStats);
	}

	CRenderBatchWidget *pRenderBatchStats = new CRenderBatchWidget(pStatsMenu, this);

	if(pRenderBatchStats)
	{
		m_widgets.push_back(pRenderBatchStats);
	}




	CreateCVarMenuItem(pStatsMenu, "Debug Gun", "e_debugDraw", 0, 16 );
	CreateCVarMenuItem(pStatsMenu, "Poly / Lod info", "e_debugDraw", 0, 1 );
	CreateCVarMenuItem(pStatsMenu, "Texture Memory Usage", "e_debugDraw", 0, 4 );
	CreateCVarMenuItem(pStatsMenu, "Detailed Render Stats", "r_Stats", 0, 1 );
	CreateCVarMenuItem(pStatsMenu, "Shader Stats", "r_ProfileShaders", 0, 1 );

	//
	// SYSTEM MENU
	//
	pMenu = CreateMenu("System");
	
	//FPS Buckets
	CWarningsWidget *pWarningsWidget = new CWarningsWidget(pMenu, this);

	if(pWarningsWidget)
	{
		m_widgets.push_back(pWarningsWidget);
	}

	CreateCVarMenuItem(pMenu, "Profiler", "profile", 0, 1 );
	CreateCVarMenuItem(pMenu, "Thread Summary", "r_showmt", 0, 1 );
	CreateCVarMenuItem(pMenu, "Track File Access", "pak_logInvalidFileAccess", 0, 1 );
	
	//FPS Buckets
	CFpsWidget *pFpsBuckets = new CFpsWidget(pMenu, this);

	if(pFpsBuckets)
	{
		pFpsBuckets->LoadBudgets(perfXML);
		m_widgets.push_back(pFpsBuckets);
	}
	
	//
	// SETUP MENU
	//
	pMenu = CreateMenu("Setup");
	CreateCallbackMenuItem(pMenu, "Reset HUD", CPerfHUD::ResetCallback, NULL);
	CreateCallbackMenuItem(pMenu, "Reload Budgets", CPerfHUD::ReloadBudgetsCallback, NULL);
	CreateCallbackMenuItem(pMenu, "Save Stats", CPerfHUD::SaveStatsCallback, NULL);

	//
	// TEST MENU
	//
	/*	
	pMenu = CreateMenu("Test");

	//Test Table
	{
		CMiniTable* pTable = (CMiniTable*)CreateTableMenuItem(pMenu, "TEST TABLE");
	
		int columnIndex = pTable->AddColumn("Name");
		pTable->AddData(columnIndex, "Character");
		pTable->AddData(columnIndex, "Terrain");
		pTable->AddData(columnIndex, "Gun");
		
		columnIndex = pTable->AddColumn("Poly Count");
		pTable->AddData(columnIndex, "50");
		pTable->AddData(columnIndex, "1000000");
		pTable->AddData(columnIndex, "123456");
		
		columnIndex = pTable->AddColumn("Texture Usage");
		pTable->AddData(columnIndex, "5k");
		pTable->AddData(columnIndex, "20mb");
		pTable->AddData(columnIndex, "10mb");
		
		columnIndex = pTable->AddColumn("Num Lods");
		pTable->AddData(columnIndex, "2");
		pTable->AddData(columnIndex, "7");
		pTable->AddData(columnIndex, "4");
	}
	*/

	//save default windows 
	pGUI->SaveState();
	
	m_hudCreated = true;
}
//////////////////////////////////////////////////////////////////////////
// GUI CREATION HELPER FUNCS
//////////////////////////////////////////////////////////////////////////

IMiniCtrl* CPerfHUD::CreateMenu( const char* name, IMiniCtrl* pParent )
{
	assert(name);

	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		bool subMenu = false;
		
		if(pParent)
		{
			assert(pParent->GetType() == eCtrlType_Menu);
			subMenu=true;
		}

		const float ButtonWidth = 10.f; //arbitrary, button will be scaled based on contained text

		int ctrlFlags = 0;
		
		if(!subMenu)
		{
			ctrlFlags = eCtrl_TextAlignCentre | eCtrl_AutoResize;
		}
	
		Rect rcMenuBtn = Rect(m_menuStartX,m_menuStartY,m_menuStartX+ButtonWidth,m_menuStartY+20);

		IMiniCtrl *pMenu = pGUI->CreateCtrl( pParent,1,eCtrlType_Menu,ctrlFlags,rcMenuBtn,name );
	
		if(pMenu && !subMenu)
		{
			const float MenuBtnSepration = 10.f;

			//Update Menu Positions
			rcMenuBtn = pMenu->GetRect();
			m_menuStartX = rcMenuBtn.right + MenuBtnSepration; 
			m_menuStartY = rcMenuBtn.top; 

			m_rootMenus.push_back(pMenu);
		}

		return pMenu;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CPerfHUD::CreateCVarMenuItem( IMiniCtrl *pMenu, const char* name, const char* controlVar, float controlVarOn, float controlVarOff )
{
	assert(pMenu&&name&&controlVar);
	assert(pMenu->GetType()==eCtrlType_Menu);
	
	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		IMiniCtrl* pCtrl = pGUI->CreateCtrl( pMenu,100,eCtrlType_Button,eCtrl_CheckButton,Rect(0,0,100,20), name);

		if(pCtrl)
		{
			if(controlVar)
			{
				pCtrl->SetControlCVar( controlVar, controlVarOn, controlVarOff );
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPerfHUD::CreateCallbackMenuItem( IMiniCtrl *pMenu, const char* name, ClickCallback clickCallback, void *pCallbackData )
{
	assert(pMenu&&name&&clickCallback);
	assert(pMenu->GetType()==eCtrlType_Menu);
	
	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		IMiniCtrl* pCtrl = pGUI->CreateCtrl( pMenu,100,eCtrlType_Button,0/*eCtrl_CheckButton*/,Rect(0,0,100,20), name);

		if(pCtrl)
		{
			pCtrl->SetClickCallback( clickCallback, pCallbackData );
			return true;
		}
	}
	return false;

}

//////////////////////////////////////////////////////////////////////////
IMiniCtrl* CPerfHUD::CreateInfoMenuItem( IMiniCtrl *pMenu, const char* name, RenderCallback renderCallback, const Rect& rect, bool onAtStart)
{
	assert(pMenu->GetType()==eCtrlType_Menu);
	
	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		IMiniCtrl* pCtrl = pGUI->CreateCtrl( pMenu,100,eCtrlType_Button,eCtrl_CheckButton,Rect(0,0,100,20), name);

		int infoFlags = eCtrl_Moveable|eCtrl_CloseButton;

		if(!renderCallback)
		{
			infoFlags |= eCtrl_AutoResize;
		}

		IMiniCtrl* pInfo	= pGUI->CreateCtrl( NULL, 200, eCtrlType_InfoBox, infoFlags, rect, name );

		if(pCtrl)
		{
			pCtrl->SetConnectedCtrl( pInfo );

			if(onAtStart)
			{
				pCtrl->SetFlag(eCtrl_Checked);
			}
			else
			{
				pInfo->SetVisible(false);
			}

			if(renderCallback)
			{
				pInfo->SetRenderCallback(renderCallback);
			}

			return pInfo;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
IMiniCtrl* CPerfHUD::CreateTableMenuItem( IMiniCtrl *pMenu, const char* name )
{
	assert(pMenu&&name);
	assert(pMenu->GetType()==eCtrlType_Menu);

	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		IMiniCtrl *pTable = pGUI->CreateCtrl( NULL, 200, eCtrlType_Table, eCtrl_AutoResize|eCtrl_Moveable|eCtrl_CloseButton, Rect(400,300,750,550), name );

		if(pTable)
		{
			IMiniCtrl *pCtrl = pGUI->CreateCtrl( pMenu,100,eCtrlType_Button,eCtrl_CheckButton,Rect(0,0,100,20), name);

			if(pCtrl)
			{
				pCtrl->SetConnectedCtrl( pTable );

				pTable->SetVisible(false);

				return pTable;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CPerfHUD::OnCommand( SCommand &cmd )
{
	//CryLog( "Button Clicked = %d",cmd.nCtrlID );
}

//////////////////////////////////////////////////////////////////////////
bool CPerfHUD::OnInputEvent(const SInputEvent &rInputEvent)
{







	const EKeyId L1 = eKI_XI_ShoulderL;
	const EKeyId L2 = eKI_XI_TriggerLBtn;
	const EKeyId R1 = eKI_XI_ShoulderR;
	const EKeyId R2 = eKI_XI_TriggerRBtn;
	const EKeyId X = eKI_XI_A;


	if(	rInputEvent.deviceId==eDI_XI && 
			rInputEvent.deviceIndex==0 )
	{
		int frameNum = gEnv->pRenderer->GetFrameID(false);

		if( rInputEvent.state==eIS_Pressed ) 
		{
			bool checkState=false;

			switch (rInputEvent.keyId)
			{
			case L1:
				m_L1Pressed = true;
				checkState = true;
				break;

			/*case L2:
				m_L2Pressed = true;
				checkState = true;
				break;*/

			case R1:
				m_R1Pressed = true;
				checkState = true;
				break;

			/*case R2:
				m_R2Pressed = true;
				checkState = true;
				break;*/

			case X:
				if(m_changingState)
				{
					SetNextState();
				}
				break;
			}

			//if(checkState&&m_L1Pressed&&m_L2Pressed&&m_R1Pressed&&m_R2Pressed)
			if(checkState&&m_L1Pressed&&m_R1Pressed)
			{
				m_triggersDownStartTime = gEnv->pTimer->GetAsyncCurTime();
			}
		}
		else if( rInputEvent.state==eIS_Down )
		{
			if( m_triggersDownStartTime > 0.f && gEnv->pTimer->GetAsyncCurTime() - m_triggersDownStartTime > ACTIVATE_TIME )
			{
				m_changingState = true;

				const char *hudStateStr = NULL;

				switch(m_hudState)
				{
				case eHudInFocus:
					hudStateStr = "CryPerfHUD Edit Mode";
					break;

				case eHudOutOfFocus:
					hudStateStr = "CryPerfHUD Game Mode";
					break;

				case eHudOff:
					hudStateStr = "CryPerfHUD Off";
					break;

				default:
					hudStateStr = "CryPerfHud unknown";
					break;
				}
				
				float col[4] = {1.f,1.f,1.f,1.f};
				gEnv->pRenderer->Draw2dLabel(450.f,200.f,2.f,col,false,hudStateStr);



				gEnv->pRenderer->Draw2dLabel(450.f,220.f,2.f,col,false,"Press A to change Mode");

			}
		}
		else if( rInputEvent.state==eIS_Released )
		{
			bool triggerReleased=false;

			switch (rInputEvent.keyId)
			{
			case L1:
				m_L1Pressed = false;
				triggerReleased=true;
				break;

			case L2:
				m_L2Pressed = false;
				triggerReleased=true;
				break;

			case R1:
				m_R1Pressed = false;
				triggerReleased=true;
				break;

			case R2:
				m_R2Pressed = false;
				triggerReleased=true;
				break;
			}
			
			if(triggerReleased)
			{
				m_triggersDownStartTime = 0.f;

				if(m_changingState)
				{
					m_changingState=false;

					//workaround: hardware mouse resets all input states when enabled
					//this breaks perfhud selection mode (as triggers are released)
					//don't enable mouse until we've finished selection mode
					if(m_hudState==eHudInFocus)
					{
						if(!m_hwMouseEnabled)
						{
							gEnv->pHardwareMouse->IncrementCounter();
							m_hwMouseEnabled = true;
						}
					}
					else if(m_hwMouseEnabled)
					{	
						gEnv->pHardwareMouse->DecrementCounter();
						m_hwMouseEnabled = false;
					}
				}
			}
		}

		if(m_hudState==eHudInFocus)
		{
			//PerfHUD takes control of the input
			return true;
		}
	}
	return false;
}

void CPerfHUD::SetNextState()
{
	m_hudState = (EHudState)((m_hudState+1) % eHudNumStates);
	//CryLogAlways("Setting new HUD state: %d", m_hudState);
	
	//what to do about cvar?
	//ICVar *pPerfCVar = GetISystem()->GetIConsole()->GetCVar("sys_perfhud");
	//pPerfCVar->Set( 1 );
}

void CPerfHUD::Reset()
{
	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		pGUI->Reset();
	}
}

//////////////////////////////////////////////////////////////////////////
// CLICK CALLBACKS
//////////////////////////////////////////////////////////////////////////

void CPerfHUD::ResetCallback(void* data, bool status)
{
	cryshared_ptr<ICryPerfHUD> pPerfHUD;
	if (CryCreateClassInstanceForInterface( cryiidof<ICryPerfHUD>(),pPerfHUD ))
	{
		pPerfHUD->Reset();
	}
}

void CPerfHUD::ReloadBudgetsCallback(void* data, bool status)
{
	cryshared_ptr<ICryPerfHUD> pPerfHUD;
	if (CryCreateClassInstanceForInterface( cryiidof<ICryPerfHUD>(),pPerfHUD ))
	{
		pPerfHUD->LoadBudgets();
	}
}

void CPerfHUD::SaveStatsCallback(void* data, bool status)
{
	cryshared_ptr<ICryPerfHUD> pPerfHUD;
	if (CryCreateClassInstanceForInterface( cryiidof<ICryPerfHUD>(),pPerfHUD ))
	{
		pPerfHUD->SaveStats();
	}
}

//////////////////////////////////////////////////////////////////////////
// RENDER CALLBACKS
//////////////////////////////////////////////////////////////////////////

/*
void CPerfHUD::DisplayRenderInfoCallback(const Rect& rect)
{
	cryshared_ptr<IMiniGUI> pGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<IMiniGUI>(),pGUI ))
	{
		//right aligned display
		float x = rect.right;
		float y = rect.top;
		float step = 13.f;

    gEnv->p3DEngine->DisplayInfo(x, y, step, true);
	}
}*/

void CPerfHUD::EnableWidget(ICryPerfHUDWidget::EWidgetID id)
{
	const int nWidgets = m_widgets.size();

	for(int i=0; i<nWidgets; i++)
	{
		if(m_widgets[i]->m_id == id)
		{
			m_widgets[i]->Enable();
			return;
		}
	}
}

void CPerfHUD::DisableWidget(ICryPerfHUDWidget::EWidgetID id)
{
	const int nWidgets = m_widgets.size();

	for(int i=0; i<nWidgets; i++)
	{
		if(m_widgets[i]->m_id == id)
		{
			m_widgets[i]->Disable();
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Widget Specific Interface
//////////////////////////////////////////////////////////////////////////
void CPerfHUD::AddWarning(float duration, const char* fmt, va_list argList)
{
	//could cache warnings window ptr for efficiency
	const int nWidgets = m_widgets.size();

	for(int i=0; i<nWidgets; i++)
	{
		if(m_widgets[i]->m_id == ICryPerfHUDWidget::eWidget_Warnings)
		{
			CWarningsWidget* warnings = (CWarningsWidget*)m_widgets[i].get();
			
			if(warnings->ShouldUpdate())
			{
				warnings->AddWarningV(duration, fmt, argList);
				break;
			}
		}
	}
}

bool CPerfHUD::WarningsWindowEnabled() const
{
	const int nWidgets = m_widgets.size();

	for(int i=0; i<nWidgets; i++)
	{
		if(m_widgets[i]->m_id == ICryPerfHUDWidget::eWidget_Warnings)
		{
			return m_widgets[i]->ShouldUpdate();
		}
	}

	return false;
}
	
const ICryPerfHUD::FpsBucket* CPerfHUD::GetFpsBuckets(int &numBuckets, float &totalTime) const
{
	const int nWidgets = m_widgets.size();

	for(int i=0; i<nWidgets; i++)
	{
		if(m_widgets[i]->m_id == ICryPerfHUDWidget::eWidget_FpsBuckets)
		{
			return ((CFpsWidget*)m_widgets[i].get())->GetFpsBuckets(numBuckets, totalTime);
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
//FPS Buckets Widget
//////////////////////////////////////////////////////////////////////////

int CFpsWidget::m_cvarPerfHudFpsExclusive = 0;

CFpsWidget::CFpsWidget(IMiniCtrl* pParentMenu, ICryPerfHUD *pPerfHud) : ICryPerfHUDWidget(eWidget_FpsBuckets, pPerfHud)
{
	m_fpsBucketSize = 5.f;
	m_fpsBudget = 30.f;

	//FPS Buckets
	IMiniCtrl* pFPSMenu = pPerfHud->CreateMenu("FPS", pParentMenu);
	m_pInfoBox = (CMiniInfoBox*)pPerfHud->CreateInfoMenuItem(pFPSMenu, "FPS Buckets", NULL, Rect(850,395,860,405));
	pPerfHud->CreateCallbackMenuItem(pFPSMenu, "Reset Buckets", CFpsWidget::ResetCallback, this);

	//Display framerates buckets as inclusive or exclusive
	REGISTER_CVAR2("sys_perfhud_fpsBucketsExclusive", &m_cvarPerfHudFpsExclusive, 0, VF_CHEAT, "Toggle FPS Buckets exclusive / inclusive");
	
	Reset();
}

void CFpsWidget::LoadBudgets(XmlNodeRef PerfXML)
{
	if(PerfXML)
	{
		XmlNodeRef fpsNode = PerfXML->findChild("fpsBucketSize");

		if(fpsNode)
		{
			fpsNode->getAttr("value", m_fpsBucketSize);
			Reset();
		}
	}
}

void CFpsWidget::UpdateBuckets(FpsBucketsStat &bucketStat, float frameTime, const char* name)
{
	char entryBuffer[CMiniInfoBox::MAX_TEXT_LENGTH] = {0};
	
	bucketStat.totalTime += frameTime;

	//Accumulate stats
	float fps = 1.f / frameTime;

	for(uint32 i=0; i<NUM_FPS_BUCKETS; i++)
	{
		if(fps>=bucketStat.buckets[i].targetFps)
		{
			bucketStat.buckets[i].timeAtTarget += frameTime;
		}
	}
		
	sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "%s: %.2f", name, fps);
	m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);

	//render exclusive stats
	if(m_cvarPerfHudFpsExclusive)
	{
		for(uint32 i=0; i<NUM_FPS_BUCKETS; i++)
		{
			//inclusive time
			float timeAtTarget = bucketStat.buckets[i].timeAtTarget;

			if(i>0)
			{
				//exclusive time
				timeAtTarget -= bucketStat.buckets[i-1].timeAtTarget;

				//Add info to gui
				float percentAtTarget = 100.f * (timeAtTarget / bucketStat.totalTime);
				sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "%.2f%%%% of time %.1f -> %.1f FPS", percentAtTarget, bucketStat.buckets[i].targetFps, bucketStat.buckets[i-1].targetFps);
			}
			else
			{
				float percentAtTarget = 100.f * (bucketStat.buckets[i].timeAtTarget / bucketStat.totalTime);
				sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "%.2f%%%% of time >= %.1f FPS", percentAtTarget, bucketStat.buckets[i].targetFps);
			}
			m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
		}
	}
	else //render inclusive stats
	{
		for(uint32 i=0; i<NUM_FPS_BUCKETS; i++)
		{
			float percentAtTarget = 100.f * (bucketStat.buckets[i].timeAtTarget / bucketStat.totalTime);
			sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "%.2f%%%% of time >= %.1f FPS", percentAtTarget, bucketStat.buckets[i].targetFps);
			m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
		}
	}
}

void CFpsWidget::Update()
{
	m_pInfoBox->ClearEntries();

	//
	// FPS
	//
	{
		float frameTime = gEnv->pTimer->GetRealFrameTime();
		UpdateBuckets(m_fpsBuckets, frameTime, "FPS");
	}

	//
	// GPU FPS
	//
	{
		float gpuFrameTime = gEnv->pRenderer->GetGPUFrameTime();

		if(gpuFrameTime>0.f)
		{
			m_pInfoBox->AddEntry("", CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
			UpdateBuckets(m_gpuBuckets, gpuFrameTime, "GPU FPS");
		}
	}

	m_pInfoBox->AutoResize();
}

void CFpsWidget::Reset()
{
	float targetFPS = m_fpsBudget;

	for(uint32 i=0; i<NUM_FPS_BUCKETS; i++)
	{
		m_fpsBuckets.buckets[i].targetFps = targetFPS;
		m_fpsBuckets.buckets[i].timeAtTarget = 0.f;
		
		m_gpuBuckets.buckets[i].targetFps = targetFPS;
		m_gpuBuckets.buckets[i].timeAtTarget = 0.f;

		targetFPS -= m_fpsBucketSize;
	}

	m_fpsBuckets.totalTime = 0.f;
	m_gpuBuckets.totalTime = 0.f;
}

void CFpsWidget::ResetCallback(void* data, bool status)
{
	assert(data);
	((CFpsWidget*)data)->Reset();
}
	
bool CFpsWidget::ShouldUpdate()
{
	return !m_pInfoBox->CheckFlag(eCtrl_Hidden);
}
	
//TODO
void CFpsWidget::SaveStats(XmlNodeRef statsXML)
{
	if(statsXML)
	{
		XmlNodeRef fpsNode;

		if(m_fpsBuckets.totalTime>0.f)
		{
			if( (fpsNode=statsXML->newChild("fpsBuckets")) )
			{
				XmlNodeRef child;

				for(uint32 i=0; i<NUM_FPS_BUCKETS; i++)
				{		
					float percentAtTarget = 100.f * (m_fpsBuckets.buckets[i].timeAtTarget / m_fpsBuckets.totalTime);

					if( (child=fpsNode->newChild("bucket")) )
					{
						child->setAttr("targetFps", m_fpsBuckets.buckets[i].targetFps);
					}

					if( (child=fpsNode->newChild("percentAtTime")) )
					{
						child->setAttr("value", percentAtTarget);
					}
				}
			}
		}
	}
}

const ICryPerfHUD::FpsBucket* CFpsWidget::GetFpsBuckets(int &numBuckets, float &totalTime) const
{
	numBuckets = NUM_FPS_BUCKETS;
	totalTime = m_fpsBuckets.totalTime;
	return m_fpsBuckets.buckets;
}

//////////////////////////////////////////////////////////////////////////
//Render Stats Widget
//////////////////////////////////////////////////////////////////////////

CRenderStatsWidget::CRenderStatsWidget(IMiniCtrl* pParentMenu, ICryPerfHUD *pPerfHud) : ICryPerfHUDWidget(eWidget_RenderStats, pPerfHud)
{
	m_fpsBudget = 30.f;
	m_dpBudget = 2000;
	m_polyBudget = 500000;
	m_postEffectBudget = 3;
	m_shadowCastBudget = 2;
	m_particlesBudget = 1000;
	m_pInfoBox = NULL;

	ZeroStruct(m_runtimeData);

	m_pInfoBox = (CMiniInfoBox*)pPerfHud->CreateInfoMenuItem(pParentMenu, "Scene Summary", NULL, Rect(45,350,100,400), true);
}

void CRenderStatsWidget::Update()
{
	IRenderer* pRenderer = gEnv->pRenderer;
	
	char entryBuffer[CMiniInfoBox::MAX_TEXT_LENGTH] = {0};

	//Clear old entries
	m_pInfoBox->ClearEntries();
	ZeroStruct(m_runtimeData);
	
	//
	// Render Type
	//
	const char* pRenderType = NULL;

	switch(gEnv->pRenderer->GetRenderType())
	{
		case eRT_DX9: pRenderType = "PC - DX9"; break;
		case eRT_DX11: pRenderType = "PC - DX11"; break;
		case eRT_Xbox360: pRenderType = "Xbox 360"; break;
		case eRT_PS3: pRenderType = "PS3"; break;
		case eRT_Null: pRenderType = "Null"; break;
		case eRT_Undefined:
		default: assert(0); pRenderType = "Undefined"; break;
	}
	m_pInfoBox->AddEntry(pRenderType, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
  
	//
	// FPS
	//
	m_runtimeData.fps = min(9999.f, gEnv->pTimer->GetFrameRate());

	sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "FPS: %.2f (%.2f)", m_runtimeData.fps, m_fpsBudget);
	
	if(m_runtimeData.fps>=m_fpsBudget)
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
	}
	else
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
		CryPerfHUDWarning(1.f, "FPS Too Low");

		//PerfHud  / AuxRenderer causes us to be RenderThread limited 
		//Need to investigate vertex buffer locks in AuxRenderer before enabling
		
		/*
		// Fran: please call me before re-enabling this code

		SRenderTimes renderTimes;
		pRenderer->GetRenderTimes(renderTimes);

		//wait for main is never 0
		if(renderTimes.fWaitForMain>renderTimes.fWaitForRender && renderTimes.fWaitForMain>renderTimes.fWaitForGPU)
		{
			m_pInfoBox->AddEntry("Main Thread Limited", CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
			m_pPerfHud->AddWarning("Main Thread Limited",1.f);
		}
		else if(renderTimes.fWaitForRender>renderTimes.fWaitForGPU)
		{
			m_pInfoBox->AddEntry("Render Thread Limited", CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
			m_pPerfHud->AddWarning("Render Thread Limited",1.f);
		}
		else
		{
			m_pInfoBox->AddEntry("GPU Limited", CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
			m_pPerfHud->AddWarning("GPU Limited",1.f);
		}*/
	}
	
	//
	// GPU Time
	//
	float gpuTime = pRenderer->GetGPUFrameTime();
	
	if(gpuTime>0.f)
	{
		float gpuFPS = 1.f/gpuTime;

		sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "GPU FPS: %.2f (%.2f)", gpuFPS, m_fpsBudget);
		if(gpuFPS>=m_fpsBudget)
		{
			m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
		}
		else
		{
			m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
			CryPerfHUDWarning(1.f, "GPU FPS Too Low");
		}
	}


	//
	// Memory
	//









	//
	// HDR
	//
	if(pRenderer->EF_Query(EFQ_HDRModeEnabled)!=0)
	{
		sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "HDR Enabled");
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
		m_runtimeData.hdrEnabled = true;
	}

	//
	// Render Thread
	//
	static ICVar* pMultiThreaded = gEnv->pConsole->GetCVar("r_MultiThreaded");
	if (pMultiThreaded && pMultiThreaded->GetIVal() > 0)
	{
		//sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Render Thread Enabled");
		//m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
		m_runtimeData.renderThreadEnabled = true;
	}
	else
	{
		sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Render Thread Disabled");
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
		m_runtimeData.renderThreadEnabled = false;
		CryPerfHUDWarning(1.f, "Render Thread Disabled");
	}


















	//
	// Camera
	//
	Matrix33 m = Matrix33(pRenderer->GetCamera().GetMatrix());
  m_runtimeData.cameraRot = RAD2DEG(Ang3::GetAnglesXYZ(m));
	m_runtimeData.cameraPos = pRenderer->GetCamera().GetPosition();
	
	sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Camera Pos(%.2f/%.2f/%.2f)", m_runtimeData.cameraPos.x, m_runtimeData.cameraPos.y, m_runtimeData.cameraPos.z);
	m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
	
	sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Camera Rot(%.2f/%.2f/%.2f)", m_runtimeData.cameraRot.x, m_runtimeData.cameraRot.y, m_runtimeData.cameraRot.z);
	m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);

	//
  // Polys / Draw Prims
	//
	int nShadowVolPolys;
	pRenderer->GetPolyCount(m_runtimeData.nPolys,nShadowVolPolys);
	m_runtimeData.nDrawPrims = pRenderer->GetCurrentNumberOfDrawCalls();

	sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Draw Calls: %d (%d)", m_runtimeData.nDrawPrims, m_dpBudget);

	if(m_runtimeData.nDrawPrims<=m_dpBudget)
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
	}
	else
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
		CryPerfHUDWarning(1.f, "Too Many Draw Calls");
	}
	
	sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Tris: %d (%d)", m_runtimeData.nPolys, m_polyBudget);

	if(m_runtimeData.nPolys<=m_polyBudget)
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
	}
	else
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
		CryPerfHUDWarning(1.f, "Too Many Tris");
	}

	//
	// Post Effects
	//
	
	m_runtimeData.nPostEffects = *((int*)gEnv->pRenderer->EF_Query(EFQ_NumActivePostEffects));
	
	sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Post Effects: %d (%d)", m_runtimeData.nPostEffects, m_postEffectBudget);

	if(m_runtimeData.nPostEffects<=m_postEffectBudget)
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
	}
	else
	{
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
		CryPerfHUDWarning(1.f, "Too Many Post Effects");
	}
	
	//
	// Forward Lights
	//
	PodArray<CDLight*> * pLights = gEnv->p3DEngine->GetDynamicLightSources();
	const uint32 nDynamicLights = pLights->Count();
	
	if(nDynamicLights)
	{
		uint32 nShadowCastingLights = 0;

		for(int i=0; i<nDynamicLights; i++)
		{
			CDLight* pLight = pLights->GetAt(i);

			if(pLight->m_Flags & DLF_CASTSHADOW_MAPS)
			{
				nShadowCastingLights++;
			}
		}
		
		sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Forward Lights: %d (~)", nDynamicLights);
		m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);

		if(nShadowCastingLights)
		{
			sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Forward Shadow Casting Lights: %d (%d)", nShadowCastingLights, m_shadowCastBudget);

			if(nShadowCastingLights<=m_shadowCastBudget)
			{
				m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
			}
			else
			{
				m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
				CryPerfHUDWarning(1.f, "Too Many Fwd Shadow Casting Lights");
			}
		}

		m_runtimeData.nFwdLights = nDynamicLights;
		m_runtimeData.nFwdShadowLights = nShadowCastingLights;
	}

	//
	// Deferred Lights
	//

	uint32 nTotalDeferredLights = 0;
	uint32 nDeferredCubemaps = 0;
	uint32 nDeferredShadowCasters = 0;
	for(int eLightType = 0;eLightType < eDLT_NumLightTypes;++eLightType)
	{
		TArray<CDLight>* deferredLights = pRenderer->EF_GetDeferredLights((eDeferredLightType)eLightType);
		
		if(deferredLights)
		{
			const uint32 nDeferredLights = deferredLights->size(); 
			nTotalDeferredLights += deferredLights->size();

			if(nDeferredLights)
			{

				for(uint32 i=0; i<nDeferredLights; i++)
				{
					CDLight& light = (*deferredLights)[i];

					if(light.GetSpecularCubemap())
					{
						nDeferredCubemaps++;
					}

					if(light.m_Flags & DLF_CASTSHADOW_MAPS)
					{
						nDeferredShadowCasters++;
					}
				}
			}
		}

		if(nTotalDeferredLights)
		{
			sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Deferred Lights: %d (~)", nTotalDeferredLights);
			m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);

			if(nDeferredShadowCasters)
			{
				sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Deferred Shadow Casting Lights: %d (%d)", nDeferredShadowCasters, m_shadowCastBudget);

				if(nDeferredShadowCasters<=m_shadowCastBudget)
				{
					m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
				}
				else
				{
					m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
					CryPerfHUDWarning(1.f, "Too Many Def Shadow Casting Lights");
				}
			}

			if(nDeferredCubemaps)
			{
				sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Deferred Cubemap Lights: %d (~)", nDeferredCubemaps);
				m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);
			}

			m_runtimeData.nDefLights = nTotalDeferredLights;
			m_runtimeData.nDefShadowLights = nDeferredShadowCasters;
			m_runtimeData.nDefCubeMaps = nDeferredCubemaps;
		}
	}
	
	//
	// Particles
	//
	IParticleManager *pParticleMgr = gEnv->p3DEngine->GetParticleManager();

	if(pParticleMgr)
	{
		SParticleCounts particleCounts;
		pParticleMgr->GetCounts(particleCounts);

		m_runtimeData.nParticles = (int)particleCounts.ParticlesRendered;

		sprintf_s(entryBuffer, CMiniInfoBox::MAX_TEXT_LENGTH, "Num Particles Rendered: %d (%d)", (int)particleCounts.ParticlesRendered, m_particlesBudget);

		if(particleCounts.ParticlesRendered<=m_particlesBudget)
		{
			m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_NORM, CPerfHUD::TEXT_SIZE_NORM);			
		}
		else
		{
			m_pInfoBox->AddEntry(entryBuffer, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_ERROR);
			CryPerfHUDWarning(1.f, "Too Many Particles");
		}
	}
}
bool CRenderStatsWidget::ShouldUpdate()
{
	if ( !m_pInfoBox->CheckFlag(eCtrl_Hidden) ||
				m_pPerfHud->WarningsWindowEnabled() )
	{
		return true;
	}
	return false;
}

void CRenderStatsWidget::LoadBudgets(XmlNodeRef perfXML)
{
	if(perfXML)
	{
		XmlNodeRef xmlNode;

		if( (xmlNode=perfXML->findChild("fps")) )
		{
			xmlNode->getAttr("value", m_fpsBudget);
		}

		if( (xmlNode=perfXML->findChild("drawPrim")) )
		{
			xmlNode->getAttr("value", m_dpBudget);
		}

		if( (xmlNode=perfXML->findChild("tris")) )
		{
			xmlNode->getAttr("value", m_polyBudget);
		}

		if( (xmlNode=perfXML->findChild("postEffects")) )
		{
			xmlNode->getAttr("value", m_postEffectBudget);
		}

		if( (xmlNode=perfXML->findChild("shadowCastingLights")) )
		{
			xmlNode->getAttr("value", m_shadowCastBudget);
		}
		
		if( (xmlNode=perfXML->findChild("particles")) )
		{
			xmlNode->getAttr("value", m_particlesBudget);
		}
	}
}

void CRenderStatsWidget::SaveStats(XmlNodeRef statsXML)
{
	if(!ShouldUpdate())
	{
		//Force update of stats, widget may not be currently enabled
		Update();
	}

	if(statsXML)
	{
		XmlNodeRef renderNode;

		if( (renderNode=statsXML->newChild("RenderStats")) )
		{
			XmlNodeRef child;

			if( (child=renderNode->newChild("fps")) )
			{
				child->setAttr("value", m_runtimeData.fps);
			}

			if( (child=renderNode->newChild("hdr")) )
			{
				child->setAttr("value", m_runtimeData.hdrEnabled);
			}

			if( (child=renderNode->newChild("renderThread")) )
			{
				child->setAttr("value", m_runtimeData.renderThreadEnabled);
			}

			if( (child=renderNode->newChild("cameraPos")) )
			{
				child->setAttr("x", m_runtimeData.cameraPos.x);
				child->setAttr("y", m_runtimeData.cameraPos.y);
				child->setAttr("z", m_runtimeData.cameraPos.z);
			}

			if( (child=renderNode->newChild("cameraRot")) )
			{
				child->setAttr("x", m_runtimeData.cameraRot.x);
				child->setAttr("y", m_runtimeData.cameraRot.y);
				child->setAttr("z", m_runtimeData.cameraRot.z);
			}

			if( (child=renderNode->newChild("drawPrims")) )
			{
				child->setAttr("value", m_runtimeData.nDrawPrims);
			}

			if( (child=renderNode->newChild("numPolys")) )
			{
				child->setAttr("value", m_runtimeData.nPolys);
			}

			if( (child=renderNode->newChild("numPostEffects")) )
			{
				child->setAttr("value", m_runtimeData.nPostEffects);
			}

			if( (child=renderNode->newChild("numFwdLights")) )
			{
				child->setAttr("value", m_runtimeData.nFwdLights);
			}

			if( (child=renderNode->newChild("numFwdShadowLights")) )
			{
				child->setAttr("value", m_runtimeData.nFwdShadowLights);
			}

			if( (child=renderNode->newChild("numDefLights")) )
			{
				child->setAttr("value", m_runtimeData.nDefLights);
			}

			if( (child=renderNode->newChild("numDefShadowLights")) )
			{
				child->setAttr("value", m_runtimeData.nDefShadowLights);
			}

			if( (child=renderNode->newChild("numDefCubeMaps")) )
			{
				child->setAttr("value", m_runtimeData.nDefCubeMaps);
			}
			
			if( (child=renderNode->newChild("numParticles")) )
			{
				child->setAttr("value", m_runtimeData.nParticles);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//Warnings Widget
//////////////////////////////////////////////////////////////////////////

CWarningsWidget::CWarningsWidget(IMiniCtrl* pParentMenu, ICryPerfHUD *pPerfHud) : ICryPerfHUDWidget(eWidget_Warnings,pPerfHud)
{
	m_pInfoBox = (CMiniInfoBox*)pPerfHud->CreateInfoMenuItem(pParentMenu, "Warnings", NULL, Rect(890,45,900,50), true);
	
	m_nMainThreadId = CryGetCurrentThreadId();
}

void CWarningsWidget::Reset()
{
	m_warnings.clear();
}

void CWarningsWidget::Update()
{
	//must be Main thread for MT warnings to work
	assert(CryGetCurrentThreadId()==m_nMainThreadId);

	//Update from multithreaded queue
	while (!m_threadWarnings.empty())
	{
		SWarning warning;
		if (m_threadWarnings.try_pop(warning))
		{
			AddWarning(warning.remainingDuration, warning.text);
		}
	}

	float frameTime = gEnv->pTimer->GetRealFrameTime();

	//delete old warnings
	// [K01]: fixing Linux crash
	TSWarnings::iterator iter = m_warnings.begin();
	while (iter != m_warnings.end())
	{
		SWarning* pW = &(*iter);
		pW->remainingDuration -= frameTime;

		if(pW->remainingDuration<=0.f)
			iter = m_warnings.erase(iter);
		else
			++iter;
	}

	int nWarnings = m_warnings.size();
	m_pInfoBox->ClearEntries();

	//display warnings
	for(int i=0; i<nWarnings; i++)
	{
		m_pInfoBox->AddEntry(m_warnings[i].text, CPerfHUD::COL_ERROR, CPerfHUD::TEXT_SIZE_WARN);
	}
}

bool CWarningsWidget::ShouldUpdate()
{
	return !m_pInfoBox->CheckFlag(eCtrl_Hidden);
}

void CWarningsWidget::SaveStats(XmlNodeRef statsXML)
{
	if(statsXML)
	{
		const int nWarnings = m_warnings.size();

		if(nWarnings>0)
		{
			XmlNodeRef warningNode;

			if( (warningNode=statsXML->newChild("warnings")) )
			{
				XmlNodeRef child;
				
				for(int i=0; i<nWarnings; i++)
				{
					if( (child=warningNode->newChild("warning")) )
					{
						child->setAttr("value", m_warnings[i].text);
					}	
				}
			}
		}
	}
}

void CWarningsWidget::AddWarningV(float duration, const char* fmt, va_list argList)
{
	char warningText[WARNING_LENGTH];

	int written = vsnprintf_s(warningText, WARNING_LENGTH, WARNING_LENGTH-1, fmt, argList);

	if (written == -1)
	{
		warningText[WARNING_LENGTH-1] = '\0';
	}

	AddWarning(duration, warningText);
}

void CWarningsWidget::AddWarning(float duration, const char* warning)
{
	if(CryGetCurrentThreadId() == m_nMainThreadId)
	{
		const int nWarnings = m_warnings.size();
		bool bNewWarning = true;

		for(int i=0; i<nWarnings; i++)
		{
			if(strcmp(m_warnings[i].text,warning)==0)
			{
				//warning already exists, update duration
				m_warnings[i].remainingDuration = duration;
				bNewWarning = false;
				break;
			}
		}

		if(bNewWarning)
		{
			SWarning newWarning;
			newWarning.remainingDuration = duration;
			strcpy_s(newWarning.text, WARNING_LENGTH, warning);

			m_warnings.push_back(newWarning);
		}
	}
	else
	{
		//add to thread safe queue, warning will be added next update
		SWarning newWarning;
		newWarning.remainingDuration = duration;
		strcpy_s(newWarning.text, WARNING_LENGTH, warning);
		m_threadWarnings.push(newWarning);
	}

}

//////////////////////////////////////////////////////////////////////////
//Render Batch Stats Widget
//////////////////////////////////////////////////////////////////////////

CRenderBatchWidget::CRenderBatchWidget(IMiniCtrl* pParentMenu, ICryPerfHUD *pPerfHud) : ICryPerfHUDWidget(eWidget_RenderBatchStats,pPerfHud)
{
	//m_pInfoBox = (CMiniInfoBox*)pPerfHud->CreateInfoMenuItem(pParentMenu, "Warnings", NULL, Rect(890,45,900,50), true);
	m_pTable = (CMiniTable*)pPerfHud->CreateTableMenuItem(pParentMenu, "Render Batch Stats");
	
	m_pTable->AddColumn("Name");
	m_pTable->AddColumn("Num Batches");
	m_pTable->AddColumn("Num Verts");
	m_pTable->AddColumn("Num Tris");
	m_pTable->AddColumn("GPU Time (ms)");

	m_pRStatsCVar = GetISystem()->GetIConsole()->GetCVar("r_stats");
}

void CRenderBatchWidget::Reset()
{
}	

void CRenderBatchWidget::SaveStats(XmlNodeRef statsXML)
{
}

bool CRenderBatchWidget::ShouldUpdate()
{
	return !m_pTable->CheckFlag(eCtrl_Hidden);
}

void CRenderBatchWidget::Update()
{








































































	m_pTable->ClearTable();
	m_pTable->AddData(0, "Not supported for this platform");

}

