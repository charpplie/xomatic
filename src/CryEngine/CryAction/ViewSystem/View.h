/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: View System interfaces.
  
 -------------------------------------------------------------------------
  History:
  - 24:9:2004 : Created by Filippo De Luca

*************************************************************************/
#ifndef __VIEW_H__
#define __VIEW_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "IViewSystem.h"

class CGameObject;

class CView : public IView
{
public:

	CView(ISystem *pSystem);
	virtual ~CView();

	//shaking
	struct SShake
	{
		bool updating;
		bool flip;
		bool doFlip;
		bool groundOnly;
		bool permanent;
		bool interrupted; // when forcefully stopped
		
		int ID;

		float nextShake;
		float timeDone;
		float sustainDuration;
		float fadeInDuration;
		float fadeOutDuration;

		float frequency;
		float ratio;

		float randomness;
		
		Quat goalShake;
		Vec3 goalShakeVector;
				
		Ang3 amount;
		Vec3 amountVector;

		Quat shakeQuat;
		Vec3 shakeVector;

		SShake(int shakeID)
		{
			memset(this,0,sizeof(SShake));

			goalShake.SetIdentity();
			shakeQuat.SetIdentity();

			randomness = 0.5f;

			ID = shakeID;
		}
	};


	// IView
	virtual void Release();
	VIRTUAL void Update(float frameTime, bool isActive);
	virtual void ProcessShaking(float frameTime);
	virtual void ProcessShake(SShake *pShake,float frameTime);
	VIRTUAL void ResetShaking();
	//FIXME: keep CGameObject *  or use IGameObject *?
	VIRTUAL void LinkTo(IGameObject *follow);
	VIRTUAL void LinkTo(IEntity* follow);
	VIRTUAL EntityId GetLinkedId() {return m_linkedTo;};
	VIRTUAL void SetCurrentParams( SViewParams &params ) { m_viewParams = params; };
	VIRTUAL const SViewParams * GetCurrentParams() {return &m_viewParams;}
	VIRTUAL void SetViewShake(Ang3 shakeAngle,Vec3 shakeShift,float duration,float frequency,float randomness,int shakeID, bool bFlipVec = true, bool bUpdateOnly=false, bool bGroundOnly=false);
	VIRTUAL void SetViewShakeEx(  const SShakeParams& params );
	VIRTUAL void StopShake( int shakeID );
	// ~IView

	ListenerID GetSoundListenerID() { return m_SoundListenerID; }
	void SetSoundListenerID( ListenerID nListenerID );

	void Serialize(TSerialize ser);
	CCamera& GetCamera() { return m_camera; };

	void GetMemoryStatistics(ICrySizer * s);

protected:
	CGameObject * GetLinkedGameObject();
	IEntity* GetLinkedEntity();
	void ProcessShake_FinalDamping( SShake *pShake, float frameTime);
	void ProcessShake_CalcRatio( SShake *pShake, float frameTime, float endSustain );
	void ProcessShake_DoShaking( SShake *pShake, float frameTime);
	

	bool m_active;
	EntityId m_linkedTo;
	
	SViewParams m_viewParams;
	CCamera m_camera;

	ISystem *m_pSystem;
	
	std::vector<SShake> m_shakes;

	ListenerID m_SoundListenerID;
};

#endif //__VIEW_H__
