#include "StdAfx.h"

#include "Option.h"

#include "Game.h"
#include "IPlayerProfiles.h"

//////////////////////////////////////////////////////////////////////////


COption::COption()
: m_currentProfile(0)
{
}



COption::COption(const char* name, const char* value)
: m_currentProfile(0)
{
	currentName = name;
	currentValue = value;
}



COption::~COption()
{
}



void COption::InitializeFromProfile()
{
	if(IPlayerProfile* profile = GetPlayerProfile())
	{
		profile->GetAttribute(GetName(),currentValue);
	}
}



void COption::InitializeFromCVar()
{
}



void COption::Set(const char* param)
{
	currentValue = param;
	if(IPlayerProfile* profile = GetPlayerProfile())
	{
		profile->SetAttribute(GetName(),currentValue);
	}
}



string COption::Get()
{
	return currentValue;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CCVarOption::CCVarOption()
{
}



CCVarOption::CCVarOption(const char* name, const char* defaultvalue, const char* cvar)
{
	currentName = name;
	currentValue = defaultvalue;
	cVar = cvar;
	defaultValue = defaultvalue;
}



CCVarOption::~CCVarOption()
{
}



void CCVarOption::InitializeFromProfile()
{
	if(IPlayerProfile* profile = GetPlayerProfile())
	{
		profile->GetAttribute(GetName(), currentValue);
	}

	ICVar* var = gEnv->pConsole->GetCVar(cVar);

	// Marcio: Let's not override *.cfg files if we're in Editor mode!
	if (var && ((var->GetFlags() & VF_WASINCONFIG) == 0))
		var->Set(currentValue.c_str());
}



void CCVarOption::InitializeFromCVar()
{
	ICVar* var = gEnv->pConsole->GetCVar(cVar);
	if(var)
		currentValue = var->GetString();

	if(IPlayerProfile* profile = GetPlayerProfile())
	{
		profile->SetAttribute(GetName(),currentValue);
	}
}



void CCVarOption::ResetDefault()
{
	Set(defaultValue.c_str());
}



void CCVarOption::Set(const char* param)
{
	currentValue = param;
	if(IPlayerProfile* profile = GetPlayerProfile())
	{
		profile->SetAttribute(GetName(),currentValue);
	}

	ICVar* var = gEnv->pConsole->GetCVar(cVar);
	// Marcio: Let's not override *.cfg files if we're in Editor mode!
	if (var && (!gEnv->bEditor || ((var->GetFlags() & VF_WASINCONFIG) == 0)))
		var->Set(currentValue.c_str());
}



string CCVarOption::Get()
{
	return currentValue;
}


//////////////////////////////////////////////////////////////////////////
