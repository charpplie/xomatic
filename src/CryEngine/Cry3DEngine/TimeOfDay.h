////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   TimeOfDay.h
//  Version:     v1.00
//  Created:     25/10/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TimeOfDay_h__
#define __TimeOfDay_h__

#define MAX_VAR_PARAM_COUNT 3

//////////////////////////////////////////////////////////////////////////
// ITimeOfDay interface implementation.
//////////////////////////////////////////////////////////////////////////
class CTimeOfDay : public ITimeOfDay
{
public:
	CTimeOfDay();
	~CTimeOfDay();

	//////////////////////////////////////////////////////////////////////////
	// ITimeOfDay 
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL int GetVariableCount() { return m_vars.size(); };
	VIRTUAL bool GetVariableInfo( int nIndex,SVariableInfo &varInfo );
	VIRTUAL void SetVariableValue( int nIndex,float fValue[3] );

	VIRTUAL void ResetVariables();

	// Time of day is specified in hours.
	VIRTUAL void SetTime( float fHour,bool bForceUpdate=false );
	VIRTUAL float GetTime() { return m_fTime; };

	VIRTUAL void SetPaused(bool paused) { m_bPaused = paused; }

	VIRTUAL void SetAdvancedInfo( const SAdvancedInfo &advInfo );
	VIRTUAL void GetAdvancedInfo( SAdvancedInfo &advInfo );

	float GetHDRMultiplier() { return m_fHDRMultiplier; }

	VIRTUAL void Update( bool bInterpolate=true,bool bForceUpdate=false );
	VIRTUAL void SetUpdateCallback(ITimeOfDayUpdateCallback* pCallback);

	VIRTUAL void Serialize( XmlNodeRef &node,bool bLoading );
	VIRTUAL void Serialize( TSerialize ser );

	VIRTUAL void SetTimer( ITimer * pTimer );

	VIRTUAL void NetSerialize( TSerialize ser, float lag, uint32 flags );

	VIRTUAL void Tick();
	//////////////////////////////////////////////////////////////////////////

	void BeginEditMode() { m_bEditMode = true; };
	void EndEditMode() { m_bEditMode = false; };

private:
	int AddVar( const char *group,const char *displayName,const char *name,int nParamId,EVariableType type,float defVal0=0,float defVal1=0,float defVal2=0 );
	SVariableInfo& GetVar( ETimeOfDayParamID id );
	void UpdateEnvLighting( bool forceUpdate );

private:
	std::vector<SVariableInfo>																m_vars;
	std::map<const char*,int,stl::less_strcmp<const char*> >	m_varsMap;
	float																											m_fTime;
	bool																											m_bEditMode;
	bool																											m_bPaused;

	SAdvancedInfo																							m_advancedInfo;
	ITimer *																									m_pTimer;
	float																											m_fHDRMultiplier;
	ICVar *																										m_pTimeOfDaySpeedCVar;
	ITimeOfDayUpdateCallback*																	m_pUpdateCallback;
};

#endif //__TimeOfDay_h__
