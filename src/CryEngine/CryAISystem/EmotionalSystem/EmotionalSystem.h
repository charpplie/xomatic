/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   EmotionalSystem.h
Description: Main interface to create and destroy EmotionalPersonal
classes for each EntityId.
---------------------------------------------------------------------
History:
- 08:05:2008 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _EMOTIONALSYSTEM_H_
	#define _EMOTIONALSYSTEM_H_

	#include "IEmotionalSystem.h"
	#include "IFileChangeMonitor.h"
	#include "IGameFramework.h"

	#define EMOTIONAL_INFO_PATH "Scripts/AI/Emotional/"

class CPersonalEmotional;
class CEmotionalInfo;

class CEmotionalSystem : public IEmotionalSystem
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CEmotionalSystem();
		~				CEmotionalSystem();
		bool			Init();
		void			Shutdown();
		void			Reset();
		void			Update( float fDelta );
		void			OnEditorSetGameMode( bool bGameMode );
		VIRTUAL bool	AddListener( IEmotionalSystemListener* pListener );
		VIRTUAL bool	RemoveListener( IEmotionalSystemListener* pListener );

		/*$1- Util ---------------------------------------------------------------*/
		VIRTUAL CPersonalEmotional*					CreatePersonalEmotional( EntityId idEntity, const char* sEmotionalInfo );
		CPersonalEmotional*							GetPersonalEmotional( EntityId idEntity );
		CPersonalEmotional const*					GetPersonalEmotional( EntityId idEntity ) const;
		VIRTUAL bool								DeletePersonalEmotional( EntityId idEntity );
		VIRTUAL bool								SetPersonality( EntityId idEntity, float fExtraversion, float fStability, float fCalm );
		VIRTUAL bool								ReceiveEvent( EntityId idSender, const char* sEvent, int iValue = -1 );
		VIRTUAL bool								SetFixedValues( EntityId idSender, float fClarity, float fPleasure, float fArousal );
		VIRTUAL EEmotionalStates		GetEmotionalState( EntityId idEntity ) const;
		float										GetTimePassed() const;

		/*$1- Debug --------------------------------------------------------------*/
		void	UpdateDraw() const;
		void	SetDebug( const char* sActorName );

	private:

		/*$1- Utils --------------------------------------------------------------*/
		bool										LoadInfoFolder();
		bool										LoadInfo( const char* sFileName );
		CEmotionalInfo const*		GetEmotionalInfo( const char* sName ) const;

	private:

		/*$1- Members ------------------------------------------------------------*/
		static CEmotionalSystem*					m_pInstance;
		bool															m_bInit;
		float															m_fNow;
		string														m_sConfigFile;
		std::vector<CEmotionalInfo *>			m_vecInfos;
		std::vector<CPersonalEmotional *> m_vecPersonals;
		std::list<IEmotionalSystemListener *> m_listeners;

		/*$1- Debug stuff --------------------------------------------------------*/
		bool m_bDebug;
		string m_sDebugEntity;
};

/*$1- GetTimePassed ----------------------------------------------------------*/
inline float CEmotionalSystem::GetTimePassed() const
{
	return( m_fNow );
}

#endif // _EMOTIONALSYSTEM_H_
