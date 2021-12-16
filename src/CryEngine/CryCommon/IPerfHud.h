////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   ICryPerHud.h
//  Created:     19/11/2009 by Timur.
//  Description: Interface to the Performance HUD
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __IPERFHUD_H_
#define __IPERFHUD_H_

#include <CryExtension/ICryUnknown.h>
#include <CryExtension/Impl/ClassWeaver.h>
#include <ICryMiniGUI.h>
#include <IXml.h>

struct ICryPerfHUDWidget : public _reference_target_t
{
	//
	enum EWidgetID
	{
		eWidget_Warnings = 0,
		eWidget_RenderStats,
		eWidget_RenderBatchStats,
		eWidget_FpsBuckets,
		eWidget_Num, //number of widgets
	};

	ICryPerfHUDWidget(EWidgetID id, ICryPerfHUD *pPerfHud) :
	m_id(id),
	m_pPerfHud(pPerfHud)
	{}

	virtual ~ICryPerfHUDWidget() {}

	virtual void Reset()=0;
	virtual void Update()=0;
	virtual bool ShouldUpdate()=0;
	virtual void LoadBudgets(XmlNodeRef perfXML)=0;
	virtual void SaveStats(XmlNodeRef statsXML)=0;
	virtual void Enable()=0;
	virtual void Disable()=0;

	EWidgetID m_id;
	ICryPerfHUD *m_pPerfHud;
};

// Base Interface for all engine module extensions
struct ICryPerfHUD : public ICryUnknown
{
	CRYINTERFACE_DECLARE(ICryPerfHUD, 0x268d142e043d464c, 0xa0776580f81b988a);

	struct FpsBucket
	{
		float targetFps;
		float timeAtTarget;
	};
	
	enum EHudState
	{
		eHudOff=0,
		eHudInFocus,
		eHudOutOfFocus,
		eHudNumStates,
	};

	// Called once to initialize HUD.
	virtual void Init() = 0;
	virtual void Done() = 0;
	virtual void Draw() = 0;
	virtual void LoadBudgets() = 0;
	virtual void SaveStats(const char* filename = NULL) = 0;
	virtual void ResetWidgets() = 0;
	virtual void SetState(EHudState state)=0;
	virtual void Reset()=0;

	// Retrieve name of the extension module.
	virtual void Show() = 0;
	
	virtual minigui::IMiniCtrl*	CreateMenu( const char* name, minigui::IMiniCtrl *pParent=NULL ) = 0;
	virtual bool								CreateCVarMenuItem( minigui::IMiniCtrl *pMenu, const char* name, const char* controlVar, float controlVarOn, float controlVarOff ) = 0;
	virtual bool								CreateCallbackMenuItem( minigui::IMiniCtrl *pMenu, const char* name, minigui::ClickCallback clickCallback, void *pCallbackData) = 0;
	virtual minigui::IMiniCtrl*	CreateInfoMenuItem( minigui::IMiniCtrl *pMenu, const char* name, minigui::RenderCallback renderCallback, const minigui::Rect& rect, bool onAtStart=false) = 0;
	virtual minigui::IMiniCtrl*	CreateTableMenuItem( minigui::IMiniCtrl *pMenu, const char* name) = 0;

	virtual void EnableWidget(ICryPerfHUDWidget::EWidgetID id)=0;
	virtual void DisableWidget(ICryPerfHUDWidget::EWidgetID id)=0;

	//Warnings - Widget Specific interface
	virtual void AddWarning(float duration, const char* fmt, va_list argList) = 0;
	virtual bool WarningsWindowEnabled() const = 0;
	
	//FPS - Widget Specific interface
	virtual const FpsBucket* GetFpsBuckets(int &numBuckets, float &totalTime) const = 0;
};

void CryPerfHUDWarning(float duration, const char *, ...) PRINTF_PARAMS(2, 3);
inline void CryPerfHUDWarning( float duration, const char *format,... )
{
	if (gEnv && gEnv->pSystem)		
	{
		ICryPerfHUD *pPerfHud = gEnv->pSystem->GetPerfHUD();

		if(pPerfHud)
		{
			va_list args;
			va_start(args,format);
			pPerfHud->AddWarning( duration, format, args );
			va_end(args);
		}
	}
}

#endif