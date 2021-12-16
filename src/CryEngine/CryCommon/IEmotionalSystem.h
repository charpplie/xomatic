/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2008.
---------------------------------------------------------------------
File name:   IEmotionalSystem.h
Description: Main interface to create and destroy EmotionalPersonal
classes for each EntityId.
---------------------------------------------------------------------
History:
- 08:05:2008 : Created by Kevin

*********************************************************************/
#include DEVIRTUALIZE_HEADER_FIX(IEmotionalSystem.h)

#ifndef _IEMOTIONALSYSTEM_H_
#define _IEMOTIONALSYSTEM_H_

class CPersonalEmotional;

enum EEmotionalStates
{
	EMO_NULL,
	EMO_JOY,
	EMO_CONTENTMENT,
	EMO_ANGER,
	EMO_INDIGNATION,
	EMO_ANXIETY,
	EMO_DISORIENTED,
	EMO_FEAR,
	EMO_SADNESS,
	EMO_COUNT
};

// Emotional system listener
struct IEmotionalSystemListener
{
	virtual void					OnEmotionalChange( EntityId idEntity, int iNewEmotion ) = 0;
};

// Emotional system that creates and destroys EmotionalPersonal classes for each EntityId
UNIQUE_IFACE struct IEmotionalSystem
{
	/*$1- Basics -------------------------------------------------------------*/
	virtual ~						IEmotionalSystem() {}
	virtual bool					AddListener( IEmotionalSystemListener* pListener ) = 0;
	virtual bool					RemoveListener( IEmotionalSystemListener* pListener ) = 0;
	
	/*$1- Util ---------------------------------------------------------------*/
	virtual CPersonalEmotional*		CreatePersonalEmotional( EntityId idEntity, const char* sEmotionalInfo ) = 0;
	virtual bool							DeletePersonalEmotional( EntityId idEntity ) = 0;
	virtual bool							SetPersonality( EntityId idEntity, float fExtraversion, float fStability, float fCalm ) = 0;
	virtual bool							ReceiveEvent( EntityId idSender, const char* sEvent, int iValue = -1 ) = 0;
	virtual bool							SetFixedValues( EntityId idSender, float fClarity, float fPleasure, float fArousal ) = 0;
	virtual EEmotionalStates	GetEmotionalState( EntityId idEntity ) const = 0;
};

#endif //_IEMOTIONALSYSTEM_H_