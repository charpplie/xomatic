////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   config.cpp
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "config.h"

//////////////////////////////////////////////////////////////////////////
Config::Config() :m_pConfigKeyRegistry(0)
{
}

//////////////////////////////////////////////////////////////////////////
Config::~Config()
{
}


//////////////////////////////////////////////////////////////////////////
bool Config::CheckForUnknownKeys() const
{
	if(!m_pConfigKeyRegistry)
	{
		assert(m_pConfigKeyRegistry);		// this means SetConfigKeyRegistry() should be called
		return false;
	}

	bool result = true;

	const Map::const_iterator end=m_map.end();
	for(Map::const_iterator it=m_map.begin(); it!=end; ++it)
	{
		const char* keyName = it->first.m_sKeyName.c_str();
		if(!m_pConfigKeyRegistry->VerifyKeyRegistration2(keyName))
		{
			result = false;
		}
	}

	return result;
}


void Config::SetConfigKeyRegistry( IConfigKeyRegistry *pConfigKeyRegistry )
{
	m_pConfigKeyRegistry = pConfigKeyRegistry;
}



//////////////////////////////////////////////////////////////////////////
bool Config::HasKey( const char *key ) const
{
	string sValue;

	return Get(key,sValue,eCP_PriorityHighest);
}

//////////////////////////////////////////////////////////////////////////
bool Config::GetImpl( const char *key, float &value, const EConfigPriority ePriMask ) const
{
	string sValue;

	if(Get(key,sValue,ePriMask))
	{
		const char* szValue = sValue;

		if (1 == sscanf (szValue, "%f", &value))
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool Config::GetImpl( const char *key, int &value, const EConfigPriority ePriMask) const
{
	string sValue;

	if(Get(key,sValue,ePriMask))
	{
		const char* szValue = sValue;

		if (1 == sscanf (szValue, "%d", &value))
			return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
int Config::GetSum( const char *szKeyName ) const
{
	int ret=0;

	assert(m_pConfigKeyRegistry);		// this means SetConfigKeyRegistry() should be called

	if(m_pConfigKeyRegistry)
		m_pConfigKeyRegistry->VerifyKeyRegistration(szKeyName);

	string sValue;

	SKey key;

	key.m_sKeyName=szKeyName;
	key.m_eKeyPri=eCP_PriorityHighest;

	Map::const_iterator it = m_map.lower_bound(key);

	key.m_eKeyPri=eCP_PriorityLowest;

	Map::const_iterator end = m_map.upper_bound(key);

	if(it!=m_map.end())
	{

		while(it!=m_map.end())
		{
			const SKey &foundkey = it->first;

			if(stricmp(foundkey.m_sKeyName.c_str(),key.m_sKeyName.c_str())!=0)
				break;

			sValue = it->second;
			int localvalue;
	
			if(1 == sscanf (sValue.c_str(), "%d", &localvalue))
				ret+=localvalue;

			++it;
		}
	}

	return ret;
}
//////////////////////////////////////////////////////////////////////////
bool Config::GetImpl( const char *szKeyName,const char* &value, const EConfigPriority ePriMask ) const
{
	assert(m_pConfigKeyRegistry);		// this means SetConfigKeyRegistry() should be called

	if(m_pConfigKeyRegistry)
		m_pConfigKeyRegistry->VerifyKeyRegistration(szKeyName);

	SKey key;

	key.m_sKeyName=szKeyName;
	key.m_eKeyPri=eCP_PriorityHighest;

	Map::const_iterator it = m_map.lower_bound(key);

	key.m_eKeyPri=eCP_PriorityLowest;

	Map::const_iterator end = m_map.upper_bound(key);

	for(;it!=end;++it)
	{
		const SKey &foundkey = it->first;

		if(ePriMask==eCP_PriorityNotFile)
		{
			if(foundkey.m_eKeyPri==eCP_PriorityFile)
				continue;
		}
		else if(ePriMask!=eCP_PriorityHighest && foundkey.m_eKeyPri!=ePriMask)
			continue;

		if(stricmp(foundkey.m_sKeyName.c_str(),key.m_sKeyName.c_str())==0)
		{
//			printf("foundkey: %s=%s (%d)\n",foundkey.m_sKeyName.c_str(),it->second.c_str(),foundkey.m_eKeyPri);
			value = it->second.c_str();
			return true;
		}
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////
bool Config::GetImpl( const char *key, bool &value, const EConfigPriority ePriMask ) const
{
	string sValue;

	if(Get(key,sValue,ePriMask))
	{
		const char* szValue = sValue;
		int nTryInt;
		if (1 == sscanf (szValue, "%d", &nTryInt))
		{
			value = nTryInt != 0;
			return true;
		}

		if (!stricmp(szValue, "true")
			||!stricmp(szValue, "yes")
			||!stricmp(szValue, "enable")
			||!stricmp(szValue, "y")
			||!stricmp(szValue, "t"))
		{
			value = true;
			return true;
		}

		if (!stricmp(szValue, "false")
			||!stricmp(szValue, "no")
			||!stricmp(szValue, "disable")
			||!stricmp(szValue, "n")
			||!stricmp(szValue, "f"))
		{
			value = false;
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
void Config::Merge( const IConfig *inpConfig )
{
	assert(inpConfig);

	const Config *pConfig = inpConfig->GetInternalRepresentation();

	assert(pConfig);if(!pConfig)return;

	Map::const_iterator it;

	for(it=pConfig->m_map.begin();it!=pConfig->m_map.end();++it)
	{
		const SKey &key = it->first;
		const string value = it->second;

		Set(key.m_eKeyPri,key.m_sKeyName,value.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
const Config *Config::GetInternalRepresentation() const
{
	return this;	
}

//////////////////////////////////////////////////////////////////////////
uint32 Config::ClearPriorityUsage( const EConfigPriority ePri )
{
	uint32 dwRet=0;

	Map::const_iterator it=m_map.begin(), end=m_map.end();

	for(;it!=end;)
	{
		Map::const_iterator itNext=it;	++itNext;

		const SKey &key = it->first;

		if(key.m_eKeyPri==ePri)
		{
//			printf("ClearPriorityUsage %s=%s\n",key.m_sKeyName.c_str(),it->second.c_str());
			m_map.erase(key);
			++dwRet;
		}

		it=itNext;
	}

	return dwRet;
}



//////////////////////////////////////////////////////////////////////////
void Config::Set( const EConfigPriority ePri, const char *szKeyName,const char* value )
{
	if(*szKeyName==0)
		return;

	SKey key;

	key.m_sKeyName=szKeyName;
	key.m_eKeyPri=ePri;

	if(value==0)
		m_map.erase(key);
	 else
		m_map[key] = value;
}



//////////////////////////////////////////////////////////////////////////
void Config::SetToConfig( const EConfigPriority ePri, IConfigSink *pDestConfig )
{
	Map::const_iterator it=m_map.begin(), end=m_map.end();

	for(;it!=end;)
	{
		const SKey &key = it->first;

		if(key.m_eKeyPri==ePri)
			pDestConfig->Set(ePri,key.m_sKeyName,it->second);

		++it;
	}
}

