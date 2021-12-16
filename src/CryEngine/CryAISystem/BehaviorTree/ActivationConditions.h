/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   ActivationConditions.h
$Id$
$DateTime$
Description: Main interface for receiving signals that will affect the
state of each puppet. This state then will be read by the Behavior Tree
to select the proper tree node to run by this puppet.
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _ACTIVATION_CONDITIONS_H_
#define _ACTIVATION_CONDITIONS_H_

#include "IFileChangeMonitor.h"
#include "IGameFramework.h"

class CPersonalActivationConditions;
class CDefaultActivationConditions;
class CSignalConditions;
struct SSignalReaction;

// -------------------------------------------------------------------
class CActivationConditions : public IFileChangeListener
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CActivationConditions( const string& sName );
	virtual ~CActivationConditions();
	void Init();
	void Shutdown();
	bool Reload( const string& sFile );
	void Reset();
	void Update( float fDeltaTime );

	/*$1- Inherited functions ------------------------------------------------*/
	void OnFileChange( const char* sFilename );
	void OnEditorSetGameMode( bool bGameMode );

	/*$1- Utils --------------------------------------------------------------*/
	CPersonalActivationConditions* CreatePersonalActivationConditions( TBSSProfileUserId userId );
	CPersonalActivationConditions* GetPersonalActivationConditions( TBSSProfileUserId userId );
	CPersonalActivationConditions const* GetPersonalActivationConditions( TBSSProfileUserId userId ) const;

	SSignalReaction const* GetReaction( const char* sSignal ) const;

	bool DeletePersonalActivationConditions( TBSSProfileUserId userId );
	bool ReceiveAnonymousSignal( const char* sSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA );
	bool CheckCondition( int iPreCompiledCode, TBSSProfileUserId userId ) const;
	bool IsDebug() const;
	void SetDebug( bool bDebug );

	const char* GetName() const;

protected:

	/*$1- Internal -----------------------------------------------------------*/
	void Register();
	void UnRegister();

private:

	/*$1- Members ------------------------------------------------------------*/
	bool m_bInit;
	bool m_bDebug;
	bool m_bReloadOnGameModeExit;
	bool m_bRegistered;
	string m_sConfigFile;
	string m_sName;
	string m_sTable;
	float m_fUpdateOncePerSec;
	static CActivationConditions *m_pInstance;
	CDefaultActivationConditions const *m_pDefaults;
	CSignalConditions const *m_pSignals;
	std::vector<CPersonalActivationConditions *> m_vecPersonals;
};

#endif // _ACTIVATION_CONDITIONS_H_
