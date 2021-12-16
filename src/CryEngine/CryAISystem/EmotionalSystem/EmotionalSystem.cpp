/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   EmotionalSystem.cpp
Description: Main interface to create and destroy EmotionalPersonal
classes for each EntityId.
---------------------------------------------------------------------
History:
- 08:05:2008 : Created by ricardo
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#include "StdAfx.h"
#include "EmotionalSystem.h"
#include "EmotionalInfo.h"
#include "PersonalEmotional.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CEmotionalSystem::CEmotionalSystem() : m_bInit( false ), m_bDebug( false ), m_fNow( 0.0f )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CEmotionalSystem::~CEmotionalSystem()
{
	Shutdown();
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::Init()
{
	m_bInit = LoadInfoFolder();
	return( m_bInit );
}

// Description:
//
// Arguments:
//
// Return:
//
void CEmotionalSystem::Shutdown()
{
	// The state might be empty if it failed to initialise, which should be no problem

	{
		std::vector<CPersonalEmotional *>::iterator itC = m_vecPersonals.begin();
		std::vector<CPersonalEmotional *>::iterator itEnd = m_vecPersonals.end();
		for( ; itC != itEnd; ++itC )
		{
			SAFE_DELETE( (*itC) );
		}

		m_vecPersonals.clear();
	}

	{
		std::vector<CEmotionalInfo *>::iterator itC = m_vecInfos.begin();
		std::vector<CEmotionalInfo *>::iterator itEnd = m_vecInfos.end();
		for( ; itC != itEnd; ++itC )
		{
			SAFE_DELETE( (*itC) );
		}

		m_vecInfos.clear();
	}

	m_bInit = false;
}

// Description:
//
// Arguments:
//
// Return:
//
void CEmotionalSystem::OnEditorSetGameMode( bool bGameMode )
{
	if( bGameMode == false )
	{
		Reset();

		std::vector<CEmotionalInfo *>::iterator itC = m_vecInfos.begin();
		std::vector<CEmotionalInfo *>::iterator itEnd = m_vecInfos.end();
		for( ; itC != itEnd; ++itC )
		{
			( *itC )->OnEditorSetGameMode( bGameMode );
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::AddListener( IEmotionalSystemListener* pListener )
{
	bool bRet = false;

	if (std::find(m_listeners.begin(), m_listeners.end(), pListener) == m_listeners.end())
	{
		bRet = true;
		m_listeners.push_back(pListener);
	}

	return ( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::RemoveListener( IEmotionalSystemListener* pListener )
{
	bool bRet = false;

	std::list<IEmotionalSystemListener *>::iterator it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
	if (it != m_listeners.end())
	{
		bRet = true;
		m_listeners.erase(it);
	}

	return ( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::LoadInfoFolder()
{
	bool bRet = false;

	ICryPak*	pack = gEnv->pCryPak;
	_finddata_t fd;
	string sLibPath = PathUtil::Make( EMOTIONAL_INFO_PATH, "*", "xml" );
	intptr_t handle = pack->FindFirst( sLibPath, &fd );

	if( handle >= 0 )
	{
		do
		{
			bRet = LoadInfo( fd.name );
			if( bRet == false )
			{
				AIError( "[Could not read Emotional Info file [%s]", fd.name );
			}
		} while( pack->FindNext(handle, &fd) >= 0 );

		pack->FindClose( handle );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::LoadInfo( const char* sFileName )
{
	assert( sFileName != NULL );

	bool bRet = false;

	CEmotionalInfo*		pInfo = new CEmotionalInfo();
	bRet = pInfo->Init( sFileName );

	if( bRet == true )
	{
		m_vecInfos.push_back( pInfo );
	}
	else
	{
		SAFE_DELETE( pInfo );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CEmotionalSystem::Update( float fDelta )
{
	assert( m_bInit == true );

	m_fNow += fDelta;

	std::vector<CPersonalEmotional *>::iterator itC = m_vecPersonals.begin();
	std::vector<CPersonalEmotional *>::iterator itEnd = m_vecPersonals.end();
	for( ; itC != itEnd; ++itC )
	{
		( *itC )->Update( fDelta );
	}
}

// Description:
//   Reload configuration
// Arguments:
//
// Return:
//
void CEmotionalSystem::Reset()
{
	assert( m_bInit == true );
	{
		std::vector<CPersonalEmotional *>::iterator itC = m_vecPersonals.begin();
		std::vector<CPersonalEmotional *>::iterator itEnd = m_vecPersonals.end();
		for( ; itC != itEnd; ++itC )
		{
			SAFE_DELETE( (*itC) );
		}

		m_vecPersonals.clear();
	}

	m_fNow = 0.0f;
}

// Description:
//
// Arguments:
//
// Return:
//
CPersonalEmotional* CEmotionalSystem::CreatePersonalEmotional( EntityId idEntity, const char* sEmotionalInfo )
{
	assert( m_bInit == true );
	assert( idEntity > 0 );
	assert( sEmotionalInfo != NULL );

	CPersonalEmotional*		pPersonal = GetPersonalEmotional( idEntity );

	if( pPersonal == NULL )
	{
		CEmotionalInfo const*		pInfo = GetEmotionalInfo( sEmotionalInfo );

		if( pInfo != NULL )
		{
			pPersonal = new CPersonalEmotional( this );

			if( pPersonal->Init(idEntity, pInfo) == true )
			{
				m_vecPersonals.push_back( pPersonal );
			}
			else
			{
				SAFE_DELETE( pPersonal );
			}
		}
	}

	return( pPersonal );
}

// Description:
//
// Arguments:
//
// Return:
//
CEmotionalInfo const* CEmotionalSystem::GetEmotionalInfo( const char* sName ) const
{
	assert( m_bInit == true );
	assert( sName != NULL );

	CEmotionalInfo const*													pInfo = NULL;
	std::vector<CEmotionalInfo *>::const_iterator itC = m_vecInfos.begin();
	std::vector<CEmotionalInfo *>::const_iterator itEnd = m_vecInfos.end();
	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->GetName().compareNoCase(sName) == 0 )
		{
			pInfo = *itC;
			break;
		}
	}

	return( pInfo );
}

// Description:
//
// Arguments:
//
// Return:
//
CPersonalEmotional* CEmotionalSystem::GetPersonalEmotional( EntityId idEntity )
{
	assert( m_bInit == true );
	assert( idEntity > 0 );

	CPersonalEmotional*													pPersonal = NULL;
	std::vector<CPersonalEmotional *>::iterator itC = m_vecPersonals.begin();
	std::vector<CPersonalEmotional *>::iterator itEnd = m_vecPersonals.end();
	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->GetEntityId() == idEntity )
		{
			pPersonal = *itC;
			break;
		}
	}

	return( pPersonal );
}

// Description:
//
// Arguments:
//
// Return:
//
CPersonalEmotional const* CEmotionalSystem::GetPersonalEmotional( EntityId idEntity ) const
{
	assert( m_bInit == true );
	assert( idEntity > 0 );

	CPersonalEmotional const*													pPersonal = NULL;
	std::vector<CPersonalEmotional *>::const_iterator itC = m_vecPersonals.begin();
	std::vector<CPersonalEmotional *>::const_iterator itEnd = m_vecPersonals.end();
	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->GetEntityId() == idEntity )
		{
			pPersonal = *itC;
			break;
		}
	}

	return( pPersonal );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::DeletePersonalEmotional( EntityId idEntity )
{
	assert( m_bInit == true );
	assert( idEntity > 0 );

	bool bRet = false;

	std::vector<CPersonalEmotional *>::iterator itC = m_vecPersonals.begin();
	std::vector<CPersonalEmotional *>::iterator itEnd = m_vecPersonals.end();
	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->GetEntityId() == idEntity )
		{
			SAFE_DELETE( (*itC) );
			m_vecPersonals.erase( itC );
			bRet = true;
			break;
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::SetPersonality( EntityId idEntity, float fExtraversion, float fStability, float fCalm )
{
	assert( m_bInit == true );
	assert( idEntity > 0 );

	bool bRet = false;

	std::vector<CPersonalEmotional *>::iterator itC = m_vecPersonals.begin();
	std::vector<CPersonalEmotional *>::iterator itEnd = m_vecPersonals.end();
	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->GetEntityId() == idEntity )
		{
			( *itC )->SetPersonality( fExtraversion, fStability, fCalm );
			bRet = true;
			break;
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::ReceiveEvent( EntityId idSender, const char* sEvent, int iValue )
{
	assert( m_bInit == true );
	assert( sEvent != NULL );
	assert( idSender > 0 );

	bool bRet = false;
	CPersonalEmotional*		pPersonal = GetPersonalEmotional( idSender );

	if( pPersonal != NULL )
	{
		bRet = pPersonal->ReceiveEvent( sEvent, iValue );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalSystem::SetFixedValues( EntityId idSender, float fClarity, float fPleasure, float fArousal )
{
	assert( m_bInit == true );
	assert( idSender > 0 );

	bool bRet = false;
	CPersonalEmotional*		pPersonal = GetPersonalEmotional( idSender );

	if( pPersonal != NULL )
	{
		bRet = true; // TODO
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
EEmotionalStates CEmotionalSystem::GetEmotionalState( EntityId idEntity ) const
{
	EEmotionalStates emoState = EMO_NULL;

	const CPersonalEmotional*		pPersonal = GetPersonalEmotional( idEntity );

	if( pPersonal != NULL )
	{
		emoState = pPersonal->GetCurrentEmotionalState();
	}

	return emoState;
}

// Description:
//
// Arguments:
//
// Return:
//
void CEmotionalSystem::SetDebug( const char* sActorName )
{
	assert( m_bInit == true );
	assert( sActorName != NULL );

	m_sDebugEntity.assign( sActorName );
}

// Description:
//
// Arguments:
//
// Return:
//
void CEmotionalSystem::UpdateDraw() const
{
	if( m_sDebugEntity.empty() == false )
	{
		std::vector<CPersonalEmotional *>::const_iterator itC = m_vecPersonals.begin();
		std::vector<CPersonalEmotional *>::const_iterator itEnd = m_vecPersonals.end();
		for( ; itC != itEnd; ++itC )
		{
			if( m_sDebugEntity.compareNoCase((*itC)->GetActor()->GetName()) == 0 )
			{
				( *itC )->DebugDraw();
				break;
			}
		}
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IEmotionalSystem)