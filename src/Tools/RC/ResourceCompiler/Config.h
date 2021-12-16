////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   config.h
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __config_h__
#define __config_h__
#pragma once


#include "iconfig.h"
#include "StlUtils.h"

/** Implementation of IConfig interface.
*/
class Config : public IConfig
{
public:
	// constructor
	Config();
	// destructor
	virtual ~Config();

	void SetConfigKeyRegistry( IConfigKeyRegistry *pConfigKeyRegistry ); 

	// interface IConfig ----------------------------------------------

	virtual void Release() { delete this; };
	virtual bool CheckForUnknownKeys() const;
	virtual bool HasKey( const char *key ) const;
	virtual bool GetImpl( const char *key, float &value, const EConfigPriority ePriMask ) const;
	virtual bool GetImpl( const char *key, int &value, const EConfigPriority ePriMask ) const;
	virtual int GetSum( const char *key ) const;
	virtual bool GetImpl( const char *key, bool &value, const EConfigPriority ePriMask ) const;
	virtual bool GetImpl( const char *key, const char* &value, const EConfigPriority ePriMask ) const;
	virtual void Merge( const IConfig *inpConfig );
	virtual const Config *GetInternalRepresentation() const;
	virtual uint32 ClearPriorityUsage( const EConfigPriority ePri );
	virtual void SetToConfig( const EConfigPriority ePri, IConfigSink *pDestConfig );

	// interface IConfigSink ------------------------------------------

	virtual void Set( const EConfigPriority ePri, const char *szKeyName, const char* value );

private: // ---------------------------------------------------------------------

	struct SKey
	{
		string						m_sKeyName;					//
		EConfigPriority		m_eKeyPri;					//

		bool operator< ( const SKey &b ) const
		{
			// sort by m_sKeyName
			if(stricmp(m_sKeyName.c_str(),b.m_sKeyName.c_str())<0)return true;
			if(stricmp(m_sKeyName.c_str(),b.m_sKeyName.c_str())>0)return false;

			// sort by m_eKeyPri
			if(m_eKeyPri>b.m_eKeyPri)return true;
			if(m_eKeyPri<b.m_eKeyPri)return false;

			return false;
		}
	};

	typedef std::map<SKey,string> Map;

	Map														m_map;									//
	IConfigKeyRegistry *					m_pConfigKeyRegistry;		// to verify key registration
};

#endif // __config_h__