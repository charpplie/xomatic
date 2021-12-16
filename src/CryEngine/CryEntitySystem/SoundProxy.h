////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SoundProxy.h
//  Version:     v1.00
//  Created:     26/9/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SoundProxy_h__
#define __SoundProxy_h__
#pragma once

#include <ISound.h>

// forward declarations.
struct SEntityEvent;
struct ISkeletonPose;

//////////////////////////////////////////////////////////////////////////
// Description:
//    Handles sounds in the entity.
//////////////////////////////////////////////////////////////////////////
struct CSoundProxy : public IEntitySoundProxy, public ISoundEventListener
{
	CSoundProxy( CEntity *pEntity );
	VIRTUAL IEntity* GetEntity() const { return (IEntity*) m_pEntity; };

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType() { return ENTITY_PROXY_SOUND; }
	virtual void Release();
	virtual void Done() {};
	virtual	void Update( SEntityUpdateContext &ctx ) {}
	virtual	void ProcessEvent( SEntityEvent &event );
	virtual bool Init( IEntity *pEntity,SEntitySpawnParams &params ) { return true; }
	virtual void SerializeXML( XmlNodeRef &entityNode,bool bLoading ) {};
	virtual void Serialize( TSerialize ser );
	virtual bool NeedSerialize() { return false; };
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntitySoundProxy interface.
	//////////////////////////////////////////////////////////////////////////
	// Play a sound, return internal slot in entity where this sound is played.
	VIRTUAL void SetFlags( int nSoundProxyFlags ) { m_nFlags = nSoundProxyFlags; };
	VIRTUAL int  GetFlags() const { return m_nFlags; };
	VIRTUAL ISound* GetSound( tSoundID nSoundId );
	VIRTUAL ISound* GetSoundByIndex(uint32 nIndex); // will return NULL on invalid index
	VIRTUAL bool PlaySound( ISound *pSound, const Vec3 &vPos=Vec3(0,0,0),const Vec3 &vDirection=FORWARD_DIRECTION,float fSoundScale=1.0f, bool bLipSync=true );
	// new calls
	VIRTUAL tSoundID PlaySound( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, ESoundSemantic eSemantic);
	VIRTUAL tSoundID PlaySound( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, ESoundSemantic eSemantic, EntityId *pSkipEnts,int nSkipEnts);
	VIRTUAL tSoundID PlaySoundEx( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, float fVolume, float fMinRadius,float fMaxRadius, ESoundSemantic eSemantic);
	VIRTUAL tSoundID PlaySoundEx( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, float fVolume, float fMinRadius,float fMaxRadius, ESoundSemantic eSemantic, EntityId *pSkipEnts,int nSkipEnts);
	VIRTUAL bool SetStaticSound(tSoundID nSoundId, bool bStatic);
	VIRTUAL bool GetStaticSound(const tSoundID nSoundId);
	VIRTUAL void PauseSound( tSoundID nSoundId, bool bPause );
	VIRTUAL void StopSound( tSoundID nSoundId, ESoundStopMode eStopMode=ESoundStopMode_EventFade );
	VIRTUAL void StopAllSounds();
	VIRTUAL void SetSoundPos( tSoundID nSoundId, const Vec3 &vPos );
	VIRTUAL Vec3 GetSoundPos( tSoundID nSoundId );
	VIRTUAL void SetSoundDirection( tSoundID nSoundId, const Vec3 &dir );
	VIRTUAL Vec3 GetSoundDirection( tSoundID nSoundId );
	VIRTUAL void SetEffectRadius(float fEffectRadius ) { m_fEffectRadius = fEffectRadius; };
	VIRTUAL float GetEffectRadius( ) { return m_fEffectRadius; };
	VIRTUAL const char* GetTailName() { return m_sTailname.c_str(); }
	VIRTUAL void CheckVisibilityForTailName(const float fLength, const float fDistanceToRecalculate);

	VIRTUAL void UpdateSounds();
	
	// Used by RWI callback
	inline void	SetTailName(char const* const pcNewTailName) { m_sTailname = pcNewTailName; }

	//////////////////////////////////////////////////////////////////////////
	// ISoundEventListener implementation
	//////////////////////////////////////////////////////////////////////////
	virtual void OnSoundEvent( ESoundCallbackEvent event,ISound *pSound );
	//////////////////////////////////////////////////////////////////////////

	virtual void GetMemoryUsage(ICrySizer *pSizer )const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_sTailname);
		pSizer->AddObject(m_SoundsAttached);		
	}
private:
	//////////////////////////////////////////////////////////////////////////
	struct SAttachedSound
	{
		tSoundID nSoundID;
		_smart_ptr<ISound> pSound;
		Vec3 vOffset;
		Vec3 vDirection;
		bool bOffset;
		bool bStatic;
		void GetMemoryUsage(ICrySizer *pSizer )const
		{
				pSizer->AddObject(pSound);
		}
	};
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Private methods.
	//////////////////////////////////////////////////////////////////////////
	void OnMove();
	void OnEnter(IEntity *pWho, IEntity *pArea);
	void OnLeave(IEntity *pWho, IEntity *pArea);
	void OnLeaveNear(IEntity *pWho, IEntity *pArea);
	void OnMoveNear(IEntity *pWho, IEntity *pArea, float fFade, float fDistanceSq);
	void OnMoveInside(IEntity *pWho, IEntity *pArea);
	void OnExclusiveMoveInside(IEntity *pWho, IEntity *pAreaHigh, IEntity *pAreaLow);

	SAttachedSound* FindAttachedSound( tSoundID nSoundId );
	bool AddAttachedSound(ISound* pSound, const bool bOffset, const Vec3 &vOffset, const Vec3 &vDirection);
	bool RemoveAttachedSound( tSoundID nSoundId );
	void OnHide(bool bHide);
	
	void PrecacheHeadBone();
	void SetSoundPosition(ISound *pSound, const Vec3 *vOffset, const Vec3 *vDirection);
	ILINE bool IsSoundAnimEventsHandledExternally() const { return ((m_nFlags & FLAG_DELEGATE_SOUND_ANIM_EVENTS) != 0); }

private:
	//////////////////////////////////////////////////////////////////////////
	// Private member variables.
	//////////////////////////////////////////////////////////////////////////
	// Host entity.
	CEntity *m_pEntity;
	bool		m_bHide;
		
	float m_fEffectRadius;
	string m_sTailname;
	Vec3 m_vLastVisibilityPos;
	tSoundID m_currentLipSyncId;
	int m_nFlags;

	// Head members for voice files
	Matrix34 m_Headtm;
	ISkeletonPose *m_pSkeletonPose;
	int m_nBoneHead;

	// Map of currently playing sounds.
	typedef std::vector<SAttachedSound>			AttachedSoundsVec;
	typedef std::vector<_smart_ptr<ISound> >	SoundSmartVec;
	AttachedSoundsVec m_SoundsAttached;
};

#endif //__SoundProxy_h__
