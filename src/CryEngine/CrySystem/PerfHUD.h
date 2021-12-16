////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   PerfHUD.h
//  Created:     26/08/2009 by Timur.
//  Description: Button implementation in the MiniGUI
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PerfHUD_h__
#define __PerfHUD_h__

#include <IPerfHud.h>

#include "IInput.h"
#include "MiniGUI/MiniInfoBox.h"
#include "MiniGUI/MiniTable.h"

//////////////////////////////////////////////////////////////////////////
// Root window all other controls derive from
class CPerfHUD : public ICryPerfHUD,public minigui::IMiniGUIEventListener, public IInputEventListener
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(ICryPerfHUD)
	CRYINTERFACE_END()
	CRYGENERATE_SINGLETONCLASS(CPerfHUD, "PerfHUD", 0x006945f9985e4ce2, 0x872120bfdec09ca5)

public:
	//////////////////////////////////////////////////////////////////////////
	// ICryPerfHUD implementation
	//////////////////////////////////////////////////////////////////////////
	virtual void Init();
	virtual void Done();
	virtual void Draw();
	virtual void LoadBudgets();
	virtual void SaveStats(const char* filename);
	virtual void ResetWidgets();
	virtual void Reset();

	//Set state through code (rather than using joypad input)
	virtual void SetState(EHudState state) { m_hudState = state; }

	virtual void Show();
	
	//////////////////////////////////////////////////////////////////////////
	// Gui Creation helper funcs
	//////////////////////////////////////////////////////////////////////////
	virtual minigui::IMiniCtrl*	CreateMenu( const char* name, minigui::IMiniCtrl *pParent=NULL );
	virtual bool								CreateCVarMenuItem( minigui::IMiniCtrl *pMenu, const char* name, const char* controlVar, float controlVarOn, float controlVarOff );
	virtual bool								CreateCallbackMenuItem( minigui::IMiniCtrl *pMenu, const char* name, minigui::ClickCallback callback, void *pCallbackData);
	virtual minigui::IMiniCtrl*	CreateInfoMenuItem( minigui::IMiniCtrl *pMenu, const char* name, minigui::RenderCallback renderCallback, const minigui::Rect& rect, bool onAtStart=false);
	virtual minigui::IMiniCtrl*	CreateTableMenuItem( minigui::IMiniCtrl *pMenu, const char* name);
	
	virtual void EnableWidget(ICryPerfHUDWidget::EWidgetID id);
	virtual void DisableWidget(ICryPerfHUDWidget::EWidgetID id);

	//////////////////////////////////////////////////////////////////////////
	// WARNINGS - Widget Specific Interface
	//////////////////////////////////////////////////////////////////////////
	virtual void AddWarning(float duration, const char* fmt, va_list argList);
	virtual bool WarningsWindowEnabled() const;
	
	//////////////////////////////////////////////////////////////////////////
	// FPS - Widget Specific Interface
	//////////////////////////////////////////////////////////////////////////
	virtual const FpsBucket* GetFpsBuckets(int &numBuckets, float &totalTime) const;

	//////////////////////////////////////////////////////////////////////////
	// IMiniGUIEventListener implementation
	//////////////////////////////////////////////////////////////////////////
	virtual void OnCommand( minigui::SCommand &cmd );
	//////////////////////////////////////////////////////////////////////////
	
	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);

	//////////////////////////////////////////////////////////////////////////
	// CLICK CALLBACKS
	//////////////////////////////////////////////////////////////////////////
	
	static void ResetCallback(void* data, bool status);
	static void ReloadBudgetsCallback(void* data, bool status);
	static void SaveStatsCallback(void* data, bool status);
	
	//////////////////////////////////////////////////////////////////////////
	// RENDER CALLBACKS
	//////////////////////////////////////////////////////////////////////////

	static void DisplayRenderInfoCallback(const minigui::Rect& rect);

	//////////////////////////////////////////////////////////////////////////
	// CVAR CALLBACK
	//////////////////////////////////////////////////////////////////////////

	static void CVarChangeCallback(ICVar *pCvar);

	//////////////////////////////////////////////////////////////////////////
	// Static Data
	//////////////////////////////////////////////////////////////////////////

	static const float OVERSCAN_X;
	static const float OVERSCAN_Y;	
	
	static const ColorB COL_NORM;
	static const ColorB COL_WARN;
	static const ColorB COL_ERROR;
	
	static const float TEXT_SIZE_NORM;
	static const float TEXT_SIZE_WARN;
	static const float TEXT_SIZE_ERROR;
	
	static const float ACTIVATE_TIME;

protected:
	void InitUI( minigui::IMiniGUI *pGUI );
	void SetNextState();

protected:

	static int m_sys_perfhud;

	int m_sys_perfhud_prev;
	
	//record last menu position
	float m_menuStartX;
	float m_menuStartY;

	bool m_hudCreated;

	bool m_L1Pressed;
	bool m_L2Pressed;
	bool m_R1Pressed;
	bool m_R2Pressed;
	bool m_changingState;
	bool m_hwMouseEnabled;

	float m_triggersDownStartTime;

	EHudState m_hudState;
	EHudState m_hudLastState;

	std::vector< _smart_ptr<ICryPerfHUDWidget> > m_widgets;
	std::vector<minigui::IMiniCtrl*> m_rootMenus;
};

class CFpsWidget : public ICryPerfHUDWidget
{
public:
	
	CFpsWidget(minigui::IMiniCtrl *pParentMenu, ICryPerfHUD *pPerfHud);

	virtual void Reset();
	virtual void Update();
	virtual bool ShouldUpdate();
	virtual void LoadBudgets(XmlNodeRef perfXML);
	virtual void SaveStats(XmlNodeRef statsXML);
	virtual void Enable() { m_pInfoBox->SetVisible(true); }
	virtual void Disable() { m_pInfoBox->SetVisible(false); }

	const ICryPerfHUD::FpsBucket* GetFpsBuckets(int &numBuckets, float &totalTime) const; 

	static void ResetCallback(void* data, bool status);

protected:

	static const uint32	NUM_FPS_BUCKETS = 6;

	struct FpsBucketsStat
	{
		ICryPerfHUD::FpsBucket buckets[NUM_FPS_BUCKETS];
		float totalTime;
	};
	
	void UpdateBuckets(FpsBucketsStat &buckets, float frameTime, const char* name);

	// Data
	static int m_cvarPerfHudFpsExclusive;	

	FpsBucketsStat m_fpsBuckets;
	FpsBucketsStat m_gpuBuckets;

	//Common to all stats, cpu gpu etc
	float m_fpsBucketSize;
	float m_fpsBudget;
	
	minigui::CMiniInfoBox *m_pInfoBox;
};

class CRenderStatsWidget : public ICryPerfHUDWidget
{
public:
	
	CRenderStatsWidget(minigui::IMiniCtrl *pParentMenu, ICryPerfHUD *pPerfHud);

	virtual void Reset() {}
	virtual void Update();
	virtual bool ShouldUpdate();
	virtual void LoadBudgets(XmlNodeRef perfXML);
	virtual void SaveStats(XmlNodeRef statsXML);
	virtual void Enable() { m_pInfoBox->SetVisible(true); }
	virtual void Disable() { m_pInfoBox->SetVisible(false); }

protected:

	//budgets
	float m_fpsBudget;
	int m_dpBudget;
	int m_polyBudget;
	int m_postEffectBudget;
	int m_shadowCastBudget;
	int m_particlesBudget;

	//runtime data
	struct SRuntimeData
	{
		Vec3 cameraPos;
		Ang3 cameraRot;
		float fps;
		uint32 nDrawPrims;
		int nPolys;
		uint32 nPostEffects;
		uint32 nFwdLights;
		uint32 nFwdShadowLights;
		uint32 nDefLights;
		uint32 nDefShadowLights;
		uint32 nDefCubeMaps;
		int nParticles;
		bool hdrEnabled;
		bool renderThreadEnabled;
	};

	SRuntimeData m_runtimeData;

	minigui::CMiniInfoBox *m_pInfoBox;
};

class CWarningsWidget : public ICryPerfHUDWidget
{
public:
	
	CWarningsWidget(minigui::IMiniCtrl *pParentMenu, ICryPerfHUD *pPerfHud );

	virtual void Reset();
	virtual void Update();
	virtual bool ShouldUpdate();
	virtual void LoadBudgets(XmlNodeRef perfXML) {};
	virtual void SaveStats(XmlNodeRef statsXML);
	virtual void Enable() { m_pInfoBox->SetVisible(true); }
	virtual void Disable() { m_pInfoBox->SetVisible(false); }

	void AddWarningV(float duration, const char* fmt, va_list argList);
	void AddWarning(float duration, const char* warning);

protected:

	static const uint32 WARNING_LENGTH = 64;

	struct SWarning
	{
		char text[WARNING_LENGTH];
		float remainingDuration;
	};
	
	minigui::CMiniInfoBox *m_pInfoBox;
	
	typedef std::vector< SWarning > TSWarnings;

	TSWarnings m_warnings;
	
	CryMT::queue<SWarning> m_threadWarnings;
	
	uint32 m_nMainThreadId;
};

class CRenderBatchWidget : public ICryPerfHUDWidget
{
public:

	CRenderBatchWidget(minigui::IMiniCtrl *pParentMenu, ICryPerfHUD *pPerfHud );
	
	virtual void Reset();
	virtual void Update();
	virtual bool ShouldUpdate();
	virtual void LoadBudgets(XmlNodeRef perfXML) {};
	virtual void SaveStats(XmlNodeRef statsXML);
	virtual void Enable() { m_pTable->SetVisible(true); }
	virtual void Disable() { m_pTable->SetVisible(false); }

protected:

	struct BatchInfo
	{
		const char* name;
		uint32 nBatches;
		float gpuTime;
		int numVerts;
		int numIndices;
	};
	
	struct BatchInfoSort
	{
		inline bool operator() (const BatchInfo &lhs, const BatchInfo &rhs) const
		{
			return lhs.gpuTime > rhs.gpuTime;
		}
	};

	minigui::CMiniTable *m_pTable;
	ICVar *m_pRStatsCVar;
};


#endif __PerfHUD_h__
