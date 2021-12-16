/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Iron Sight

-------------------------------------------------------------------------
History:
- 28:10:2005   16:00 : Created by Márcio Martins

*************************************************************************/
#pragma once

#ifndef __IRONSIGHT_H__
#define __IRONSIGHT_H__

#include <IViewSystem.h>
#include "Weapon.h"
#include "ItemParamReader.h"
#include "GameParameters.h"
#include "Recoil.h"
#include "WeaponSharedParams.h"

class CIronSight : public IZoomMode
{
private:
	struct EnterZoomAction;
	struct DisableTurnOffAction;
	struct EnableTurnOffAction;

public:

	CIronSight();
	virtual ~CIronSight();
	virtual void GetMemoryUsage(ICrySizer * s) const;

	// IZoomMode
	virtual void Init(IWeapon *pWeapon, const struct IItemParamsNode *params, uint32 id) { CRY_ASSERT_MESSAGE(0, "Don't use Init use InitZoomMode instead"); };
	virtual void InitZoomMode(IWeapon *pWeapon, const SParentZoomModeParams* pParams, uint32 id);

	const SParentZoomModeParams* GetParentShared() { return m_parentZoomParams; }
	void ResetSharedParams(const SZoomModeParams* pParams) { m_zoomParams = pParams; }

	virtual void Update(float frameTime, uint32 frameId);
	virtual void Release();

	virtual void ResetParams(const struct IItemParamsNode *params) {};
	virtual void PatchParams(const struct IItemParamsNode *patch) {};
	virtual void ModifyParams(bool modify, bool modified = true) {};

	virtual void Activate(bool activate);

	virtual bool CanZoom() const;
	virtual bool StartZoom(bool stayZoomed = false, bool fullZoomout = true, int zoomStep = 1);
	virtual void StopZoom();
	virtual void ExitZoom(bool force);

	virtual int GetCurrentStep() const {return m_currentStep;}
	virtual float GetZoomFoVScale(int step) const;

	virtual void ZoomIn();
	virtual bool ZoomOut();

	virtual bool IsZoomed() const;
	virtual bool IsZoomingIn() const;
	virtual bool IsZoomingInOrOut() const;
	virtual EZoomState GetZoomState() const;
	virtual float GetZoomInTime() const;
	virtual float GetZoomTransition() const;

	virtual bool AllowsZoomSnap() const;

	virtual void Enable(bool enable);
	virtual bool IsEnabled() const;

	virtual void Serialize(TSerialize ser);

	virtual void UpdateFPView(float frameTime){}

	virtual int  GetMaxZoomSteps() const;

	virtual void ApplyZoomMod(IFireMode* pFM);

	virtual bool IsToggle();

	virtual void FilterView(SViewParams &viewparams);
	virtual void PostFilterView(SViewParams & viewparams);
	// ~IZoomMode

	virtual void ResetTurnOff();
	virtual void TurnOff(bool enable, bool smooth=true, bool anim=true);

	virtual bool IsScope() const { return false; }

protected:
	virtual void EnterZoom(float time, const char *zoom_layer=0, bool smooth=true, int zoomStep = 1);
	virtual void ScheduleLeaveZoom();
	virtual void LeaveZoom();

	virtual void ZoomIn(float time, float from, float to, bool smooth, int nextStep);
	virtual void ZoomOut(float time, float from, float to, bool smooth, int nextStep);

	virtual void OnEnterZoom();
	virtual void OnZoomedIn();

	virtual void OnLeaveZoom();
	virtual void OnZoomedOut();

	virtual void OnZoom(float time, int nextStep);

	virtual void OnZoomStep(bool zoomingIn, float t);

	virtual void UpdateDepthOfField(float frameTime, float t);

	virtual void SetActorFoVScale(float fov, bool recoil, bool hbob);
	virtual float GetActorFoVScale() const;

	virtual float GetHBobFromFoVScale(float scale) const;
	virtual float GetRecoilFromFoVScale(float scale) const;
  
	virtual float GetMagFromFoVScale(float scale) const;
	virtual float GetFoVScaleFromMag(float mag) const;

	void ClearDoF();
	void ClearBlur();

	void SetIsZoomed(bool isZoomed);
	void AdjustNearFov(float time, bool zoomIn);
	void ResetNearFov();

	void ZoomSway(float time, float &x, float&y);

	CWeapon				*m_pWeapon;

	const SParentZoomModeParams*	m_parentZoomParams;
	const SZoomModeParams*				m_zoomParams;  
	uint32									m_zmIdx;

	//Beni - TODO: Many of the var members below could be static
  float					m_savedFoVScale;
	
	bool					m_zoomed;
	bool					m_zoomingIn;
	float					m_zoomTimer;
	float					m_zoomTime;
	float					m_focus;
	float					m_minDoF;
	float					m_maxDoF;
	float					m_averageDoF;

	float					m_leaveZoomTimeDelay;
	bool					m_zoomOutScheduled;

	float					m_startFoV;
	float					m_endFoV;
	bool					m_smooth;
	int						m_currentStep;

	float					m_initialNearFov;

	bool					m_enabled;

	float         m_swayTime;
	float         m_swayCycle;
//float					m_antiSniperEffect;

	float         m_lastRecoil;
};

#endif // __IRONSIGHT_H__
