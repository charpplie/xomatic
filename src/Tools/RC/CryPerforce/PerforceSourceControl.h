#ifndef __PERFORCESOURCECONTROL_H__
#define __PERFORCESOURCECONTROL_H__

#pragma once

#include "ICrySourceControl.h"
#include "p4api/clientapi.h"     // Perforce SDK


class CClientUser
	: public ClientUser          // Perforce SDK class
{
public:
	CClientUser();
	~CClientUser();

	void Reset();

	virtual void HandleError( Error *e );

	virtual void Edit( FileSys *f1, Error *e );

	virtual void OutputStat( StrDict *varList );

	virtual void OutputBinary( const char* data, int size );
	virtual void OutputText( const char* data, int length );
	virtual void OutputInfo( char level, const char* data );
	virtual void OutputError( const char* data );

	virtual int OutputBinary( char* data, int length );
	virtual int OutputText( char* data, int length );
	virtual int OutputInfo( char level, char* data );
	virtual int OutputError( char* data );

	const CSourceControlFileInfo& GetFileInfo() const;
	const CSourceControlUserInfo& GetUserInfo() const;
	const CSourceControlTimeInfo& GetTimeInfo() const;

	Error& GetError();

private:
    CSourceControlFileInfo m_fileInfo;
    CSourceControlUserInfo m_userInfo;
    CSourceControlTimeInfo m_timeInfo;
	Error m_e;                   // Perforce SDK class
};



class CPerforceSourceControl
	: public ICrySourceControl
{
public:
	/////////////////////////////////////////////////////////
	// ICrySourceControl implementation
	/////////////////////////////////////////////////////////
	virtual bool GetLastCheckOutInfo(
		const char* fileName, 
		const char* clientName, 
		CSourceControlFileInfo& fileInfo, 
		CSourceControlUserInfo& userInfo, 
		CSourceControlTimeInfo& timeInfo);


	static ICrySourceControl* Create();
	static void Destroy(ICrySourceControl* pSC);

	CPerforceSourceControl();
	~CPerforceSourceControl();

private:
//	bool Run(const char* func, int nArgs, char* argv[], bool bOnlyFatal=false, bool bTestPasswd=true);

	void MakeFullFileName( const char* fileName, char* fullFileName );

private:
	bool m_bIsFailedToConnect;
	ClientApi m_client;          // Perforce SDK class
	CClientUser m_cu;
};


#endif // __PERFORCESOURCECONTROL_H__
