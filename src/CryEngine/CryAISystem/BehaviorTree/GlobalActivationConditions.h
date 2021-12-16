/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   GlobalActivationConditions.h
$Id$
$DateTime$
Description: Handles the global list of activation conditions which
are shared by all trees. Special flavor of personal activation conditions,
it manages its own default values and loads them.
---------------------------------------------------------------------
History:
- 08:02:2009 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _GLOBAL_ACTIVATION_CONDITIONS_H_
#define _GLOBAL_ACTIVATION_CONDITIONS_H_

#include "IFileChangeMonitor.h"
#include "IGameFramework.h"
#include "PersonalActivationConditions.h"

class CDefaultActivationConditions;
class CSignalConditions;

class CGlobalActivationConditions : public CPersonalActivationConditions, public IFileChangeListener
{

public:

	/*$1- Basics -------------------------------------------------------------*/
	CGlobalActivationConditions( const char* sName );
	~CGlobalActivationConditions();
	void InitGlobals();
	bool ReLoad( const string& sFile );
	void OnEditorSetGameMode( bool bMode );

	/*$1- Inherited functions ------------------------------------------------*/
	void OnFileChange( const char* sFilename );

	/*$1- Utils --------------------------------------------------------------*/
	bool SetGlobalSignal( const char* sSignal, const ScriptAnyValue &argValue = NULL_BSS_SIGNAL_DATA );

protected:

	/*$1- Internal -----------------------------------------------------------*/
	void Register();
	void UnRegister();

private:
	virtual bool AssembleFinalCode( string const& sCode, string& sFinalCode ) const;

private:
	bool m_bReloadOnGameModeExit;
	bool m_bRegistered;
	string m_sFile;

	CDefaultActivationConditions const* m_pGlobalDefaults;
	CSignalConditions const* m_pSignals;
};

#endif // _GLOBAL_ACTIVATION_CONDITIONS_H_
